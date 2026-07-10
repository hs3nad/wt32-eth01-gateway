#include "modbus_tcp.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "lwip/tcp.h"

#include "config.h"
#include "rcu_client.h"
#include "register_map.h"

static const char* TAG = "floor-modbus";

#define MODBUS_PROTO_ID 0
#define MODBUS_MAX_BODY 253
#define MODBUS_MAX_PDU 252
#define MODBUS_TCP_BACKLOG 4
#define MODBUS_MAX_CLIENTS MODBUS_TCP_BACKLOG
#define MODBUS_SOCKET_POLL_TIMEOUT_SEC 2
#define MODBUS_SEND_STALL_TIMEOUT_SEC 10
#define MODBUS_TCP_KEEPALIVE_IDLE_SEC 30
#define MODBUS_TCP_KEEPALIVE_INTERVAL_SEC 10
#define MODBUS_TCP_KEEPALIVE_COUNT 3
#define MODBUS_CLIENT_TASK_STACK 4096

#define MB_FC_READ_COILS 0x01
#define MB_FC_READ_DISCRETE_INPUTS 0x02
#define MB_FC_READ_HOLDING_REGS 0x03
#define MB_FC_READ_INPUT_REGS 0x04
#define MB_FC_WRITE_SINGLE_COIL 0x05
#define MB_FC_WRITE_SINGLE_REG 0x06
#define MB_FC_DIAGNOSTICS 0x08
#define MB_FC_WRITE_MULTIPLE_COILS 0x0F
#define MB_FC_WRITE_MULTIPLE_REGS 0x10
#define MB_FC_MASK_WRITE_REG 0x16
#define MB_FC_READ_WRITE_MULTIPLE_REGS 0x17
#define MB_FC_ENCAPSULATED_INTERFACE 0x2B

#define MB_MEI_READ_DEVICE_ID 0x0E
#define MB_EX_ILLEGAL_FUNCTION 0x01
#define MB_EX_ILLEGAL_DATA_ADDRESS 0x02
#define MB_EX_ILLEGAL_DATA_VALUE 0x03

static constexpr uint16_t CONTROL_HOLDING_REG = RCU_OUTBUF_CONTROL / 2;
static constexpr bool CONTROL_HOLDING_REG_HIGH_BYTE = (RCU_OUTBUF_CONTROL % 2) == 1;
static constexpr uint16_t NODE_ID_HOLDING_REG = RCU_OUTBUF_NODE_ID / 2;
static constexpr bool NODE_ID_HOLDING_REG_HIGH_BYTE = (RCU_OUTBUF_NODE_ID % 2) == 1;

// Modbus register image exposed to clients. Holding registers mirror RCU
// Outbuf[] pairs; input registers mirror Inpbuf[] pairs; discrete inputs expose
// gateway/RCU health bits that do not exist in the original RCU register map.
static uint16_t s_holdingRegs[MODBUS_HOLDING_REG_COUNT];
static uint16_t s_inputRegs[MODBUS_INPUT_REG_COUNT];
static bool s_discreteInputs[MODBUS_DISCRETE_INPUT_COUNT];
static portMUX_TYPE s_regLock = portMUX_INITIALIZER_UNLOCKED;
static SemaphoreHandle_t s_clientSlots = nullptr;

struct ModbusClientCtx {
  int sock;
  struct sockaddr_in sourceAddr;
};

// Read a Modbus big-endian 16-bit value from the request PDU.
static uint16_t be16Read(const uint8_t* data) {
  return ((uint16_t)data[0] << 8) | data[1];
}

// Write a Modbus big-endian 16-bit value into the response PDU.
static void be16Write(uint8_t* data, uint16_t value) {
  data[0] = (uint8_t)(value >> 8);
  data[1] = (uint8_t)value;
}

// Pack two RCU bytes into one Modbus register. The project convention is:
//   register N low byte  = byte[N * 2]
//   register N high byte = byte[N * 2 + 1]
// Example: HR2 = high Outbuf[5], low Outbuf[4].
static uint16_t packPair(const uint8_t* bytes, uint16_t byteCount, uint16_t regIndex) {
  uint16_t byteIndex = (uint16_t)(regIndex * 2);
  uint16_t lo = byteIndex < byteCount ? bytes[byteIndex] : 0;
  uint16_t hi = (byteIndex + 1) < byteCount ? bytes[byteIndex + 1] : 0;
  return (uint16_t)((hi << 8) | lo);
}

// Select one byte from the packed Modbus register without changing the other
// byte. This is used heavily by coil mapping and control side effects.
static uint8_t selectPairByte(uint16_t value, bool highByte) {
  return highByte ? (uint8_t)(value >> 8) : (uint8_t)value;
}

// Convert a coil address back to the holding register that contains the mapped
// Outbuf byte. Eight coils make one byte; two bytes make one holding register.
static uint16_t coilAddrToHoldingReg(uint16_t coilAddr) {
  return (uint16_t)((coilAddr / 8) / 2);
}

// True when the coil maps to the high byte of its packed holding register.
static bool coilAddrIsHighByte(uint16_t coilAddr) {
  return ((coilAddr / 8) % 2) == 1;
}

// Replace exactly one byte inside a packed holding register and preserve the
// neighbor byte. This keeps coil/register writes from damaging adjacent state.
static uint16_t updatePairByte(uint16_t value, bool highByte, uint8_t byteValue) {
  return highByte ? (uint16_t)((value & 0x00FF) | ((uint16_t)byteValue << 8))
                  : (uint16_t)((value & 0xFF00) | byteValue);
}

// Read one coil directly from the holding-register mirror. The caller must hold
// s_regLock because this function assumes the register array is stable.
static bool holdingCoilReadLocked(uint16_t coilAddr) {
  uint16_t regIndex = coilAddrToHoldingReg(coilAddr);
  uint16_t bitMask = (uint16_t)(1U << (coilAddr % 8));
  uint8_t byteValue = selectPairByte(s_holdingRegs[regIndex], coilAddrIsHighByte(coilAddr));
  return (byteValue & bitMask) != 0;
}

