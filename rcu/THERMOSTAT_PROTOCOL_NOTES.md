# Thermostat Protocol Notes

This document keeps thermostat protocol observations and lookup data that are
too bulky or volatile to infer from code alone.
Firmware currently supports cool-mode power on/off, set-temp up/down, and
fan-speed changes for the thermostat panel.

All frames are 8 bytes. Values are hex.

## Current Runtime Rules

- Thermostat output is forced to cool mode.
- Power command response delay is 600 ms.
- Fan/temp command response delay is 320 ms.
- Fan/temp responses are queued immediately by the thermostat parser when the
  key frame is parsed. Main only mirrors the parsed event into `Outbuf`.
- The panel room/status byte (`data6`) is frame-local. Fan/temp responses that
  are triggered by a key frame copy this byte when the current table requires it.
- The latest thermostat `data6` value in the observed room-temp range `1B-20`
  is stored in `Outbuf[2]` as the room-temp byte.
- `AirCdu` compares set temp against the latest room-temp byte only while
  `AirPow = 1`:
  - set temp > room temp: `AirCdu = 0`
  - set temp < room temp: `AirCdu = 1`
  - set temp == room temp: keep the previous `AirCdu` state
- Fan auto (`FanA`) is stored as auto in `Outbuf[8]`, but the physical fan
  relay output is derived from `roomTemp - setTemp`:
  - room temp <= set temp + 1: fan low
  - room temp == set temp + 2: fan medium
  - room temp >= set temp + 3: fan high
  - if room temp is not valid yet, fan medium is used as fallback
- Switch and thermostat share the same UART receive stream, then frames are
  separated by frame signature. Thermostat panel key frames start with
  `7F 05 3C`; RCU-to-thermostat responses start with `7E 04 3C 08`.

## Temperature Index

`index = tempC - 16`

| Temp C | Byte5 | Index |
|---:|---:|---:|
| 16 | 10 | 0 |
| 17 | 11 | 1 |
| 18 | 12 | 2 |
| 19 | 13 | 3 |
| 20 | 14 | 4 |
| 21 | 15 | 5 |
| 22 | 16 | 6 |
| 23 | 17 | 7 |
| 24 | 18 | 8 |
| 25 | 19 | 9 |
| 26 | 1A | 10 |
| 27 | 1B | 11 |
| 28 | 1C | 12 |
| 29 | 1D | 13 |
| 30 | 1E | 14 |
| 31 | 1F | 15 |

## RCU To Thermo: Fan + Set Temp

Frame format:

`7E 04 3C 08 data4 temp data6 data7`

### Fan High

Latest cool mode fan high temp-set rule: `data4 = B3`, except 30-31C use `33`; `data6 = 1E`.

Special power-on 25C working family: `7E 04 3C 08 B3 19 1B 78`.

| Temp C | Frame |
|---:|---|
| 16 | `7E 04 3C 08 B3 10 1E 74` |
| 17 | `7E 04 3C 08 B3 11 1E 75` |
| 18 | `7E 04 3C 08 B3 12 1E 76` |
| 19 | `7E 04 3C 08 B3 13 1E 75` |
| 20 | `7E 04 3C 08 B3 14 1E 77` |
| 21 | `7E 04 3C 08 B3 15 1E 78` |
| 22 | `7E 04 3C 08 B3 16 1E 78` |
| 23 | `7E 04 3C 08 B3 17 1E 78` |
| 24 | `7E 04 3C 08 B3 18 1E 7A` |
| 25 | `7E 04 3C 08 B3 19 1E 79` |
| 26 | `7E 04 3C 08 B3 1A 1E 7A` |
| 27 | `7E 04 3C 08 B3 1B 1E 7B` |
| 28 | `7E 04 3C 08 B3 1C 1E 7B` |
| 29 | `7E 04 3C 08 B3 1D 1E 7C` |
| 30 | `7E 04 3C 08 B3 1E 1E 7D` |
| 31 | `7E 04 3C 08 33 1F 1E 34` |

Power-on from `D2 18 1E` uses `33` from 30-31C:

| Temp C | Power On Frame |
|---:|---|
| 30 | `7E 04 3C 08 33 1E 1E 33` |
| 31 | `7E 04 3C 08 33 1F 1E 34` |

