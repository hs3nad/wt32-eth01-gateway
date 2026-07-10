#include <string.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_eth.h"
#include "esp_eth_mac.h"
#include "esp_eth_netif_glue.h"
#include "esp_eth_phy.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "lwip/apps/netbiosns.h"

#include "config.h"
#include "modbus_tcp.h"
#include "ota_update.h"
#include "rcu_client.h"
#include "state_cache.h"

static const char* TAG = "gateway";
static constexpr uint32_t GATEWAY_LOOP_DELAY_MS = 5;
static constexpr EventBits_t ETH_READY_BIT = BIT0;

static_assert(RCU_NODE_COUNT == 1, "PS18V62_2 fixed frames do not include room addresses; use one gateway per room bus");
static_assert(sizeof(GATEWAY_HOSTNAME) <= 16, "NetBIOS hostname must be 15 characters or less");

static esp_netif_t* ethNetif = nullptr;
static EventGroupHandle_t gatewayEvents = nullptr;
static RoomCache rooms[RCU_NODE_COUNT];
static uint8_t roomMisses[RCU_NODE_COUNT];
static uint32_t lastRoomPoll = 0;
static size_t nextPollRoom = 0;
static uint8_t boardId = 1;
static uint8_t boardDipRaw = 0xFF;
static bool boardIdValid = false;
static i2c_master_bus_handle_t i2cBus = nullptr;
static i2c_master_dev_handle_t mcpDev = nullptr;

// Return the ESP timer in milliseconds. All gateway timing uses this helper so
// offline checks and poll intervals share the same monotonic clock source.
static uint32_t nowMs() {
  return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

// Disable the ESP-IDF task watchdog while the gateway is still being debugged on
// real hardware. This prevents long serial/network tests from resetting the MCU.
static void disableTaskWatchdogForDebug() {
  esp_err_t err = esp_task_wdt_deinit();
  if (err == ESP_OK) {
    ESP_LOGW(TAG, "Task watchdog disabled for debugging");
  } else if (err != ESP_ERR_INVALID_STATE) {
    ESP_LOGW(TAG, "Task watchdog disable failed: %s", esp_err_to_name(err));
  }
}

// Start NetBIOS name service so Windows LAN scanners can display the configured
// room name, for example "Room001", instead of showing only the IP address.
static void initNetworkNameResponder() {
  netbiosns_init();
  netbiosns_set_name(GATEWAY_HOSTNAME);
  ESP_LOGI(TAG, "Network hostname: %s", GATEWAY_HOSTNAME);
}

// Write one MCP23017 register. The board ID DIP switch is read through this I2C
// expander, so these tiny helpers keep the init path compact and readable.
static esp_err_t mcpWriteReg(uint8_t reg, uint8_t value) {
  if (!mcpDev) return ESP_ERR_INVALID_STATE;
  uint8_t data[2] = {reg, value};
  return i2c_master_transmit(mcpDev, data, sizeof(data), 100);
}

// Read one MCP23017 register. A missing MCP23017 is not fatal; the gateway falls
// back to boardId=1 and reports board_id_valid=false to Modbus.
static esp_err_t mcpReadReg(uint8_t reg, uint8_t* value) {
  if (!value) return ESP_ERR_INVALID_ARG;
  if (!mcpDev) return ESP_ERR_INVALID_STATE;
  return i2c_master_transmit_receive(mcpDev, &reg, 1, value, 1, 100);
}

// Initialize I2C and read the board DIP switch from MCP23017 GPIOA. The DIP is
// active-low, so the raw byte is inverted before becoming boardId.
static void initBoardIdFromDip() {
  i2c_master_bus_config_t busConfig = {};
  busConfig.i2c_port = I2C_NUM_0;
  busConfig.sda_io_num = (gpio_num_t)MCP23017_I2C_SDA_PIN;
  busConfig.scl_io_num = (gpio_num_t)MCP23017_I2C_SCL_PIN;
  busConfig.clk_source = I2C_CLK_SRC_DEFAULT;
  busConfig.glitch_ignore_cnt = 7;
  busConfig.flags.enable_internal_pullup = true;

  esp_err_t err = i2c_new_master_bus(&busConfig, &i2cBus);
  if (err == ESP_OK) {
    i2c_device_config_t devConfig = {};
    devConfig.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    devConfig.device_address = MCP23017_I2C_ADDR;
    devConfig.scl_speed_hz = MCP23017_I2C_HZ;
    err = i2c_master_bus_add_device(i2cBus, &devConfig, &mcpDev);
  }
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "I2C init failed: %s; using fallback board ID %u", esp_err_to_name(err), boardId);
    return;
  }

  static constexpr uint8_t MCP_IODIRA = 0x00;
  static constexpr uint8_t MCP_GPPUA = 0x0C;
  static constexpr uint8_t MCP_GPIOA = 0x12;

  err = mcpWriteReg(MCP_IODIRA, 0xFF);
  if (err == ESP_OK) err = mcpWriteReg(MCP_GPPUA, 0xFF);
  if (err == ESP_OK) err = mcpReadReg(MCP_GPIOA, &boardDipRaw);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "MCP23017 DIP read failed: %s; using fallback board ID %u", esp_err_to_name(err), boardId);
    return;
  }

  boardId = (uint8_t)~boardDipRaw;
  boardIdValid = true;
  ESP_LOGI(TAG, "Board ID from DIP: %u raw=0x%02X", boardId, boardDipRaw);
}