// Wrapper for socket options: log failures but keep the server alive because
// keepalive/timeouts are helpful, not required for protocol correctness.
static void setSocketOptWarn(int sock, int level, int optname, const void* optval, socklen_t optlen, const char* name) {
  if (setsockopt(sock, level, optname, optval, optlen) != 0) {
    ESP_LOGW(TAG, "setsockopt %s failed errno=%d", name, errno);
  }
}

// Configure one accepted Modbus TCP client socket. Timeouts prevent dead clients
// from pinning a task forever; keepalive helps detect unplugged peers.
static void configureClientSocket(int sock) {
  struct timeval timeout = {};
  timeout.tv_sec = MODBUS_SOCKET_POLL_TIMEOUT_SEC;
  int opt = 1;

  setSocketOptWarn(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout), "SO_RCVTIMEO");
  setSocketOptWarn(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout), "SO_SNDTIMEO");
  setSocketOptWarn(sock, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt), "SO_KEEPALIVE");

#ifdef TCP_NODELAY
  setSocketOptWarn(sock, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt), "TCP_NODELAY");
#endif
#ifdef TCP_KEEPIDLE
  opt = MODBUS_TCP_KEEPALIVE_IDLE_SEC;
  setSocketOptWarn(sock, IPPROTO_TCP, TCP_KEEPIDLE, &opt, sizeof(opt), "TCP_KEEPIDLE");
#endif
#ifdef TCP_KEEPINTVL
  opt = MODBUS_TCP_KEEPALIVE_INTERVAL_SEC;
  setSocketOptWarn(sock, IPPROTO_TCP, TCP_KEEPINTVL, &opt, sizeof(opt), "TCP_KEEPINTVL");
#endif
#ifdef TCP_KEEPCNT
  opt = MODBUS_TCP_KEEPALIVE_COUNT;
  setSocketOptWarn(sock, IPPROTO_TCP, TCP_KEEPCNT, &opt, sizeof(opt), "TCP_KEEPCNT");
#endif
}

// Receive exactly length bytes unless the socket closes or the idle timeout
// expires. Modbus TCP MBAP/PDU parsing needs full fields, not partial reads.
static int recvAll(int sock, uint8_t* buffer, size_t length) {
  size_t offset = 0;
  TickType_t lastProgressTick = xTaskGetTickCount();
  const TickType_t idleTimeoutTicks = pdMS_TO_TICKS(MODBUS_CLIENT_IDLE_TIMEOUT_SEC * 1000);

  while (offset < length) {
    int read = recv(sock, buffer + offset, length - offset, 0);
    if (read <= 0) {
      if (read < 0 && errno == EINTR) continue;
      if (read < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        if (MODBUS_CLIENT_IDLE_TIMEOUT_SEC == 0 ||
            (xTaskGetTickCount() - lastProgressTick) < idleTimeoutTicks) {
          continue;
        }
      }
      return read;
    }
    offset += read;
    lastProgressTick = xTaskGetTickCount();
  }

  return (int)offset;
}

// Send an entire Modbus TCP response. A short send is retried until all bytes are
// written or the peer stalls for too long.
static int sendAll(int sock, const uint8_t* buffer, size_t length) {
  size_t offset = 0;
  TickType_t lastProgressTick = xTaskGetTickCount();
  const TickType_t stallTimeoutTicks = pdMS_TO_TICKS(MODBUS_SEND_STALL_TIMEOUT_SEC * 1000);

  while (offset < length) {
    int sent = send(sock, buffer + offset, length - offset, 0);
    if (sent <= 0) {
      if (sent < 0 && errno == EINTR) continue;
      if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        if ((xTaskGetTickCount() - lastProgressTick) < stallTimeoutTicks) continue;
        errno = ETIMEDOUT;
      }
      return sent;
    }
    offset += sent;
    lastProgressTick = xTaskGetTickCount();
  }

  return (int)offset;
}

// Return the configured Modbus unit ID. Kept as a helper so future per-room or
// DIP-based unit selection has one obvious hook point.
static uint8_t currentUnitId() {
  return MODBUS_UNIT_ID;
}

// Validate a Modbus address range. Uses subtraction to avoid start+quantity
// overflow and rejects zero-length requests.
static bool checkRange(uint16_t start, uint16_t quantity, uint16_t maxCount) {
  if (quantity == 0 || start >= maxCount) return false;
  return quantity <= (maxCount - start);
}

// Build a standard Modbus exception PDU: function|0x80 followed by the exception
// code. The return value is the response PDU length.
static size_t makeException(uint8_t* responsePdu, uint8_t functionCode, uint8_t exceptionCode) {
  responsePdu[0] = functionCode | 0x80;
  responsePdu[1] = exceptionCode;
  return 2;
}

// Apply legacy RCU actions for control bits that have existing PS18 commands.
// DND, MUR, and Guest have old switch/keybox frames; other bits still use the
// private register write path after the holding register is updated.
static RcuActionResult applyControlDelta(uint8_t oldValue, uint8_t newValue) {
  struct BitAction {
    uint16_t mask;
    const char* enableAction;
    const char* disableAction;
  };
  static const BitAction actions[] = {
    {Dnd, "do_not_disturb", "clear_do_not_disturb"},
    {Mur, "makeup_room", "finish_cleaning"},
    {Guest, "keycard_inserted", "keycard_removed"},
  };

  for (const BitAction& action : actions) {
    bool oldEnabled = (oldValue & action.mask) != 0;
    bool newEnabled = (newValue & action.mask) != 0;
    if (oldEnabled == newEnabled) continue;

    RcuActionResult result = rcuApplyAction(newEnabled ? action.enableAction : action.disableAction);
    if (result != RCU_ACTION_SENT && result != RCU_ACTION_ALREADY_APPLIED) return result;
  }

  return RCU_ACTION_ALREADY_APPLIED;
}

