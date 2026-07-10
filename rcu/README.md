# PS18V62 AHF-V4 Switch / Thermostat Firmware

Firmware for the PS18V62 board using the AHF-V4 room-control protocol over RS485.

## Current Scope

Enabled features:

```c
#define _SWITCH
#define _THERMOSTAT
```

The active room workflow is:

```text
switch panels + thermostat panel + keybox panel <-> RCU relay
```

RCU no longer discards a switch-panel key after panel sleep. The panel may send
its own wake/backlight frame first; any mapped key frame that reaches the RCU
is accepted immediately.

## Communication

```text
Physical bus: RS485
UART: 9600 bps, 8N1
Frame length: 8 bytes
Protocol: AHF-V4 fixed frames
```

Important protocol rule:

```text
Replay exact frames. Byte 8 is not treated as a calculated checksum.
```

## Panels

```text
01 = Keycard / keybox
07 = Master + Bathroom
0D = Entrance + DND + MUR
0E = Ceiling-2 + Ceiling-1 + Reading-L
0F = Reading-R + Ceiling-1 + Ceiling-2
11 = Master
12 = Bedside + Master
3C = Thermostat
```

Detailed frame tables are in `AHF_V4_Room_Control_Protocol.md`.

## Gateway Integration

The WT32-ETH01 gateway is accepted as a separate ETH device:

```c
#define SWITCH_PANEL_ETH_ADDRESS 0x31u
```

Normal gateway action frames reuse the existing switch/keybox frame family:

```text
7F 01 31 05 FF 08 00 00 = DND key
7F 01 31 07 FF 08 00 00 = MUR key
7F 01 31 03 00 00 00 1A = keycard inserted
7F 01 31 02 00 00 00 1A = keycard removed
```

Gateway register synchronization uses the private `7F E1` frame family so it
does not collide with switch/keybox `7F 01` or thermostat `7F 05` traffic.
See [PS18V62_2 Bus Protocol](../docs/ps18v62_2_bus.md) for the active command
format.

## LED Rules

- Normal panel LEDs follow `Outbuf`.
- DND LED follows `Outbuf[7].Dnd` directly.
- MUR LED follows `Outbuf[7].Mur` directly.
- DND and MUR are mutually exclusive.
- Master LEDs do not use a `Master` bit.
- Master LEDs use the configured master output group.
- Master LEDs stay on after Master ON/OFF action.
- Master LEDs turn off after Master OFF when any grouped output is manually turned on.
- `Outbuf[7].Master` is a status bit: `0` when all configured master outputs are off, `1` when any configured master output is on.
- Backlight OFF is a separate command path and is not part of the normal LED state frame table.
- Master OFF requests the separate Backlight OFF command, but it is sent after all pending panel LED frames.
- `USE_SWITCH_BACKLIGHT_OFF` enables the separate Backlight OFF command. Comment it out to disable this command path.

Master group:

```c
#define MASTER1_1_ON  (Out5 | Out6 | Out7 | Out8)
#define MASTER1_1_OFF (Out5 | Out6 | Out7 | Out8)
#define MASTER1_2_ON  (Out9 | Out10 | Out11)
#define MASTER1_2_OFF (Out9 | Out10 | Out11)
#define MASTER1_3_ON  0x00
#define MASTER1_3_OFF 0x00
#define MASTER1_4_ON  0x00
#define MASTER1_4_OFF 0x00
```

## Keycard

```text
Inserted = 7F 01 01 03 00 00 00 1A
Removed  = 7F 01 01 02 00 00 00 1A
```

When a guest card is inserted, panel LEDs refresh from `Outbuf` after the welcome outputs are set.
Thermostat power ON is sent on keycard insert. Thermostat power OFF is sent on
keycard removal.

## Panel Sleep

```text
RCU does not apply a 25-second sleep guard.
Mapped switch-panel key frames are accepted immediately.
Panel wake/backlight-only frames are handled by the panel/protocol and are not an extra RCU discard.
```

Keycard and thermostat events are not blocked by any panel sleep guard.

Switch-panel input accepts the new `7F 01 ...` key frames only. Legacy `7E 02 ... C1/41 ...` switch-input parsing has been removed.

## RS485 TX Rule

Switch and thermostat transmitters send one complete 8-byte frame while `DIR` is high, then wait for `EUSART_is_tx_done()` before lowering `DIR`.

Switch TX interrupt is not used. `PIE1bits.TXIE` is kept disabled.

## Important Files

- `main.c`: scheduler, output state, master/service LED rules
- `switch/switch_input.c`: AHF-V4 RX parser and panel key map
- `switch/switch_led.c`: panel LED state cache and exact TX frame lookup
- `switch/switch_led.h`: LED driver API
- `thermostat/thermostat_panel.c`: thermostat panel command sync
- `AHF_V4_Room_Control_Protocol.md`: latest confirmed protocol frames
- `THERMOSTAT_PROTOCOL_NOTES.md`: detailed thermostat lookup tables and parser notes

## Build

Use the MPLAB/XC8 generated makefile:

```sh
make -f nbproject/Makefile-default.mk build
```

Production hex:

```text
dist/default/production/PS18V62.X.production.hex
```

Latest verified build:

```text
Program space: 6753 words (82.4%)
Data space:     319 bytes (31.2%)
```