// Publish the current room snapshot plus local board information into the
// Modbus server registers. This is the single bridge from gateway state to the
// Modbus TCP register image.
static void publishRoomToModbus(RoomCache& r) {
  modbusTcpSetBoardInfo(boardId, boardDipRaw, boardIdValid);
  modbusTcpSetRoomState(r);
}

// Copy a decoded RS485 bus event into the room cache. The RCU bus is treated as
// the physical state source in Phase 1, so any accepted bus event immediately
// refreshes the Modbus mirror.
static void applyBusEventToRoom(RoomCache& r, const RcuBusEvent& ev) {
  memcpy(r.inpbuf, ev.inpbuf, sizeof(r.inpbuf));
  memcpy(r.outbuf, ev.outbuf, sizeof(r.outbuf));
  memcpy(r.lastFrame, ev.frame, sizeof(r.lastFrame));
  r.lastEventId++;
  r.rcuOnline = true;
  r.lastSeenMs = nowMs();
  publishRoomToModbus(r);
}

// Update only the online flag for the room and republish it. Register data is
// left as-is so a temporary bus silence does not clear useful cached state.
static void markRcuOnline(RoomCache& r, bool online) {
  r.rcuOnline = online;
  r.lastSeenMs = nowMs();
  publishRoomToModbus(r);
}

// Pull the latest cached RCU snapshot from rcu_client. A missing snapshot counts
// as a miss; after enough misses the room is marked offline while the previous
// register values remain available to Modbus.
static bool syncRoom(RoomCache& r) {
  uint8_t inpbuf[RCU_INPBUF_COUNT] = {};
  uint8_t outbuf[RCU_OUTBUF_COUNT] = {};
  int idx = (int)(&r - rooms);

  if (!rcuSnapshot(r.aa, FLOOR_ID, inpbuf, outbuf)) {
    if (idx >= 0 && idx < RCU_NODE_COUNT && roomMisses[idx] < UINT8_MAX) roomMisses[idx]++;
    if (idx < 0 || idx >= RCU_NODE_COUNT || roomMisses[idx] >= RCU_OFFLINE_MISSES) markRcuOnline(r, false);
    return false;
  }

  if (idx >= 0 && idx < RCU_NODE_COUNT) roomMisses[idx] = 0;
  memcpy(r.inpbuf, inpbuf, sizeof(r.inpbuf));
  memcpy(r.outbuf, outbuf, sizeof(r.outbuf));
  r.rcuOnline = rcuPing(r.aa, FLOOR_ID);
  r.lastSeenMs = nowMs();
  publishRoomToModbus(r);
  return true;
}

// Non-blocking receive path for RS485. If one complete 8-byte bus frame is
// decoded by rcu_client, mirror it into room 0 because this firmware supports
// one room per local PS18V62_2 bus.
static void handleIncomingEvent() {
  RcuBusEvent ev;
  if (!rcuReadBusEvent(&ev) || RCU_NODE_COUNT <= 0) return;
  applyBusEventToRoom(rooms[0], ev);
}

// Periodic polling path. With fixed PS18V62_2 frames there is no room address in
// the frame, but this shape keeps the code ready if RCU_NODE_COUNT ever changes.
static void pollOneRoom() {
  if (RCU_NODE_COUNT <= 0) return;
  RoomCache& r = rooms[nextPollRoom];
  nextPollRoom = (nextPollRoom + 1) % RCU_NODE_COUNT;
  syncRoom(r);
}

// Ethernet link callback. Link down clears the network-online discrete input so
// a BMS can see that Modbus connectivity is not currently usable.
static void ethEventHandler(void* arg, esp_event_base_t base, int32_t eventId, void* eventData) {
  (void)arg;
  (void)base;
  (void)eventData;
  switch (eventId) {
    case ETHERNET_EVENT_CONNECTED:
      ESP_LOGI(TAG, "Ethernet link up");
      break;
    case ETHERNET_EVENT_DISCONNECTED:
      ESP_LOGW(TAG, "Ethernet link down");
      xEventGroupClearBits(gatewayEvents, ETH_READY_BIT);
      modbusTcpSetNetworkOnline(false);
      break;
    case ETHERNET_EVENT_STOP:
      xEventGroupClearBits(gatewayEvents, ETH_READY_BIT);
      modbusTcpSetNetworkOnline(false);
      break;
    default:
      break;
  }
}

// DHCP/IP callback. Once an IP is assigned the Modbus server can be reached, so
// discrete input 0 is set through modbusTcpSetNetworkOnline(true).
static void ipEventHandler(void* arg, esp_event_base_t base, int32_t eventId, void* eventData) {
  (void)arg;
  (void)base;
  if (eventId != IP_EVENT_ETH_GOT_IP) return;
  ip_event_got_ip_t* event = (ip_event_got_ip_t*)eventData;
  ESP_LOGI(TAG, "Ethernet got IP: " IPSTR, IP2STR(&event->ip_info.ip));
  xEventGroupSetBits(gatewayEvents, ETH_READY_BIT);
  modbusTcpSetNetworkOnline(true);
  otaUpdateNotifyNetworkReady();
}