// Central validation point for holding-register writes. Outbuf[11] carries the
// gateway node ID and is persisted to flash, so it is range-limited to the valid
// Modbus slave/unit range used by the project: 1..247.
static bool validateHoldingWrite(uint16_t addr, uint16_t value) {
  if (addr >= MODBUS_HOLDING_REG_COUNT) return false;
  if (addr == NODE_ID_HOLDING_REG) {
    uint8_t nodeId = selectPairByte(value, NODE_ID_HOLDING_REG_HIGH_BYTE);
    if (nodeId < 1 || nodeId > 247) return false;
  }
  return true;
}

// Persist writable configuration bytes that live inside the holding-register
// image. At the moment this is only Outbuf[11] Node ID; normal runtime outputs
// continue through mirrorHoldingRegToRcuCache().
static bool applyHoldingConfigWrites(uint16_t addr, uint16_t value) {
  if (addr != NODE_ID_HOLDING_REG) return true;
  uint8_t nodeId = selectPairByte(value, NODE_ID_HOLDING_REG_HIGH_BYTE);
  return rcuSetNodeId(nodeId);
}

// Execute side effects that are required before committing a holding-register
// write. For the control register this sends legacy DND/MUR/Guest frames when
// those bits change.
static bool applyHoldingSideEffects(uint16_t addr, uint16_t value) {
  if (addr != CONTROL_HOLDING_REG) return true;

  uint16_t oldValue;
  portENTER_CRITICAL(&s_regLock);
  oldValue = s_holdingRegs[CONTROL_HOLDING_REG];
  portEXIT_CRITICAL(&s_regLock);

  uint8_t oldControl = selectPairByte(oldValue, CONTROL_HOLDING_REG_HIGH_BYTE);
  uint8_t newControl = selectPairByte(value, CONTROL_HOLDING_REG_HIGH_BYTE);
  RcuActionResult result = applyControlDelta(oldControl, newControl);
  return result == RCU_ACTION_SENT || result == RCU_ACTION_ALREADY_APPLIED;
}

// After a Modbus holding-register write, mirror both packed bytes into the RCU
// client cache and send private WRITE_OUTBUF_BYTE commands for writable bytes.
// Outbuf[11] is the Node ID, so it is cached from the persisted runtime value
// but not written to the RCU private protocol.
static void mirrorHoldingRegToRcuCache(uint16_t addr, uint16_t value) {
  uint16_t byteIndex = (uint16_t)(addr * 2);
  if (byteIndex < RCU_OUTBUF_COUNT) {
    uint8_t outIndex = (uint8_t)byteIndex;
    uint8_t outValue = (uint8_t)value;
    if (outIndex == RCU_OUTBUF_NODE_ID) rcuSetCachedOutputByte(outIndex, outValue);
    else rcuWriteOutputByte(outIndex, outValue);
  }
  if ((byteIndex + 1) < RCU_OUTBUF_COUNT) {
    uint8_t outIndex = (uint8_t)(byteIndex + 1);
    uint8_t outValue = (uint8_t)(value >> 8);
    if (outIndex == RCU_OUTBUF_NODE_ID) rcuSetCachedOutputByte(outIndex, outValue);
    else rcuWriteOutputByte(outIndex, outValue);
  }
}

// Shared handler for FC03/FC04 register reads. The caller supplies the register
// array and count so holding and input reads stay identical.
static size_t handleReadRegs(uint8_t functionCode, const uint16_t* regs, uint16_t regCount,
                             const uint8_t* requestPdu, size_t requestLen, uint8_t* responsePdu) {
  if (requestLen != 5) return makeException(responsePdu, functionCode, MB_EX_ILLEGAL_DATA_VALUE);

  uint16_t start = be16Read(&requestPdu[1]);
  uint16_t quantity = be16Read(&requestPdu[3]);
  if (!checkRange(start, quantity, regCount) || quantity > 125) {
    return makeException(responsePdu, functionCode, MB_EX_ILLEGAL_DATA_ADDRESS);
  }

  responsePdu[0] = functionCode;
  responsePdu[1] = (uint8_t)(quantity * 2);

  portENTER_CRITICAL(&s_regLock);
  for (uint16_t i = 0; i < quantity; ++i) {
    be16Write(&responsePdu[2 + (i * 2)], regs[start + i]);
  }
  portEXIT_CRITICAL(&s_regLock);

  return 2 + (quantity * 2);
}

// Shared handler for bit reads that are stored as bool arrays, currently used by
// FC02 discrete inputs. Coils use handleReadCoils because they are derived from
// packed holding registers instead of a separate bool array.
static size_t handleReadBits(uint8_t functionCode, const bool* bits, uint16_t bitCount,
                             const uint8_t* requestPdu, size_t requestLen, uint8_t* responsePdu) {
  if (requestLen != 5) return makeException(responsePdu, functionCode, MB_EX_ILLEGAL_DATA_VALUE);

  uint16_t start = be16Read(&requestPdu[1]);
  uint16_t quantity = be16Read(&requestPdu[3]);
  if (!checkRange(start, quantity, bitCount) || quantity > 2000) {
    return makeException(responsePdu, functionCode, MB_EX_ILLEGAL_DATA_ADDRESS);
  }

  uint8_t byteCount = (uint8_t)((quantity + 7) / 8);
  responsePdu[0] = functionCode;
  responsePdu[1] = byteCount;
  memset(&responsePdu[2], 0, byteCount);

  portENTER_CRITICAL(&s_regLock);
  for (uint16_t i = 0; i < quantity; ++i) {
    if (bits[start + i]) responsePdu[2 + (i / 8)] |= (uint8_t)(1U << (i % 8));
  }
  portEXIT_CRITICAL(&s_regLock);

  return 2 + byteCount;
}

