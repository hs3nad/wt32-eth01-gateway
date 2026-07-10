# PS18V62_2 Bus Protocol

The WT32-ETH01 gateway joins the existing PS18V62_2 bus as an ETH device. It
does not use MQTT, AHF register commands, or a separate room-address protocol.
Network transport is Modbus TCP; this document only covers the RS485 side.

## Bus

```text
Physical : RS485
UART     : 9600 bps, 8N1
Frame    : fixed 8 bytes
Target   : rcu
```

Byte 8 is treated as part of the confirmed frame data. The firmware does not
calculate it as a checksum.

## Device IDs

Existing RCU bus IDs:

```text
01 = Keycard / keybox
07 = Master + Bathroom panel
0D = Entrance + DND + MUR panel
0E = 3-gang panel
0F = 3-gang panel
11 = Master panel
12 = Bedside + Master panel
3C = Thermostat panel
31 = WT32-ETH01 gateway / ETH device
```

The gateway ID is configured in `gateway/include/config.h`:

```c
#define ETH_RCU_DEVICE_ID 0x31
```

The matching RCU parser support is in
`rcu/switch/switch_input.c`.

## Input Frames From Gateway

Modbus control writes are converted by the gateway into the same fixed 8-byte
input-frame style already used by PS18V62_2 room devices.

```text
makeup_room / finish_cleaning
7F 01 31 07 FF 08 00 00

do_not_disturb / clear_do_not_disturb
7F 01 31 05 FF 08 00 00

keycard_inserted / guest_inserted
7F 01 31 03 00 00 00 1A

keycard_removed / guest_removed
7F 01 31 02 00 00 00 1A
```

The DND and MUR commands are toggle-style panel commands. The gateway checks its
cached state before sending so `do_not_disturb` only sends when DND is currently
off, and `clear_do_not_disturb` only sends when DND is currently on.

## Private Gateway Frames

Phase 1 uses a private fixed 8-byte frame family for register synchronization
between `gateway` and the RCU. It is intentionally separated from the
existing switch/keybox and thermostat families:

```text
7F 01 ... = switch/keybox
7F 05 ... = thermostat
7F E1 ... = gateway private register sync
```

Initial private commands:

```text
WRITE_OUTBUF_BYTE
Gateway -> RCU: 7F E1 31 11 INDEX VALUE 00 SEQ
RCU -> Gateway: 7F E1 01 91 INDEX VALUE SEQ STATUS

WRITE_OUTBUF_BIT
Gateway -> RCU: 7F E1 31 12 INDEX MASK ONOFF SEQ
RCU -> Gateway: 7F E1 01 92 INDEX VALUE SEQ STATUS

READ_ALL_STATUS
Gateway -> RCU: 7F E1 31 14 OFFSET COUNT SEQ 00
RCU -> Gateway: 7F E1 01 94 OFFSET COUNT DATA0 DATA1
```

`STATUS` values:

```text
00 = OK
01 = bad command
02 = bad address
```

`Outbuf[11]` is the gateway-owned room/node ID and is not writable through
private write commands to the RCU. The gateway stores this byte in ESP32 flash
using NVS and treats it as the primary room identifier for Modbus/BMS use.
Modbus can read/write it through holding register `5` high byte; valid values
are `1..247`. For `READ_ALL_STATUS`, offset `0..5` maps to `Inpbuf[0..5]`;
offset `6..17` maps to `Outbuf[0..11]`. Each response carries up to two data
bytes.

The gateway ignores any node ID value reported by the RCU. If an RCU status
frame or private status response includes `Outbuf[11]`, the gateway replaces it
with the value stored in gateway NVS before publishing Modbus registers. The
source byte in `7F E1` private ACK frames is also not used as a room number.

## State Frames From RCU

The gateway listens for normal RCU LED/status frames:

```text
7E 02 PANEL_ID C1 STATE 00 00 xx
```

The gateway maps the observed LED state into its local cache:

```text
Panel 0D bit1 = DND
Panel 0D bit2 = MUR
Keycard frames update guest state
Master status is inferred from grouped output bits
```

The gateway maps observed frames into the Modbus register mirror:

```text
Input registers  = Inpbuf[0..5] packed in pairs
Holding registers = Outbuf[0..11] packed in pairs
Coils = Outbuf[0..10] bits, excluding Outbuf[11]
```

Each Modbus register carries two RCU bytes. The odd index is the high byte and
the even index is the low byte, for example `Outbuf[1] << 8 | Outbuf[0]`.
Each coil maps to one real `Outbuf` bit: `coil = Outbuf index * 8 + bit index`.
For example, `Outbuf[4] bit0 Dnd` is coil `32`, and `Outbuf[4] bit5 Ccs` is
coil `37`.

The current packed holding register layout is:

```text
0 = high Outbuf[1] lamp 9..16, low Outbuf[0] lamp 1..8
1 = high Outbuf[3] lamp 25..32, low Outbuf[2] lamp 17..24
2 = high Outbuf[5] air control, low Outbuf[4] control bits
3 = high Outbuf[7] real hour, low Outbuf[6] actual temperature
4 = high Outbuf[9] alarm hour, low Outbuf[8] real minute
5 = high Outbuf[11] node id, low Outbuf[10] alarm minute
```

Modbus writes and RCU bus frames both update the gateway's register cache.
Single-coil writes update only the addressed bit in the mapped `Outbuf` byte.
Writing holding register `5` high byte updates the gateway node ID in flash, so
the value is restored after reboot. This persisted gateway value is the primary
room/node number for the bridge. The RCU node ID and private ACK source byte are
ignored for room mapping.

## Limitation

PS18V62_2 fixed frames do not contain a room address. A gateway instance is
therefore mapped to one room and one RS485 bus.