// Bring up the WT32-ETH01 LAN8720 Ethernet interface. The hostname is set before
// start so DHCP and NetBIOS both advertise the configured room/gateway name.
static void startEthernet() {
  ESP_ERROR_CHECK(gpio_set_direction((gpio_num_t)ETH_PHY_POWER_PIN, GPIO_MODE_OUTPUT));
  ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)ETH_PHY_POWER_PIN, 1));
  vTaskDelay(pdMS_TO_TICKS(100));

  esp_netif_config_t netifConfig = ESP_NETIF_DEFAULT_ETH();
  ethNetif = esp_netif_new(&netifConfig);
  ESP_ERROR_CHECK(ethNetif ? ESP_OK : ESP_ERR_NO_MEM);
  ESP_ERROR_CHECK(esp_netif_set_hostname(ethNetif, GATEWAY_HOSTNAME));

  eth_mac_config_t macConfig = ETH_MAC_DEFAULT_CONFIG();
  eth_phy_config_t phyConfig = ETH_PHY_DEFAULT_CONFIG();
  phyConfig.phy_addr = ETH_PHY_ADDR;
  phyConfig.reset_gpio_num = -1;

  eth_esp32_emac_config_t esp32MacConfig = ETH_ESP32_EMAC_DEFAULT_CONFIG();
  esp32MacConfig.smi_gpio.mdc_num = ETH_PHY_MDC_PIN;
  esp32MacConfig.smi_gpio.mdio_num = ETH_PHY_MDIO_PIN;

  esp_eth_mac_t* mac = esp_eth_mac_new_esp32(&esp32MacConfig, &macConfig);
  esp_eth_phy_t* phy = esp_eth_phy_new_generic(&phyConfig);
  ESP_ERROR_CHECK(mac ? ESP_OK : ESP_ERR_NO_MEM);
  ESP_ERROR_CHECK(phy ? ESP_OK : ESP_ERR_NO_MEM);
  esp_eth_config_t ethConfig = ETH_DEFAULT_CONFIG(mac, phy);
  esp_eth_handle_t ethHandle = nullptr;

  ESP_ERROR_CHECK(esp_eth_driver_install(&ethConfig, &ethHandle));
  esp_eth_netif_glue_handle_t ethGlue = esp_eth_new_netif_glue(ethHandle);
  ESP_ERROR_CHECK(ethGlue ? ESP_OK : ESP_ERR_NO_MEM);
  ESP_ERROR_CHECK(esp_netif_attach(ethNetif, ethGlue));
  ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, ethEventHandler, nullptr));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, ipEventHandler, nullptr));
  ESP_ERROR_CHECK(esp_eth_start(ethHandle));
}

// Main background task: continuously drains RS485 bus frames, then periodically
// republishes the latest cached room state into Modbus.
static void gatewayTask(void* arg) {
  (void)arg;
  while (true) {
    handleIncomingEvent();

    uint32_t now = nowMs();
    if (now - lastRoomPoll > ROOM_POLL_INTERVAL_MS) {
      lastRoomPoll = now;
      pollOneRoom();
    }

    vTaskDelay(pdMS_TO_TICKS(GATEWAY_LOOP_DELAY_MS));
  }
}

// Initialize every configured room cache entry and publish the empty initial
// state. This makes Modbus registers valid even before the first RCU frame.
static void initRoomCache() {
  for (int i = 0; i < RCU_NODE_COUNT; ++i) {
    rooms[i] = {RCU_NODES[i].room, RCU_NODES[i].aa, false, {}, {}, {}, 0, 0};
    rooms[i].outbuf[RCU_OUTBUF_NODE_ID] = rcuGetNodeId();
    roomMisses[i] = 0;
    publishRoomToModbus(rooms[i]);
  }
}

// ESP-IDF entry point. The order matters: initialize storage/network services,
// prepare board/RCU/Modbus state, start Ethernet, then run the gateway task.
extern "C" void app_main(void) {
  esp_err_t nvsErr = nvs_flash_init();
  if (nvsErr == ESP_ERR_NVS_NO_FREE_PAGES || nvsErr == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    nvsErr = nvs_flash_init();
  }
  ESP_ERROR_CHECK(nvsErr);
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  disableTaskWatchdogForDebug();
  initNetworkNameResponder();

  gatewayEvents = xEventGroupCreate();
  ESP_ERROR_CHECK(gatewayEvents ? ESP_OK : ESP_ERR_NO_MEM);

  initBoardIdFromDip();
  rcuBegin();
  ESP_ERROR_CHECK(modbusTcpStart());
  otaUpdateStart();
  initRoomCache();

  startEthernet();

  BaseType_t gatewayTaskCreated = xTaskCreate(gatewayTask, "gateway_task", 6144, nullptr, 5, nullptr);
  ESP_ERROR_CHECK(gatewayTaskCreated == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);
}