// FC01 coil reads. Coils are not stored separately; they are a bit view of
// Outbuf[0..10] through the packed holding-register mirror.
static size_t handleReadCoils(const uint8_t* requestPdu, size_t requestLen, uint8_t* responsePdu) {
  if (requestLen != 5) return makeException(responsePdu, MB_FC_READ_COILS, MB_EX_ILLEGAL_DATA_VALUE);

  uint16_t start = be16Read(&requestPdu[1]);
  uint16_t quantity = be16Read(&requestPdu[3]);
  if (!checkRange(start, quantity, MODBUS_COIL_COUNT) || quantity > 2000) {
    return makeException(responsePdu, MB_FC_READ_COILS, MB_EX_ILLEGAL_DATA_ADDRESS);
  }

  uint8_t byteCount = (uint8_t)((quantity + 7) / 8);
  responsePdu[0] = MB_FC_READ_COILS;
  responsePdu[1] = byteCount;
  memset(&responsePdu[2], 0, byteCount);

  portENTER_CRITICAL(&s_regLock);
  for (uint16_t i = 0; i < quantity; ++i) {
    if (holdingCoilReadLocked((uint16_t)(start + i))) responsePdu[2 + (i / 8)] |= (uint8_t)(1U << (i % 8));
  }
  portEXIT_CRITICAL(&s_regLock);

  return 2 + byteCount;
}

// FC08 diagnostics. Only subfunction 0x0000 "Return Query Data" is supported,
// which is enough for simple Modbus TCP connectivity checks.
static size_t handleDiagnostics(const uint8_t* requestPdu, size_t requestLen, uint8_t* responsePdu) {
  if (requestLen < 5) return makeException(responsePdu, MB_FC_DIAGNOSTICS, MB_EX_ILLEGAL_DATA_VALUE);
  if (be16Read(&requestPdu[1]) != 0x0000) return makeException(responsePdu, MB_FC_DIAGNOSTICS, MB_EX_ILLEGAL_FUNCTION);
  memcpy(responsePdu, requestPdu, requestLen);
  return requestLen;
}

// FC06 write one holding register. The value is committed to the Modbus mirror,
// then mirrored into the RCU cache/private protocol.
static size_t handleWriteSingleReg(const uint8_t* requestPdu, size_t requestLen, uint8_t* responsePdu) {
  if (requestLen != 5) return makeException(responsePdu, MB_FC_WRITE_SINGLE_REG, MB_EX_ILLEGAL_DATA_VALUE);

  uint16_t addr = be16Read(&requestPdu[1]);
  uint16_t value = be16Read(&requestPdu[3]);
  if (addr >= MODBUS_HOLDING_REG_COUNT) return makeException(responsePdu, MB_FC_WRITE_SINGLE_REG, MB_EX_ILLEGAL_DATA_ADDRESS);
  if (!validateHoldingWrite(addr, value)) return makeException(responsePdu, MB_FC_WRITE_SINGLE_REG, MB_EX_ILLEGAL_DATA_VALUE);
  if (!applyHoldingSideEffects(addr, value)) return makeException(responsePdu, MB_FC_WRITE_SINGLE_REG, MB_EX_ILLEGAL_DATA_VALUE);
  if (!applyHoldingConfigWrites(addr, value)) return makeException(responsePdu, MB_FC_WRITE_SINGLE_REG, MB_EX_ILLEGAL_DATA_VALUE);

  portENTER_CRITICAL(&s_regLock);
  s_holdingRegs[addr] = value;
  portEXIT_CRITICAL(&s_regLock);
  mirrorHoldingRegToRcuCache(addr, value);

  memcpy(responsePdu, requestPdu, 5);
  return 5;
}

// Apply one coil write using read-modify-write on the containing Outbuf byte.
// This keeps all other bits in the same byte unchanged and sends a private
// WRITE_OUTBUF_BIT command to the RCU.
static bool applyCoilWrite(uint16_t addr, bool enabled) {
  if (addr >= MODBUS_COIL_COUNT) return false;

  uint16_t regIndex = coilAddrToHoldingReg(addr);
  bool highByte = coilAddrIsHighByte(addr);
  uint16_t bitMask = (uint16_t)(1U << (addr % 8));

  uint16_t oldRegValue;
  portENTER_CRITICAL(&s_regLock);
  oldRegValue = s_holdingRegs[regIndex];
  portEXIT_CRITICAL(&s_regLock);

  uint8_t byteValue = selectPairByte(oldRegValue, highByte);
  if (enabled) {
    byteValue = (uint8_t)(byteValue | bitMask);
  } else {
    byteValue = (uint8_t)(byteValue & (uint8_t)~bitMask);
  }
  uint16_t newRegValue = updatePairByte(oldRegValue, highByte, byteValue);

  if (!validateHoldingWrite(regIndex, newRegValue)) return false;
  if (!applyHoldingSideEffects(regIndex, newRegValue)) return false;

  portENTER_CRITICAL(&s_regLock);
  s_holdingRegs[regIndex] = newRegValue;
  portEXIT_CRITICAL(&s_regLock);
  rcuWriteOutputBit((uint8_t)(addr / 8), (uint8_t)bitMask, enabled);
  return true;
}

// FC05 write one coil. Modbus uses 0xFF00 for ON and 0x0000 for OFF.
static size_t handleWriteSingleCoil(const uint8_t* requestPdu, size_t requestLen, uint8_t* responsePdu) {
  if (requestLen != 5) return makeException(responsePdu, MB_FC_WRITE_SINGLE_COIL, MB_EX_ILLEGAL_DATA_VALUE);

  uint16_t addr = be16Read(&requestPdu[1]);
  uint16_t value = be16Read(&requestPdu[3]);
  if (addr >= MODBUS_COIL_COUNT) return makeException(responsePdu, MB_FC_WRITE_SINGLE_COIL, MB_EX_ILLEGAL_DATA_ADDRESS);
  if (value != 0x0000 && value != 0xFF00) return makeException(responsePdu, MB_FC_WRITE_SINGLE_COIL, MB_EX_ILLEGAL_DATA_VALUE);
  if (!applyCoilWrite(addr, value == 0xFF00)) return makeException(responsePdu, MB_FC_WRITE_SINGLE_COIL, MB_EX_ILLEGAL_DATA_VALUE);

  memcpy(responsePdu, requestPdu, 5);
  return 5;
}