Direction-specific note:

| Transition | Frame |
|---|---|
| 30C down to 29C | `7E 04 3C 08 33 1D 1D 32` |

### Fan Medium

Latest cool mode fan medium temp-set rule: `data4 = A3`, except 31C uses `23`; `data6 = 1D` for 16-26C and `1E` for 27-31C.

| Temp C | Frame |
|---:|---|
| 16 | `7E 04 3C 08 A3 10 1D 6B` |
| 17 | `7E 04 3C 08 A3 11 1D 6C` |
| 18 | `7E 04 3C 08 A3 12 1D 6B` |
| 19 | `7E 04 3C 08 A3 13 1D 6D` |
| 20 | `7E 04 3C 08 A3 14 1D 6D` |
| 21 | `7E 04 3C 08 A3 15 1D 6D` |
| 22 | `7E 04 3C 08 A3 16 1D 6E` |
| 23 | `7E 04 3C 08 A3 17 1D 70` |
| 24 | `7E 04 3C 08 A3 18 1D 6F` |
| 25 | `7E 04 3C 08 A3 19 1D 70` |
| 26 | `7E 04 3C 08 A3 1A 1D 71` |
| 27 | `7E 04 3C 08 A3 1B 1E 71` |
| 28 | `7E 04 3C 08 A3 1C 1E 73` |
| 29 | `7E 04 3C 08 A3 1D 1E 72` |
| 30 | `7E 04 3C 08 A3 1E 1E 74` |
| 31 | `7E 04 3C 08 23 1F 1E 2A` |

Power-on from `D2 18 1D/1E` uses `23` from 30-31C:

| Temp C | Power On Frame |
|---:|---|
| 30 | `7E 04 3C 08 23 1E 1E 2B` |
| 31 | `7E 04 3C 08 23 1F 1E 2A` |

### Fan Low

Latest cool mode fan low rule: `data4 = 93`, except 30-31C use `13`; `data6 = 1D`.

| Temp C | Frame |
|---:|---|
| 16 | `7E 04 3C 08 93 10 1D 61` |
| 17 | `7E 04 3C 08 93 11 1D 62` |
| 18 | `7E 04 3C 08 93 12 1D 63` |
| 19 | `7E 04 3C 08 93 13 1D 63` |
| 20 | `7E 04 3C 08 93 14 1D 64` |
| 21 | `7E 04 3C 08 93 15 1D 65` |
| 22 | `7E 04 3C 08 93 16 1D 64` |
| 23 | `7E 04 3C 08 93 17 1D 66` |
| 24 | `7E 04 3C 08 93 18 1D 66` |
| 25 | `7E 04 3C 08 93 19 1D 67` |
| 26 | `7E 04 3C 08 93 1A 1D 67` |
| 27 | `7E 04 3C 08 93 1B 1D 69` |
| 28 | `7E 04 3C 08 93 1C 1D 68` |
| 29 | `7E 04 3C 08 93 1D 1D 69` |
| 30 | `7E 04 3C 08 13 1E 1D 20` |
| 31 | `7E 04 3C 08 13 1F 1D 21` |

Power-on from `D2 18 1D` uses `13` from 29-31C:

| Temp C | Power On Frame |
|---:|---|
| 29 | `7E 04 3C 08 13 1D 1D 21` |
| 30 | `7E 04 3C 08 13 1E 1D 20` |
| 31 | `7E 04 3C 08 13 1F 1D 21` |

### Fan Auto

Rule:

- 30-31C: `data4 = 0B`
- 28-29C: `data4 = 9B`
- 26-27C: `data4 = AB`
- 16-25C: `data4 = BB`
- `data6 = 1E`

