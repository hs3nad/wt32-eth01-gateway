#include "rcu_client.h"
#include "register_map.h"
#include "config.h"

#include <string.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"

static const uart_port_t RCU_UART_PORT = UART_NUM_2;
static constexpr size_t UART_BUFFER_SIZE = 2048;
static constexpr uint32_t UART_READ_SLICE_MS = 1;
static constexpr uint32_t BUS_ACTIVITY_ONLINE_MS = 30000;
static constexpr const char* RCU_NVS_NAMESPACE = "rcu";
static constexpr const char* RCU_NVS_NODE_ID_KEY = "node_id";
static constexpr uint8_t GATEWAY_PRIVATE_HEADER = 0x7F;
static constexpr uint8_t GATEWAY_PRIVATE_PROTO = 0xE1;
static constexpr uint8_t GATEWAY_CMD_WRITE_OUTBUF = 0x11;
static constexpr uint8_t GATEWAY_CMD_WRITE_OUTBUF_BIT = 0x12;
static constexpr uint8_t GATEWAY_CMD_READ_ALL_STATUS = 0x14;
static constexpr uint8_t GATEWAY_RESP_WRITE_OUTBUF = 0x91;
static constexpr uint8_t GATEWAY_RESP_WRITE_OUTBUF_BIT = 0x92;
static constexpr uint8_t GATEWAY_RESP_STATUS = 0x94;

static uint8_t rxFrame[8];
static uint8_t rxCount = 0;
static uint8_t cachedInpbuf[RCU_INPBUF_COUNT] = {};
static uint8_t cachedOutbuf[RCU_OUTBUF_COUNT] = {};
static uint8_t cachedLastFrame[8] = {};
static bool cacheValid = false;
static uint8_t privateSeq = 0;
static uint8_t runtimeNodeId = NodeId;
static uint32_t lastBusActivityMs = 0;

// Return a monotonic millisecond timestamp used for RS485 bus online checks.
static uint32_t nowMs() {
  return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

// True when the frame came from this gateway's ETH device ID. This helps
// distinguish our echoed/private actions from real keybox frames.
static bool isGatewayDeviceFrame(const uint8_t frame[8]) {
  return frame[2] == ETH_RCU_DEVICE_ID;
}

// Node ID is stored in NVS so installers can change it through Modbus and keep
// the value after power loss. The default remains the RCU register-map NodeId.
static bool isValidNodeId(uint8_t nodeId) {
  return nodeId >= 1 && nodeId <= 247;
}

// Read the persisted node ID from flash. Invalid or missing values fall back to
// the compile-time default without failing gateway startup.
static void loadNodeIdFromNvs() {
  uint8_t storedNodeId = NodeId;
  nvs_handle_t handle = 0;
  esp_err_t err = nvs_open(RCU_NVS_NAMESPACE, NVS_READONLY, &handle);
  if (err == ESP_OK) {
    err = nvs_get_u8(handle, RCU_NVS_NODE_ID_KEY, &storedNodeId);
    nvs_close(handle);
  }
  runtimeNodeId = (err == ESP_OK && isValidNodeId(storedNodeId)) ? storedNodeId : NodeId;
}

// Save a new node ID to flash. Commit is called immediately because this value
// is configuration, not high-rate runtime state.
static bool saveNodeIdToNvs(uint8_t nodeId) {
  nvs_handle_t handle = 0;
  esp_err_t err = nvs_open(RCU_NVS_NAMESPACE, NVS_READWRITE, &handle);
  if (err == ESP_OK) err = nvs_set_u8(handle, RCU_NVS_NODE_ID_KEY, nodeId);
  if (err == ESP_OK) err = nvs_commit(handle);
  if (handle) nvs_close(handle);
  return err == ESP_OK;
}

// Initialize UART2 for the local PS18V62_2 RS485 bus. The node ID byte is seeded
// in the cached Outbuf so Modbus can report it even before the first bus frame.
void rcuBegin(void) {
  loadNodeIdFromNvs();
  cachedOutbuf[RCU_OUTBUF_NODE_ID] = runtimeNodeId;

  uart_config_t uartConfig = {};
  uartConfig.baud_rate = RS485_BAUD;
  uartConfig.data_bits = UART_DATA_8_BITS;
  uartConfig.parity = UART_PARITY_DISABLE;
  uartConfig.stop_bits = UART_STOP_BITS_1;
  uartConfig.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
  uartConfig.rx_flow_ctrl_thresh = 0;
  uartConfig.source_clk = UART_SCLK_DEFAULT;
  ESP_ERROR_CHECK(uart_driver_install(RCU_UART_PORT, UART_BUFFER_SIZE, UART_BUFFER_SIZE, 0, nullptr, 0));
  ESP_ERROR_CHECK(uart_param_config(RCU_UART_PORT, &uartConfig));
  ESP_ERROR_CHECK(uart_set_pin(RCU_UART_PORT, RS485_TX_PIN, RS485_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
#if RS485_USE_EN
  ESP_ERROR_CHECK(gpio_set_direction((gpio_num_t)RS485_EN_PIN, GPIO_MODE_OUTPUT));
  ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)RS485_EN_PIN, 0));
#endif
}

