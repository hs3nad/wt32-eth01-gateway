# Gateway Firmware

ESP-IDF firmware for WT32-ETH01. This firmware exposes the PS18V62_2 RCU room
bus as a Modbus TCP server.

## Runtime

```text
Ethernet  = LAN8720 via esp_eth
Modbus    = TCP server, port 502, unit id 1
UART      = UART2, 9600 8N1
Protocol  = PS18V62_2 fixed 8-byte frames
Device ID = ETH_RCU_DEVICE_ID 0x31
```

## Build

```bash
cd gateway
source /home/kaina/esp/esp-idf/export.sh
./build.sh
```

Flash:

```bash
PORT=/dev/ttyUSB0 ./flash.sh
```

## Configuration

Edit `include/config.h`:

```c
#define HOTEL_ID "amphan"
#define GATEWAY_ID "Room001"
#define GATEWAY_HOSTNAME GATEWAY_ID
#define FLOOR_ID 2
#define GATEWAY_FW_VERSION "0.4.2"
#define MODBUS_TCP_PORT 502
#define MODBUS_UNIT_ID 1
#define ETH_RCU_DEVICE_ID 0x31
#define GATEWAY_OTA_MANIFEST_URL "https://raw.githubusercontent.com/hs3nad/wt32-eth01-gateway/main/docs/gateway_ota.json"
```

`GATEWAY_HOSTNAME` is the DHCP hostname shown on the LAN. By default it follows
`GATEWAY_ID`, so changing `GATEWAY_ID` to another room name also changes the
network name. The firmware also responds to NetBIOS name queries with the same
name, which helps Windows LAN scanners such as Advanced IP Scanner display it.

`GATEWAY_OTA_MANIFEST_URL` points to the public raw GitHub JSON manifest used by
auto OTA. Leave it empty to disable update checks. The tested OTA path updated a
real gateway from `0.4.1` to `0.4.2`, wrote the new image to `ota_1`, and
rebooted from offset `0x400000`. See
[`../docs/ota_update.md`](../docs/ota_update.md).

## Modbus Register Map

The source-of-truth register map is
[`../docs/register_map.md`](../docs/register_map.md). The gateway and `rcu`
use the same `Inpbuf`/`Outbuf` byte layout.

Supported functions match `esp32-p4-gateway`: `01`, `02`, `03`, `04`, `05`,
`06`, `08`, `0F`, `10`, `16`, `17`, and `2B/0E`.

```text
Input registers
0 = high Inpbuf[1] input 9..16, low Inpbuf[0] service/master input bits
1 = high Inpbuf[3] input 25..32, low Inpbuf[2] input 17..24
2 = high Inpbuf[5] input 41..48, low Inpbuf[4] input 33..40

Holding registers
0 = high Outbuf[1] lamp output 9..16, low Outbuf[0] lamp output 1..8
1 = high Outbuf[3] lamp output 25..32, low Outbuf[2] lamp output 17..24
2 = high Outbuf[5] air control, low Outbuf[4] control bits
3 = high Outbuf[7] real hour, low Outbuf[6] actual temperature
4 = high Outbuf[9] alarm hour, low Outbuf[8] real minute
5 = high Outbuf[11] node id, low Outbuf[10] alarm minute

Discrete inputs
0 = Ethernet has IP
1 = Modbus server task initialized
2 = RCU bus online
3 = board_id_valid

Coils
0..87 = Outbuf[0]..Outbuf[10] bits, bit order is LSB first
coil address = (Outbuf index * 8) + bit index
Outbuf[11] is excluded from the coil map

Examples:
32 = Outbuf[4] bit0 Dnd
33 = Outbuf[4] bit1 Mur
37 = Outbuf[4] bit5 Ccs
39 = Outbuf[4] bit7 Guest
```

The Modbus unit id is fixed by `MODBUS_UNIT_ID`; it is not stored in a holding
register. The room/node ID is owned by the gateway, stored as `Outbuf[11]`, and
persisted in ESP32 flash using NVS. BMS/client software should use this value as
the primary room number. Read/write it through holding register `5` high byte;
valid values are `1..247`. Any RCU-reported node ID is ignored and replaced with
the gateway-owned value before Modbus reads are served. Writing the low byte of
holding register `2`, or the matching coils for `Outbuf[4]`, sends the matching
PS18 frame when these bits change: `Dnd`, `Mur`, and `Guest`.

Modbus writes update the gateway register cache, so the next Modbus read returns
the new value just like a state update received from the RCU bus. Coil writes
use read-modify-write and change only the requested bit. A later RCU bus frame
can still update the same register if the physical RCU state changes.

Example confirmed state:

```text
FC05 coil 37 ON -> Outbuf[4] bit5 Ccs = 1
FC03 holding register 2 -> 0x0020
FC03 holding register 5 -> 0x0100, high Outbuf[11] NodeId = 0x01
FC06 holding register 5 = 0x0200 -> save NodeId = 0x02 to flash
```

RS485/UART pins:

```text
CH1 RS485:
GPIO17 = TXD2
GPIO5  = RXD2
GPIO33 = DE/RE

CH2 RS485 via SN3257:
GPIO1  = TXD0
GPIO3  = RXD0
GPIO14 = DE/RE

I2C MCP23017:
GPIO32 = SDA
GPIO4  = SCL

DIP switch board ID:
MCP23017 GPA0-GPA7 map to DIP1-DIP8. The switch is active-low
(`OFF=1`, `ON=0`), so firmware reports `board_id = (~GPIOA) & 0xFF`.
If the DIP switch cannot be read, firmware falls back to `board_id = 1`.
Telemetry exposes `board_id_valid` as discrete input `3`; the RCU input
registers stay reserved for `Inpbuf[0..5]`.

Boot:
GPIO0 = BOOT only
```

## RCU Bus Behavior

The gateway sends command frames as device `0x31`:

```text
MUR on/off       7F 01 31 07 FF 08 00 00
DND on/off       7F 01 31 05 FF 08 00 00
Guest inserted   7F 01 31 03 00 00 00 1A
Guest removed    7F 01 31 02 00 00 00 1A
```

It listens to RCU LED/state frames to update the cached `Inpbuf[0..5]` and
`Outbuf[0..11]` values for Modbus reads.

More detail:

- [PS18V62_2 Bus Protocol](../docs/ps18v62_2_bus.md)
- [Hardware](../docs/hardware.md)
- [WT32-ETH01 Pinout and Wiring](../docs/wt32_eth01_pinout_and_wiring.md)