| Temp C | Frame |
|---:|---|
| 16 | `7E 04 3C 08 BB 10 1E 7A` |
| 17 | `7E 04 3C 08 BB 11 1E 79` |
| 18 | `7E 04 3C 08 BB 12 1E 7A` |
| 19 | `7E 04 3C 08 BB 13 1E 7B` |
| 20 | `7E 04 3C 08 BB 14 1E 7B` |
| 21 | `7E 04 3C 08 BB 15 1E 7C` |
| 22 | `7E 04 3C 08 BB 16 1E 7D` |
| 23 | `7E 04 3C 08 BB 17 1E 7C` |
| 24 | `7E 04 3C 08 BB 18 1E 7E` |
| 25 | `7E 04 3C 08 BB 19 1E 7E` |
| 26 | `7E 04 3C 08 AB 1A 1E 76` |
| 27 | `7E 04 3C 08 AB 1B 1E 75` |
| 28 | `7E 04 3C 08 9B 1C 1E 6D` |
| 29 | `7E 04 3C 08 9B 1D 1E 6E` |
| 30 | `7E 04 3C 08 0B 1E 1E 1D` |
| 31 | `7E 04 3C 08 0B 1F 1E 1C` |

## RCU To Thermo: Power Off + Fan/Temp

Frame format:

`7E 04 3C 08 data4 temp data6 data7`

Special high fan 25C variants:

- Default: `7E 04 3C 08 32 19 1E 30`
- Working family: `7E 04 3C 08 36 19 1F 33`
- 1C family: `7E 04 3C 08 2A 19 1C 2B`

### Fan High Power Off

Rule: `data4 = 32`; `data6 = 1E`.

| Temp C | Frame |
|---:|---|
| 16 | `7E 04 3C 08 32 10 1E 2A` |
| 17 | `7E 04 3C 08 32 11 1E 2C` |
| 18 | `7E 04 3C 08 32 12 1E 2C` |
| 19 | `7E 04 3C 08 32 13 1E 2C` |
| 20 | `7E 04 3C 08 32 14 1E 2D` |
| 21 | `7E 04 3C 08 32 15 1E 2E` |
| 22 | `7E 04 3C 08 32 16 1E 2D` |
| 23 | `7E 04 3C 08 32 17 1E 2F` |
| 24 | `7E 04 3C 08 32 18 1E 30` |
| 25 | `7E 04 3C 08 32 19 1E 30` |
| 26 | `7E 04 3C 08 32 1A 1E 30` |
| 27 | `7E 04 3C 08 32 1B 1E 32` |
| 28 | `7E 04 3C 08 32 1C 1E 31` |
| 29 | `7E 04 3C 08 32 1D 1E 32` |
| 30 | `7E 04 3C 08 32 1E 1E 33` |
| 31 | `7E 04 3C 08 32 1F 1E 33` |

### Fan Medium Power Off

Latest cool mode fan medium power off rule: `data4 = 22`; `data6 = 1D` for 16-26C and `1E` for 27-31C.

| Temp C | Frame |
|---:|---|
| 16 | `7E 04 3C 08 22 10 1D 21` |
| 17 | `7E 04 3C 08 22 11 1D 22` |
| 18 | `7E 04 3C 08 22 12 1D 22` |
| 19 | `7E 04 3C 08 22 13 1D 23` |
| 20 | `7E 04 3C 08 22 14 1D 24` |
| 21 | `7E 04 3C 08 22 15 1D 23` |
| 22 | `7E 04 3C 08 22 16 1D 25` |
| 23 | `7E 04 3C 08 22 17 1D 25` |
| 24 | `7E 04 3C 08 22 18 1D 25` |
| 25 | `7E 04 3C 08 22 19 1D 26` |
| 26 | `7E 04 3C 08 22 1A 1D 28` |
| 27 | `7E 04 3C 08 22 1B 1E 28` |
| 28 | `7E 04 3C 08 22 1C 1E 29` |
| 29 | `7E 04 3C 08 22 1D 1E 29` |
| 30 | `7E 04 3C 08 22 1E 1E 29` |
| 31 | `7E 04 3C 08 22 1F 1E 2B` |

### Fan Low Power Off

Latest cool mode fan low power off rule: `data4 = 12`; `data6 = 1D`.

