# System Overview

## Purpose

This project connects a PS18V62_2 hotel room RCU to a Modbus TCP client through
a WT32-ETH01 Ethernet gateway.

```text
Modbus TCP client / BMS
        |
        | Modbus TCP, port 502
        v
WT32-ETH01 gateway
        |
        | UART2 / RS485, PS18V62_2 fixed 8-byte frames
        v
PS18V62_2 RCU
```

## Current Scope

- One room per RS485 bus.
- No HC-12 floor-bus mode.
- `rcu` is the local RCU firmware target.
- `gateway` uses ETH device ID `0x31` on the PS18V62_2 bus.
- Modbus TCP exposes `Inpbuf[0..5]` and `Outbuf[0..11]` using the RCU register map.
- Modbus coils mirror real `Outbuf[0..10]` bits; `Outbuf[11]` is not exposed as coils.
- `Outbuf[11]` is the gateway-owned room/node ID and is persisted in ESP32 flash using NVS.
- Auto OTA is enabled through a public GitHub raw manifest and dual OTA app partitions.
- The register map source of truth is [register_map.md](register_map.md).

## State Ownership

```text
RCU             Source of truth for physical room state
gateway         Source of truth for room/node ID, latest-state cache, Modbus TCP server
Modbus client   Operator/BMS display and command entry
```

The system is state-first. The latest correct room state is more important than
preserving every intermediate event.

Room numbering is gateway-first. BMS/client software should identify the room by
the persisted gateway node ID in `Outbuf[11]` / holding register `5` high byte,
not by any fixed byte inside the legacy PS18V62_2 frames. Any node ID value
reported by the RCU is ignored and replaced with the gateway-owned value before
the Modbus register mirror is published.

## Supported Commands

The Modbus client writes control bits through the low byte of holding register
`2`, or through the matching coils, which map to `Outbuf[4]`:

```text
Coil 32 = Outbuf[4] bit0 Dnd
Coil 33 = Outbuf[4] bit1 Mur
Coil 37 = Outbuf[4] bit5 Ccs
Coil 39 = Outbuf[4] bit7 Guest
```

The gateway converts supported bit changes to 8-byte PS18V62_2 input frames from
ETH device ID `0x31`.

Modbus writes update the same gateway register cache used by RCU bus updates.
That means a Modbus write is readable back through coils and holding registers
without waiting for another RCU frame. If the RCU later reports a different
physical state, the cache is refreshed from that bus state.
