# Hotel Register Map (Latest)

## Input Register

---

# `0x00 : REG_INP0_ROOM_STATUS`

```c
/* Inpbuf[0] ROOM STATUS */

#define Dndin     Bit0    //DND Input
#define Murin     Bit1    //Makeup Room Input
#define Bellin    Bit2    //Bell Input
#define Guein     Bit3    //Guest Input
#define Maidin    Bit4    //Maid Input
#define Ccsin     Bit5    //CCS/PMS Input
#define Masin     Bit6    //Master Input
#define Doorin    Bit7    //Door Sensor Input
```

| Bit | Name | Description |
|---|---|---|
| Bit0 | DND_IN | Do Not Disturb |
| Bit1 | MUR_IN | Makeup Room |
| Bit2 | BELL_IN | Bell |
| Bit3 | GUEST_IN | Guest Status |
| Bit4 | MAID_IN | Maid Status |
| Bit5 | CCS_IN | PMS/CCS |
| Bit6 | MASTER_IN | Master |
| Bit7 | DOOR_IN | Door Sensor |

---

# `0x01 : REG_INP1_EXPAND`

```c
/* Inpbuf[1] */

#define Inp9in    Bit0
#define Inp10in   Bit1
#define Inp11in   Bit2
#define Inp12in   Bit3
#define Inp13in   Bit4
#define Inp14in   Bit5
#define Inp15in   Bit6
#define Inp16in   Bit7
```

---

# `0x02 : REG_INP2_EXPAND`

```c
/* Inpbuf[2] */

#define Inp17in   Bit0
#define Inp18in   Bit1
#define Inp19in   Bit2
#define Inp20in   Bit3
#define Inp21in   Bit4
#define Inp22in   Bit5
#define Inp23in   Bit6
#define Inp24in   Bit7
```

---

# `0x03 : REG_INP3_SLAVE`

```c
/* Inpbuf[3] */

#define Inp25in   Bit0
#define Inp26in   Bit1
#define Inp27in   Bit2
#define Inp28in   Bit3
#define Inp29in   Bit4
#define Inp30in   Bit5
#define Inp31in   Bit6
#define Inp32in   Bit7
```

---

# `0x04 : REG_INP4_SLAVE`

```c
/* Inpbuf[4] */

#define Inp33in   Bit0
#define Inp34in   Bit1
#define Inp35in   Bit2
#define Inp36in   Bit3
#define Inp37in   Bit4
#define Inp38in   Bit5
#define Inp39in   Bit6
#define Inp40in   Bit7
```

---

# `0x05 : REG_INP5_SLAVE`

```c
/* Inpbuf[5] */

#define Inp41in   Bit0
#define Inp42in   Bit1
#define Inp43in   Bit2
#define Inp44in   Bit3
#define Inp45in   Bit4
#define Inp46in   Bit5
#define Inp47in   Bit6
#define Inp48in   Bit7
```

---

# Output Register

---

# `0x10 : REG_OUT0_LAMP`

```c
/* Outbuf[0] */

#define Out1      Bit0
#define Out2      Bit1
#define Out3      Bit2
#define Out4      Bit3
#define Out5      Bit4
#define Out6      Bit5
#define Out7      Bit6
#define Out8      Bit7
```

---

# `0x11 : REG_OUT1_LAMP`

```c
/* Outbuf[1] */

#define Out9      Bit0
#define Out10     Bit1
#define Out11     Bit2
#define Out12     Bit3
#define Out13     Bit4
#define Out14     Bit5
#define Out15     Bit6
#define Out16     Bit7
```

---

# `0x12 : REG_OUT2_LAMP`

```c
/* Outbuf[2] */

#define Out17     Bit0
#define Out18     Bit1
#define Out19     Bit2
#define Out20     Bit3
#define Out21     Bit4
#define Out22     Bit5
#define Out23     Bit6
#define Out24     Bit7
```

---

# `0x13 : REG_OUT3_LAMP`

```c
/* Outbuf[3] */

#define Out25     Bit0
#define Out26     Bit1
#define Out27     Bit2
#define Out28     Bit3
#define Out29     Bit4
#define Out30     Bit5
#define Out31     Bit6
#define Out32     Bit7
```