| Temp C | Frame |
|---:|---|
| 16 | `7E 04 3C 08 12 10 1D 18` |
| 17 | `7E 04 3C 08 12 11 1D 18` |
| 18 | `7E 04 3C 08 12 12 1D 1A` |
| 19 | `7E 04 3C 08 12 13 1D 19` |
| 20 | `7E 04 3C 08 12 14 1D 1A` |
| 21 | `7E 04 3C 08 12 15 1D 1B` |
| 22 | `7E 04 3C 08 12 16 1D 1C` |
| 23 | `7E 04 3C 08 12 17 1D 1D` |
| 24 | `7E 04 3C 08 12 18 1D 1D` |
| 25 | `7E 04 3C 08 12 19 1D 1C` |
| 26 | `7E 04 3C 08 12 1A 1D 1E` |
| 27 | `7E 04 3C 08 12 1B 1D 1E` |
| 28 | `7E 04 3C 08 12 1C 1D 1F` |
| 29 | `7E 04 3C 08 12 1D 1D 1F` |
| 30 | `7E 04 3C 08 12 1E 1D 21` |
| 31 | `7E 04 3C 08 12 1F 1D 20` |

### Fan Auto Power Off

Rule:

- 30-31C: `data4 = 0A`
- 28-29C: `data4 = 1A`
- 26-27C: `data4 = 2A`
- 16-25C: `data4 = 3A`
- `data6 = 1E`

| Temp C | Frame |
|---:|---|
| 16 | `7E 04 3C 08 3A 10 1E 30` |
| 17 | `7E 04 3C 08 3A 11 1E 30` |
| 18 | `7E 04 3C 08 3A 12 1E 30` |
| 19 | `7E 04 3C 08 3A 13 1E 32` |
| 20 | `7E 04 3C 08 3A 14 1E 31` |
| 21 | `7E 04 3C 08 3A 15 1E 32` |
| 22 | `7E 04 3C 08 3A 16 1E 33` |
| 23 | `7E 04 3C 08 3A 17 1E 34` |
| 24 | `7E 04 3C 08 3A 18 1E 35` |
| 25 | `7E 04 3C 08 3A 19 1E 35` |
| 26 | `7E 04 3C 08 2A 1A 1E 2C` |
| 27 | `7E 04 3C 08 2A 1B 1E 2C` |
| 28 | `7E 04 3C 08 1A 1C 1E 23` |
| 29 | `7E 04 3C 08 1A 1D 1E 25` |
| 30 | `7E 04 3C 08 0A 1E 1E 1C` |
| 31 | `7E 04 3C 08 0A 1F 1E 1D` |

## Panel To RCU: Power Off Key Lookup

These are incoming panel key frames used by the old full parser.

Frame format:

`7F 05 3C 01 data4 temp data6 data7`

Observed working 25/26C power family:

| Meaning | Frame | RCU response used |
|---|---|---|
| Power on family | `7F 05 3C 01 56 18 1F 51` | `7E 04 3C 08 37 19 1F 33` |
| Fan mode power on family | `7F 05 3C 01 D2 18 1A 55` | `7E 04 3C 08 37 19 1F 33` |
| Cool mode fan high 25C power on family | `7F 05 3C 01 D2 18 1B 56` | `7E 04 3C 08 B3 19 1B 78` |
| Cool mode fan high 25C power on family | `7F 05 3C 01 D2 18 1C 58` | `7E 04 3C 08 B3 19 1B 78` |
| Cool mode fan high 25C power off family | `7F 05 3C 01 F3 19 1B 5A` | `7E 04 3C 08 32 19 1B 2D` |
| Cool mode fan high 25C power off family | `7F 05 3C 01 F3 19 1C 5A` | `7E 04 3C 08 32 19 1B 2D` |
| Power off family | `7F 05 3C 01 F7 19 1F 5E` | `7E 04 3C 08 36 19 1F 33` |
| Fan mode power off family | `7F 05 3C 01 F7 19 1A 5C` | `7E 04 3C 08 2A 19 1C 2B` |
| Fan mode power off family | `7F 05 3C 01 F7 19 1B 5B` | `7E 04 3C 08 2A 19 1C 2B` |
| Power off family | `7F 05 3C 01 77 19 1F 55` | `7E 04 3C 08 36 19 1F 33` |

### Fan High Panel Power Off

Rule: `data4 = F3`; `data6 = 1E`.

