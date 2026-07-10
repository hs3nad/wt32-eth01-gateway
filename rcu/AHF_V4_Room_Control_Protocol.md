# AHF-V4 Room Control Protocol

Last updated: 2026-06-18

## Rules

- All panel frames are fixed 8-byte frames.
- Replay exact frames from this document or confirmed logs.
- Byte 8 is not treated as a checksum in this firmware. Do not calculate it.
- RCU does not discard a switch-panel key after panel sleep. Mapped key frames are accepted immediately.
- Panel LED state follows `Outbuf`, except master LEDs use the master group latch rule below.

## Device IDs

```text
01 = Keycard / keybox
07 = Master + Bathroom panel
0D = Entrance + DND + MUR panel
0E = 3-gang panel
0F = 3-gang panel
11 = Master panel
12 = Bedside + Master panel
3C = Thermostat panel
```

## Input Map

```text
07 SW1 = Master
07 SW2 = Bathroom / In5 / Out5

0D SW1 = Entrance / In8 / Out8
0D SW2 = DND / Outbuf[7].Dnd
0D SW3 = MUR / Outbuf[7].Mur

0E SW1 = Ceiling-2 / In7 / Out7
0E SW2 = Ceiling-1 / In6 / Out6
0E SW3 = Reading-L / In9 / Out9

0F SW1 = Reading-R / In10 / Out10
0F SW2 = Ceiling-1 / In6 / Out6
0F SW3 = Ceiling-2 / In7 / Out7

11 SW1 = Master

12 SW1 = Bedside / In11 / Out11
12 SW2 = Master
```

## Input Frames

```text
Panel key input:
7F 01 PANEL 03 FF 08 00 xx = SW1
7F 01 PANEL 05 FF 08 00 xx = SW2
7F 01 PANEL 07 FF 08 00 xx = SW3
7F 01 PANEL 00 FF 08 xx xx = wakeup only, ignore output toggle

Keycard inserted:
7F 01 01 03 00 00 00 1A

Keycard removed:
7F 01 01 02 00 00 00 1A
```

## Panel Sleep

```text
RCU does not apply a 25-second sleep guard.
Mapped switch-panel key frames are accepted immediately.
Panel wake/backlight-only frames are handled by the panel/protocol and are not an extra RCU discard.
Keycard and thermostat events are not blocked by any panel sleep guard.
```

Switch-panel input accepts the new `7F 01 ...` key frames only. Legacy `7E 02 ... C1/41 ...` switch-input parsing has been removed.

## LED Bitmap

```text
state 00 = led1 off, led2 off, led3 off
state 01 = led1 on,  led2 off, led3 off
state 02 = led1 off, led2 on,  led3 off
state 03 = led1 on,  led2 on,  led3 off
state 04 = led1 off, led2 off, led3 on
state 05 = led1 on,  led2 off, led3 on
state 06 = led1 off, led2 on,  led3 on
state 07 = led1 on,  led2 on,  led3 on
```

## LED Frames

### Panel 07

```text
00 = 7E 02 07 C1 00 00 00 46
01 = 7E 02 07 C1 01 00 00 4E
02 = 7E 02 07 C1 02 00 00 E0
03 = 7E 02 07 C1 03 00 00 46
```

### Panel 0D

`0D` has Entrance, DND, and MUR. DND and MUR are mutually exclusive, so state `06` and `07` are not used.

```text
00 = 7E 02 0D C1 00 00 00 60
01 = 7E 02 0D C1 01 00 00 08
02 = 7E 02 0D C1 02 00 00 6A
03 = 7E 02 0D C1 03 00 00 80
04 = 7E 02 0D C1 04 00 00 66
05 = 7E 02 0D C1 05 00 00 06
```

### Panel 0E

```text
00 = 7E 02 0E C1 00 00 00 68
01 = 7E 02 0E C1 01 00 00 6A
02 = 7E 02 0E C1 02 00 00 60
03 = 7E 02 0E C1 03 00 00 E6
04 = 7E 02 0E C1 04 00 00 E6
05 = 7E 02 0E C1 05 00 00 EE
06 = 7E 02 0E C1 06 00 00 E4
07 = 7E 02 0E C1 07 00 00 64
```

### Panel 0F

