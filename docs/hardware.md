# Hardware

This hardware setup supports one integration path:

```text
WT32-ETH01 Ethernet -> Modbus TCP
WT32-ETH01 UART2    -> CH1 PS18V62_2 RCU RS485 bus
```

The WT32-ETH01 is treated as a separate ETH device on the RCU bus. The default
device ID is `0x31`.

## Gateway Board

Target board:

```text
WT32-ETH01
ESP32 + LAN8720 Ethernet PHY
```

Detected flash on the current module:

```text
Detected flash size = 8MB = 64Mbit
```

The gateway firmware is configured for this 8MB flash size and uses two OTA app
partitions.

Ethernet pins configured in `gateway/include/config.h`:

```text
PHY address = 1
MDC         = GPIO23
MDIO        = GPIO18
PHY power   = GPIO16
```

## UART / RS485

Gateway UART pins:

```text
CH1 RS485:
GPIO17 = UART2 TX -> RS485 DI
GPIO5  = UART2 RX <- RS485 RO
GPIO33 = RS485 DE/RE

CH2 RS485 via SN3257:
GPIO1  = UART0 TX -> SN3257 / RS485 DI
GPIO3  = UART0 RX <- SN3257 / RS485 RO
GPIO14 = RS485 DE/RE
```

Current firmware default:

```c
#define RS485_USE_EN 1
```

The active PS18V62_2 bus currently uses CH1/UART2. CH2 is defined in
`config.h` for the SN3257 path, but it is not used by the runtime yet.
GPIO1/GPIO3 are also ESP32 UART0 console pins, so using CH2 for runtime traffic
will require moving or disabling the console.

## I2C

MCP23017 pins:

```text
GPIO32 = SDA
GPIO4  = SCL
```

The MCP23017 Port A DIP switches are used as the board ID. GPA0-GPA7 map to
DIP1-DIP8 with active-low logic (`OFF=1`, `ON=0`), so firmware reports:

```text
board_id = (~GPIOA) & 0xFF
```

Modbus telemetry includes `board_id_valid` as discrete input `3`. The Modbus
input registers remain reserved for the RCU `Inpbuf[0..5]` map.

If MCP23017/DIP read fails, firmware uses fallback `board_id = 1` and reports
`board_id_valid = false`.

## Boot

```text
GPIO0 = BOOT only
```

## Bus Settings

```text
Baud rate = 9600
Data bits = 8
Parity    = none
Stop bits = 1
```

## Room Mapping

PS18V62_2 fixed 8-byte frames do not contain a room address. The runtime model
is therefore one gateway per room bus:

```c
static const RcuNode RCU_NODES[] = {
  {"201", 0x01},
};
```

`"201"` is only the local cache label. The gateway-owned runtime node ID in
holding register `5` high byte (`Outbuf[11]`) is the primary room/node number
for Modbus/BMS use, and the gateway stores that byte in ESP32 flash using NVS.
PS18V62_2 fixed frames still do not carry a room address.