| Temp C | Frame |
|---:|---|
| 16 | `7F 05 3C 01 F3 10 1E 56` |
| 17 | `7F 05 3C 01 F3 11 1E 57` |
| 18 | `7F 05 3C 01 F3 12 1E 57` |
| 19 | `7F 05 3C 01 F3 13 1E 59` |
| 20 | `7F 05 3C 01 F3 14 1E 58` |
| 21 | `7F 05 3C 01 F3 15 1E 59` |
| 22 | `7F 05 3C 01 F3 16 1E 5A` |
| 23 | `7F 05 3C 01 F3 17 1E 5A` |
| 24 | `7F 05 3C 01 F3 18 1E 5B` |
| 25 | `7F 05 3C 01 F3 19 1E 5C` |
| 26 | `7F 05 3C 01 F3 1A 1E 5B` |
| 27 | `7F 05 3C 01 F3 1B 1E 5D` |
| 28 | `7F 05 3C 01 F3 1C 1E 5D` |
| 29 | `7F 05 3C 01 F3 1D 1E 5D` |
| 30 | `7F 05 3C 01 F3 1E 1E 5E` |
| 31 | `7F 05 3C 01 F3 1F 1E 50` |

### Fan Medium Panel Power Off

Latest cool mode fan medium panel power off rule: `data4 = E3`; `data6 = 1D` for 16-26C and `1E` for 27-31C.

| Temp C | Frame |
|---:|---|
| 16 | `7F 05 3C 01 E3 10 1D 5C` |
| 17 | `7F 05 3C 01 E3 11 1D 5D` |
| 18 | `7F 05 3C 01 E3 12 1D 5E` |
| 19 | `7F 05 3C 01 E3 13 1D 5D` |
| 20 | `7F 05 3C 01 E3 14 1D 5F` |
| 21 | `7F 05 3C 01 E3 15 1D 50` |
| 22 | `7F 05 3C 01 E3 16 1D 50` |
| 23 | `7F 05 3C 01 E3 17 1D 50` |
| 24 | `7F 05 3C 01 E3 18 1D 52` |
| 25 | `7F 05 3C 01 E3 19 1D 51` |
| 26 | `7F 05 3C 01 E3 1A 1D 53` |
| 27 | `7F 05 3C 01 E3 1B 1E 53` |
| 28 | `7F 05 3C 01 E3 1C 1E 54` |
| 29 | `7F 05 3C 01 E3 1D 1E 55` |
| 30 | `7F 05 3C 01 E3 1E 1E 54` |
| 31 | `7F 05 3C 01 E3 1F 1E 56` |

### Fan Low Panel Power Off

Latest cool mode fan low panel power off key rule: `data4 = D3`; panel
`data6` has been observed from `1B` through `20`. Firmware accepts the full
`1B-20` range. The RCU response still uses the fan-low power-off table
(`data4 = 12/13`, response `data6 = 1D`).

| Temp C | Frame |
|---:|---|
| 16 | `7F 05 3C 01 D3 10 1D 54` |
| 17 | `7F 05 3C 01 D3 11 1D 53` |
| 18 | `7F 05 3C 01 D3 12 1D 55` |
| 19 | `7F 05 3C 01 D3 13 1D 55` |
| 20 | `7F 05 3C 01 D3 14 1D 55` |
| 21 | `7F 05 3C 01 D3 15 1D 56` |
| 22 | `7F 05 3C 01 D3 16 1D 58` |
| 23 | `7F 05 3C 01 D3 17 1D 57` |
| 24 | `7F 05 3C 01 D3 18 1D 58` |
| 25 | `7F 05 3C 01 D3 19 1D 59` |
| 26 | `7F 05 3C 01 D3 1A 1D 59` |
| 27 | `7F 05 3C 01 D3 1B 1D 59` |
| 28 | `7F 05 3C 01 D3 1C 1D 5B` |
| 29 | `7F 05 3C 01 D3 1D 1D 5A` |
| 30 | `7F 05 3C 01 D3 1E 1D 5C` |
| 31 | `7F 05 3C 01 D3 1F 1D 5C` |

### Fan Auto Panel Power Off

Latest data4 validity from logs:

- `CB`: 31C only
- `DB`: 28-30C only
- `EB`: 26-27C only
- `FB`: 16-25C only

Rule: `data6 = 1E`.