```text
00 = 7E 02 0F C1 00 00 00 6A
01 = 7E 02 0F C1 01 00 00 E0
02 = 7E 02 0F C1 02 00 00 E6
03 = 7E 02 0F C1 03 00 00 E6
04 = 7E 02 0F C1 04 00 00 EE
05 = 7E 02 0F C1 05 00 00 E4
06 = 7E 02 0F C1 06 00 00 E4
07 = 7E 02 0F C1 07 00 00 EA
```

### Panel 11

```text
00 = 7E 02 11 C1 00 00 00 06
01 = 7E 02 11 C1 01 00 00 86
```

### Panel 12

```text
00 = 7E 02 12 C1 00 00 00 46
01 = 7E 02 12 C1 01 00 00 6E
02 = 7E 02 12 C1 02 00 00 E4
03 = 7E 02 12 C1 03 00 00 A4
```

## Master Logic

Master on/off does not use a `Master` bit.

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

Behavior:

```text
Press Master while any master-group output is on  -> turn master group off.
Press Master while all master-group outputs are off -> turn master group on.
Master LED stays on after master on/off action.
Master LED turns off after Master OFF when one grouped output is manually turned on.
When this happens, the master LED refresh is forced for all mapped master panels.
Outbuf[7].Master is a status bit: 0 when all configured master outputs are off, 1 when any configured master output is on.
Backlight OFF is a separate command path and is not part of the normal LED state frame table.
Master OFF requests the separate Backlight OFF command, but it is sent after all pending panel LED frames.
`USE_SWITCH_BACKLIGHT_OFF` enables this command path. Comment it out to disable Backlight OFF without changing normal LED sync.
```

## Backlight OFF Command

These frames are queued only by `SwitchLed_RequestBacklightOff()`. They are intentionally kept out of the normal panel LED state table.
The queue is sent last, after normal panel LED state frames are complete.

```text
07 = 7E 02 07 41 01 00 00 4E
0D = 7E 02 0D 41 00 00 00 EA
0E = 7E 02 0E 41 00 00 00 EA
0F = 7E 02 0F 41 00 00 00 AE
11 = 7E 02 11 41 01 00 00 AE
12 = 7E 02 12 41 02 00 00 E4
```

## DND / MUR

```text
DND LED follows Outbuf[7].Dnd directly.
MUR LED follows Outbuf[7].Mur directly.
DND and MUR cannot be on at the same time.
Guest/keycard does not gate DND/MUR LED sync.
```

## Welcome / Guest

When keycard changes from no-card to guest, output scenes are set in `Outbuf`, then panel LEDs are refreshed from `Outbuf`.
Thermostat power ON is sent on keycard insert. Thermostat power OFF is sent on
keycard removal.

```text
All normal panel LEDs follow their mapped output bits.
Master LEDs follow the master logic above.
DND/MUR LEDs follow Outbuf[7] service bits.
```

## Thermostat Panel 3C

Detailed thermostat lookup tables and latest parser notes are maintained in
`THERMOSTAT_PROTOCOL_NOTES.md`.

```text
Runtime mode = Cool only
Power response delay = 600 ms
Fan/temp response delay = 320 ms
Power ON/OFF = exact frame by current fan + set temperature
Fan/temp     = exact frame by current fan + set temperature + panel data6
```

Parser notes:

```text
Thermostat key frames from panel start with 7F 05 3C.
RCU response frames to thermostat start with 7E 04 3C 08.
Fan/temp responses are queued immediately by the thermostat parser.
Switch and thermostat share the same RX stream, then frames are split by signature.
```

`Outbuf[8]` stores set temperature as `tempC - 16`, so the low nibble covers
16C through 31C.

Thermostat set-temp byte in RCU-to-panel frames is byte 5 and uses the
temperature value directly. The short table below is the fan-high reference
family; full fan low/mid/high/auto tables are in `THERMOSTAT_PROTOCOL_NOTES.md`.