// The RCU is considered online only after at least one valid bus/cache update
// and while bus activity is fresh. Modbus-written cache alone does not refresh
// lastBusActivityMs.
static bool busOnline() {
  return cacheValid && (nowMs() - lastBusActivityMs <= BUS_ACTIVITY_ONLINE_MS);
}

// Compare a previous snapshot with the current cache so the caller can decide
// whether this frame changed Modbus-visible state or was only a refresh.
static bool buffersChanged(const uint8_t oldInpbuf[RCU_INPBUF_COUNT], const uint8_t oldOutbuf[RCU_OUTBUF_COUNT]) {
  return memcmp(oldInpbuf, cachedInpbuf, RCU_INPBUF_COUNT) != 0 ||
         memcmp(oldOutbuf, cachedOutbuf, RCU_OUTBUF_COUNT) != 0;
}

// Small helper for RCU bitmaps. All RCU bits are LSB-first exactly like the
// original Inpbuf/Outbuf register map.
static void setBit(uint8_t* value, uint8_t mask, bool enabled) {
  if (enabled) *value |= mask;
  else *value &= (uint8_t)~mask;
}

// Resynchronize the byte stream by dropping the oldest byte. This lets the
// parser recover if it starts reading in the middle of an 8-byte frame.
static void shiftFrameLeft() {
  memmove(rxFrame, rxFrame + 1, sizeof(rxFrame) - 1);
  rxCount = sizeof(rxFrame) - 1;
}

// Transmit one fixed 8-byte PS18 frame. When RS485 DE/RE is enabled, hold the bus
// in transmit mode until the UART reports the bytes are sent, then return to RX.
static bool sendFixedFrame(const uint8_t frame[8]) {
#if RS485_USE_EN
  gpio_set_level((gpio_num_t)RS485_EN_PIN, 1);
  vTaskDelay(pdMS_TO_TICKS(1));
#endif
  bool ok = uart_write_bytes(RCU_UART_PORT, frame, 8) == 8;
  uart_wait_tx_done(RCU_UART_PORT, pdMS_TO_TICKS(SERIAL_RESPONSE_TIMEOUT_MS));
#if RS485_USE_EN
  gpio_set_level((gpio_num_t)RS485_EN_PIN, 0);
#endif
  return ok;
}

// Send one private gateway-to-RCU register-sync frame:
//   7F E1 31 CMD INDEX VALUE DATA SEQ
// These commands are separate from 7F 01 switch/keybox and 7F 05 thermostat
// traffic so Phase 1 bridge writes do not break existing room devices.
static bool sendPrivateFrame(uint8_t command, uint8_t index, uint8_t value, uint8_t data) {
  uint8_t frame[8] = {
    GATEWAY_PRIVATE_HEADER,
    GATEWAY_PRIVATE_PROTO,
    ETH_RCU_DEVICE_ID,
    command,
    index,
    value,
    data,
    ++privateSeq
  };
  return sendFixedFrame(frame);
}