// FC0F write multiple coils. Each requested bit is applied individually so the
// same safety rules as FC05 are preserved for mixed-byte writes.
static size_t handleWriteMultipleCoils(const uint8_t* requestPdu, size_t requestLen, uint8_t* responsePdu) {
  if (requestLen < 6) return makeException(responsePdu, MB_FC_WRITE_MULTIPLE_COILS, MB_EX_ILLEGAL_DATA_VALUE);

  uint16_t start = be16Read(&requestPdu[1]);
  uint16_t quantity = be16Read(&requestPdu[3]);
  uint8_t byteCount = requestPdu[5];
  if (!checkRange(start, quantity, MODBUS_COIL_COUNT) ||
      quantity > 1968 || byteCount != (uint8_t)((quantity + 7) / 8) ||
      requestLen != (size_t)(6 + byteCount)) {
    return makeException(responsePdu, MB_FC_WRITE_MULTIPLE_COILS, MB_EX_ILLEGAL_DATA_VALUE);
  }

  for (uint16_t i = 0; i < quantity; ++i) {
    bool enabled = (requestPdu[6 + (i / 8)] & (uint8_t)(1U << (i % 8))) != 0;
    if (!applyCoilWrite((uint16_t)(start + i), enabled)) {
      return makeException(responsePdu, MB_FC_WRITE_MULTIPLE_COILS, MB_EX_ILLEGAL_DATA_VALUE);
    }
  }

  responsePdu[0] = MB_FC_WRITE_MULTIPLE_COILS;
  be16Write(&responsePdu[1], start);
  be16Write(&responsePdu[3], quantity);
  return 5;
}

// FC10 write multiple holding registers. Values are validated first, then side
// effects are checked, then the register image is committed in one locked block.
static size_t handleWriteMultipleRegs(const uint8_t* requestPdu, size_t requestLen, uint8_t* responsePdu) {
  if (requestLen < 6) return makeException(responsePdu, MB_FC_WRITE_MULTIPLE_REGS, MB_EX_ILLEGAL_DATA_VALUE);

  uint16_t start = be16Read(&requestPdu[1]);
  uint16_t quantity = be16Read(&requestPdu[3]);
  uint8_t byteCount = requestPdu[5];
  if (!checkRange(start, quantity, MODBUS_HOLDING_REG_COUNT) ||
      quantity > 123 || byteCount != quantity * 2 || requestLen != (size_t)(6 + byteCount)) {
    return makeException(responsePdu, MB_FC_WRITE_MULTIPLE_REGS, MB_EX_ILLEGAL_DATA_VALUE);
  }

  uint16_t values[MODBUS_HOLDING_REG_COUNT] = {};
  for (uint16_t i = 0; i < quantity; ++i) {
    values[i] = be16Read(&requestPdu[6 + (i * 2)]);
    if (!validateHoldingWrite(start + i, values[i])) {
      return makeException(responsePdu, MB_FC_WRITE_MULTIPLE_REGS, MB_EX_ILLEGAL_DATA_VALUE);
    }
  }

  for (uint16_t i = 0; i < quantity; ++i) {
    if (!applyHoldingSideEffects(start + i, values[i])) {
      return makeException(responsePdu, MB_FC_WRITE_MULTIPLE_REGS, MB_EX_ILLEGAL_DATA_VALUE);
    }
  }
  for (uint16_t i = 0; i < quantity; ++i) {
    if (!applyHoldingConfigWrites(start + i, values[i])) {
      return makeException(responsePdu, MB_FC_WRITE_MULTIPLE_REGS, MB_EX_ILLEGAL_DATA_VALUE);
    }
  }

  portENTER_CRITICAL(&s_regLock);
  for (uint16_t i = 0; i < quantity; ++i) {
    s_holdingRegs[start + i] = values[i];
  }
  portEXIT_CRITICAL(&s_regLock);
  for (uint16_t i = 0; i < quantity; ++i) {
    mirrorHoldingRegToRcuCache((uint16_t)(start + i), values[i]);
  }

  responsePdu[0] = MB_FC_WRITE_MULTIPLE_REGS;
  be16Write(&responsePdu[1], start);
  be16Write(&responsePdu[3], quantity);
  return 5;
}

// FC16 mask write register. Modbus computes the new value as:
//   (current AND and_mask) OR (or_mask AND NOT and_mask)
// After that it follows the same side-effect and mirror path as FC06/FC10.
static size_t handleMaskWriteReg(const uint8_t* requestPdu, size_t requestLen, uint8_t* responsePdu) {
  if (requestLen != 7) return makeException(responsePdu, MB_FC_MASK_WRITE_REG, MB_EX_ILLEGAL_DATA_VALUE);

  uint16_t addr = be16Read(&requestPdu[1]);
  if (addr >= MODBUS_HOLDING_REG_COUNT) return makeException(responsePdu, MB_FC_MASK_WRITE_REG, MB_EX_ILLEGAL_DATA_ADDRESS);

  uint16_t updated;
  portENTER_CRITICAL(&s_regLock);
  uint16_t current = s_holdingRegs[addr];
  portEXIT_CRITICAL(&s_regLock);
  updated = (current & be16Read(&requestPdu[3])) | (be16Read(&requestPdu[5]) & (uint16_t)~be16Read(&requestPdu[3]));

  if (!validateHoldingWrite(addr, updated)) return makeException(responsePdu, MB_FC_MASK_WRITE_REG, MB_EX_ILLEGAL_DATA_VALUE);
  if (!applyHoldingSideEffects(addr, updated)) return makeException(responsePdu, MB_FC_MASK_WRITE_REG, MB_EX_ILLEGAL_DATA_VALUE);
  if (!applyHoldingConfigWrites(addr, updated)) return makeException(responsePdu, MB_FC_MASK_WRITE_REG, MB_EX_ILLEGAL_DATA_VALUE);

  portENTER_CRITICAL(&s_regLock);
  s_holdingRegs[addr] = updated;
  portEXIT_CRITICAL(&s_regLock);
  mirrorHoldingRegToRcuCache(addr, updated);

  memcpy(responsePdu, requestPdu, 7);
  return 7;
}

