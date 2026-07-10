#pragma once
#include <stdint.h>

#define RCU_INPBUF_COUNT 6
#define RCU_OUTBUF_COUNT 12
#define RCU_INPBUF_REG_COUNT ((RCU_INPBUF_COUNT + 1) / 2)
#define RCU_OUTBUF_REG_COUNT ((RCU_OUTBUF_COUNT + 1) / 2)
#define RCU_OUTBUF_COIL_BYTE_COUNT (RCU_OUTBUF_COUNT - 1)
#define RCU_OUTBUF_COIL_COUNT (RCU_OUTBUF_COIL_BYTE_COUNT * 8)

#define Bit0 0x01
#define Bit1 0x02
#define Bit2 0x04
#define Bit3 0x08
#define Bit4 0x10
#define Bit5 0x20
#define Bit6 0x40
#define Bit7 0x80

/* Inpbuf[0] service/master input aliases */
#define Dndin  Bit0
#define Murin  Bit1
#define Bellin Bit2
#define Guein  Bit3
#define Maidin Bit4
#define Ccsin  Bit5
#define Masin  Bit6
#define Doorin Bit7

/* Inpbuf[0] general input aliases */
#define Inp1in Bit0
#define Inp2in Bit1
#define Inp3in Bit2
#define Inp4in Bit3
#define Inp5in Bit4
#define Inp6in Bit5
#define Inp7in Bit6
#define Inp8in Bit7

/* Inpbuf[1] */
#define Inp9in  Bit0
#define Inp10in Bit1
#define Inp11in Bit2
#define Inp12in Bit3
#define Inp13in Bit4
#define Inp14in Bit5
#define Inp15in Bit6
#define Inp16in Bit7

/* Inpbuf[2] */
#define Inp17in Bit0
#define Inp18in Bit1
#define Inp19in Bit2
#define Inp20in Bit3
#define Inp21in Bit4
#define Inp22in Bit5
#define Inp23in Bit6
#define Inp24in Bit7

/* Inpbuf[3] */
#define Inp25in Bit0
#define Inp26in Bit1
#define Inp27in Bit2
#define Inp28in Bit3
#define Inp29in Bit4
#define Inp30in Bit5
#define Inp31in Bit6
#define Inp32in Bit7

/* Inpbuf[4] */
#define Inp33in Bit0
#define Inp34in Bit1
#define Inp35in Bit2
#define Inp36in Bit3
#define Inp37in Bit4
#define Inp38in Bit5
#define Inp39in Bit6
#define Inp40in Bit7

/* Inpbuf[5] */
#define Inp41in Bit0
#define Inp42in Bit1
#define Inp43in Bit2
#define Inp44in Bit3
#define Inp45in Bit4
#define Inp46in Bit5
#define Inp47in Bit6
#define Inp48in Bit7

#define IN_DND    Dndin
#define IN_MUR    Murin
#define IN_BELL   Bellin
#define IN_GUEST  Guein
#define IN_MAID   Maidin
#define IN_CCS    Ccsin
#define IN_MASTER Masin
#define IN_DOOR   Doorin

/* Outbuf[0] LAMP */
#define Out1 Bit0
#define Out2 Bit1
#define Out3 Bit2
#define Out4 Bit3
#define Out5 Bit4
#define Out6 Bit5
#define Out7 Bit6
#define Out8 Bit7

/* Outbuf[1] LAMP */
#define Out9  Bit0
#define Out10 Bit1
#define Out11 Bit2
#define Out12 Bit3
#define Out13 Bit4
#define Out14 Bit5
#define Out15 Bit6
#define Out16 Bit7

/* Outbuf[2] LAMP */
#define Out17 Bit0
#define Out18 Bit1
#define Out19 Bit2
#define Out20 Bit3
#define Out21 Bit4
#define Out22 Bit5
#define Out23 Bit6
#define Out24 Bit7

/* Outbuf[3] LAMP */
#define Out25 Bit0
#define Out26 Bit1
#define Out27 Bit2
#define Out28 Bit3
#define Out29 Bit4
#define Out30 Bit5
#define Out31 Bit6
#define Out32 Bit7

/* Outbuf[4] CONTROL */
#define Dnd    Bit0
#define Mur    Bit1
#define Aux1Out Bit2
#define Aux2Out Bit3
#define Master Aux2Out
#define Door   Bit4
#define Ccs    Bit5
#define Maid   Bit6
#define Guest  Bit7

/* Outbuf[5] AIR */
#define Temp     0x0f
#define FanL     0x00
#define FanM     0x10
#define FanH     0x20
#define FanA     0x30
#define FanSpeed 0x30
#define AirCdu   Bit6
#define AirPow   Bit7

#define RCU_OUTBUF_LAMP1 0
#define RCU_OUTBUF_LAMP2 1
#define RCU_OUTBUF_LAMP3 2
#define RCU_OUTBUF_LAMP4 3
#define RCU_OUTBUF_CONTROL 4
#define RCU_OUTBUF_AIR 5
#define RCU_OUTBUF_TEMP_ACTUAL 6
#define RCU_OUTBUF_HOUR 7
#define RCU_OUTBUF_MINUTE 8
#define RCU_OUTBUF_ALARM_HOUR 9
#define RCU_OUTBUF_ALARM_MINUTE 10
#define RCU_OUTBUF_NODE_ID 11

#define AlmHours 0x3f
#define Alarms   Bit6
#define Alarmp   Bit7
#define AlmMin   0xff
#define NodeId   0x01