// Decode normal RCU LED/status frames:
//   7E 02 PANEL C1 STATE 00 00 xx
// The gateway does not control the panels here; it mirrors their observed LED
// state into cached Outbuf bits so Modbus reads match the physical bus.
static bool applyLedFrame(const uint8_t frame[8]) {
  if (frame[0] != 0x7E || frame[1] != 0x02 || frame[3] != 0xC1) return false;

  uint8_t oldInpbuf[RCU_INPBUF_COUNT];
  uint8_t oldOutbuf[RCU_OUTBUF_COUNT];
  memcpy(oldInpbuf, cachedInpbuf, sizeof(oldInpbuf));
  memcpy(oldOutbuf, cachedOutbuf, sizeof(oldOutbuf));
  uint8_t panel = frame[2];
  uint8_t state = frame[4] & 0x07;

  switch (panel) {
    case 0x07:
      setBit(&cachedOutbuf[RCU_OUTBUF_LAMP1], Out5, (state & 0x02) != 0);
      break;
    case 0x0D:
      setBit(&cachedOutbuf[RCU_OUTBUF_LAMP1], Out8, (state & 0x01) != 0);
      setBit(&cachedOutbuf[RCU_OUTBUF_CONTROL], Dnd, (state & 0x02) != 0);
      setBit(&cachedOutbuf[RCU_OUTBUF_CONTROL], Mur, (state & 0x04) != 0);
      break;
    case 0x0E:
      setBit(&cachedOutbuf[RCU_OUTBUF_LAMP1], Out7, (state & 0x01) != 0);
      setBit(&cachedOutbuf[RCU_OUTBUF_LAMP1], Out6, (state & 0x02) != 0);
      setBit(&cachedOutbuf[RCU_OUTBUF_LAMP2], Out9, (state & 0x04) != 0);
      break;
    case 0x0F:
      setBit(&cachedOutbuf[RCU_OUTBUF_LAMP2], Out10, (state & 0x01) != 0);
      setBit(&cachedOutbuf[RCU_OUTBUF_LAMP1], Out6, (state & 0x02) != 0);
      setBit(&cachedOutbuf[RCU_OUTBUF_LAMP1], Out7, (state & 0x04) != 0);
      break;
    case 0x12:
      setBit(&cachedOutbuf[RCU_OUTBUF_LAMP2], Out11, (state & 0x01) != 0);
      break;
    default:
      break;
  }

  setBit(&cachedOutbuf[RCU_OUTBUF_CONTROL], Master,
         (cachedOutbuf[RCU_OUTBUF_LAMP1] & (Out5 | Out6 | Out7 | Out8)) != 0 ||
         (cachedOutbuf[RCU_OUTBUF_LAMP2] & (Out9 | Out10 | Out11)) != 0);
  setBit(&cachedInpbuf[0], Dndin, (cachedOutbuf[RCU_OUTBUF_CONTROL] & Dnd) != 0);
  setBit(&cachedInpbuf[0], Murin, (cachedOutbuf[RCU_OUTBUF_CONTROL] & Mur) != 0);
  cachedOutbuf[RCU_OUTBUF_NODE_ID] = runtimeNodeId;
  memcpy(cachedLastFrame, frame, 8);
  cacheValid = true;
  lastBusActivityMs = nowMs();

  return buffersChanged(oldInpbuf, oldOutbuf);
}