// FC17 read/write multiple registers. Writes are committed first, then the read
// response is generated from the updated holding-register mirror.
static size_t handleReadWriteMultipleRegs(const uint8_t* requestPdu, size_t requestLen, uint8_t* responsePdu) {
  if (requestLen < 10) return makeException(responsePdu, MB_FC_READ_WRITE_MULTIPLE_REGS, MB_EX_ILLEGAL_DATA_VALUE);

  uint16_t readStart = be16Read(&requestPdu[1]);
  uint16_t readQuantity = be16Read(&requestPdu[3]);
  uint16_t writeStart = be16Read(&requestPdu[5]);
  uint16_t writeQuantity = be16Read(&requestPdu[7]);
  uint8_t writeByteCount = requestPdu[9];

  if (!checkRange(readStart, readQuantity, MODBUS_HOLDING_REG_COUNT) ||
      readQuantity > 125 ||
      !checkRange(writeStart, writeQuantity, MODBUS_HOLDING_REG_COUNT) ||
      writeQuantity > 121 ||
      writeByteCount != writeQuantity * 2 ||
      requestLen != (size_t)(10 + writeByteCount)) {
    return makeException(responsePdu, MB_FC_READ_WRITE_MULTIPLE_REGS, MB_EX_ILLEGAL_DATA_VALUE);
  }

  uint16_t values[MODBUS_HOLDING_REG_COUNT] = {};
  for (uint16_t i = 0; i < writeQuantity; ++i) {
    values[i] = be16Read(&requestPdu[10 + (i * 2)]);
    if (!validateHoldingWrite(writeStart + i, values[i])) {
      return makeException(responsePdu, MB_FC_READ_WRITE_MULTIPLE_REGS, MB_EX_ILLEGAL_DATA_VALUE);
    }
  }

  for (uint16_t i = 0; i < writeQuantity; ++i) {
    if (!applyHoldingSideEffects(writeStart + i, values[i])) {
      return makeException(responsePdu, MB_FC_READ_WRITE_MULTIPLE_REGS, MB_EX_ILLEGAL_DATA_VALUE);
    }
  }
  for (uint16_t i = 0; i < writeQuantity; ++i) {
    if (!applyHoldingConfigWrites(writeStart + i, values[i])) {
      return makeException(responsePdu, MB_FC_READ_WRITE_MULTIPLE_REGS, MB_EX_ILLEGAL_DATA_VALUE);
    }
  }

  responsePdu[0] = MB_FC_READ_WRITE_MULTIPLE_REGS;
  responsePdu[1] = (uint8_t)(readQuantity * 2);

  portENTER_CRITICAL(&s_regLock);
  for (uint16_t i = 0; i < writeQuantity; ++i) {
    s_holdingRegs[writeStart + i] = values[i];
  }
  for (uint16_t i = 0; i < readQuantity; ++i) {
    be16Write(&responsePdu[2 + (i * 2)], s_holdingRegs[readStart + i]);
  }
  portEXIT_CRITICAL(&s_regLock);
  for (uint16_t i = 0; i < writeQuantity; ++i) {
    mirrorHoldingRegToRcuCache((uint16_t)(writeStart + i), values[i]);
  }

  return 2 + (readQuantity * 2);
}

// Append one object to a Modbus MEI Read Device Identification response.
static size_t appendDeviceIdObject(uint8_t* responsePdu, size_t offset, uint8_t objectId, const char* value) {
  size_t len = strlen(value);
  if (len > 245) len = 245;

  responsePdu[offset++] = objectId;
  responsePdu[offset++] = (uint8_t)len;
  memcpy(&responsePdu[offset], value, len);
  return offset + len;
}

// FC2B/0E Read Device Identification. Returns hotel/model/firmware strings so a
// Modbus client can identify the gateway without reading custom registers.
static size_t handleReadDeviceId(const uint8_t* requestPdu, size_t requestLen, uint8_t* responsePdu) {
  if (requestLen != 4 || requestPdu[1] != MB_MEI_READ_DEVICE_ID) {
    return makeException(responsePdu, MB_FC_ENCAPSULATED_INTERFACE, MB_EX_ILLEGAL_DATA_VALUE);
  }

  uint8_t readDeviceIdCode = requestPdu[2];
  uint8_t objectId = requestPdu[3];
  if (readDeviceIdCode < 0x01 || readDeviceIdCode > 0x04 || objectId > 0x02) {
    return makeException(responsePdu, MB_FC_ENCAPSULATED_INTERFACE, MB_EX_ILLEGAL_DATA_VALUE);
  }

  responsePdu[0] = MB_FC_ENCAPSULATED_INTERFACE;
  responsePdu[1] = MB_MEI_READ_DEVICE_ID;
  responsePdu[2] = readDeviceIdCode;
  responsePdu[3] = 0x01;
  responsePdu[4] = 0x00;
  responsePdu[5] = 0x00;
  responsePdu[6] = 0x03;

  size_t offset = 7;
  offset = appendDeviceIdObject(responsePdu, offset, 0x00, HOTEL_ID);
  offset = appendDeviceIdObject(responsePdu, offset, 0x01, GATEWAY_MODEL);
  offset = appendDeviceIdObject(responsePdu, offset, 0x02, GATEWAY_FW_VERSION);
  return offset;
}

