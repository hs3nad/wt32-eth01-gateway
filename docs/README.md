# Documentation Index

This repository has one supported integration path:

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

The gateway exposes the RCU state directly through Modbus TCP.

## Documents

- [System Overview](system_overview.md)
- [PS18V62_2 Bus Protocol](ps18v62_2_bus.md)
- [Hardware](hardware.md)
- [Gateway Firmware](../gateway/README.md)
- [Gateway Auto OTA](ota_update.md)
- [Register Map](register_map.md)
- [WT32-ETH01 Pinout and Wiring](wt32_eth01_pinout_and_wiring.md)

## Source Layout

```text
gateway/             WT32-ETH01 ESP-IDF firmware
rcu/                  Local RCU firmware copy used by this gateway
```