// Decode switch/keybox-style input frames:
//   7F 01 DEVICE COMMAND ...
// For the Phase 1 bridge only guest/keycard in/out is mirrored here. Other panel
// keys are still handled by the RCU firmware and later reported via LED frames.
static bool applyInputFrame(const uint8_t frame[8]) {
  if (frame[0] != 0x7F || frame[1] != 0x01) return false;

  uint8_t oldInpbuf[RCU_INPBUF_COUNT];
  uint8_t oldOutbuf[RCU_OUTBUF_COUNT];
  memcpy(oldInpbuf, cachedInpbuf, sizeof(oldInpbuf));
  memcpy(oldOutbuf, cachedOutbuf, sizeof(oldOutbuf));
  if ((frame[2] == 0x01 || isGatewayDeviceFrame(frame)) && frame[3] == 0x03) {
    cachedInpbuf[0] |= Guein;
    cachedOutbuf[RCU_OUTBUF_CONTROL] |= Guest;
  } else if ((frame[2] == 0x01 || isGatewayDeviceFrame(frame)) && frame[3] == 0x02) {
    cachedInpbuf[0] &= (uint8_t)~Guein;
    cachedOutbuf[RCU_OUTBUF_CONTROL] &= (uint8_t)~Guest;
  }

  cachedOutbuf[RCU_OUTBUF_NODE_ID] = runtimeNodeId;
  memcpy(cachedLastFrame, frame, 8);
  cacheValid = true;
  lastBusActivityMs = nowMs();
  return buffersChanged(oldInpbuf, oldOutbuf);
}

// Decode private RCU responses:
//   7F E1 01 91/92 INDEX VALUE SEQ STATUS
//   7F E1 01 94 OFFSET COUNT DATA0 DATA1
// The source byte is deliberately ignored. The room/node number belongs to the
// gateway NVS value, so a fixed or changed RCU node ID must not affect mapping.
// Successful ACKs update the same cache that normal bus frames use.
static bool applyPrivateFrame(const uint8_t frame[8]) {
  if (frame[0] != GATEWAY_PRIVATE_HEADER || frame[1] != GATEWAY_PRIVATE_PROTO) return false;
  if (frame[3] != GATEWAY_RESP_WRITE_OUTBUF &&
      frame[3] != GATEWAY_RESP_WRITE_OUTBUF_BIT &&
      frame[3] != GATEWAY_RESP_STATUS) {
    return false;
  }

  uint8_t oldInpbuf[RCU_INPBUF_COUNT];
  uint8_t oldOutbuf[RCU_OUTBUF_COUNT];
  memcpy(oldInpbuf, cachedInpbuf, sizeof(oldInpbuf));
  memcpy(oldOutbuf, cachedOutbuf, sizeof(oldOutbuf));

  if ((frame[3] == GATEWAY_RESP_WRITE_OUTBUF || frame[3] == GATEWAY_RESP_WRITE_OUTBUF_BIT) &&
      frame[7] == 0x00 && frame[4] < RCU_OUTBUF_COUNT) {
    cachedOutbuf[frame[4]] = frame[4] == RCU_OUTBUF_NODE_ID ? runtimeNodeId : frame[5];
  } else if (frame[3] == GATEWAY_RESP_STATUS) {
    uint8_t offset = frame[4];
    uint8_t count = frame[5];
    if (count > 2) count = 2;
    for (uint8_t i = 0; i < count; ++i) {
      uint8_t pos = offset + i;
      uint8_t value = i == 0 ? frame[6] : frame[7];
      if (pos < RCU_INPBUF_COUNT) {
        cachedInpbuf[pos] = value;
      } else {
        uint8_t outIndex = pos - RCU_INPBUF_COUNT;
        if (outIndex < RCU_OUTBUF_COUNT) cachedOutbuf[outIndex] = outIndex == RCU_OUTBUF_NODE_ID ? runtimeNodeId : value;
      }
    }
  }

  cachedOutbuf[RCU_OUTBUF_NODE_ID] = runtimeNodeId;
  memcpy(cachedLastFrame, frame, 8);
  cacheValid = true;
  lastBusActivityMs = nowMs();
  return buffersChanged(oldInpbuf, oldOutbuf);
}

// Assemble one complete fixed 8-byte frame from UART bytes. Frames must start
// with 0x7E or 0x7F; anything else is shifted out until a known header appears.
static bool readFixedFrame(uint8_t out[8]) {
  uint8_t byte = 0;
  int n = uart_read_bytes(RCU_UART_PORT, &byte, 1, pdMS_TO_TICKS(UART_READ_SLICE_MS));
  if (n <= 0) return false;

  if (rxCount >= sizeof(rxFrame)) shiftFrameLeft();
  rxFrame[rxCount++] = byte;
  if (rxCount < sizeof(rxFrame)) return false;

  bool knownHeader = rxFrame[0] == 0x7E || rxFrame[0] == 0x7F;
  if (!knownHeader) {
    shiftFrameLeft();
    return false;
  }

  memcpy(out, rxFrame, sizeof(rxFrame));
  rxCount = 0;
  return true;
}