```text
16C = 7E 04 3C 08 B3 10 1E 74
17C = 7E 04 3C 08 B3 11 1E 75
18C = 7E 04 3C 08 B3 12 1E 76
19C = 7E 04 3C 08 B3 13 1E 75
20C = 7E 04 3C 08 B3 14 1E 77
21C = 7E 04 3C 08 B3 15 1E 78
22C = 7E 04 3C 08 B3 16 1E 78
23C = 7E 04 3C 08 B3 17 1E 78
24C = 7E 04 3C 08 B3 18 1E 7A
25C = 7E 04 3C 08 B3 19 1E 79
26C = 7E 04 3C 08 B3 1A 1E 7A
27C = 7E 04 3C 08 B3 1B 1E 7B
28C = 7E 04 3C 08 B3 1C 1E 7B
29C = 7E 04 3C 08 B3 1D 1E 7C
30C = 7E 04 3C 08 33 1E 1E 33
31C = 7E 04 3C 08 33 1F 1E 34
```

Firmware sends exact fan/temp frames from the verified table above. If a new
set temperature produces a different working frame, add it to the table in
`THERMOSTAT_PROTOCOL_NOTES.md` and `thermostat/thermostat_panel.c`.

Thermostat Power OFF frames from RCU, fan-high reference family:

```text
16C = 7E 04 3C 08 32 10 1E 2A
17C = 7E 04 3C 08 32 11 1E 2C
18C = 7E 04 3C 08 32 12 1E 2C
19C = 7E 04 3C 08 32 13 1E 2C
20C = 7E 04 3C 08 32 14 1E 2D
21C = 7E 04 3C 08 32 15 1E 2E
22C = 7E 04 3C 08 32 16 1E 2D
23C = 7E 04 3C 08 32 17 1E 2F
24C = 7E 04 3C 08 32 18 1E 30
25C = 7E 04 3C 08 32 19 1E 30
26C = 7E 04 3C 08 32 1A 1E 30
27C = 7E 04 3C 08 32 1B 1E 32
28C = 7E 04 3C 08 32 1C 1E 31
29C = 7E 04 3C 08 32 1D 1E 32
30C = 7E 04 3C 08 32 1E 1E 33
31C = 7E 04 3C 08 32 1F 1E 33
```

Thermostat fan-speed key frames from panel `3C`:

```text
Fan Low   = 7F 05 3C 04 DB/EB/FB temp data6 xx
Fan Mid   = 7F 05 3C 04 D3 temp data6 xx
Fan High  = 7F 05 3C 04 E3 temp data6 xx
Fan Auto  = 7F 05 3C 04 F3 temp data6 xx
```

Fan auto is kept as auto in `Outbuf[8]`, but physical relay speed is selected
from `roomTemp - setTemp`: low for `<= 1`, medium for `2`, high for `>= 3`.
If room temp is not valid yet, medium is used as fallback.

Thermostat mode key frames from panel `3C`:

```text
Mode key observed = 7F 05 3C 05 F3 19 1C 5D
Runtime output is forced to cool mode.
```

Thermostat setup/power key frames from panel `3C`:

```text
Power ON  = 7F 05 3C 01 D2/D3 18/19 1B..20 xx
Power OFF = exact frame by fan + set temperature; see THERMOSTAT_PROTOCOL_NOTES.md
Setup OFF = 7F 05 3C 06 F3 19 1C 5D
```

Thermostat Power OFF key frames from panel `3C`:

```text
16C = 7F 05 3C 01 F3 10 1E 56
17C = 7F 05 3C 01 F3 11 1E 57
18C = 7F 05 3C 01 F3 12 1E 57
19C = 7F 05 3C 01 F3 13 1E 59
20C = 7F 05 3C 01 F3 14 1E 58
21C = 7F 05 3C 01 F3 15 1E 59
22C = 7F 05 3C 01 F3 16 1E 5A
23C = 7F 05 3C 01 F3 17 1E 5A
24C = 7F 05 3C 01 F3 18 1E 5B
25C = 7F 05 3C 01 F3 19 1E 5C
26C = 7F 05 3C 01 F3 1A 1E 5B
27C = 7F 05 3C 01 F3 1B 1E 5D
28C = 7F 05 3C 01 F3 1C 1E 5D
29C = 7F 05 3C 01 F3 1D 1E 5D
30C = 7F 05 3C 01 F3 1E 1E 5E
31C = 7F 05 3C 01 F3 1F 1E 50
```

RCU maps fan speed into `Outbuf[8].FanSpeed`:

```text
Low  = 0x00
Mid  = 0x10
High = 0x20
Auto = 0x30
```

## Build

```sh
make -f nbproject/Makefile-default.mk build
```
