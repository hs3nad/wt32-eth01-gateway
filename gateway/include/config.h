#pragma once
#include <stdint.h>

#include "register_map.h"

#define HOTEL_ID "amphan"
#define GATEWAY_ID "Room001"
#define GATEWAY_HOSTNAME GATEWAY_ID
#define FLOOR_ID 2
#define GATEWAY_MODEL "WT32-ETH01"
#define GATEWAY_FW_VERSION "0.4.1"
#define GATEWAY_PROTOCOL_MODE "modbus_tcp"

#define GATEWAY_OTA_ENABLED 1
#define GATEWAY_OTA_MANIFEST_URL "https://raw.githubusercontent.com/hs3nad/wt32-eth01-gateway/main/docs/gateway_ota.json"
#define GATEWAY_OTA_CHECK_INTERVAL_MS (6UL * 60UL * 60UL * 1000UL)
#define GATEWAY_OTA_FIRST_CHECK_DELAY_MS (30UL * 1000UL)

#define RS485_CH1_TX_PIN 17
#define RS485_CH1_RX_PIN 5
#define RS485_CH1_EN_PIN 33

#define RS485_CH2_TX_PIN 1
#define RS485_CH2_RX_PIN 3
#define RS485_CH2_EN_PIN 14

#define RS485_TX_PIN RS485_CH1_TX_PIN
#define RS485_RX_PIN RS485_CH1_RX_PIN
#define RS485_EN_PIN RS485_CH1_EN_PIN
#define RS485_USE_EN 1
#define RS485_BAUD 9600
#define ETH_RCU_DEVICE_ID 0x31

#define MCP23017_I2C_SDA_PIN 32
#define MCP23017_I2C_SCL_PIN 4
#define MCP23017_I2C_ADDR 0x20
#define MCP23017_I2C_HZ 100000

#define ETH_PHY_ADDR 1
#define ETH_PHY_POWER_PIN 16
#define ETH_PHY_MDC_PIN 23
#define ETH_PHY_MDIO_PIN 18

#define MODBUS_TCP_PORT 502
#define MODBUS_UNIT_ID 1
#define MODBUS_HOLDING_REG_COUNT RCU_OUTBUF_REG_COUNT
#define MODBUS_INPUT_REG_COUNT RCU_INPBUF_REG_COUNT
#define MODBUS_COIL_COUNT RCU_OUTBUF_COIL_COUNT
#define MODBUS_DISCRETE_INPUT_COUNT 16
#define MODBUS_CLIENT_IDLE_TIMEOUT_SEC 300
#define ROOM_POLL_INTERVAL_MS 100UL
#define RCU_OFFLINE_MISSES 3
#define SERIAL_RESPONSE_TIMEOUT_MS 250UL

struct RcuNode {
  const char* room;
  uint8_t aa;
};

static const RcuNode RCU_NODES[] = {
  // Fixed PS18V62_2 frames do not carry room addresses. This entry is only the
  // local cache slot; the primary room/node number is Outbuf[11] in gateway NVS.
  {"201", 0x01},
};

static const int RCU_NODE_COUNT = sizeof(RCU_NODES) / sizeof(RCU_NODES[0]);