// Public online probe used by main.cpp. The aa/floor arguments are kept for API
// shape compatibility, but PS18V62_2 fixed frames do not carry room addressing.
bool rcuPing(uint8_t aa, uint8_t floor) {
  (void)aa;
  (void)floor;
  return busOnline();
}

// Copy the latest cached RCU state into caller buffers. This function is cache
// based by design: it returns the most recent accepted bus/private state without
// blocking the Modbus task on RS485 traffic.
bool rcuSnapshot(uint8_t aa, uint8_t floor, uint8_t inpbuf[RCU_INPBUF_COUNT], uint8_t outbuf[RCU_OUTBUF_COUNT]) {
  (void)aa;
  (void)floor;
  if (!inpbuf || !outbuf || !cacheValid) return false;
  memcpy(inpbuf, cachedInpbuf, RCU_INPBUF_COUNT);
  memcpy(outbuf, cachedOutbuf, RCU_OUTBUF_COUNT);
  return true;
}

// Read and classify at most one bus event. The caller runs this frequently from
// gatewayTask so RS485 frames are mirrored into Modbus with low latency.
bool rcuReadBusEvent(RcuBusEvent* event) {
  uint8_t frame[8];
  if (!readFixedFrame(frame)) return false;

  bool changed = false;
  const char* reason = "ps18_bus_frame";
  if (frame[0] == 0x7E) {
    changed = applyLedFrame(frame);
    reason = changed ? "ps18_led_state" : "ps18_led_refresh";
  } else if (frame[0] == GATEWAY_PRIVATE_HEADER && frame[1] == GATEWAY_PRIVATE_PROTO) {
    changed = applyPrivateFrame(frame);
    reason = changed ? "gateway_private_state" : "gateway_private_ack";
  } else if (frame[0] == 0x7F) {
    changed = applyInputFrame(frame);
    reason = isGatewayDeviceFrame(frame) ? "ps18_eth_device_frame" : (changed ? "ps18_panel_input" : "ps18_panel_frame");
  }

  if (event) {
    memcpy(event->frame, frame, sizeof(event->frame));
    memcpy(event->inpbuf, cachedInpbuf, sizeof(event->inpbuf));
    memcpy(event->outbuf, cachedOutbuf, sizeof(event->outbuf));
    event->stateChanged = changed;
    event->reason = reason;
  }
  return true;
}

// Update only the local cached Outbuf byte. Used when Modbus writes need to be
// readable back immediately, even before the RCU ACK/status frame arrives.
bool rcuSetCachedOutputByte(uint8_t index, uint8_t value) {
  if (index >= RCU_OUTBUF_COUNT) return false;
  cachedOutbuf[index] = index == RCU_OUTBUF_NODE_ID ? runtimeNodeId : value;
  cachedOutbuf[RCU_OUTBUF_NODE_ID] = runtimeNodeId;
  cacheValid = true;
  return true;
}

uint8_t rcuGetNodeId(void) {
  return runtimeNodeId;
}

bool rcuSetNodeId(uint8_t nodeId) {
  if (!isValidNodeId(nodeId)) return false;
  runtimeNodeId = nodeId;
  cachedOutbuf[RCU_OUTBUF_NODE_ID] = runtimeNodeId;
  cacheValid = true;
  return saveNodeIdToNvs(runtimeNodeId);
}

// Write one whole Outbuf byte to the physical RCU through the private protocol.
// Outbuf[11] is the node ID, so it is deliberately not writable.
bool rcuWriteOutputByte(uint8_t index, uint8_t value) {
  if (index >= RCU_OUTBUF_COUNT || index == RCU_OUTBUF_NODE_ID) return false;
  bool ok = sendPrivateFrame(GATEWAY_CMD_WRITE_OUTBUF, index, value, 0);
  rcuSetCachedOutputByte(index, value);
  return ok;
}

