# AMPHAN PS18V62_2 Modbus Gateway

Firmware and reference RCU sources for the AMPHAN room gateway integration.

```text
Modbus TCP client / BMS
        |
        | Modbus TCP, port 502
        v
WT32-ETH01 gateway
        |
        | RS485, PS18V62_2 fixed 8-byte frames
        v
PS18V62_2 RCU
```

The supported network interface is Modbus TCP from `gateway`.

## Layout

```text
gateway/             ESP-IDF firmware for WT32-ETH01
rcu/                  Local PS18V62_2 RCU firmware target
docs/                Documentation for this repository
```

## Key Documents

- [Documentation Index](docs/README.md)
- [System Overview](docs/system_overview.md)
- [PS18V62_2 Bus Protocol](docs/ps18v62_2_bus.md)
- [Hardware](docs/hardware.md)
- [Register Map](docs/register_map.md)
- [WT32-ETH01 Pinout and Wiring](docs/wt32_eth01_pinout_and_wiring.md)
- [Gateway Firmware](gateway/README.md)

## Quick Start

Build the gateway firmware:

```bash
cd gateway
source /home/kaina/esp/esp-idf/export.sh
./build.sh
```

Flash the WT32-ETH01:

```bash
PORT=/dev/ttyUSB0 ./flash.sh
```

Build the RCU firmware when XC8/MPLAB tooling is available:

```bash
cd rcu
make -f nbproject/Makefile-default.mk build
```

## Modbus Summary

```text
TCP port  = 502
Unit ID   = MODBUS_UNIT_ID in gateway/include/config.h
Holding   = RCU Outbuf[0..11]
Input     = RCU Inpbuf[0..5]
Packing   = high byte is odd index, low byte is even index
Control   = holding register 2 low byte = Outbuf[4]
```