// Dispatch one Modbus PDU to the matching function-code handler. Handlers return
// only PDU length; MBAP framing is added later by modbusClientLoop().
static size_t modbusHandlePdu(const uint8_t* requestPdu, size_t requestLen, uint8_t* responsePdu) {
  if (requestLen == 0) return 0;

  uint8_t functionCode = requestPdu[0];
  switch (functionCode) {
    case MB_FC_READ_COILS:
      return handleReadCoils(requestPdu, requestLen, responsePdu);
    case MB_FC_READ_DISCRETE_INPUTS:
      return handleReadBits(functionCode, s_discreteInputs, MODBUS_DISCRETE_INPUT_COUNT, requestPdu, requestLen, responsePdu);
    case MB_FC_READ_HOLDING_REGS:
      return handleReadRegs(functionCode, s_holdingRegs, MODBUS_HOLDING_REG_COUNT, requestPdu, requestLen, responsePdu);
    case MB_FC_READ_INPUT_REGS:
      return handleReadRegs(functionCode, s_inputRegs, MODBUS_INPUT_REG_COUNT, requestPdu, requestLen, responsePdu);
    case MB_FC_WRITE_SINGLE_COIL:
      return handleWriteSingleCoil(requestPdu, requestLen, responsePdu);
    case MB_FC_WRITE_SINGLE_REG:
      return handleWriteSingleReg(requestPdu, requestLen, responsePdu);
    case MB_FC_DIAGNOSTICS:
      return handleDiagnostics(requestPdu, requestLen, responsePdu);
    case MB_FC_WRITE_MULTIPLE_COILS:
      return handleWriteMultipleCoils(requestPdu, requestLen, responsePdu);
    case MB_FC_WRITE_MULTIPLE_REGS:
      return handleWriteMultipleRegs(requestPdu, requestLen, responsePdu);
    case MB_FC_MASK_WRITE_REG:
      return handleMaskWriteReg(requestPdu, requestLen, responsePdu);
    case MB_FC_READ_WRITE_MULTIPLE_REGS:
      return handleReadWriteMultipleRegs(requestPdu, requestLen, responsePdu);
    case MB_FC_ENCAPSULATED_INTERFACE:
      return handleReadDeviceId(requestPdu, requestLen, responsePdu);
    default:
      return makeException(responsePdu, functionCode, MB_EX_ILLEGAL_FUNCTION);
  }
}

// Process one connected Modbus TCP client. The loop reads MBAP header, validates
// protocol/unit id, dispatches the PDU, then writes MBAP + response PDU back.
static void modbusClientLoop(int sock) {
  uint8_t header[7];
  uint8_t body[MODBUS_MAX_BODY];
  uint8_t response[7 + MODBUS_MAX_PDU];

  while (true) {
    int read = recvAll(sock, header, sizeof(header));
    if (read <= 0) break;

    uint16_t transactionId = be16Read(&header[0]);
    uint16_t protocolId = be16Read(&header[2]);
    uint16_t length = be16Read(&header[4]);
    uint8_t unitId = header[6];

    if (protocolId != MODBUS_PROTO_ID || length < 2 || length > MODBUS_MAX_BODY) {
      ESP_LOGW(TAG, "Invalid MBAP transaction=%u proto=%u length=%u", transactionId, protocolId, length);
      break;
    }

    read = recvAll(sock, body, length - 1);
    if (read <= 0) break;

    uint8_t expectedUnitId = currentUnitId();
    if (unitId != expectedUnitId) {
      ESP_LOGD(TAG, "Ignoring Modbus request for unit=%u, configured unit=%u", unitId, expectedUnitId);
      continue;
    }

    size_t responsePduLen = modbusHandlePdu(body, length - 1, &response[7]);
    if (responsePduLen == 0) continue;

    be16Write(&response[0], transactionId);
    be16Write(&response[2], MODBUS_PROTO_ID);
    be16Write(&response[4], (uint16_t)(responsePduLen + 1));
    response[6] = unitId;

    if (sendAll(sock, response, 7 + responsePduLen) <= 0) {
      ESP_LOGW(TAG, "Modbus response send failed errno=%d", errno);
      break;
    }
  }
}

// FreeRTOS task wrapper for a single Modbus client socket. A counting semaphore
// controls how many of these tasks may run at once.
static void modbusClientTask(void* arg) {
  ModbusClientCtx* client = (ModbusClientCtx*)arg;
  ESP_LOGI(TAG, "Modbus TCP client connected from %s", inet_ntoa(client->sourceAddr.sin_addr));
  modbusClientLoop(client->sock);
  ESP_LOGI(TAG, "Modbus TCP client disconnected");

  shutdown(client->sock, 0);
  close(client->sock);
  xSemaphoreGive(s_clientSlots);
  free(client);
  vTaskDelete(nullptr);
}