// Write exactly one Outbuf bit to the physical RCU. Modbus coil writes use this
// path so a coil update does not overwrite neighboring bits in the same byte.
bool rcuWriteOutputBit(uint8_t index, uint8_t mask, bool enabled) {
  if (index >= RCU_OUTBUF_COUNT || index == RCU_OUTBUF_NODE_ID) return false;
  uint8_t value = cachedOutbuf[index];
  if (enabled) value |= mask;
  else value &= (uint8_t)~mask;
  bool ok = sendPrivateFrame(GATEWAY_CMD_WRITE_OUTBUF_BIT, index, mask, enabled ? 1 : 0);
  rcuSetCachedOutputByte(index, value);
  return ok;
}

// Convert high-level room actions into legacy switch/keybox frames. These are
// kept for compatibility with the original RCU logic, especially DND/MUR toggle
// behavior and keycard inserted/removed actions.
RcuActionResult rcuApplyAction(const char* action) {
  const uint8_t dndFrame[8] = {0x7F, 0x01, ETH_RCU_DEVICE_ID, 0x05, 0xFF, 0x08, 0x00, 0x00};
  const uint8_t murFrame[8] = {0x7F, 0x01, ETH_RCU_DEVICE_ID, 0x07, 0xFF, 0x08, 0x00, 0x00};
  const uint8_t keycardInFrame[8] = {0x7F, 0x01, ETH_RCU_DEVICE_ID, 0x03, 0x00, 0x00, 0x00, 0x1A};
  const uint8_t keycardOutFrame[8] = {0x7F, 0x01, ETH_RCU_DEVICE_ID, 0x02, 0x00, 0x00, 0x00, 0x1A};

  if (!action) return RCU_ACTION_UNSUPPORTED;
  if (strcmp(action, "makeup_room") == 0) {
    if (!busOnline()) return RCU_ACTION_CACHE_NOT_READY;
    if ((cachedOutbuf[RCU_OUTBUF_CONTROL] & Mur) != 0) return RCU_ACTION_ALREADY_APPLIED;
    return sendFixedFrame(murFrame) ? RCU_ACTION_SENT : RCU_ACTION_TX_FAILED;
  }
  if (strcmp(action, "finish_cleaning") == 0) {
    if (!busOnline()) return RCU_ACTION_CACHE_NOT_READY;
    if ((cachedOutbuf[RCU_OUTBUF_CONTROL] & Mur) == 0) return RCU_ACTION_ALREADY_APPLIED;
    return sendFixedFrame(murFrame) ? RCU_ACTION_SENT : RCU_ACTION_TX_FAILED;
  }
  if (strcmp(action, "do_not_disturb") == 0) {
    if (!busOnline()) return RCU_ACTION_CACHE_NOT_READY;
    if ((cachedOutbuf[RCU_OUTBUF_CONTROL] & Dnd) != 0) return RCU_ACTION_ALREADY_APPLIED;
    return sendFixedFrame(dndFrame) ? RCU_ACTION_SENT : RCU_ACTION_TX_FAILED;
  }
  if (strcmp(action, "clear_do_not_disturb") == 0) {
    if (!busOnline()) return RCU_ACTION_CACHE_NOT_READY;
    if ((cachedOutbuf[RCU_OUTBUF_CONTROL] & Dnd) == 0) return RCU_ACTION_ALREADY_APPLIED;
    return sendFixedFrame(dndFrame) ? RCU_ACTION_SENT : RCU_ACTION_TX_FAILED;
  }
  if (strcmp(action, "keycard_inserted") == 0 || strcmp(action, "guest_inserted") == 0) {
    return sendFixedFrame(keycardInFrame) ? RCU_ACTION_SENT : RCU_ACTION_TX_FAILED;
  }
  if (strcmp(action, "keycard_removed") == 0 || strcmp(action, "guest_removed") == 0) {
    return sendFixedFrame(keycardOutFrame) ? RCU_ACTION_SENT : RCU_ACTION_TX_FAILED;
  }
  return RCU_ACTION_UNSUPPORTED;
}