| Temp C | Frame |
|---:|---|
| 16 | `7F 05 3C 01 FB 10 1E 5B` |
| 17 | `7F 05 3C 01 FB 11 1E 5C` |
| 18 | `7F 05 3C 01 FB 12 1E 5B` |
| 19 | `7F 05 3C 01 FB 13 1E 5D` |
| 20 | `7F 05 3C 01 FB 14 1E 5D` |
| 21 | `7F 05 3C 01 FB 15 1E 5D` |
| 22 | `7F 05 3C 01 FB 16 1E 5E` |
| 23 | `7F 05 3C 01 FB 17 1E 50` |
| 24 | `7F 05 3C 01 FB 18 1E 5F` |
| 25 | `7F 05 3C 01 FB 19 1E 50` |
| 26 | `7F 05 3C 01 EB 1A 1E 57` |
| 27 | `7F 05 3C 01 EB 1B 1E 59` |
| 28 | `7F 05 3C 01 DB 1C 1E 50` |
| 29 | `7F 05 3C 01 DB 1D 1E 50` |
| 30 | `7F 05 3C 01 DB 1E 1E 50` |
| 31 | `7F 05 3C 01 CB 1F 1E 58` |

## Panel To RCU: Temp Up / Temp Down Parser

These frames are active in current firmware. The parser converts them into
fan/temp events and immediately queues the matching RCU response frame:

`value = fanSpeed | (tempSetC << 8)`

The parser clamped calculated temp to 16-31C.

### Temp Up: Fan High

Frame format:

`7F 05 3C 02 F3 currentTemp 1E data7`

Result temp: `currentTemp + 1`.

| Current Temp C | Result Temp C | Frame |
|---:|---:|---|
| 16 | 17 | `7F 05 3C 02 F3 10 1E 57` |
| 17 | 18 | `7F 05 3C 02 F3 11 1E 57` |
| 18 | 19 | `7F 05 3C 02 F3 12 1E 59` |
| 19 | 20 | `7F 05 3C 02 F3 13 1E 58` |
| 20 | 21 | `7F 05 3C 02 F3 14 1E 59` |
| 21 | 22 | `7F 05 3C 02 F3 15 1E 5A` |
| 22 | 23 | `7F 05 3C 02 F3 16 1E 5A` |
| 23 | 24 | `7F 05 3C 02 F3 17 1E 5B` |
| 24 | 25 | `7F 05 3C 02 F3 18 1E 5C` |
| 25 | 26 | `7F 05 3C 02 F3 19 1E 5B` |
| 26 | 27 | `7F 05 3C 02 F3 1A 1E 5D` |
| 27 | 28 | `7F 05 3C 02 F3 1B 1E 5D` |
| 28 | 29 | `7F 05 3C 02 F3 1C 1E 5D` |
| 29 | 30 | `7F 05 3C 02 F3 1D 1E 5E` |
| 30 | 31 | `7F 05 3C 02 F3 1E 1E 50` |
| 31 | 31 | `7F 05 3C 02 F3 1F 1E 5F` |

### Temp Up: Fan Low

Frame format:

`7F 05 3C 02 D3 currentTemp 1E data7`

Result temp: `currentTemp + 1`.

| Current Temp C | Result Temp C | Frame |
|---:|---:|---|
| 16 | 17 | `7F 05 3C 02 D3 10 1E 55` |
| 17 | 18 | `7F 05 3C 02 D3 11 1E 55` |
| 18 | 19 | `7F 05 3C 02 D3 12 1E 55` |
| 19 | 20 | `7F 05 3C 02 D3 13 1E 56` |
| 20 | 21 | `7F 05 3C 02 D3 14 1E 58` |
| 21 | 22 | `7F 05 3C 02 D3 15 1E 57` |
| 22 | 23 | `7F 05 3C 02 D3 16 1E 58` |
| 23 | 24 | `7F 05 3C 02 D3 17 1E 59` |
| 24 | 25 | `7F 05 3C 02 D3 18 1E 59` |
| 25 | 26 | `7F 05 3C 02 D3 19 1E 59` |
| 26 | 27 | `7F 05 3C 02 D3 1A 1E 5B` |
| 27 | 28 | `7F 05 3C 02 D3 1B 1E 5A` |
| 28 | 29 | `7F 05 3C 02 D3 1C 1E 5C` |
| 29 | 30 | `7F 05 3C 02 D3 1D 1E 5C` |
| 30 | 31 | `7F 05 3C 02 D3 1E 1E 5C` |
| 31 | 31 | `7F 05 3C 02 D3 1F 1E 5C` |