---

# `0x14 : REG_OUT4_CONTROL`

```c
/* Outbuf[4] CONTROL */

#define Dnd        Bit0    //DND Output
#define Mur        Bit1    //Makeup Room Output
#define Aux1Out    Bit2    //Auxiliary Output 1
#define Aux2Out    Bit3    //Auxiliary Output 2
#define Door       Bit4    //Door Output
#define Ccs        Bit5    //CCS/PMS Output
#define Maid       Bit6    //Maid Output
#define Guest      Bit7    //Guest Output
```

| Bit | Name | Description |
|---|---|---|
| Bit0 | DND_OUT | DND Output |
| Bit1 | MUR_OUT | Makeup Room |
| Bit2 | AUX1_OUT | Auxiliary Output 1 |
| Bit3 | AUX2_OUT | Auxiliary Output 2 |
| Bit4 | DOOR_OUT | Door Output |
| Bit5 | CCS_OUT | PMS/CCS |
| Bit6 | MAID_OUT | Maid Output |
| Bit7 | GUEST_OUT | Guest Output |

---

# `0x15 : REG_AIR_CONTROL`

```c
/* Outbuf[5] AIR */

#define Temp      0x0F    //Temperature Setpoint Mask
#define FanSpeed  0x30    //Fan Speed Mask

#define AirCdu    Bit6    //Condenser Unit Enable
#define AirPow    Bit7    //Air Conditioner Power

#define FanL      0x00    //Low Speed
#define FanM      0x10    //Medium Speed
#define FanH      0x20    //High Speed
#define FanA      0x30    //Auto Speed
```

| Bits | Description |
|---|---|
| Bit0-3 | Temperature Setpoint |
| Bit4-5 | Fan Speed |
| Bit6 | Condenser Enable |
| Bit7 | Air Power |

Temperature:

```c
TempValue = (REG_AIR & 0x0F) + 15;
```

---

# Time / Alarm Register

| Address | Register | Description |
|---|---|---|
| 0x16 | REG_TEMP_ACTUAL | Actual Temperature |
| 0x17 | REG_REAL_HOUR | Real Time Hour |
| 0x18 | REG_REAL_MIN | Real Time Minute |

---

# `0x19 : REG_ALARM_HOUR`

```c
#define AlmHours  0x3F    //Alarm Hour
#define Alarms    Bit6    //Alarm Setup
#define Alarmp    Bit7    //Alarm Enable
```

---

# `0x1A : REG_ALARM_MIN`

```c
#define AlmMin    0xFF    //Alarm Minute
```

---

# `0x1B : REG_NODE_ID`

```c
#define NodeId    0x01    //Default slave/node ID
```

In the gateway firmware this byte is exposed as `Outbuf[11]`, which is the high
byte of Modbus holding register `5` (`HR5 = Outbuf[11] << 8 | Outbuf[10]`).
The gateway stores it in ESP32 flash using NVS, restores it on boot, and treats
it as the primary room/node identifier for Modbus/BMS use. Valid runtime values
are `1..247`. `Outbuf[11]` is not included in the Modbus coil map and is not
sent to the RCU through private write commands. If the RCU reports its own
`Outbuf[11]` value, the gateway ignores it and publishes the gateway-owned NVS
value instead.

---

# Register Address Summary

| Address | Register |
|---|---|
| 0x00 | ROOM STATUS |
| 0x01 | INPUT 9-16 |
| 0x02 | INPUT 17-24 |
| 0x03 | INPUT 25-32 |
| 0x04 | INPUT 33-40 |
| 0x05 | INPUT 41-48 |
| 0x10 | OUTPUT 1-8 |
| 0x11 | OUTPUT 9-16 |
| 0x12 | OUTPUT 17-24 |
| 0x13 | OUTPUT 25-32 |
| 0x14 | CONTROL OUTPUT |
| 0x15 | AIR CONTROL |
| 0x16 | ACTUAL TEMP |
| 0x17 | REAL HOUR |
| 0x18 | REAL MIN |
| 0x19 | ALARM HOUR |
| 0x1A | ALARM MIN |
| 0x1B | NODE ID |