// Main Modbus TCP listener task. It owns the listening socket, accepts clients,
// configures their sockets, and starts one client task per accepted connection.
static void modbusTcpTask(void* arg) {
  (void)arg;

  while (true) {
    struct sockaddr_in listenAddr = {};
    listenAddr.sin_family = AF_INET;
    listenAddr.sin_port = htons(MODBUS_TCP_PORT);
    listenAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    int listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (listenSock < 0) {
      ESP_LOGE(TAG, "Unable to create socket errno=%d", errno);
      vTaskDelay(pdMS_TO_TICKS(3000));
      continue;
    }

    int opt = 1;
    setSocketOptWarn(listenSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt), "SO_REUSEADDR");

    struct timeval pollTimeout = {};
    pollTimeout.tv_sec = MODBUS_SOCKET_POLL_TIMEOUT_SEC;
    setSocketOptWarn(listenSock, SOL_SOCKET, SO_RCVTIMEO, &pollTimeout, sizeof(pollTimeout), "SO_RCVTIMEO");

    if (bind(listenSock, (struct sockaddr*)&listenAddr, sizeof(listenAddr)) != 0) {
      ESP_LOGE(TAG, "Socket bind failed port=%d errno=%d", MODBUS_TCP_PORT, errno);
      close(listenSock);
      vTaskDelay(pdMS_TO_TICKS(3000));
      continue;
    }

    if (listen(listenSock, MODBUS_TCP_BACKLOG) != 0) {
      ESP_LOGE(TAG, "Socket listen failed errno=%d", errno);
      close(listenSock);
      vTaskDelay(pdMS_TO_TICKS(3000));
      continue;
    }

    ESP_LOGI(TAG, "Modbus TCP server listening on port %d unit=%d", MODBUS_TCP_PORT, MODBUS_UNIT_ID);
    ESP_LOGI(TAG, "Register map: coils 0-%d, discrete 0-%d, holding 0-%d, input 0-%d",
             MODBUS_COIL_COUNT - 1,
             MODBUS_DISCRETE_INPUT_COUNT - 1,
             MODBUS_HOLDING_REG_COUNT - 1,
             MODBUS_INPUT_REG_COUNT - 1);

    while (true) {
      struct sockaddr_in sourceAddr = {};
      socklen_t addrLen = sizeof(sourceAddr);
      int sock = accept(listenSock, (struct sockaddr*)&sourceAddr, &addrLen);
      if (sock < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
        ESP_LOGW(TAG, "Unable to accept connection errno=%d", errno);
        break;
      }

      configureClientSocket(sock);

      if (xSemaphoreTake(s_clientSlots, 0) != pdTRUE) {
        ESP_LOGW(TAG, "Rejecting Modbus TCP client: max clients reached");
        shutdown(sock, 0);
        close(sock);
        continue;
      }

      ModbusClientCtx* client = (ModbusClientCtx*)calloc(1, sizeof(*client));
      if (!client) {
        ESP_LOGE(TAG, "Unable to allocate Modbus client context");
        xSemaphoreGive(s_clientSlots);
        shutdown(sock, 0);
        close(sock);
        continue;
      }

      client->sock = sock;
      client->sourceAddr = sourceAddr;
      BaseType_t created = xTaskCreate(modbusClientTask, "modbus_client", MODBUS_CLIENT_TASK_STACK, client, 5, nullptr);
      if (created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create Modbus client task");
        free(client);
        xSemaphoreGive(s_clientSlots);
        shutdown(sock, 0);
        close(sock);
        continue;
      }
    }

    close(listenSock);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// Initialize the Modbus register image at boot. Holding/input registers start at
// zero; discrete input 1 is set to show the Modbus task has initialized.
static void initRegisters() {
  portENTER_CRITICAL(&s_regLock);
  memset(s_holdingRegs, 0, sizeof(s_holdingRegs));
  memset(s_inputRegs, 0, sizeof(s_inputRegs));
  memset(s_discreteInputs, 0, sizeof(s_discreteInputs));

  s_holdingRegs[NODE_ID_HOLDING_REG] =
      updatePairByte(s_holdingRegs[NODE_ID_HOLDING_REG], NODE_ID_HOLDING_REG_HIGH_BYTE, rcuGetNodeId());
  s_discreteInputs[1] = true;
  portEXIT_CRITICAL(&s_regLock);
}

// Update board-ID validity in discrete input 3. The raw values are currently not
// exported as Modbus registers, but the signature is kept for future expansion.
void modbusTcpSetBoardInfo(uint8_t boardId, uint8_t dipRaw, bool valid) {
  (void)boardId;
  (void)dipRaw;
  portENTER_CRITICAL(&s_regLock);
  s_discreteInputs[3] = valid;
  portEXIT_CRITICAL(&s_regLock);
}

// Update network-online status in discrete input 0. Ethernet link/IP events call
// this so BMS software can distinguish room-state issues from network loss.
void modbusTcpSetNetworkOnline(bool online) {
  portENTER_CRITICAL(&s_regLock);
  s_discreteInputs[0] = online;
  portEXIT_CRITICAL(&s_regLock);
}

// Publish the latest room cache into the Modbus register mirror. Inpbuf/Outbuf
// are packed in byte pairs using the project register-map convention.
void modbusTcpSetRoomState(const RoomCache& room) {
  portENTER_CRITICAL(&s_regLock);
  for (uint16_t i = 0; i < MODBUS_INPUT_REG_COUNT; ++i) {
    s_inputRegs[i] = packPair(room.inpbuf, RCU_INPBUF_COUNT, i);
  }
  for (uint16_t i = 0; i < MODBUS_HOLDING_REG_COUNT; ++i) {
    s_holdingRegs[i] = packPair(room.outbuf, RCU_OUTBUF_COUNT, i);
  }
  s_discreteInputs[2] = room.rcuOnline;
  portEXIT_CRITICAL(&s_regLock);
}

// Public start function used by app_main(). It initializes the register image,
// creates the client-slot semaphore, and starts the TCP listener task.
esp_err_t modbusTcpStart(void) {
  initRegisters();

  if (!s_clientSlots) {
    s_clientSlots = xSemaphoreCreateCounting(MODBUS_MAX_CLIENTS, MODBUS_MAX_CLIENTS);
    if (!s_clientSlots) {
      ESP_LOGE(TAG, "Failed to create Modbus client semaphore");
      return ESP_ERR_NO_MEM;
    }
  }

  BaseType_t created = xTaskCreate(modbusTcpTask, "modbus_tcp", 6144, nullptr, 5, nullptr);
  if (created != pdPASS) {
    ESP_LOGE(TAG, "Failed to create Modbus TCP task");
    return ESP_ERR_NO_MEM;
  }

  return ESP_OK;
}