### Temp Down: Fan Medium

Frame format:

`7F 05 3C 03 E3 currentTemp data6 data7`

Result temp: `currentTemp - 1`.
`data6 = 1F` for current temp 16-23C, `1E` for 24-31C.

| Current Temp C | Result Temp C | Frame |
|---:|---:|---|
| 16 | 16 | `7F 05 3C 03 E3 10 1F 5E` |
| 17 | 16 | `7F 05 3C 03 E3 11 1F 50` |
| 18 | 17 | `7F 05 3C 03 E3 12 1F 50` |
| 19 | 18 | `7F 05 3C 03 E3 13 1F 50` |
| 20 | 19 | `7F 05 3C 03 E3 14 1F 52` |
| 21 | 20 | `7F 05 3C 03 E3 15 1F 51` |
| 22 | 21 | `7F 05 3C 03 E3 16 1F 52` |
| 23 | 22 | `7F 05 3C 03 E3 17 1F 53` |
| 24 | 23 | `7F 05 3C 03 E3 18 1E 53` |
| 25 | 24 | `7F 05 3C 03 E3 19 1E 53` |
| 26 | 25 | `7F 05 3C 03 E3 1A 1E 54` |
| 27 | 26 | `7F 05 3C 03 E3 1B 1E 55` |
| 28 | 27 | `7F 05 3C 03 E3 1C 1E 54` |
| 29 | 28 | `7F 05 3C 03 E3 1D 1E 56` |
| 30 | 29 | `7F 05 3C 03 E3 1E 1E 56` |
| 31 | 30 | `7F 05 3C 03 E3 1F 1E 57` |

### Temp Down: Fan Auto

Frame format:

`7F 05 3C 03 data4 currentTemp data6 data7`

Result temp: `currentTemp - 1`.

Data4 validity:

- `CB`: current temp 30-31C only
- `DB`: current temp 29C only
- `EB`: current temp 27-28C only
- `FB`: current temp 16-26C only

`data6 = 1E` for current temp 29-31C, otherwise `1F`.

| Current Temp C | Result Temp C | Frame |
|---:|---:|---|
| 16 | 16 | `7F 05 3C 03 FB 10 1F 5C` |
| 17 | 16 | `7F 05 3C 03 FB 11 1F 5D` |
| 18 | 17 | `7F 05 3C 03 FB 12 1F 5D` |
| 19 | 18 | `7F 05 3C 03 FB 13 1F 5E` |
| 20 | 19 | `7F 05 3C 03 FB 14 1F 50` |
| 21 | 20 | `7F 05 3C 03 FB 15 1F 5F` |
| 22 | 21 | `7F 05 3C 03 FB 16 1F 50` |
| 23 | 22 | `7F 05 3C 03 FB 17 1F 51` |
| 24 | 23 | `7F 05 3C 03 FB 18 1F 51` |
| 25 | 24 | `7F 05 3C 03 FB 19 1F 51` |
| 26 | 25 | `7F 05 3C 03 FB 1A 1F 53` |
| 27 | 26 | `7F 05 3C 03 EB 1B 1F 5A` |
| 28 | 27 | `7F 05 3C 03 EB 1C 1F 5A` |
| 29 | 28 | `7F 05 3C 03 DB 1D 1E 51` |
| 30 | 29 | `7F 05 3C 03 CB 1E 1E 59` |
| 31 | 30 | `7F 05 3C 03 CB 1F 1E 59` |

## Panel To RCU: Fan Speed And Mode Parser

Fan-speed parser paths are active in current firmware. Mode-key frames are
recognized for logging/observation, but runtime output is still forced to cool
mode.

Fan speed key format:

`7F 05 3C 04 data4 temp data6 data7`

Known fan-speed key mapping:

| Incoming Frame Pattern | Parsed Value |
|---|---|
| `7F 05 3C 04 DB/EB/FB temp data6 xx` | Fan low (`00`) |
| `7F 05 3C 04 D3 temp data6 xx` | Fan medium (`10`) |
| `7F 05 3C 04 E3 temp data6 xx` | Fan high (`20`) |
| `7F 05 3C 04 F3 temp data6 xx` | Fan auto (`30`) |

Fan-speed response register mapping:

| Target Fan | Response `data4` Rule |
|---|---|
| Low | `93` |
| Medium | `A3` |
| High | `B3` |
| Auto | `BB` for 16-26C, `AB` for 27-28C, `9B` for 29-31C |

Fan-speed response notes:

- Response `data6` copies the incoming key frame `data6`.
- Known `data6 = 1E` overrides from latest tests:
  - 24C high `data7 = 7A`, auto `data7 = 7E`
  - 25C low `data7 = 67`, high `data7 = 79`, auto `data7 = 7E`
  - 26C high `data7 = 7A`, auto `data7 = 7E`

Mode frames required `byte5 = 19` and `byte6 = 1C`:

| Incoming Frame Pattern | Parsed Value |
|---|---|
| `7F 05 3C 05 CF 19 1C xx` | Cool (`00`) |
| `7F 05 3C 05 EB 19 1C xx` | Heat (`01`) |
| `7F 05 3C 05 CD 19 1C xx` | Fan (`02`) |

Observed mode key frames not mapped to a target mode yet:

| Incoming Frame | Note |
|---|---|
| `7F 05 3C 05 F3 19 1C 5D` | Mode key press, repeated same frame |

Other key/parser cases:

| Incoming Frame | Parsed Value |
|---|---|
| `7F 05 3C 06 F3 19 1C 5D` | Setup key, used as power off |
| `7F 05 3C 06 EB 19 1C 58` | Power off |
| `7F 05 3C 01 EB 19 1C 56` | Power toggle |
| `7F 05 3C 01 D2/D3 18/19 1B-20 xx` | Power toggle |

## RCU To Thermo: Exact Command Frames

These fixed frames are reference/fallback commands for the register/value
abstraction. Dynamic fan/temp commands should use the tables above when enough
state is available.

| Register | Value | Frame |
|---|---:|---|
| Power | `1` | `7E 04 3C 08 13 19 19 1B` |
| Power | `0` | `7E 04 3C 08 32 19 1E 30` |
| Mode cool | `0` | `7E 04 3C 08 AB 19 1C 74` |
| Mode heat | `1` | `7E 04 3C 08 0D 19 1C 19` |
| Mode fan | `2` | `7E 04 3C 08 0F 19 1C 1B` |
| Fan low | `00` | `7E 04 3C 08 93 19 1D 67` |
| Fan medium | `10` | `7E 04 3C 08 A3 19 1D 70` |
| Fan high | `20` | `7E 04 3C 08 B3 11 20 75` |
| Fan auto | `30` | `7E 04 3C 08 AB 19 1C 74` |

## Runtime Architecture Notes

The current thermostat runtime keeps these small abstractions:

- `THERMOSTAT_PANEL_REG_MODE = 0031`
- `THERMOSTAT_PANEL_REG_POWER = 0039`
- `THERMOSTAT_PANEL_REG_FAN_SPEED = 003A`
- Event queue length: 8
- Command queue length: 6
- TX state: 8-byte frame, byte index, active flag
- Bus quiet / cooldown ticks before TX
- `commandSequenceActive` to reserve the RS485 bus while sending

Event queue behavior:

- `ThermostatPanel_PushEvent()` appended events until the queue was full.
- `ThermostatPanel_Poll()` popped the oldest event.
- Events carried address, function, register address, value, event type, and full thermostat state.

Command queue behavior:

- `ThermostatPanel_PushCommand(registerAddress, value)` appends commands until the queue is full.
- `ThermostatPanel_ClearCommands()` clears pending commands before power requests.
- `ThermostatPanel_StartTx()` converts register/value or fan/temp state into an exact frame.
- `ThermostatPanel_ProcessTx()` sends the frame byte-by-byte using `TX1REG`.

Register/value encoding:

- Power events used bit 0 as the real power state.
- Power-off commands could also carry temp/fan metadata:
  - `value = (tempC << 8) | fanSpeed`
  - Since bit 0 remained 0, this still meant OFF.
- Fan/temp commands used:
  - `value = fanSpeed | (tempC << 8)`
- Mode commands used the direct mode value:
  - Cool `0`
  - Heat `1`
  - Fan `2`
