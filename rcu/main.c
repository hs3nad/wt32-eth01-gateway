/**
  Generated Main Source File
  
  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This is the main file generated using PIC10 / PIC12 / PIC16 / PIC18 MCUs

  Description:
    This header file provides implementations for driver APIs for all modules selected in the GUI.
    Generation Information :
        Product Revision  :  PIC10 / PIC12 / PIC16 / PIC18 MCUs - 1.81.8
        Device            :  PIC16F18325
        Driver Version    :  2.00
 */

/*
    (c) 2018 Microchip Technology Inc. and its subsidiaries. 
    
    Subject to your compliance with these terms, you may use Microchip software and any 
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party 
    license terms applicable to your use of third party software (including open source software) that 
    may accompany Microchip software.
    
    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER 
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY 
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS 
    FOR A PARTICULAR PURPOSE.
    
    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP 
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO 
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL 
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT 
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS 
    SOFTWARE.
 */

/* PCB POWERSV18V5 */
/*          PIC16F18325
         +----------------+	
    VDD -|1	 VDD    VSS 14|- VSS
    RTS -|2	 RA5    RA0 13|- SCK
    SPK -|3	 RA4    RA1 12|- SDO
    RES -|4	 RA3    RA2 11|- SDI
     RX	-|5	 RC5    RC0 10|- STR
     TX -|6	 RC4    RC1  9|- SCL
    DIR -|7	 RC3    RC2  8|- SDA
         +----------------+
 */

#include "mcc_generated_files/mcc.h"
#include "project_config.h"
#include "switch/switch_input.h"
#include "switch/switch_led.h"
#include "thermostat/thermostat_panel.h"

#define MASTER_BOARD
#define PUSHBUTTON

//#define _SETALARM       //Setup Alarm
//#define _SETTIME 				//Setup time
//#define _SETTEMP        //Setup temp
#ifdef MASTER_BOARD
//#define USE_RTC
#define USE_SERVICE
//#define USE_ADEL
//#define USE_RFID
//#define USE_DOOR    
//#define USE_CCS
//#define USE_MAID
//#define USE_TEMPLOCK
#endif

/* ID Address */
#define IDADDR    1             //Board ID  
#define POWERUP   3*2           //3 Sec 
#define OFFTIME   30*2          //30 Sec
#define DOORON    10*2          //10 Sec
#define DOOROFF   1*120         //1 Minute
#define TIMEOUT   15*120        //15 Minute
#define MOTION    10*2          //15 Sec
#define MAIN_TIMER_TICK_MS 10u
#define MAIN_INPUT_DELAY_MS 250u
#define MAIN_HALF_TIMER_MS 250u
#define MAIN_TIME_TIMER_MS 500u
#define MAXIN     24

/* Switch Select */
#ifdef PUSHBUTTON
#define INDelay (100/10)
#else
#define INDelay (100/10)
#endif

/* Data bit */
#define Bit0		0x01			//Data bit 0
#define Bit1		0x02			//Data bit 1
#define Bit2		0x04			//Data bit 2
#define Bit3		0x08			//Data bit 3
#define Bit4		0x10			//Data bit 4
#define Bit5		0x20			//Data bit 5
#define Bit6		0x40			//Data bit 6
#define Bit7		0x80			//Data bit 7

/* DS1302 */
#define ENABLE  0x00      //NONE PROTECT
#define DISBLE  0x80      //PROTECT
#define BURST   0xBE      //BURST MODE
#define SECADD  0x80      //SECOND ADDRESS
#define MINADD  0x82      //MINUTE ADDRESS
#define HSRADD  0x84      //HOUR ADDRESS
#define DATADD  0x86      //DATE ADDRESS
#define MONADD  0x88      //MONTH ADDRESS
#define DAYADD  0x8A      //DAY ADDRESS
#define YERADD  0x8C      //YEAR ADDRESS
#define CONADD  0x8E      //CONTROL ADDRESS
#define CHGADD  0x90      //CONTROL ADDRESS
#define DATA0   0xc0      //DATA ADDRESS
#define DATA1   0xc2      //DATA ADDRESS
#define DATA2   0xc4      //DATA ADDRESS
#define DATA3   0xc6      //DATA ADDRESS
#define DATA4   0xc8      //DATA ADDRESS
#define DATA5   0xca      //DATA ADDRESS
#define DATA6   0xcc      //DATA ADDRESS
#define DATA7   0xce      //DATA ADDRESS
#define DATA8   0xd0      //DATA ADDRESS
#define DATA9   0xd2      //DATA ADDRESS
#define DATA10  0xd4      //DATA ADDRESS
#define DATA11  0xd6      //DATA ADDRESS
#define DATA12  0xd8      //DATA ADDRESS
#define DATA13  0xda      //DATA ADDRESS
#define DATA14  0xdc      //DATA ADDRESS
#define DATA15  0xde      //DATA ADDRESS
#define DATA16  0xe0      //DATA ADDRESS
#define DATA17  0xe2      //DATA ADDRESS
#define DATA18  0xe4      //DATA ADDRESS
#define DATA19  0xe6      //DATA ADDRESS
#define DATA20  0xe8      //DATA ADDRESS
#define DATA21  0xea      //DATA ADDRESS
#define DATA22  0xec      //DATA ADDRESS
#define DATA23  0xee      //DATA ADDRESS
#define DATA24  0xf0      //DATA ADDRESS
#define DATA25  0xf2      //DATA ADDRESS
#define DATA26  0xf4      //DATA ADDRESS
#define DATA27  0xf6      //DATA ADDRESS
#define DATA28  0xf8      //DATA ADDRESS
#define DATA29  0xfa      //DATA ADDRESS
#define RTCMEM  0xfc      //MEMORY ADDRESS

/* KEYBOARD */
#define	Key1		0x00			//None
#define	Key2		0x01			//None
#define	Key3		0x02			//None
#define	Key4		0x03			//None
#define	Key5		0x04			//None
#define	Key6		0x05			//None
#define	Key7		0x06			//None
#define	Key8		0x07			//None
#define	Key9		0x08			//None
#define	Key10		0x09			//None
#define	Key11		0x0a			//None
#define	Key12		0x0b			//None
#define	Key13		0x0c			//None
#define	Key14		0x0d			//None
#define	Key15		0x0e			//None
#define	Key16		0x0f			//None
#define	Key17		0x10			//None
#define	Key18		0x11			//None
#define	Key19		0x12			//None
#define	Key20		0x13			//None
#define	Key21		0x14			//None
#define	Key22		0x15			//None
#define	Key23		0x16			//None
#define	Key24		0x17			//None
#define	Key25		0x18			//None
#define	Key26		0x19			//None
#define	Key27		0x20			//None
#define	Key28		0x21			//None
#define	Key29		0x22			//None
#define	Key30		0x23			//None
#define	Key31		0x24			//None
#define	Key32		0x25			//None
#define	Keyxx		0x26			//None

/* Light */
#define In1sw     Key1    //In1
#define In2sw     Key2    //In2
#define In3sw     Key3    //In3
#define In4sw     Key4    //In4
#define In5sw     Key5    //In5
#define In6sw     Key6    //In6
#define In7sw     Key7    //In7
#define In8sw     Key8    //In8
#define In9sw     Key9    //In9
#define In10sw    Key10   //In10
#define In11sw    Key11   //In11
#define In12sw    Key12   //In12
#define In13sw    Key13   //In13
#define In14sw    Key14   //In14
#define In15sw    Key15   //In15
#define In16sw    Key16   //In16
#define In17sw    Key17   //In17
#define In18sw    Key18   //In18
#define In19sw    Key19   //In19
#define In20sw    Key20   //In20
#define In21sw    Key21   //In21
#define In22sw    Key22   //In22
#define In23sw    Key23   //In23
#define In24sw    Key24   //In24
#define In25sw    Key25   //In25
#define In26sw    Key26   //In26
#define In27sw    Key27   //In27
#define In28sw    Key28   //In28
#define Mas1sw	  Key29   //Master

#ifdef _AIR
/* Air */
#define Airpowsw  Keyxx   //Air power
#define Tempupsw	Keyxx   //Temp up
#define Tempdnsw	Keyxx   //Temp down
#define Fanhighsw	Keyxx   //Fan speed high
#define Fanmedsw	Keyxx   //Fan speed med
#define Fanlowsw	Keyxx   //Fan speed low
#define Fanupsw		Keyxx   //Fan speed up
#define Fandnsw		Keyxx   //Fan speed down
#define Fanspeedsw Keyxx   //Fan speed
#endif

#ifdef _TV
/* Tv */
#define Tvpowersw	Keyxx   //Tv power
#define Tvchupsw	Keyxx   //Tv ch up
#define Tvchdnsw	Keyxx   //Tv ch down
#define Tvvolupsw	Keyxx   //Tv volume up
#define Tvvoldnsw	Keyxx   //Tv volume down
#endif

#ifdef USE_SERVICE
/* Service */
#define Dndsw     Key30   //Do not disturb
#define Mursw     Key31   //Make up room
#define Bellsw    Key32   //Bell
#endif

#ifdef _SETALARM
/* Alarm */
#define Almpowersw Keyxx  //Alarm on/off
#define Almsetsw	Keyxx   //Alarm setup
#define Almhoursw	Keyxx   //Alarm hour
#define Almminsw	Keyxx   //Alarm minute
#endif

#ifdef _SETTIME
/* Clock */
#define	Hoursw    Key21   //None
#define	Minsw     Key22   //None
#endif

/* Master */
#define RESET_MASTER
#define USE_SWITCH_BACKLIGHT_OFF
#define MASTER1_1_ON (Out5 | Out6 | Out7 | Out8)
#define MASTER1_1_OFF (Out5 | Out6 | Out7 | Out8)
#define MASTER1_2_ON (Out9 | Out10 | Out11)
#define MASTER1_2_OFF (Out9 | Out10 | Out11)
#define MASTER1_3_ON 0x00
#define MASTER1_3_OFF 0x00
#define MASTER1_4_ON 0x00
#define MASTER1_4_OFF 0x00
#define WELCOME1 (Out5 | Out6 | Out7 | Out8)
#define WELCOME2 (Out9 | Out10 | Out11 | Out12)
#define WELCOME3 0x00
#define WELCOME4 0x00
#define DOOR_OUT  Out12

#ifdef MASTER_BOARD
// On Master board 2way Input
// #define _2WAY1  In1sw      //C5/bell/input 1
// #define _2WAY2	In2sw			  //C6/mur/input 2
// #define _2WAY3	In3sw			  //C7/dnd/input 3
// #define _2WAY4  Mas1sw    //C8/master/input 4
// #define _2WAY5  In4sw   //B1/guest/input 5
// #define _2WAY6  In5sw     //B2/maid/input 6
// #define _2WAY7  In6sw     //B3/ccs/input 7
// #define _2WAY8  In7sw     //B4/door/input 8
// #define _2WAY9  In8sw     //B5/input 9
// #define _2WAY10 In9sw     //B6/input 10
// #define _2WAY11 In10sw    //B7/input 11
// #define _2WAY12 In11sw    //B8/input 12
// #define _2WAY13 Mas1sw    //A1/input 13
// #define _2WAY14 In13sw    //A2/input 14
// #define _2WAY15 In14sw    //A3/input 15
// #define _2WAY16 In11sw    //A4/input 16
// #define _2WAY17 In12sw    //A5/input 17
// #define _2WAY18 Mas1sw    //A6/input 18
// #define _2WAY19 Mas1sw    //A7/input 19
// #define _2WAY20 Mas1sw    //A8/input 20
#else
// On Slave input board 2way Input
#define _2WAY21 In1sw       //C5/bell/input 1
#define _2WAY22	In2sw			  //C6/mur/input 2
#define _2WAY23	In3sw			  //C7/dnd/input 3
#define _2WAY24 Mas1sw    //C8/master/input 4
#define _2WAY25 In1sw       //B1/guest/input 5
#define _2WAY26 In2sw       //B2/maid/input 6
#define _2WAY27 In3sw       //B3/ccs/input 7
#define _2WAY28 In4sw       //B4/door/input 8
#define _2WAY29 In5sw       //B5/input 9
#define _2WAY30 In6sw       //B6/input 10
#define _2WAY31 In7sw       //B7/input 11
#define _2WAY32 In8sw       //B8/input 12
#define _2WAY33 In9sw       //A1/input 13
#define _2WAY34 In10sw      //A2/input 14
#define _2WAY35 In11sw      //A3/input 15
#define _2WAY36 In12sw      //A4/input 16
#define _2WAY37 In13sw      //A5/input 17
#define _2WAY38 In14sw      //A6/input 18
#define _2WAY39 In15sw      //A7/input 19
#define _2WAY40 In16sw      //A8/input 20
#endif

// Switch Input
#define _MASTER						//Master Input
//#define _INPUT1						//Out 1
//#define _INPUT2						//Out 2
//#define _INPUT3						//Out 3
//#define _INPUT4						//Out 4
#define _INPUT5						//Out 5
#define _INPUT6           //Out 6
#define _INPUT7           //Out 7
#define _INPUT8          //Out 8
#define _INPUT9          //Out 9
#define _INPUT10					//Out 10
#define _INPUT11					//Out 11
//#define _INPUT12					//Out 12
//#define _INPUT13					//Out 13
//#define _INPUT14					//Out 14
//#define _INPUT15          //Out 15
//#define _INPUT16          //Out 16
//#define _INPUT17          //Out 17
//#define _INPUT18					//Out 18
//#define _INPUT19					//Out 19
//#define _INPUT20					//Out 20
//#define _INPUT21          //Out 21
//#define _INPUT22					//Out 22
//#define _INPUT23					//Out 23
//#define _INPUT24					//Out 24
//#define _INPUT25          //Out 25
//#define _INPUT26					//Out 26
//#define _INPUT27					//Out 27
//#define _INPUT28					//Out 28

#ifdef MASTER_BOARD
//On Master board Relay
#define _OUTPUT1  (AirEnable && ((Outbuf[RCU_OUTBUF_AIR] & FanSpeed) & FanM))   //Out 1
#define _OUTPUT2	(AirEnable && ((Outbuf[RCU_OUTBUF_AIR] & FanSpeed) & FanH))	  //Out 2
#define _OUTPUT3  (AirEnable && (Outbuf[RCU_OUTBUF_AIR] & AirCdu))    //Out 3
#define _OUTPUT4  (AirEnable && (Outbuf[RCU_OUTBUF_AIR] & AirPow))    //Out 4
#define _OUTPUT5  (Outbuf[RCU_OUTBUF_LAMP1] & Out5)      //Out 5
#define _OUTPUT6  (Outbuf[RCU_OUTBUF_LAMP1] & Out6)      //Out 6
#define _OUTPUT7  (Outbuf[RCU_OUTBUF_LAMP1] & Out7)      //Out 7
#define _OUTPUT8  (Outbuf[RCU_OUTBUF_LAMP1] & Out8)      //Out 8
#define _OUTPUT9	(Outbuf[RCU_OUTBUF_LAMP2] & Out9)			//Out 9
#define _OUTPUT10	(Outbuf[RCU_OUTBUF_LAMP2] & Out10)			//Out 10
#define _OUTPUT11 (Outbuf[RCU_OUTBUF_LAMP2] & Out11)      //Out 11
#define _OUTPUT12 (Outbuf[RCU_OUTBUF_LAMP2] & Out12)      //Out 12
//#define _OUTPUT13	(Outbuf[RCU_OUTBUF_LAMP2] & Out9)			//Out 13
//#define _OUTPUT14	(Outbuf[RCU_OUTBUF_LAMP2] & Out10)			//Out 14
//#define _OUTPUT15 (Outbuf[RCU_OUTBUF_LAMP2] & Out11)     //Out 15
//#define _OUTPUT16 (Outbuf[RCU_OUTBUF_LAMP2] & Out12)     //Out 16
//#define _OUTPUT17 (Outbuf[RCU_OUTBUF_LAMP2] & Out9)     //Out 17
//#define _OUTPUT18 (Outbuf[RCU_OUTBUF_LAMP2] & Out10)     //Out 18
//#define _OUTPUT19 (Outbuf[RCU_OUTBUF_LAMP2] & Out11)     //Out 19
//#define _OUTPUT20 (Outbuf[RCU_OUTBUF_LAMP2] & Out12)     //Out 20
//#define _OUTPUT21	(Outbuf[RCU_OUTBUF_LAMP2] & Out13)			//Out 21
//#define _OUTPUT22	(Outbuf[RCU_OUTBUF_LAMP2] & Out14)			//Out 22
//#define _OUTPUT23 (Outbuf[RCU_OUTBUF_LAMP2] & Out15)     //Out 23
//#define _OUTPUT24 (Outbuf[RCU_OUTBUF_LAMP2] & Out16)     //Out 24
//#define _OUTPUT25	(Outbuf[RCU_OUTBUF_LAMP4] & Out25)			//Out 25
//#define _OUTPUT26	(Outbuf[RCU_OUTBUF_LAMP4] & Out26)			//Out 26
//#define _OUTPUT27 (Outbuf[RCU_OUTBUF_LAMP4] & Out27)     //Out 27
//#define _OUTPUT28 (Outbuf[RCU_OUTBUF_LAMP4] & Out28)     //Out 28
#else
//On Slave board Relay
#define _OUTPUT29 (Outbuf[RCU_OUTBUF_LAMP1] & Out1)      //Out 1
#define _OUTPUT30	(Outbuf[RCU_OUTBUF_LAMP1] & Out2)			//Out 2
#define _OUTPUT31 (Outbuf[RCU_OUTBUF_LAMP1] & Out3)      //Out 3
#define _OUTPUT32 (Outbuf[RCU_OUTBUF_LAMP1] & Out4)      //Out 4
#define _OUTPUT33 (Outbuf[RCU_OUTBUF_LAMP1] & Out5)      //Out 5
#define _OUTPUT34 (Outbuf[RCU_OUTBUF_LAMP1] & Out6)      //Out 6
#define _OUTPUT35 (Outbuf[RCU_OUTBUF_LAMP1] & Out7)      //Out 7
#define _OUTPUT36 (Outbuf[RCU_OUTBUF_LAMP1] & Out8)      //Out 8
#define _OUTPUT37	(Outbuf[RCU_OUTBUF_LAMP2] & Out9)			//Out 9
#define _OUTPUT38	(Outbuf[RCU_OUTBUF_LAMP2] & Out10)			//Out 10
#define _OUTPUT39 (Outbuf[RCU_OUTBUF_LAMP2] & Out11)     //Out 11
#define _OUTPUT40 (Outbuf[RCU_OUTBUF_LAMP2] & Out12)     //Out 12
#define _OUTPUT41	(Outbuf[RCU_OUTBUF_LAMP2] & Out13)			//Out 13
#define _OUTPUT42	(Outbuf[RCU_OUTBUF_LAMP2] & Out14)			//Out 14
#define _OUTPUT43 (Outbuf[RCU_OUTBUF_LAMP2] & Out15)     //Out 15
#define _OUTPUT44 (Outbuf[RCU_OUTBUF_LAMP2] & Out16)     //Out 16
#define _OUTPUT45 (Outbuf[RCU_OUTBUF_LAMP3] & Out17)     //Out 17
#define _OUTPUT46 (Outbuf[RCU_OUTBUF_LAMP3] & Out18)     //Out 18
#define _OUTPUT47 (Outbuf[RCU_OUTBUF_LAMP3] & Out19)     //Out 19
#define _OUTPUT48 (Outbuf[RCU_OUTBUF_LAMP3] & Out20)     //Out 20
#define _OUTPUT49	(Outbuf[RCU_OUTBUF_LAMP3] & Out21)			//Out 21
#define _OUTPUT50	(Outbuf[RCU_OUTBUF_LAMP3] & Out22)			//Out 22
#define _OUTPUT51 (Outbuf[RCU_OUTBUF_LAMP3] & Out23)     //Out 23
#define _OUTPUT52 (Outbuf[RCU_OUTBUF_LAMP3] & Out24)     //Out 24
#define _OUTPUT53	(Outbuf[RCU_OUTBUF_LAMP4] & Out25)			//Out 25
#define _OUTPUT54	(Outbuf[RCU_OUTBUF_LAMP4] & Out26)			//Out 26
#define _OUTPUT55 (Outbuf[RCU_OUTBUF_LAMP4] & Out27)     //Out 27
#define _OUTPUT56 (Outbuf[RCU_OUTBUF_LAMP4] & Out28)     //Out 28
#endif

#define USE_DNDLED      //Use DND LED
#define USE_MURLED      //Use MUR LED
#define USE_BELSPK      //Use BEL SPK
#define USE_KEYLED      //Use KEY LED

/* SYSFLAG REGISTER */
bool Timeflag; //Timer flag
bool Halfflag; //half Time flag
bool PowUpflag; //Power flag
bool Powerled; //Power flag
bool Inpflag; //Inp flag
bool Keyflag; //Key flag
bool Rcvflag; //Serial flag
bool Rcflag; //Serial flag
bool Txdflag; //Txd flag
bool Rxdflag; //Rxd flag
bool Dndflag; //Dnd flag
bool upDateFlag1;
bool upDateFlag2;

uint8_t Timer; //Time counter
uint8_t PowUpcnt; //Power counter
uint8_t Doorcnt; //Door counter
uint8_t Inpcnt; //Power counter
uint8_t Offcnt; //Off counter
uint8_t Keybuf; //Keyboard Buffer
uint8_t Boxbuf; //Keybox buffer
uint8_t Dndcnt; //Do not counter
uint8_t Blkcnt; //Blink counter

uint8_t Swibuf[3]; //Switch input
uint8_t Inpbuf[6]; //Input buffer
uint8_t Inpmem[6]; //Input buffer
uint8_t Relbuf[4]; //Relay output
uint8_t Outbuf[12]; //Relay buffer
uint8_t upDateCnt1;

#if defined(_SERIAL)
uint8_t Rcvbuf[36]; //Rxd Buffer
uint8_t *Rcvptr; //Rxd Poiter
uint8_t Rcvdly; //Rxd Delay
uint8_t Rxdaddr; //Rxd address
uint8_t Txdcom; //Txd command
uint8_t Rxdcom; //Rxd command
#endif

#if defined(USE_RFID)
bool Boxflag; //Keybox flag
uint8_t Keybox[16]; //Keybox Buffer
#endif

typedef enum {
  SHOWTEMP = 0, // Show temp
  SETTEMP, // Set temp
  SETFAN, // Set fan
  SETPOWER, // Set power
  SHOWHUMI, // Show humidity
  PUTKEY // Send key
} CMD_TYPE;

typedef enum {
  NONE = 0, // No Card
  GUEST, // Guest
  FLOOR, // Floor & Maid
  BUILDING, // Building
  EMERGENCY, // Emergency
  MASTER // Master
} CARD_TYPE;

//typedef enum {
//  _ALL = 0, // All
//  _KEY, // Keypad
//  _INPUT, // Input
//  _AIR, // Air
//  NULL1, // Building
//  _RFID, // Rfid
//  _MQTT // Mqtt
//} NODE_TYPE;

struct {
  uint8_t cnt;
  unsigned low : 1;
  unsigned high : 1;
  unsigned out : 1;
  unsigned inp : 1;
} inpBuff[MAXIN];

// Master Board
/* Inpbuf[0] ROOM STATUS */
#define	Dndin   Bit0       			//Dnd Input
#define	Murin   Bit1       			//Mur Input
#define	Bellin  Bit2       			//Bell Input
#define	Guein   Bit3       			//Guest Input
#define	Maidin  Bit4       			//Maid Input
#define	Ccsin   Bit5       			//CCS Input
#define	Masin   Bit6       			//Master Input
#define	Doorin  Bit7       			//Door Input

/* Inpbuf[0] */
#define	Inp1in  Bit0       			//In1 Input
#define	Inp2in  Bit1       			//In2 Input
#define	Inp3in  Bit2       			//In3 Input
#define	Inp4in  Bit3       			//In4 Input
#define	Inp5in  Bit4       			//In5 Input
#define	Inp6in  Bit5       			//In6 Input
#define	Inp7in  Bit6       			//In7 Input
#define	Inp8in  Bit7       			//In8 Input

/* Inpbuf[1] */
#define	Inp9in  Bit0       			//In9 Input
#define	Inp10in Bit1       			//In10 Input
#define	Inp11in Bit2       			//In11 Input
#define	Inp12in Bit3       			//In12 Input
#define	Inp13in Bit4       			//In13 Input
#define	Inp14in Bit5       			//In14 Input
#define	Inp15in Bit6       			//In15 Input
#define	Inp16in Bit7       			//In16 Input

/* Inpbuf[2] */
#define	Inp17in Bit0       			//In17 Input
#define	Inp18in Bit1       			//In18 Input
#define	Inp19in Bit2       			//In19 Input
#define	Inp20in Bit3       			//In20 Input
#define	Inp21in Bit4       			//In21 Input
#define	Inp22in Bit5       			//In22 Input
#define	Inp23in Bit6       			//In23 Input
#define	Inp24in Bit7       			//In24 Input

// Slave Board
/* Inpbuf[3] */
#define	Inp25in Bit0       			//In1 Input
#define	Inp26in Bit1       			//In2 Input
#define	Inp27in Bit2       			//In3 Input
#define	Inp28in Bit3       			//In4 Input
#define	Inp29in Bit4       			//In5 Input
#define	Inp30in Bit5       			//In6 Input
#define	Inp31in Bit6       			//In7 Input
#define	Inp32in Bit7       			//In8 Input

/* Inpbuf[4] */
#define	Inp33in Bit0       			//In9 Input
#define	Inp34in Bit1       			//In10 Input
#define	Inp35in Bit2       			//In11 Input
#define	Inp36in Bit3       			//In12 Input
#define	Inp37in Bit4       			//In13 Input
#define	Inp38in Bit5       			//In14 Input
#define	Inp39in Bit6       			//In15 Input
#define	Inp40in Bit7       			//In16 Input

/* Inpbuf[5] */
#define	Inp41in Bit0       			//In17 Input
#define	Inp42in Bit1       			//In18 Input
#define	Inp43in Bit2       			//In19 Input
#define	Inp44in Bit3       			//In20 Input
#define	Inp45in Bit4       			//In21 Input
#define	Inp46in Bit5       			//In22 Input
#define	Inp47in Bit6       			//In23 Input
#define	Inp48in Bit7       			//In24 Input

/* Outbuf index map */
#define RCU_OUTBUF_LAMP1        0
#define RCU_OUTBUF_LAMP2        1
#define RCU_OUTBUF_LAMP3        2
#define RCU_OUTBUF_LAMP4        3
#define RCU_OUTBUF_CONTROL      4
#define RCU_OUTBUF_AIR          5
#define RCU_OUTBUF_TEMP_ACTUAL  6
#define RCU_OUTBUF_HOUR         7
#define RCU_OUTBUF_MINUTE       8
#define RCU_OUTBUF_ALARM_HOUR   9
#define RCU_OUTBUF_ALARM_MINUTE 10
#define RCU_OUTBUF_NODE_ID      11

/* Outbuf[0] LAMP */
#define Out1      Bit0
#define Out2      Bit1
#define Out3      Bit2
#define Out4      Bit3
#define Out5      Bit4
#define Out6      Bit5
#define Out7      Bit6
#define Out8      Bit7

/* Outbuf[1] LAMP */
#define Out9      Bit0
#define Out10     Bit1
#define Out11     Bit2
#define Out12     Bit3
#define Out13     Bit4
#define Out14     Bit5
#define Out15     Bit6
#define Out16     Bit7

/* Outbuf[2] LAMP */
#define Out17     Bit0
#define Out18     Bit1
#define Out19     Bit2
#define Out20     Bit3
#define Out21     Bit4
#define Out22     Bit5
#define Out23     Bit6
#define Out24     Bit7

/* Outbuf[3] LAMP */
#define Out25     Bit0
#define Out26     Bit1
#define Out27     Bit2
#define Out28     Bit3
#define Out29     Bit4
#define Out30     Bit5
#define Out31     Bit6
#define Out32     Bit7

/* Outbuf[4] CONTROL */
#define Dnd       Bit0          //Dnd Output
#define Mur       Bit1          //Mur Output
#define	Aux1Out   Bit2       		//Auxiliary Output 1
#define	Aux2Out   Bit3       		//Auxiliary Output 2
#define	Master    Aux2Out     	//Master Output
#define	Door      Bit4       		//Door Output
#define	Ccs       Bit5       		//CCS Output
#define	Maid      Bit6       		//Maid Output
#define	Guest     Bit7       		//Guest Output

/* Outbuf[5] AIR */
#define Temp      0x0f
#define FanL      0x00
#define FanM      0x10
#define FanH      0x20
#define FanA      0x30
#define FanSpeed  0x30
#define AirCdu    Bit6          //Condenser Unit Enable
#define AirPow    Bit7
#define NodeId    0x01          //Slave Node ID
#define AirEnable (Outbuf[RCU_OUTBUF_CONTROL] & Guest)

static bool MasterOutputGroupHasAnyOn(void) {
  bool anyOn = false;

#if MASTER1_1_ON != 0u
  anyOn = anyOn || ((Outbuf[RCU_OUTBUF_LAMP1] & (uint8_t)MASTER1_1_ON) != 0u);
#endif
#if MASTER1_2_ON != 0u
  anyOn = anyOn || ((Outbuf[RCU_OUTBUF_LAMP2] & (uint8_t)MASTER1_2_ON) != 0u);
#endif
#if MASTER1_3_ON != 0u
  anyOn = anyOn || ((Outbuf[RCU_OUTBUF_LAMP3] & (uint8_t)MASTER1_3_ON) != 0u);
#endif
#if MASTER1_4_ON != 0u
  anyOn = anyOn || ((Outbuf[RCU_OUTBUF_LAMP4] & (uint8_t)MASTER1_4_ON) != 0u);
#endif

  return anyOn;
}

static void MasterOutput_SyncControlBit(void) {
  if (MasterOutputGroupHasAnyOn()) {
    Outbuf[RCU_OUTBUF_CONTROL] |= (uint8_t)Master;
  } else {
    Outbuf[RCU_OUTBUF_CONTROL] &= (uint8_t)~Master;
  }
}

#ifdef _SWITCH
static bool SwitchLedMasterRefreshPending = true;
static bool SwitchLedMasterLedEnabled = true;

static void SwitchLed_RequestMasterLedRefresh(void) {
  SwitchLedMasterLedEnabled = true;
  SwitchLedMasterRefreshPending = true;
}

static bool SwitchLed_IsMasterGroupOff(void) {
  bool allOff = true;

#if MASTER1_1_OFF != 0u
  allOff = allOff && ((Outbuf[RCU_OUTBUF_LAMP1] & MASTER1_1_OFF) == 0u);
#endif
#if MASTER1_2_OFF != 0u
  allOff = allOff && ((Outbuf[RCU_OUTBUF_LAMP2] & MASTER1_2_OFF) == 0u);
#endif
#if MASTER1_3_OFF != 0u
  allOff = allOff && ((Outbuf[RCU_OUTBUF_LAMP3] & MASTER1_3_OFF) == 0u);
#endif
#if MASTER1_4_OFF != 0u
  allOff = allOff && ((Outbuf[RCU_OUTBUF_LAMP4] & MASTER1_4_OFF) == 0u);
#endif

  return allOff;
}

static void SwitchLed_CheckManualMasterGroupOn(uint8_t key) {
  bool turnsOn = false;

  if (!SwitchLed_IsMasterGroupOff()) {
    return;
  }

  turnsOn = (((key == In5sw) && ((Outbuf[RCU_OUTBUF_LAMP1] & Out5) == 0u))
          || ((key == In6sw) && ((Outbuf[RCU_OUTBUF_LAMP1] & Out6) == 0u))
          || ((key == In7sw) && ((Outbuf[RCU_OUTBUF_LAMP1] & Out7) == 0u))
          || ((key == In8sw) && ((Outbuf[RCU_OUTBUF_LAMP1] & Out8) == 0u))
          || ((key == In9sw) && ((Outbuf[RCU_OUTBUF_LAMP2] & Out9) == 0u))
          || ((key == In10sw) && ((Outbuf[RCU_OUTBUF_LAMP2] & Out10) == 0u))
          || ((key == In11sw) && ((Outbuf[RCU_OUTBUF_LAMP2] & Out11) == 0u)));

  if (turnsOn) {
    SwitchLedMasterLedEnabled = false;
    SwitchLedMasterRefreshPending = true;
  }
}

static void SwitchLed_SyncMappedOutput(uint8_t key, bool guestOn, bool enabled) {
  uint8_t occurrence = 0u;
  uint8_t panelAddress;
  uint8_t panelKeyNumber;

  while (SwitchInput_GetPanelMappingDetails(
      key,
      occurrence,
      &panelAddress,
      &panelKeyNumber)) {
    SwitchLed_SetPanelKey(panelAddress, panelKeyNumber, (guestOn && enabled));
    occurrence++;
  }
}

static bool SwitchLed_IsMasterLedOn(void) {
  return SwitchLedMasterLedEnabled;
}

static void SwitchLed_SyncFromOutputs(void) {
  static uint8_t lastLamp1 = 0u;
  static uint8_t lastLamp2 = 0u;
  static uint8_t lastLamp3 = 0u;
  static uint8_t lastLamp4 = 0u;
  static uint8_t lastControl = 0u;
  static bool hasLastSnapshot = false;
  uint8_t lamp1Snapshot = Outbuf[RCU_OUTBUF_LAMP1];
  uint8_t lamp2Snapshot = Outbuf[RCU_OUTBUF_LAMP2];
  uint8_t lamp3Snapshot = Outbuf[RCU_OUTBUF_LAMP3];
  uint8_t lamp4Snapshot = Outbuf[RCU_OUTBUF_LAMP4];
  uint8_t controlSnapshot;
  bool guestOn;
  bool forcePanelRefresh;
  bool masterRefreshNow = (SwitchLedMasterRefreshPending == true);

  MasterOutput_SyncControlBit();
  controlSnapshot = Outbuf[RCU_OUTBUF_CONTROL];
  guestOn = ((controlSnapshot & Guest) != 0u);
  forcePanelRefresh = (!hasLastSnapshot
                    || ((lastControl & Guest) != (controlSnapshot & Guest)));

  if (forcePanelRefresh) {
    SwitchLedMasterLedEnabled = true;
  }

  if (hasLastSnapshot
   && (lastLamp1 == lamp1Snapshot)
   && (lastLamp2 == lamp2Snapshot)
   && (lastLamp3 == lamp3Snapshot)
   && (lastLamp4 == lamp4Snapshot)
   && (lastControl == controlSnapshot)
   && (SwitchLedMasterRefreshPending == false)) {
    return;
  }

  lastLamp1 = lamp1Snapshot;
  lastLamp2 = lamp2Snapshot;
  lastLamp3 = lamp3Snapshot;
  lastLamp4 = lamp4Snapshot;
  lastControl = controlSnapshot;
  hasLastSnapshot = true;

  if (forcePanelRefresh || masterRefreshNow) {
    SwitchLed_SyncMappedOutput(Mas1sw, guestOn, SwitchLed_IsMasterLedOn());
    SwitchLedMasterRefreshPending = false;
  }

  SwitchLed_SyncMappedOutput(In1sw, guestOn, (Outbuf[RCU_OUTBUF_LAMP1] & Out1) != 0u);
  SwitchLed_SyncMappedOutput(In2sw, guestOn, (Outbuf[RCU_OUTBUF_LAMP1] & Out2) != 0u);
  SwitchLed_SyncMappedOutput(In3sw, guestOn, (Outbuf[RCU_OUTBUF_LAMP1] & Out3) != 0u);
  SwitchLed_SyncMappedOutput(In4sw, guestOn, (Outbuf[RCU_OUTBUF_LAMP1] & Out4) != 0u);
  SwitchLed_SyncMappedOutput(In5sw, guestOn, (Outbuf[RCU_OUTBUF_LAMP1] & Out5) != 0u);
  SwitchLed_SyncMappedOutput(In6sw, guestOn, (Outbuf[RCU_OUTBUF_LAMP1] & Out6) != 0u);
  SwitchLed_SyncMappedOutput(In7sw, guestOn, (Outbuf[RCU_OUTBUF_LAMP1] & Out7) != 0u);
  SwitchLed_SyncMappedOutput(In8sw, guestOn, (Outbuf[RCU_OUTBUF_LAMP1] & Out8) != 0u);
  SwitchLed_SyncMappedOutput(In9sw, guestOn, (Outbuf[RCU_OUTBUF_LAMP2] & Out9) != 0u);
  SwitchLed_SyncMappedOutput(In10sw, guestOn, (Outbuf[RCU_OUTBUF_LAMP2] & Out10) != 0u);
  SwitchLed_SyncMappedOutput(In11sw, guestOn, (Outbuf[RCU_OUTBUF_LAMP2] & Out11) != 0u);
#ifdef USE_SERVICE
  SwitchLed_SyncMappedOutput(Dndsw, true, (Outbuf[RCU_OUTBUF_CONTROL] & Dnd) != 0u);
  SwitchLed_SyncMappedOutput(Mursw, true, (Outbuf[RCU_OUTBUF_CONTROL] & Mur) != 0u);
#endif

  if (forcePanelRefresh || masterRefreshNow) {
    SwitchLed_RequestRefresh();
  }
}

#endif

#ifdef _THERMOSTAT
static void ThermostatPanel_SyncCommandFromAirOutput(bool force);

static bool ThermostatPanel_IsAirEnabled(void) {
  return (AirEnable != 0u);
}

static void ThermostatPanel_ClearAirOutput(void) {
  Outbuf[RCU_OUTBUF_AIR] &= (uint8_t)~AirPow;
  Outbuf[RCU_OUTBUF_AIR] &= (uint8_t)~FanSpeed;
  Outbuf[RCU_OUTBUF_AIR] &= (uint8_t)~AirCdu;
}

static void ThermostatPanel_HandleTxdEvent(void) {
#ifdef _SWITCH
  SwitchLed_SyncFromOutputs();
#endif
  ThermostatPanel_SyncCommandFromAirOutput(false);
}

static void ThermostatPanel_UpdateAirCdu(const THERMOSTAT_PANEL_STATE *state) {
  if ((state == (const THERMOSTAT_PANEL_STATE *)0)
   || (!ThermostatPanel_IsAirEnabled())
   || ((Outbuf[RCU_OUTBUF_AIR] & (uint8_t)AirPow) == 0u)
   || (state->mode != THERMOSTAT_PANEL_MODE_COOL)) {
    Outbuf[RCU_OUTBUF_AIR] &= (uint8_t)~AirCdu;
    return;
  }

  if ((state->tempSetC >= 16u) && (state->tempSetC <= 31u)
   && (state->roomTempByte >= 0x1Bu) && (state->roomTempByte <= 0x20u)) {
    if (state->tempSetC > state->roomTempByte) {
      Outbuf[RCU_OUTBUF_AIR] &= (uint8_t)~AirCdu;
    } else if (state->tempSetC < state->roomTempByte) {
      Outbuf[RCU_OUTBUF_AIR] |= AirCdu;
    }
  }
}

static void ThermostatPanel_UpdateOutputAir(const THERMOSTAT_PANEL_EVENT *event) {
  if (event == (const THERMOSTAT_PANEL_EVENT *)0) {
    return;
  }

  if ((event->state.roomTempByte >= 0x1Bu)
   && (event->state.roomTempByte <= 0x20u)) {
    Outbuf[RCU_OUTBUF_TEMP_ACTUAL] = event->state.roomTempByte;
  }

  if ((event->type == THERMOSTAT_PANEL_EVENT_POWER)
   || (event->type == THERMOSTAT_PANEL_EVENT_MODE)
   || (event->type == THERMOSTAT_PANEL_EVENT_FAN_SPEED)) {
    if (!ThermostatPanel_IsAirEnabled()) {
      ThermostatPanel_ClearAirOutput();
    } else if (event->state.power) {
      Outbuf[RCU_OUTBUF_AIR] |= AirPow;
    } else {
      Outbuf[RCU_OUTBUF_AIR] &= (uint8_t)~AirPow;
      Outbuf[RCU_OUTBUF_AIR] &= (uint8_t)~AirCdu;
    }

    if ((event->type == THERMOSTAT_PANEL_EVENT_POWER)
     || (event->type == THERMOSTAT_PANEL_EVENT_FAN_SPEED)) {
      Outbuf[RCU_OUTBUF_AIR] &= (uint8_t)~FanSpeed;
      Outbuf[RCU_OUTBUF_AIR] |= (uint8_t)(event->state.fanSpeed & FanSpeed);
    }

    if ((event->state.tempSetC >= 16u) && (event->state.tempSetC <= 31u)) {
      Outbuf[RCU_OUTBUF_AIR] &= (uint8_t)~Temp;
      Outbuf[RCU_OUTBUF_AIR] |= (uint8_t)(event->state.tempSetC - 16u);
    }

    ThermostatPanel_UpdateAirCdu(&event->state);
    if (event->type == THERMOSTAT_PANEL_EVENT_POWER) {
      ThermostatPanel_SyncCommandFromAirOutput(true);
    }
  } else if (event->type == THERMOSTAT_PANEL_EVENT_ROOM_TEMP) {
    ThermostatPanel_UpdateAirCdu(&event->state);
  }

  if (!ThermostatPanel_IsAirEnabled()) {
    ThermostatPanel_ClearAirOutput();
  }
}

static void ThermostatPanel_SyncCommandFromAirOutput(bool force) {
  static bool hasLastPower = false;
  static bool lastPower = false;
  static uint8_t lastFanSpeed = FanL;
  static uint8_t lastTempSetC = 17u;
  bool guestNow;
  bool powerNow;
  bool powerChanged;
  uint8_t fanSpeedNow;
  uint8_t tempSetC;

  if (!PowUpflag) {
    return;
  }

  guestNow = ((Outbuf[RCU_OUTBUF_CONTROL] & Guest) != 0u);
  fanSpeedNow = (uint8_t)(Outbuf[RCU_OUTBUF_AIR] & FanSpeed);
  tempSetC = (uint8_t)((Outbuf[RCU_OUTBUF_AIR] & Temp) + 16u);
  powerNow = (guestNow && ((Outbuf[RCU_OUTBUF_AIR] & AirPow) != 0u));
  if ((!guestNow) && hasLastPower) {
    fanSpeedNow = lastFanSpeed;
    tempSetC = lastTempSetC;
  }
  powerChanged = false;

  if (!hasLastPower) {
    lastPower = powerNow;
    lastFanSpeed = fanSpeedNow;
    lastTempSetC = tempSetC;
    hasLastPower = true;
    if (!powerNow) {
      ThermostatPanel_ClearAirOutput();
      return;
    }
  } else {
    if ((!force)
     && (lastPower == powerNow)
     && ((!powerNow) || ((lastFanSpeed == fanSpeedNow) && (lastTempSetC == tempSetC)))) {
      return;
    }

    powerChanged = (lastPower != powerNow);
    lastPower = powerNow;
    lastFanSpeed = fanSpeedNow;
    lastTempSetC = tempSetC;
  }

  if (powerNow) {
    Outbuf[RCU_OUTBUF_AIR] |= AirPow;
    if (powerChanged || force) {
      lastFanSpeed = fanSpeedNow;
      lastTempSetC = tempSetC;
      ThermostatPanel_RequestPowerFanSpeedTemp(true, fanSpeedNow, tempSetC);
    } else {
      ThermostatPanel_RequestFanSpeedTemp(fanSpeedNow, tempSetC);
    }
  } else {
    ThermostatPanel_ClearAirOutput();
    ThermostatPanel_RequestPowerFanSpeedTemp(false, fanSpeedNow, tempSetC);
  }
}
#endif

#if defined(_SWITCH) || defined(_THERMOSTAT)
#define GATEWAY_PRIVATE_HEADER       0x7Fu
#define GATEWAY_PRIVATE_PROTO        0xE1u
#define GATEWAY_PRIVATE_SOURCE_ETH   0x31u
#define GATEWAY_PRIVATE_SOURCE_RCU   NodeId
#define GATEWAY_CMD_WRITE_OUTBUF     0x11u
#define GATEWAY_CMD_WRITE_OUTBUF_BIT 0x12u
#define GATEWAY_CMD_READ_ALL_STATUS  0x14u
#define GATEWAY_RESP_WRITE_OUTBUF    0x91u
#define GATEWAY_RESP_WRITE_OUTBUF_BIT 0x92u
#define GATEWAY_RESP_STATUS          0x94u
#define GATEWAY_STATUS_OK            0x00u
#define GATEWAY_STATUS_BAD_CMD       0x01u
#define GATEWAY_STATUS_BAD_ADDR      0x02u

static void GatewayPrivate_SendFrame(const uint8_t frame[8]) {
  uint8_t i;

  DIR_SetHigh();
  for (i = 0u; i < 8u; i++) {
    EUSART_Write(frame[i]);
  }
  while (!EUSART_is_tx_done());
  DIR_SetLow();
}

static void GatewayPrivate_MarkOutputsChanged(uint8_t index) {
  Outbuf[RCU_OUTBUF_NODE_ID] = NodeId;
  if (index == RCU_OUTBUF_CONTROL) {
    if ((Outbuf[RCU_OUTBUF_CONTROL] & Dnd) != 0u) {
      Outbuf[RCU_OUTBUF_CONTROL] &= (uint8_t)~Mur;
    } else if ((Outbuf[RCU_OUTBUF_CONTROL] & Mur) != 0u) {
      Outbuf[RCU_OUTBUF_CONTROL] &= (uint8_t)~Dnd;
    }
  }
  MasterOutput_SyncControlBit();
  Txdflag = true;
#ifdef _SWITCH
  SwitchLed_SyncFromOutputs();
#endif
#ifdef _THERMOSTAT
  if (index == RCU_OUTBUF_AIR || index == RCU_OUTBUF_CONTROL) {
    ThermostatPanel_SyncCommandFromAirOutput(true);
  }
#endif
}

static void GatewayPrivate_SendWriteResponse(uint8_t cmd, uint8_t index, uint8_t value, uint8_t seq, uint8_t status) {
  uint8_t response[8];
  response[0] = GATEWAY_PRIVATE_HEADER;
  response[1] = GATEWAY_PRIVATE_PROTO;
  response[2] = GATEWAY_PRIVATE_SOURCE_RCU;
  response[3] = cmd;
  response[4] = index;
  response[5] = value;
  response[6] = seq;
  response[7] = status;
  GatewayPrivate_SendFrame(response);
}

static void GatewayPrivate_SendStatusResponse(uint8_t offset, uint8_t count) {
  uint8_t data0 = 0u;
  uint8_t data1 = 0u;
  uint8_t i;
  uint8_t response[8];

  if (count > 2u) count = 2u;
  for (i = 0u; i < count; i++) {
    uint8_t pos = (uint8_t)(offset + i);
    if (pos < sizeof(Inpbuf)) {
      if (i == 0u) data0 = Inpbuf[pos];
      else data1 = Inpbuf[pos];
    } else if ((uint8_t)(pos - sizeof(Inpbuf)) < sizeof(Outbuf)) {
      if (i == 0u) data0 = Outbuf[(uint8_t)(pos - sizeof(Inpbuf))];
      else data1 = Outbuf[(uint8_t)(pos - sizeof(Inpbuf))];
    }
  }

  response[0] = GATEWAY_PRIVATE_HEADER;
  response[1] = GATEWAY_PRIVATE_PROTO;
  response[2] = GATEWAY_PRIVATE_SOURCE_RCU;
  response[3] = GATEWAY_RESP_STATUS;
  response[4] = offset;
  response[5] = count;
  response[6] = data0;
  response[7] = data1;
  GatewayPrivate_SendFrame(response);
}

static void GatewayPrivate_ProcessFrame(const uint8_t frame[8]) {
  uint8_t index = frame[4];
  uint8_t value = frame[5];
  uint8_t seq = frame[6];
  uint8_t status = GATEWAY_STATUS_OK;

  if (frame[2] != GATEWAY_PRIVATE_SOURCE_ETH) {
    return;
  }

  if (frame[3] == GATEWAY_CMD_WRITE_OUTBUF) {
    if (index >= sizeof(Outbuf) || index == RCU_OUTBUF_NODE_ID) {
      status = GATEWAY_STATUS_BAD_ADDR;
    } else {
      Outbuf[index] = value;
      GatewayPrivate_MarkOutputsChanged(index);
      value = Outbuf[index];
    }
    GatewayPrivate_SendWriteResponse(GATEWAY_RESP_WRITE_OUTBUF, index, value, seq, status);
    return;
  }

  if (frame[3] == GATEWAY_CMD_WRITE_OUTBUF_BIT) {
    uint8_t mask = frame[5];
    bool enabled = (frame[6] != 0u);
    seq = frame[7];
    if (index >= sizeof(Outbuf) || index == RCU_OUTBUF_NODE_ID) {
      status = GATEWAY_STATUS_BAD_ADDR;
      value = 0u;
    } else {
      if (enabled) Outbuf[index] |= mask;
      else Outbuf[index] &= (uint8_t)~mask;
      GatewayPrivate_MarkOutputsChanged(index);
      value = Outbuf[index];
    }
    GatewayPrivate_SendWriteResponse(GATEWAY_RESP_WRITE_OUTBUF_BIT, index, value, seq, status);
    return;
  }

  if (frame[3] == GATEWAY_CMD_READ_ALL_STATUS) {
    GatewayPrivate_SendStatusResponse(frame[4], frame[5]);
    return;
  }

  GatewayPrivate_SendWriteResponse((uint8_t)(frame[3] | 0x80u), index, value, seq, GATEWAY_STATUS_BAD_CMD);
}

#endif

#if defined(_SWITCH) || defined(_THERMOSTAT)
typedef enum {
  BUS_RX_DEVICE_NONE = 0u,
  BUS_RX_DEVICE_GATEWAY_PRIVATE,
  BUS_RX_DEVICE_SWITCH,
  BUS_RX_DEVICE_THERMOSTAT
} BUS_RX_DEVICE;

#define BUS_RX_FRAME_LENGTH 8u

static uint8_t BusRxFrame[BUS_RX_FRAME_LENGTH];
static uint8_t BusRxCount;

static void BusRx_ShiftLeft(void) {
  uint8_t i;

  for (i = 1u; i < BUS_RX_FRAME_LENGTH; i++) {
    BusRxFrame[i - 1u] = BusRxFrame[i];
  }
  BusRxCount = (uint8_t)(BUS_RX_FRAME_LENGTH - 1u);
}

static BUS_RX_DEVICE BusRx_ClassifyFrame(void) {
  if ((BusRxFrame[0] == GATEWAY_PRIVATE_HEADER)
   && (BusRxFrame[1] == GATEWAY_PRIVATE_PROTO)) {
    return BUS_RX_DEVICE_GATEWAY_PRIVATE;
  }

#ifdef _THERMOSTAT
  if ((BusRxFrame[0] == 0x7Fu)
   && (BusRxFrame[1] == 0x05u)
   && (BusRxFrame[2] == THERMOSTAT_PANEL_DEFAULT_ADDRESS)) {
    return BUS_RX_DEVICE_THERMOSTAT;
  }
#endif

#ifdef _SWITCH
  if ((BusRxFrame[0] == 0x7Fu) && (BusRxFrame[1] == 0x01u)) {
    return BUS_RX_DEVICE_SWITCH;
  }
#endif

  return BUS_RX_DEVICE_NONE;
}

static void BusRx_DispatchFrame(BUS_RX_DEVICE device) {
  uint8_t i;

  if (device == BUS_RX_DEVICE_GATEWAY_PRIVATE) {
    GatewayPrivate_ProcessFrame(BusRxFrame);
  }
#ifdef _THERMOSTAT
  else if (device == BUS_RX_DEVICE_THERMOSTAT) {
    for (i = 0u; i < BUS_RX_FRAME_LENGTH; i++) {
      ThermostatPanel_ReceiveByte(BusRxFrame[i]);
    }
  }
#endif
#ifdef _SWITCH
  else if (device == BUS_RX_DEVICE_SWITCH) {
    for (i = 0u; i < BUS_RX_FRAME_LENGTH; i++) {
      SwitchInput_ReceiveByte(BusRxFrame[i]);
    }
  }
#endif
}

static void BusRx_ReceiveByte(uint8_t data) {
  BUS_RX_DEVICE device;

  if (BusRxCount >= BUS_RX_FRAME_LENGTH) {
    BusRx_ShiftLeft();
  }

  BusRxFrame[BusRxCount++] = data;
  if (BusRxCount < BUS_RX_FRAME_LENGTH) {
    return;
  }

  device = BusRx_ClassifyFrame();
  if (device != BUS_RX_DEVICE_NONE) {
    BusRx_DispatchFrame(device);
    BusRxCount = 0u;
    return;
  }

  if (BusRxFrame[0] == 0x7Fu) {
    BusRxCount = 0u;
  } else {
    BusRx_ShiftLeft();
  }
}

#if defined(_SWITCH) && defined(_THERMOSTAT)
static void SwitchThermostat_ProcessBus(void) {
  bool switchPending;

  switchPending = SwitchLed_HasPendingWork();
  if (switchPending) {
    ThermostatPanel_MarkBusActivity();
  }

  if (SwitchLed_IsBusy()
   || (switchPending && !ThermostatPanel_IsTxLocked())
   || !ThermostatPanel_IsBusReserved()) {
    SwitchLed_Process();
  }

  switchPending = SwitchLed_HasPendingWork();
  if (switchPending) {
    return;
  }

  ThermostatPanel_Process();
}
#endif
#endif

/* Outbuf[RCU_OUTBUF_ALARM_HOUR] HOURS */
#define AlmHours  0x3f
#define Alarms    Bit6
#define Alarmp    Bit7

/* Outbuf[RCU_OUTBUF_ALARM_MINUTE] MINUTE */
#define AlmMin    0xff

/* Swibuf[0] */
#define SW1       Bit7
#define SW2       Bit6
#define SW3       Bit5
#define SW4       Bit3
#define SW5       Bit2
#define SW6       Bit1
#define SW7       Bit0
#define SW8       Bit4

/* Swibuf[1] */
#define SW9       Bit7
#define SW10      Bit6
#define SW11      Bit5
#define SW12      Bit3
#define SW13      Bit4
#define SW14      Bit0
#define SW15      Bit1
#define SW16      Bit2

/* Swibuf[2] */
#define SW17      Bit7
#define SW18      Bit6
#define SW19      Bit5
#define SW20      Bit3
#define SW21      Bit2
#define SW22      Bit1
#define SW23      Bit0
#define SW24      Bit4
static const uint8_t SwitchInputMasks[MAXIN] = {
  SW1, SW2, SW3, SW4, SW5, SW6, SW7, SW8,
  SW9, SW10, SW11, SW12, SW13, SW14, SW15, SW16,
  SW17, SW18, SW19, SW20, SW21, SW22, SW23, SW24
};

/* Relbuf[0] */
#define RY3       Bit2
#define RY4       Bit3
#define RY8       Bit4
#define RY7       Bit5
#define RY6       Bit6
#define RY5       Bit7
#define RY2       Bit1
#define RY1       Bit0

/* Relbuf[1] */
#define RY9       Bit0
#define RY10      Bit1
#define RY11      Bit2
#define RY12      Bit3
#define DNDLED    Bit4
#define MURLED    Bit5
#define POWLED    Bit6
#define SPEAKER   Bit6
#define KEYLED    Bit7

/* Relbuf[2] */
#define RY13      Bit5
#define RY14      Bit4
#define RY15      Bit6
#define RY16      Bit0
#define RY17      Bit7
#define RY18      Bit1
#define RY19      Bit3
#define RY20      Bit2

/* Relbuf[3] */
#define RY21      Bit5
#define RY22      Bit4
#define RY23      Bit6
#define RY24      Bit0
#define RY25      Bit7
#define RY26      Bit1
#define RY27      Bit3
#define RY28      Bit2

static const uint8_t Relay0LightMasks[16] = {
  0u,
  RY5,
  RY6,
  RY5 | RY6,
  RY7,
  RY5 | RY7,
  RY6 | RY7,
  RY5 | RY6 | RY7,
  RY8,
  RY5 | RY8,
  RY6 | RY8,
  RY5 | RY6 | RY8,
  RY7 | RY8,
  RY5 | RY7 | RY8,
  RY6 | RY7 | RY8,
  RY5 | RY6 | RY7 | RY8
};

#ifdef _SETTIME

/********** Function UpBCD **********/
uint8_t UpBCD(uint8_t data) {
  data++;
  if ((data & 0x0f) > 0x09)
    data += 0x06;
  return (data);
}
#endif

#if defined(_SERIAL)

/********** Function atohr **********/
uint8_t atohr(uint8_t a) { // change ascii to hex (R)
  if ((a >= 'a' && a <= 'f') || (a >= 'A' && a <= 'F'))
    return ((a + 9) & 0xf);
  else return (a & 0xf);
}

/********** Function atohl **********/
uint8_t atohl(uint8_t a) { // change ascii to hex (L)
  a = atohr(a);
  a = a << 4;
  return (a);
}

/********** Function htoar **********/
uint8_t htoar(uint8_t h) { // change hex to ascii (R)
  h = h & 0xf;
  if (h > 9) return ((h - 9) | 0x40);
  else return (h | 0x30);
}

/********** Function htoal **********/
uint8_t htoal(uint8_t h) { // Change hex to ascii (L)
  return (htoar(h >> 4));
}

/********** Function Read Serial **********/
void ReadSerial() {
  // Logic to echo received data
  uint8_t rxData;
  if (EUSART_is_rx_ready()) {
    rxData = EUSART_Read();
#ifdef USE_ADEL     
    if (rxData == 0xA9) {
      Rcvptr = &Rcvbuf[0];
      Rcflag = true;
    }
    if (Rcflag) {
      *Rcvptr = rxData;
      if (Rcvptr < &Rcvbuf[35]) {
        Rcvptr++;
      }
      if (rxData == 0xCD) {
        Rxdflag = true;
        Rcflag = false;
      }
    }
#else
    if (rxData == ':') {
      Rcvptr = &Rcvbuf[0];
      Rcflag = true;
    } else if (Rcflag) {
      *Rcvptr = rxData;
      if (Rcvptr < &Rcvbuf[35]) {
        Rcvptr++;
      }
      if (rxData == '\r') {
        *Rcvptr = '\0';
        Rxdflag = true;
        Rcflag = false;
      }
    }
#endif
    Rcvflag = true;
    Rcvdly = 0;
  }
}
#endif

/********** Readinput Service **********/
void ReadInput(void) {
  uint8_t cntl;

  for (cntl = 0; cntl < MAXIN; cntl++) {
    inpBuff[cntl].inp = ((Swibuf[(uint8_t)(cntl >> 3)] & SwitchInputMasks[cntl]) != 0u) ? 1u : 0u;
  }

  Inpbuf[0] = 0u;
  Inpbuf[1] = 0u;
  Inpbuf[2] = 0u;
  for (cntl = 0; cntl < MAXIN; cntl++) {
    if (inpBuff[cntl].out) {
      Inpbuf[(uint8_t)(cntl >> 3)] |= (uint8_t)(1u << (uint8_t)(cntl & 0x07u));
    }
  }

  for (cntl = 0; cntl < MAXIN; cntl++) {
    if (inpBuff[cntl].inp) {
      inpBuff[cntl].low = false;
      if (!inpBuff[cntl].high) {
        inpBuff[cntl].high = true;
        inpBuff[cntl].cnt = 0;
      }
      if (++inpBuff[cntl].cnt >= INDelay) {
        inpBuff[cntl].high = false;
        inpBuff[cntl].out = true;
      }
    } else {
      inpBuff[cntl].high = false;
      if (!inpBuff[cntl].low) {
        inpBuff[cntl].low = true;
        inpBuff[cntl].cnt = 0;
      }
      if (++inpBuff[cntl].cnt >= INDelay) {
        inpBuff[cntl].low = false;
        inpBuff[cntl].out = false;
      }
    }
  }

}

/********** WriteOutput Service **********/
#define WriteRelayOutput(relIndex, relMask, isOn) \
  do { \
    if (isOn) Relbuf[(relIndex)] |= (relMask); \
    else Relbuf[(relIndex)] &= (uint8_t) ~(relMask); \
  } while (0)

void WriteOutput(void) {
#ifdef MASTER_BOARD
  uint8_t relay0 = Relay0LightMasks[(uint8_t)((Outbuf[RCU_OUTBUF_LAMP1] >> 4) & 0x0fu)];
  uint8_t airControl = Outbuf[RCU_OUTBUF_AIR];
  uint8_t fanControl = (uint8_t)(airControl & FanSpeed);
  uint8_t tempSetC = (uint8_t)(airControl & (uint8_t)Temp);
  uint8_t roomTempC = Outbuf[RCU_OUTBUF_TEMP_ACTUAL];

  if (AirEnable) {
    tempSetC = (uint8_t)(tempSetC + 16u);
    if (fanControl == FanA) {
      fanControl = FanM;
      if ((roomTempC >= 0x1Bu) && (roomTempC <= 0x20u)) {
        if (roomTempC >= (uint8_t)(tempSetC + 3u)) {
          fanControl = FanH;
        } else if (roomTempC >= (uint8_t)(tempSetC + 2u)) {
          fanControl = FanM;
        } else {
          fanControl = FanL;
        }
      }
    }

    if (fanControl == FanM) {
      relay0 |= RY1;
    } else if (fanControl == FanH) {
      relay0 |= RY2;
    }
    if (airControl & AirCdu) relay0 |= RY3;
    if (airControl & AirPow) relay0 |= RY4;
  }

  Relbuf[0] = relay0;
  Relbuf[1] = (uint8_t)((Relbuf[1] & 0xf0u) | (Outbuf[RCU_OUTBUF_LAMP2] & 0x0fu));
#else //Slave Board
#ifdef _OUTPUT29    
  WriteRelayOutput(0u, RY1, _OUTPUT29);
#endif
#ifdef _OUTPUT30   
  WriteRelayOutput(0u, RY2, _OUTPUT30);
#endif
#ifdef _OUTPUT31   
  WriteRelayOutput(0u, RY3, _OUTPUT31);
#endif
#ifdef _OUTPUT32   
  WriteRelayOutput(0u, RY4, _OUTPUT32);
#endif  
#ifdef _OUTPUT33
  WriteRelayOutput(0u, RY5, _OUTPUT33);
#endif
#ifdef _OUTPUT34
  WriteRelayOutput(0u, RY6, _OUTPUT34);
#endif
#ifdef _OUTPUT35
  WriteRelayOutput(0u, RY7, _OUTPUT35);
#endif
#ifdef _OUTPUT36
  WriteRelayOutput(0u, RY8, _OUTPUT36);
#endif
#ifdef _OUTPUT37
  WriteRelayOutput(1u, RY9, _OUTPUT37);
#endif
#ifdef _OUTPUT38
  WriteRelayOutput(1u, RY10, _OUTPUT38);
#endif
#ifdef _OUTPUT39
  WriteRelayOutput(1u, RY11, _OUTPUT39);
#endif
#ifdef _OUTPUT40
  WriteRelayOutput(1u, RY12, _OUTPUT40);
#endif
#ifdef _OUTPUT41    
  WriteRelayOutput(2u, RY13, _OUTPUT41);
#endif
#ifdef _OUTPUT42   
  WriteRelayOutput(2u, RY14, _OUTPUT42);
#endif
#ifdef _OUTPUT43   
  WriteRelayOutput(2u, RY15, _OUTPUT43);
#endif
#ifdef _OUTPUT44   
  WriteRelayOutput(2u, RY16, _OUTPUT44);
#endif  
#ifdef _OUTPUT45
  WriteRelayOutput(2u, RY17, _OUTPUT45);
#endif
#ifdef _OUTPUT46
  WriteRelayOutput(2u, RY18, _OUTPUT46);
#endif
#ifdef _OUTPUT47
  WriteRelayOutput(2u, RY19, _OUTPUT47);
#endif
#ifdef _OUTPUT48
  WriteRelayOutput(2u, RY20, _OUTPUT48);
#endif
#ifdef _OUTPUT49
  WriteRelayOutput(3u, RY21, _OUTPUT49);
#endif
#ifdef _OUTPUT50
  WriteRelayOutput(3u, RY22, _OUTPUT50);
#endif
#ifdef _OUTPUT51
  WriteRelayOutput(3u, RY23, _OUTPUT51);
#endif
#ifdef _OUTPUT52
  WriteRelayOutput(3u, RY24, _OUTPUT52);
#endif 
#ifdef _OUTPUT53
  WriteRelayOutput(3u, RY25, _OUTPUT53);
#endif
#ifdef _OUTPUT54
  WriteRelayOutput(3u, RY26, _OUTPUT54);
#endif
#ifdef _OUTPUT55
  WriteRelayOutput(3u, RY27, _OUTPUT55);
#endif
#ifdef _OUTPUT56
  WriteRelayOutput(3u, RY28, _OUTPUT56);
#endif
#endif

#ifdef USE_SERVICE
#ifdef USE_DNDLED
  if (Outbuf[RCU_OUTBUF_CONTROL] & Dnd) {
    Relbuf[1] |= DNDLED; //Dnd Led
    Relbuf[1] &= ~MURLED;
  } else Relbuf[1] &= ~DNDLED;
#endif
#ifdef USE_MURLED  
  if (Outbuf[RCU_OUTBUF_CONTROL] & Mur) {
    Relbuf[1] |= MURLED; //Mur Led
    Relbuf[1] &= ~DNDLED;
  } else Relbuf[1] &= ~MURLED;
#endif
#else
#ifndef USE_DNDLED
  if (Outbuf[RCU_OUTBUF_LAMP1] & Out1) Relbuf[1] |= DNDLED; //OUT Relay
  else Relbuf[1] &= ~DNDLED;
#endif
#ifndef USE_MURLED
  if (Outbuf[RCU_OUTBUF_LAMP1] & Out2) Relbuf[1] |= MURLED; //OUT Relay
  else Relbuf[1] &= ~MURLED;
#endif
#ifndef USE_BELSPK
  if (Outbuf[RCU_OUTBUF_LAMP1] & Out3) SPK_SetHigh(); //Out signal high
  else SPK_SetLow(); //Out signal low
#endif
#ifndef USE_KEYLED
  if (Outbuf[RCU_OUTBUF_LAMP1] & Out4) Relbuf[1] |= KEYLED; //OUT Relay
  else Relbuf[1] &= ~KEYLED;
#endif
#endif
#ifdef USE_KEYLED
  if (Outbuf[RCU_OUTBUF_CONTROL] & Guest) Relbuf[1] |= KEYLED; //Power Led
  else Relbuf[1] &= ~KEYLED;
#endif
}

#ifdef USE_SERVICE

/********** Function sound **********/
void Sound(uint8_t freq, int time) { //Sound Generate
  uint8_t i;
  INTERRUPT_GlobalInterruptDisable();
  while (time > 0x00) {
    CLRWDT();
#ifdef USE_BELSPK
    SPK_SetHigh(); //Out signal high
#endif
    for (i = 1; i <= freq; i++) time--;
#ifdef USE_BELSPK
    SPK_SetLow(); //Out signal low
#endif
    for (i = 1; i <= freq; i++) time--;
  }
  INTERRUPT_GlobalInterruptEnable();
}

/********** Function beep **********/
void Beep(void) { //Beep reset
  Sound(40, 30000);
  Sound(60, 30000);
}
#endif

/********** Function Read Keypad **********/
void ReadKey(void) {
  if (!Keyflag) {
#ifdef MASTER_BOARD
    // Dnd
    if ((Inpbuf[0] & Dndin) ^ (Inpmem[0] & Dndin)) {
      Inpmem[0] ^= Dndin;
      if (Inpbuf[0] & Dndin) {
        Keybuf = Dndsw;
        Keyflag = true;
      }
    }
    // Mur
    if ((Inpbuf[0] & Murin) ^ (Inpmem[0] & Murin)) {
      Inpmem[0] ^= Murin;
      if (Inpbuf[0] & Murin) {
        Keybuf = Mursw;
        Keyflag = true;
      }
    }
    // Bell
    if ((Inpbuf[0] & Bellin) ^ (Inpmem[0] & Bellin)) {
      Inpmem[0] ^= Bellin;
      if (Inpbuf[0] & Bellin) {
        Keybuf = Bellsw;
        Keyflag = true;
      }
    }

#ifdef _2WAY1
    if ((Inpbuf[0] & Inp1in) ^ (Inpmem[0] & Inp1in)) {
      Inpmem[0] ^= Inp1in;
#ifdef PUSHBUTTON
      if (Inpbuf[0] & Inp1in)
#endif
        Keyflag = true;
      Keybuf = _2WAY1;
    }
#endif 

#ifdef _2WAY2
    if ((Inpbuf[0] & Inp2in) ^ (Inpmem[0] & Inp2in)) {
      Inpmem[0] ^= Inp2in;
#ifdef PUSHBUTTON
      if (Inpbuf[0] & Inp2in)
#endif
        Keyflag = true;
      Keybuf = _2WAY2;
    }
#endif

#ifdef _2WAY3
    if ((Inpbuf[0] & Inp3in) ^ (Inpmem[0] & Inp3in)) {
      Inpmem[0] ^= Inp3in;
#ifdef PUSHBUTTON
      if (Inpbuf[0] & Inp3in)
#endif
        Keyflag = true;
      Keybuf = _2WAY3;
    }
#endif       

#ifdef _2WAY4
    if ((Inpbuf[0] & Inp4in) ^ (Inpmem[0] & Inp4in)) {
      Inpmem[0] ^= Inp4in;
#ifdef PUSHBUTTON
      if (Inpbuf[0] & Inp4in)
#endif
        Keyflag = true;
      Keybuf = _2WAY4;
    }
#endif

#ifdef _2WAY5
    if ((Inpbuf[0] & Inp5in) ^ (Inpmem[0] & Inp5in)) {
      Inpmem[0] ^= Inp5in;
#ifdef PUSHBUTTON
      if (Inpbuf[0] & Inp5in)
#endif
        Keyflag = true;
      Keybuf = _2WAY5;
    }
#endif

#ifdef _2WAY6
    if ((Inpbuf[0] & Inp6in) ^ (Inpmem[0] & Inp6in)) {
      Inpmem[0] ^= Inp6in;
#ifdef PUSHBUTTON
      if (Inpbuf[0] & Inp6in)
#endif
        Keyflag = true;
      Keybuf = _2WAY6;
    }
#endif

#ifdef _2WAY7
    if ((Inpbuf[0] & Inp7in) ^ (Inpmem[0] & Inp7in)) {
      Inpmem[0] ^= Inp7in;
#ifdef PUSHBUTTON
      if (Inpbuf[0] & Inp7in)
#endif
        Keyflag = true;
      Keybuf = _2WAY7;
    }
#endif

#ifdef _2WAY8
    if ((Inpbuf[0] & Inp8in) ^ (Inpmem[0] & Inp8in)) {
      Inpmem[0] ^= Inp8in;
#ifdef PUSHBUTTON
      if (Inpbuf[0] & Inp8in)
#endif
        Keyflag = true;
      Keybuf = _2WAY8;
    }
#endif

#ifdef _2WAY9
    if ((Inpbuf[1] & Inp9in) ^ (Inpmem[1] & Inp9in)) {
      Inpmem[1] ^= Inp9in;
#ifdef PUSHBUTTON
      if (Inpbuf[1] & Inp9in)
#endif
        Keyflag = true;
      Keybuf = _2WAY9;
    }
#endif

#ifdef _2WAY10
    if ((Inpbuf[1] & Inp10in) ^ (Inpmem[1] & Inp10in)) {
      Inpmem[1] ^= Inp10in;
#ifdef PUSHBUTTON
      if (Inpbuf[1] & Inp10in)
#endif
        Keyflag = true;
      Keybuf = _2WAY10;
    }
#endif

#ifdef _2WAY11
    if ((Inpbuf[1] & Inp11in) ^ (Inpmem[1] & Inp11in)) {
      Inpmem[1] ^= Inp11in;
#ifdef PUSHBUTTON
      if (Inpbuf[1] & Inp11in)
#endif
        Keyflag = true;
      Keybuf = _2WAY11;
    }
#endif

#ifdef _2WAY12
    if ((Inpbuf[1] & Inp12in) ^ (Inpmem[1] & Inp12in)) {
      Inpmem[1] ^= Inp12in;
#ifdef PUSHBUTTON
      if (Inpbuf[1] & Inp12in)
#endif
        Keyflag = true;
      Keybuf = _2WAY12;
    }
#endif

#ifdef _2WAY13
    if ((Inpbuf[1] & Inp13in) ^ (Inpmem[1] & Inp13in)) {
      Inpmem[1] ^= Inp13in;
#ifdef PUSHBUTTON
      if (Inpbuf[1] & Inp13in)
#endif
        Keyflag = true;
      Keybuf = _2WAY13;
    }
#endif

#ifdef _2WAY14
    if ((Inpbuf[1] & Inp14in) ^ (Inpmem[1] & Inp14in)) {
      Inpmem[1] ^= Inp14in;
#ifdef PUSHBUTTON
      if (Inpbuf[1] & Inp14in)
#endif
        Keyflag = true;
      Keybuf = _2WAY14;
    }
#endif 

#ifdef _2WAY15
    if ((Inpbuf[1] & Inp15in) ^ (Inpmem[1] & Inp15in)) {
      Inpmem[1] ^= Inp15in;
#ifdef PUSHBUTTON
      if (Inpbuf[1] & Inp15in)
#endif
        Keyflag = true;
      Keybuf = _2WAY15;
    }
#endif

#ifdef _2WAY16
    if ((Inpbuf[1] & Inp16in) ^ (Inpmem[1] & Inp16in)) {
      Inpmem[1] ^= Inp16in;
#ifdef PUSHBUTTON
      if (Inpbuf[1] & Inp16in)
#endif
        Keyflag = true;
      Keybuf = _2WAY16;
    }
#endif 

#ifdef _2WAY17
    if ((Inpbuf[2] & Inp17in) ^ (Inpmem[2] & Inp17in)) {
      Inpmem[2] ^= Inp17in;
#ifdef PUSHBUTTON
      if (Inpbuf[2] & Inp17in)
#endif
        Keyflag = true;
      Keybuf = _2WAY17;
    }
#endif 

#ifdef _2WAY18
    if ((Inpbuf[2] & Inp18in) ^ (Inpmem[2] & Inp18in)) {
      Inpmem[2] ^= Inp18in;
#ifdef PUSHBUTTON
      if (Inpbuf[2] & Inp18in)
#endif
        Keyflag = true;
      Keybuf = _2WAY18;
    }
#endif 

#ifdef _2WAY19
    if ((Inpbuf[2] & Inp19in) ^ (Inpmem[2] & Inp19in)) {
      Inpmem[2] ^= Inp19in;
#ifdef PUSHBUTTON
      if (Inpbuf[2] & Inp19in)
#endif
        Keyflag = true;
      Keybuf = _2WAY19;
    }
#endif 

#ifdef _2WAY20
    if ((Inpbuf[2] & Inp20in) ^ (Inpmem[2] & Inp20in)) {
      Inpmem[2] ^= Inp20in;
#ifdef PUSHBUTTON
      if (Inpbuf[2] & Inp20in)
#endif
        Keyflag = true;
      Keybuf = _2WAY20;
    }
#endif

#ifdef _2WAY21
    if ((Inpbuf[3] & Inp25in) ^ (Inpmem[3] & Inp25in)) {
      Inpmem[3] ^= Inp25in;
#ifdef PUSHBUTTON
      if (Inpbuf[3] & Inp25in)
#endif
        Keyflag = true;
      Keybuf = _2WAY21;
    }
#endif

#ifdef _2WAY22
    if ((Inpbuf[3] & Inp26in) ^ (Inpmem[3] & Inp26in)) {
      Inpmem[3] ^= Inp26in;
#ifdef PUSHBUTTON
      if (Inpbuf[3] & Inp26in)
#endif
        Keyflag = true;
      Keybuf = _2WAY22;
    }
#endif

#ifdef _2WAY23
    if ((Inpbuf[3] & Inp27in) ^ (Inpmem[3] & Inp27in)) {
      Inpmem[3] ^= Inp27in;
#ifdef PUSHBUTTON
      if (Inpbuf[3] & Inp27in)
#endif
        Keyflag = true;
      Keybuf = _2WAY23;
    }
#endif

#ifdef _2WAY24
    if ((Inpbuf[3] & Inp28in) ^ (Inpmem[3] & Inp28in)) {
      Inpmem[3] ^= Inp28in;
#ifdef PUSHBUTTON
      if (Inpbuf[3] & Inp28in)
#endif
        Keyflag = true;
      Keybuf = _2WAY24;
    }
#endif

#ifdef _2WAY25
    if ((Inpbuf[3] & Inp29in) ^ (Inpmem[3] & Inp29in)) {
      Inpmem[3] ^= Inp29in;
#ifdef PUSHBUTTON
      if (Inpbuf[3] & Inp29in)
#endif
        Keyflag = true;
      Keybuf = _2WAY25;
    }
#endif

#ifdef _2WAY26
    if ((Inpbuf[3] & Inp30in) ^ (Inpmem[3] & Inp30in)) {
      Inpmem[3] ^= Inp30in;
#ifdef PUSHBUTTON
      if (Inpbuf[3] & Inp30in)
#endif
        Keyflag = true;
      Keybuf = _2WAY26;
    }
#endif 

#ifdef _2WAY27
    if ((Inpbuf[3] & Inp31in) ^ (Inpmem[3] & Inp31in)) {
      Inpmem[3] ^= Inp31in;
#ifdef PUSHBUTTON
      if (Inpbuf[3] & Inp31in)
#endif
        Keyflag = true;
      Keybuf = _2WAY27;
    }
#endif

#ifdef _2WAY28
    if ((Inpbuf[3] & Inp32in) ^ (Inpmem[3] & Inp32in)) {
      Inpmem[3] ^= Inp32in;
#ifdef PUSHBUTTON
      if (Inpbuf[3] & Inp32in)
#endif
        Keyflag = true;
      Keybuf = _2WAY28;
    }
#endif 

#ifdef _2WAY29
    if ((Inpbuf[4] & Inp33in) ^ (Inpmem[4] & Inp33in)) {
      Inpmem[4] ^= Inp33in;
#ifdef PUSHBUTTON
      if (Inpbuf[4] & Inp33in)
#endif
        Keyflag = true;
      Keybuf = _2WAY29;
    }
#endif 

#ifdef _2WAY30
    if ((Inpbuf[4] & Inp34in) ^ (Inpmem[4] & Inp34in)) {
      Inpmem[4] ^= Inp34in;
#ifdef PUSHBUTTON
      if (Inpbuf[4] & Inp34in)
#endif
        Keyflag = true;
      Keybuf = _2WAY30;
    }
#endif 

#ifdef _2WAY31
    if ((Inpbuf[4] & Inp35in) ^ (Inpmem[4] & Inp35in)) {
      Inpmem[4] ^= Inp35in;
#ifdef PUSHBUTTON
      if (Inpbuf[4] & Inp35in)
#endif
        Keyflag = true;
      Keybuf = _2WAY31;
    }
#endif 

#ifdef _2WAY32
    if ((Inpbuf[4] & Inp36in) ^ (Inpmem[4] & Inp36in)) {
      Inpmem[4] ^= Inp36in;
#ifdef PUSHBUTTON
      if (Inpbuf[4] & Inp36in)
#endif
        Keyflag = true;
      Keybuf = _2WAY32;
    }
#endif

#ifdef _2WAY33
    if ((Inpbuf[4] & Inp37in) ^ (Inpmem[4] & Inp37in)) {
      Inpmem[4] ^= Inp37in;
#ifdef PUSHBUTTON
      if (Inpbuf[4] & Inp37in)
#endif
        Keyflag = true;
      Keybuf = _2WAY33;
    }
#endif

#ifdef _2WAY34
    if ((Inpbuf[4] & Inp38in) ^ (Inpmem[4] & Inp38in)) {
      Inpmem[4] ^= Inp38in;
#ifdef PUSHBUTTON
      if (Inpbuf[4] & Inp38in)
#endif
        Keyflag = true;
      Keybuf = _2WAY34;
    }
#endif

#ifdef _2WAY35
    if ((Inpbuf[4] & Inp39in) ^ (Inpmem[4] & Inp39in)) {
      Inpmem[4] ^= Inp39in;
#ifdef PUSHBUTTON
      if (Inpbuf[4] & Inp39in)
#endif
        Keyflag = true;
      Keybuf = _2WAY35;
    }
#endif

#ifdef _2WAY36
    if ((Inpbuf[4] & Inp40in) ^ (Inpmem[4] & Inp40in)) {
      Inpmem[4] ^= Inp40in;
#ifdef PUSHBUTTON
      if (Inpbuf[4] & Inp40in)
#endif
        Keyflag = true;
      Keybuf = _2WAY36;
    }
#endif

#ifdef _2WAY37
    if ((Inpbuf[5] & Inp41in) ^ (Inpmem[5] & Inp41in)) {
      Inpmem[5] ^= Inp41in;
#ifdef PUSHBUTTON
      if (Inpbuf[5] & Inp41in)
#endif
        Keyflag = true;
      Keybuf = _2WAY37;
    }
#endif

#ifdef _2WAY38
    if ((Inpbuf[5] & Inp42in) ^ (Inpmem[5] & Inp42in)) {
      Inpmem[5] ^= Inp42in;
#ifdef PUSHBUTTON
      if (Inpbuf[5] & Inp42in)
#endif
        Keyflag = true;
      Keybuf = _2WAY38;
    }
#endif

#ifdef _2WAY39
    if ((Inpbuf[5] & Inp43in) ^ (Inpmem[5] & Inp43in)) {
      Inpmem[5] ^= Inp43in;
#ifdef PUSHBUTTON
      if (Inpbuf[5] & Inp43in)
#endif
        Keyflag = true;
      Keybuf = _2WAY39;
    }
#endif

#ifdef _2WAY40
    if ((Inpbuf[5] & Inp44in) ^ (Inpmem[5] & Inp44in)) {
      Inpmem[5] ^= Inp44in;
#ifdef PUSHBUTTON
      if (Inpbuf[5] & Inp44in)
#endif
        Keyflag = true;
      Keybuf = _2WAY40;
    }
#endif

#else
    if ((Inpbuf[0] ^ Inpmem[0]) || (Inpbuf[1] ^ Inpmem[1]) || (Inpbuf[2] ^ Inpmem[2])) {
      Inpmem[0] = Inpbuf[0];
      Inpmem[1] = Inpbuf[1];
      Inpmem[2] = Inpbuf[2];
      Txdcom = _INPUT;
      Txdflag = true;
    }
#endif
  }
}

#ifdef USE_RTC

/********** Function wrbyte **********/
void rtcwrbyte(uint8_t dat) {
  uint8_t cnt;
  SDA_SetDigitalOutput(); //SET DATA OUTPUT
  for (cnt = 0; cnt < 8; cnt++) { //WRITE ADDRESS/DATA
    SCL_SetLow(); //Clock = 0
    if (dat & 0x01)
      SDA_SetHigh();
    else
      SDA_SetLow();
    dat >>= 1;
    CLRWDT();
    __delay_us(4);
    SCL_SetHigh(); //Clock = 1
    __delay_us(4);
  }
}

/********** Function rdbyte **********/
uint8_t rtcrdbyte(void) {
  uint8_t cnt, dat = 0;
  SDA_SetDigitalInput(); //SET DATA INPUT
  for (cnt = 0; cnt < 8; cnt++) { //READ DATA
    dat >>= 1;
    SCL_SetLow(); //Clock = 0
    CLRWDT();
    __delay_us(4);
    if (SDA_GetValue())
      dat |= 0x80;
    else
      dat &= 0x7f;
    SCL_SetHigh(); //Clock = 1
    __delay_us(4);
  }
  return dat;
}

/********** Function wrrtc **********/
void WriteRTC(uint8_t addr, uint8_t dat) {
  SCL_SetLow(); //Clock = 0
  RTS_SetHigh(); //RST = 1
  CLRWDT();
  rtcwrbyte(addr); //Address
  rtcwrbyte(dat); //Data
  RTS_SetLow(); //RST = 0
}

/********** Function rdrtc **********/
uint8_t ReadRTC(uint8_t addr) {
  uint8_t dat;
  SCL_SetLow(); //Clock = 0
  RTS_SetHigh(); //RST = 1
  CLRWDT();
  rtcwrbyte(addr + 1); //Address
  dat = rtcrdbyte(); //Data
  RTS_SetLow(); //RST = 0
  return dat;
}

/********** Function init **********/
void InitDS1302(void) {
  uint8_t dat;
  SCL_SetDigitalOutput(); //DS1302 CLOCK
  SDA_SetDigitalOutput(); //DS1302 DATA
  RTS_SetDigitalOutput(); //DS1302 STR
  WriteRTC(CONADD, ENABLE); //WRITE PROTECT = 0
  dat = ReadRTC(SECADD); //SECOND ADDRESS
  WriteRTC(SECADD, dat & 0x7f); //START RTC
  dat = ReadRTC(HSRADD); //HOUR ADDRESS
  WriteRTC(HSRADD, dat & 0x7f); //SET 24 HOUR MODE
  WriteRTC(CHGADD, 0b10100101); //SET TRICKER CHARGER
  //WriteRTC(CONADD, DISBLE);     //WRITE PROTECT = 1
}
#endif

/*
                         Main application
 */
void main(void) {
  // initialize the device
  SYSTEM_Initialize();

  // When using interrupts, you need to set the Global and Peripheral Interrupt Enable bits
  // Use the following macros to:

  // Enable the Global Interrupts
  INTERRUPT_GlobalInterruptEnable();

  // Enable the Peripheral Interrupts
  INTERRUPT_PeripheralInterruptEnable();

  // Disable the Global Interrupts
  //INTERRUPT_GlobalInterruptDisable();

  // Disable the Peripheral Interrupts
  //INTERRUPT_PeripheralInterruptDisable();

  if (!PCON0bits.nBOR) { //Brown-out Reset
    PCON0bits.nBOR = true;
  }
  SSP2CON1bits.SSPEN = true; //On SPI2

  PowUpflag = false;
  PowUpcnt = 0;
#if defined(_SERIAL)
  Rxdaddr = IDADDR;
  Txdcom = _ALL;
#endif
#ifdef _SWITCH
  {
    const SWITCH_LED_CONFIG switchLedConfig = { 2u };

    SwitchInput_Init();
    SwitchLed_Init(&switchLedConfig);
  }
#endif
#ifdef _THERMOSTAT
  ThermostatPanel_Init();
#endif
  Boxbuf = NONE; //RFID Code

#ifdef USE_RTC
  InitDS1302();
  if (ReadRTC(RTCMEM) != 0xA3) {
    WriteRTC(RTCMEM, 0xA3);
    Outbuf[RCU_OUTBUF_LAMP1] = 0x00;
    Outbuf[RCU_OUTBUF_LAMP2] = 0x00;
    Outbuf[RCU_OUTBUF_LAMP3] = 0x00;
    Outbuf[RCU_OUTBUF_LAMP4] = 0x00;
    Outbuf[RCU_OUTBUF_CONTROL] = 0x00;
    Outbuf[RCU_OUTBUF_AIR] = 0x00;
    Outbuf[RCU_OUTBUF_ALARM_HOUR] = 0x00;
    Outbuf[RCU_OUTBUF_ALARM_MINUTE] = 0x00;
    WriteRTC(DATA0, Outbuf[RCU_OUTBUF_LAMP1]);
    WriteRTC(DATA1, Outbuf[RCU_OUTBUF_LAMP2]);
    WriteRTC(DATA2, Outbuf[RCU_OUTBUF_LAMP3]);
    WriteRTC(DATA3, Outbuf[RCU_OUTBUF_LAMP4]);
    WriteRTC(DATA4, Outbuf[RCU_OUTBUF_CONTROL]);
    WriteRTC(DATA5, Outbuf[RCU_OUTBUF_AIR]);
    WriteRTC(DATA6, Outbuf[RCU_OUTBUF_ALARM_HOUR]);
    WriteRTC(DATA7, Outbuf[RCU_OUTBUF_ALARM_MINUTE]);
  }
  Outbuf[RCU_OUTBUF_LAMP1] = ReadRTC(DATA0);
  Outbuf[RCU_OUTBUF_LAMP2] = ReadRTC(DATA1);
  Outbuf[RCU_OUTBUF_LAMP3] = ReadRTC(DATA2);
  Outbuf[RCU_OUTBUF_LAMP4] = ReadRTC(DATA3);
  Outbuf[RCU_OUTBUF_CONTROL] = ReadRTC(DATA4);
  Outbuf[RCU_OUTBUF_AIR] = ReadRTC(DATA5);
  Outbuf[RCU_OUTBUF_ALARM_HOUR] = ReadRTC(DATA6);
  Outbuf[RCU_OUTBUF_ALARM_MINUTE] = ReadRTC(DATA7);
#endif
  Outbuf[RCU_OUTBUF_NODE_ID] = NodeId;

  while (1) {
    CLRWDT();
#if defined(_SWITCH) || defined(_THERMOSTAT)
    if (EUSART_is_rx_ready()) {
      uint8_t rxByte = EUSART_Read();
      BusRx_ReceiveByte(rxByte);
    }
#endif

#ifdef _SWITCH
    {
      uint8_t keyboxCardType;

      while (SwitchInput_PollKeybox(&keyboxCardType)) {
        if (Boxbuf != keyboxCardType) {
          Boxbuf = keyboxCardType;
          Txdflag = true;
        }
      }
    }

    if ((!Keyflag) && SwitchInput_Poll(&Keybuf)) {
      Keyflag = true;
    }
#endif

#ifdef _THERMOSTAT
    {
      THERMOSTAT_PANEL_EVENT thermostatEvent;

      while (ThermostatPanel_Poll(&thermostatEvent)) {
        ThermostatPanel_UpdateOutputAir(&thermostatEvent);
        Txdflag = true;
      }
    }
#endif

#if defined(_SWITCH) && defined(_THERMOSTAT)
    SwitchThermostat_ProcessBus();
#elif defined(_SWITCH)
    SwitchLed_Process();
#elif defined(_THERMOSTAT)
    ThermostatPanel_Process();
#endif
    /* Read Input */
    if (Inpflag) {
      Inpflag = false;

      // Write Output
      if (PowUpflag) {
        WriteOutput();
        Swibuf[2] = SPI2_ExchangeByte(Relbuf[3]);
        Swibuf[1] = SPI2_ExchangeByte(Relbuf[2]);
        Swibuf[0] = SPI2_ExchangeByte(Relbuf[1]);
        SPI2_ExchangeByte(Relbuf[0]);
#ifdef _SWITCH
        SwitchLed_SyncFromOutputs();
#endif
      } else {
        Swibuf[2] = SPI2_ExchangeByte(0x00);
        Swibuf[1] = SPI2_ExchangeByte(0x00);
        Swibuf[0] = SPI2_ExchangeByte(0x00);
        SPI2_ExchangeByte(0x00);
      }
      STR_SetHigh();
      STR_SetLow();

      /* Read Input */
      ReadInput();
      /* Read Keypad */
      ReadKey();
    }

#if defined(_SERIAL)
    /* Read Serial */
    ReadSerial();

    /* Serial Input */
    if (Rxdflag) {
      Rxdflag = false;
      Rxdcom = atohl(Rcvbuf[0]) | atohr(Rcvbuf[1]);
      Rxdaddr = Rxdcom & 0x0f;
      Rxdcom >>= 4;

#ifdef MASTER_BOARD
      if (Rxdcom == _KEY) { //Keypad
        Keybuf = atohl(Rcvbuf[2]) | atohr(Rcvbuf[3]);
        Keyflag = true;
      } else if (Rxdcom == _INPUT) { //Input
        Inpbuf[3] = atohl(Rcvbuf[2]) | atohr(Rcvbuf[3]);
        Inpbuf[4] = atohl(Rcvbuf[4]) | atohr(Rcvbuf[5]);
        Inpbuf[5] = atohl(Rcvbuf[6]) | atohr(Rcvbuf[7]);
      }
#ifdef USE_RFID
      else if (Rxdcom == _RFID) { //Keybox
        Keybox[0] = atohl(Rcvbuf[2]) | atohr(Rcvbuf[3]);
        Keybox[1] = atohl(Rcvbuf[4]) | atohr(Rcvbuf[5]);
        Keybox[2] = atohl(Rcvbuf[6]) | atohr(Rcvbuf[7]);
        Keybox[3] = atohl(Rcvbuf[8]) | atohr(Rcvbuf[9]);
        Keybox[4] = atohl(Rcvbuf[10]) | atohr(Rcvbuf[11]);
        Keybox[5] = atohl(Rcvbuf[12]) | atohr(Rcvbuf[13]);
        Keybox[6] = atohl(Rcvbuf[14]) | atohr(Rcvbuf[15]);
        Keybox[7] = atohl(Rcvbuf[16]) | atohr(Rcvbuf[17]);
        Keybox[8] = atohl(Rcvbuf[18]) | atohr(Rcvbuf[19]);
        Keybox[9] = atohl(Rcvbuf[20]) | atohr(Rcvbuf[21]);
        Keybox[10] = atohl(Rcvbuf[22]) | atohr(Rcvbuf[23]);
        Keybox[11] = atohl(Rcvbuf[24]) | atohr(Rcvbuf[25]);
        Keybox[12] = atohl(Rcvbuf[26]) | atohr(Rcvbuf[27]);
        Keybox[13] = atohl(Rcvbuf[28]) | atohr(Rcvbuf[29]);
        Keybox[14] = atohl(Rcvbuf[30]) | atohr(Rcvbuf[31]);
        Keybox[15] = atohl(Rcvbuf[32]) | atohr(Rcvbuf[33]);
        Boxflag = true;
      }
#endif
#else
      if (Rxdcom == _ALL) {
        Outbuf[RCU_OUTBUF_LAMP1] = atohl(Rcvbuf[2]) | atohr(Rcvbuf[3]);
        Outbuf[RCU_OUTBUF_LAMP2] = atohl(Rcvbuf[4]) | atohr(Rcvbuf[5]);
        Outbuf[RCU_OUTBUF_LAMP3] = atohl(Rcvbuf[6]) | atohr(Rcvbuf[7]);
      }
#endif

#ifdef USE_ADEL
      if ((Rcvbuf[1] & 0xf0) == 0x00) { //No Card
        Inpbuf[0] &= ~Guein;
        Inpbuf[0] &= ~Maidin;
      } else if ((Rcvbuf[1] & 0xf0) == 0x80) { //Maid
        Inpbuf[0] &= ~Guein;
        Inpbuf[0] |= Maidin;
      } else { //Guest
        Inpbuf[0] |= Guein;
        Inpbuf[0] &= ~Maidin;
      }
#endif
    }
#endif

    /* Serial Output*/
    MasterOutput_SyncControlBit();
#if defined(_SERIAL)
    if ((Txdflag) && (!Rcvflag)) {
#else
    if (Txdflag) {
#endif
      Txdflag = false;
#if defined(_SERIAL)      
      DIR_SetHigh();
      if (EUSART_is_tx_ready()) {
        EUSART_Write('\n');
        EUSART_Write(':');
        EUSART_Write(htoal((Txdcom << 4) | Rxdaddr));
        EUSART_Write(htoar((Txdcom << 4) | Rxdaddr));
#ifdef MASTER_BOARD
        //        EUSART_Write(htoal(Outbuf[RCU_OUTBUF_HOUR]));
        //        EUSART_Write(htoar(Outbuf[RCU_OUTBUF_HOUR]));
        //        EUSART_Write(htoal(Outbuf[RCU_OUTBUF_MINUTE]));
        //        EUSART_Write(htoar(Outbuf[RCU_OUTBUF_MINUTE]));
        //        EUSART_Write(htoal(Outbuf[RCU_OUTBUF_TEMP_ACTUAL]));
        //        EUSART_Write(htoar(Outbuf[RCU_OUTBUF_TEMP_ACTUAL]));
        EUSART_Write(htoal(Outbuf[RCU_OUTBUF_LAMP1]));
        EUSART_Write(htoar(Outbuf[RCU_OUTBUF_LAMP1]));
        EUSART_Write(htoal(Outbuf[RCU_OUTBUF_LAMP2]));
        EUSART_Write(htoar(Outbuf[RCU_OUTBUF_LAMP2]));
        EUSART_Write(htoal(Outbuf[RCU_OUTBUF_LAMP3]));
        EUSART_Write(htoar(Outbuf[RCU_OUTBUF_LAMP3]));
        EUSART_Write(htoal(Outbuf[RCU_OUTBUF_LAMP4]));
        EUSART_Write(htoar(Outbuf[RCU_OUTBUF_LAMP4]));
        EUSART_Write(htoal(Outbuf[RCU_OUTBUF_CONTROL]));
        EUSART_Write(htoar(Outbuf[RCU_OUTBUF_CONTROL]));
        //        EUSART_Write(htoal(Outbuf[RCU_OUTBUF_AIR]));
        //        EUSART_Write(htoar(Outbuf[RCU_OUTBUF_AIR]));
        //        EUSART_Write(htoal(Outbuf[RCU_OUTBUF_ALARM_HOUR]));
        //        EUSART_Write(htoar(Outbuf[RCU_OUTBUF_ALARM_HOUR]));
        //        EUSART_Write(htoal(Outbuf[RCU_OUTBUF_ALARM_MINUTE]));
        //        EUSART_Write(htoar(Outbuf[RCU_OUTBUF_ALARM_MINUTE]));
#else
        EUSART_Write(htoal(Inpbuf[0]));
        EUSART_Write(htoar(Inpbuf[0]));
        EUSART_Write(htoal(Inpbuf[1]));
        EUSART_Write(htoar(Inpbuf[1]));
        EUSART_Write(htoal(Inpbuf[2]));
        EUSART_Write(htoar(Inpbuf[2]));
#endif
        EUSART_Write('\r');
      }
      while (!EUSART_is_tx_done());
      DIR_SetLow();
#endif

#ifdef _THERMOSTAT
      ThermostatPanel_HandleTxdEvent();
#endif

#ifdef USE_RTC
      WriteRTC(DATA0, Outbuf[RCU_OUTBUF_LAMP1]);
      WriteRTC(DATA1, Outbuf[RCU_OUTBUF_LAMP2]);
      WriteRTC(DATA2, Outbuf[RCU_OUTBUF_LAMP3]);
      WriteRTC(DATA3, Outbuf[RCU_OUTBUF_LAMP4]);
      WriteRTC(DATA4, Outbuf[RCU_OUTBUF_CONTROL]);
      WriteRTC(DATA5, Outbuf[RCU_OUTBUF_AIR]);
      WriteRTC(DATA6, Outbuf[RCU_OUTBUF_ALARM_HOUR]);
      WriteRTC(DATA7, Outbuf[RCU_OUTBUF_ALARM_MINUTE]);
#endif    
    }

#ifdef USE_RFID
    //Keybox
    if (Boxflag) {
      Boxflag = false;
      if (Keybox[0] == 0x00) { //No Card
        Boxbuf = NONE;
      } else if (Keybox[0] == 0x01) { //Guest
        Boxbuf = GUEST;
      } else if (Keybox[0] == 0x02) { //Floor & Maid 
        Boxbuf = FLOOR;
      } else if (Keybox[0] == 0x03) { //Building
        Boxbuf = GUEST;
      } else if (Keybox[0] == 0x04) { //Emergency
        Boxbuf = GUEST;
      } else if (Keybox[0] == 0x05) { //Master
        Boxbuf = GUEST;
      }
    }
#endif

    /* Check Guest */
#ifdef MASTER_BOARD

#ifdef USE_CCS
    if ((Inpbuf[0] & Ccsin) && ((Boxbuf == GUEST) || (Inpbuf[0] & Guein))) {
#else
    if ((Boxbuf == GUEST) || (Inpbuf[0] & Guein)) {
#endif
      if (Inpcnt >= (MAIN_INPUT_DELAY_MS / MAIN_TIMER_TICK_MS)) {
        Inpcnt = 0;
        if (!(Outbuf[RCU_OUTBUF_CONTROL] & Guest)) {
          Inpmem[0] = Inpbuf[0];
          Inpmem[1] = Inpbuf[1];
          Inpmem[2] = Inpbuf[2];
          Inpmem[3] = Inpbuf[3];
          Inpmem[4] = Inpbuf[4];
          Inpmem[5] = Inpbuf[5];
          Outbuf[RCU_OUTBUF_LAMP1] = WELCOME1;
          Outbuf[RCU_OUTBUF_LAMP2] = WELCOME2;
          Outbuf[RCU_OUTBUF_LAMP3] = WELCOME3;
          Outbuf[RCU_OUTBUF_LAMP4] = WELCOME4;
	          Outbuf[RCU_OUTBUF_CONTROL] |= Guest;
	          Outbuf[RCU_OUTBUF_CONTROL] &= ~(Maid | Door);
	          Outbuf[RCU_OUTBUF_AIR] = (AirPow | FanH | (25u - 16u));
#ifdef _THERMOSTAT
	          ThermostatPanel_SyncCommandFromAirOutput(true);
#endif
	
#ifdef USE_DOOR
	          Outbuf[RCU_OUTBUF_CONTROL] |= Door;
          if (!(Inpbuf[0] & Doorin)) {
            Outbuf[RCU_OUTBUF_LAMP2] &= ~DOOR_OUT;
          }
#endif
          Keyflag = false;
          Txdflag = true;
#ifdef _SWITCH
          SwitchLed_SyncFromOutputs();
#endif
#ifdef USE_RTC
          WriteRTC(RTCMEM, 0xA3);
#endif
        }
      }
      Offcnt = 0;
    }/* Power Off Delay */
    else {
      Inpcnt = 0;
      if (Outbuf[RCU_OUTBUF_CONTROL] & Guest) {
        if (Timeflag) {
          if (++Offcnt >= OFFTIME) {
            Outbuf[RCU_OUTBUF_LAMP1] = 0x00; //Relay off
            Outbuf[RCU_OUTBUF_LAMP2] = 0x00;
            Outbuf[RCU_OUTBUF_LAMP3] = 0x00;
	            Outbuf[RCU_OUTBUF_LAMP4] = 0x00;
	            Outbuf[RCU_OUTBUF_CONTROL] &= (Dnd | Mur | Ccs);
	            Outbuf[RCU_OUTBUF_AIR] &= ~AirPow;
#ifdef _THERMOSTAT
	            ThermostatPanel_SyncCommandFromAirOutput(true);
#endif
	            Txdflag = true;
#ifdef USE_RTC
            WriteRTC(RTCMEM, 0x00);
#endif
          }
        }
      }
    }

#ifdef USE_DOOR
    /* Guest In */
    if (Outbuf[RCU_OUTBUF_CONTROL] & Guest) {
      // Door Senser
      if ((Outbuf[RCU_OUTBUF_CONTROL] & Door) && (Timeflag)) {
        if (Inpbuf[0] & Doorin) {
          if (!(Inpmem[0] & Doorin)) {
            if (++Doorcnt >= DOORON) { // 10 Sec
              Inpmem[0] |= Doorin;
              Doorcnt = 0;
              if (!(Outbuf[RCU_OUTBUF_LAMP2] & DOOR_OUT)) {
                Outbuf[RCU_OUTBUF_LAMP2] |= DOOR_OUT; // Door On
                Txdflag = true;
              }
            }
          }
        } else {
          if (Inpmem[0] & Doorin) {
            if (++Doorcnt >= DOOROFF) { // 1 Minute
              Inpmem[0] &= ~Doorin;
              Doorcnt = 0;
              if (Outbuf[RCU_OUTBUF_LAMP2] & DOOR_OUT) {
                Outbuf[RCU_OUTBUF_LAMP2] &= ~DOOR_OUT; // Door Off
                Txdflag = true;
              }
            }
          }
        }
      }
    }
#endif

    /******** Keyboard ********/
    if ((Keyflag) && (!Txdflag)) {
      Keyflag = false;
      Txdflag = true;
#ifdef _SERIAL
      Txdcom = _ALL;
#endif

      /******** Service ********/
#ifdef USE_SERVICE
      // Dnd
      if (Keybuf == Dndsw) { //Service Dnd Input			
        Outbuf[RCU_OUTBUF_CONTROL] ^= Dnd;
        Outbuf[RCU_OUTBUF_CONTROL] &= ~Mur;
      }

      // Mur  
      if (Keybuf == Mursw) { //Service Mur Input			
        Outbuf[RCU_OUTBUF_CONTROL] ^= Mur;
        Outbuf[RCU_OUTBUF_CONTROL] &= ~Dnd;
      }

      // Bell
      if (Keybuf == Bellsw) { //Service Bell Input			
        if (Outbuf[RCU_OUTBUF_CONTROL] & Dnd) {
          Dndflag = true;
          Dndcnt = 0;
        } else Beep();
      }
#endif

      if (Outbuf[RCU_OUTBUF_CONTROL] & Guest) {

        /******** Air ********/
#ifdef _AIR
        if (Rxdaddr == _AIR) {
          if (Keybuf == AirPow) { //Air on/off
            if (Outbuf[RCU_OUTBUF_AIR] & AirPow) {
              Outbuf[RCU_OUTBUF_AIR] &= ~AirPow;
            } else {
              Outbuf[RCU_OUTBUF_AIR] |= AirPow;
              if (!(Outbuf[RCU_OUTBUF_AIR] & FanSpeed)) {
                Outbuf[RCU_OUTBUF_AIR] |= FanH;
              }
            }
          }
          /******** Fan Speed ********/
          if (Outbuf[RCU_OUTBUF_AIR] & AirPow) {
            if ((Keybuf == Fanhighsw) || (Keybuf == Fanmedsw) || (Keybuf == Fanlowsw)) {
              Outbuf[RCU_OUTBUF_AIR] &= ~FanSpeed;
              if (Keybuf == Fanhighsw) {//Fan Speed high
                Outbuf[RCU_OUTBUF_AIR] |= FanH;
              } else if (Keybuf == Fanmedsw) {//Fan Speed med
                Outbuf[RCU_OUTBUF_AIR] |= FanM;
              } else if (Keybuf == Fanlowsw) {//Fan Speed low
                Outbuf[RCU_OUTBUF_AIR] |= FanL;
              }
            }
          }
          /******** Setup Temp ********/
#ifdef _SETTEMP
          if (Outbuf[RCU_OUTBUF_AIR] & AirPow) {
            if (Keybuf == Tempupsw || Keybuf == Tempdnsw) {
              if (Keybuf == Tempupsw) {
                if ((Outbuf[RCU_OUTBUF_AIR] & Temp) < (31u - 16u)) {
                  Outbuf[RCU_OUTBUF_AIR]++;
                }
              } else {
                if ((Outbuf[RCU_OUTBUF_AIR] & Temp) > 0u) {
                  Outbuf[RCU_OUTBUF_AIR]--;
                }
              }
              Txdcom = _AIR;
              Txdflag = true;
            }
          }
#endif
        }
#endif                   

#ifdef _MASTER      // MASTER
        if (Keybuf == Mas1sw) {
#ifdef _SWITCH
          SwitchLed_RequestMasterLedRefresh();
#endif
          if (
#ifdef _SWITCH
              SwitchLed_IsMasterGroupOff()
#else
              (!(Outbuf[RCU_OUTBUF_LAMP1] & MASTER1_1_OFF) && !(Outbuf[RCU_OUTBUF_LAMP2] & MASTER1_2_OFF) && !(Outbuf[RCU_OUTBUF_LAMP3] & MASTER1_3_OFF) && !(Outbuf[RCU_OUTBUF_LAMP4] & MASTER1_4_OFF))
#endif
             ) {
#if MASTER1_1_ON != 0u
            Outbuf[RCU_OUTBUF_LAMP1] |= MASTER1_1_ON;
#endif
#if MASTER1_2_ON != 0u
            Outbuf[RCU_OUTBUF_LAMP2] |= MASTER1_2_ON;
#endif
#if MASTER1_3_ON != 0u
            Outbuf[RCU_OUTBUF_LAMP3] |= MASTER1_3_ON;
#endif
#if MASTER1_4_ON != 0u
            Outbuf[RCU_OUTBUF_LAMP4] |= MASTER1_4_ON;
#endif            
          } else {
#if MASTER1_1_OFF != 0u
            Outbuf[RCU_OUTBUF_LAMP1] &= ~MASTER1_1_OFF;
#endif
#if MASTER1_2_OFF != 0u
            Outbuf[RCU_OUTBUF_LAMP2] &= ~MASTER1_2_OFF;
#endif
#if MASTER1_3_OFF != 0u
            Outbuf[RCU_OUTBUF_LAMP3] &= ~MASTER1_3_OFF;
#endif
#if MASTER1_4_OFF != 0u
            Outbuf[RCU_OUTBUF_LAMP4] &= ~MASTER1_4_OFF;
#endif
#if defined(_SWITCH) && defined(USE_SWITCH_BACKLIGHT_OFF)
            SwitchLed_RequestBacklightOff();
#endif
          }
        }
#endif    

#ifdef _INPUT1      // R1
        if (Keybuf == In1sw) {
#ifdef RESET_MASTER
#endif          
          Outbuf[RCU_OUTBUF_LAMP1] ^= Out1;
        }
#endif

#ifdef _INPUT2      // R2
        if (Keybuf == In2sw) {
#ifdef RESET_MASTER
#endif     
          Outbuf[RCU_OUTBUF_LAMP1] ^= Out2;
        }
#endif

#ifdef _INPUT3      // R3
        if (Keybuf == In3sw) {
#ifdef RESET_MASTER
#endif     
          Outbuf[RCU_OUTBUF_LAMP1] ^= Out3;
        }
#endif

#ifdef _INPUT4      // R4
        if (Keybuf == In4sw) {
#ifdef RESET_MASTER
#endif     
          Outbuf[RCU_OUTBUF_LAMP1] ^= Out4;
        }
#endif

#ifdef _INPUT5      // R5
        if (Keybuf == In5sw) {
#ifdef _SWITCH
          SwitchLed_CheckManualMasterGroupOn(Keybuf);
#endif
#ifdef RESET_MASTER
#endif     
          Outbuf[RCU_OUTBUF_LAMP1] ^= Out5;
        }
#endif

#ifdef _INPUT6      // R6
        if (Keybuf == In6sw) {
#ifdef _SWITCH
          SwitchLed_CheckManualMasterGroupOn(Keybuf);
#endif
#ifdef RESET_MASTER
#endif     
          Outbuf[RCU_OUTBUF_LAMP1] ^= Out6;
        }
#endif

#ifdef _INPUT7      // R7
        if (Keybuf == In7sw) {
#ifdef _SWITCH
          SwitchLed_CheckManualMasterGroupOn(Keybuf);
#endif
#ifdef RESET_MASTER
#endif     
          Outbuf[RCU_OUTBUF_LAMP1] ^= Out7;
        }
#endif

#ifdef _INPUT8      // R8
        if (Keybuf == In8sw) {
#ifdef _SWITCH
          SwitchLed_CheckManualMasterGroupOn(Keybuf);
#endif
#ifdef RESET_MASTER
#endif     
          Outbuf[RCU_OUTBUF_LAMP1] ^= Out8;
        }
#endif

#ifdef _INPUT9      // R9
        if (Keybuf == In9sw) {
#ifdef _SWITCH
          SwitchLed_CheckManualMasterGroupOn(Keybuf);
#endif
#ifdef RESET_MASTER
#endif     
          Outbuf[RCU_OUTBUF_LAMP2] ^= Out9;
        }
#endif

#ifdef _INPUT10     // R10
        if (Keybuf == In10sw) {
#ifdef _SWITCH
          SwitchLed_CheckManualMasterGroupOn(Keybuf);
#endif
#ifdef RESET_MASTER
#endif     
          Outbuf[RCU_OUTBUF_LAMP2] ^= Out10;
        }
#endif

#ifdef _INPUT11     // R11
        if (Keybuf == In11sw) {
#ifdef _SWITCH
          SwitchLed_CheckManualMasterGroupOn(Keybuf);
#endif
#ifdef RESET_MASTER
#endif   
          Outbuf[RCU_OUTBUF_LAMP2] ^= Out11;
        }
#endif

#ifdef _INPUT12     // R12
        if (Keybuf == In12sw) {
#ifdef RESET_MASTER
#endif     
          Outbuf[RCU_OUTBUF_LAMP2] ^= Out12;
        }
#endif

#ifdef _INPUT13     // R13
        if (Keybuf == In13sw) {
#ifdef RESET_MASTER
#endif     
          Outbuf[RCU_OUTBUF_LAMP2] ^= Out13;
        }
#endif

#ifdef _INPUT14     // R14
        if (Keybuf == In14sw) {
#ifdef RESET_MASTER
#endif     
          Outbuf[RCU_OUTBUF_LAMP2] ^= Out14;
        }
#endif

#ifdef _INPUT15     // R15
        if (Keybuf == In15sw) {
#ifdef RESET_MASTER
#endif     
          Outbuf[RCU_OUTBUF_LAMP2] ^= Out15;
        }
#endif

#ifdef _INPUT16     // R16
        if (Keybuf == In16sw) {
#ifdef RESET_MASTER
#endif     
          Outbuf[RCU_OUTBUF_LAMP2] ^= Out16;
        }
#endif

#ifdef _INPUT17     // R17
        if (Keybuf == In17sw) {
#ifdef RESET_MASTER
#endif     
          Outbuf[RCU_OUTBUF_LAMP3] ^= Out17;
        }
#endif

#ifdef _INPUT18     // R18
        if (Keybuf == In18sw) {
#ifdef RESET_MASTER
#endif     
          Outbuf[RCU_OUTBUF_LAMP3] ^= Out18;
        }
#endif

#ifdef _INPUT19     // R19
        if (Keybuf == In19sw) {
#ifdef RESET_MASTER
#endif     
          Outbuf[RCU_OUTBUF_LAMP3] ^= Out19;
        }
#endif

#ifdef _INPUT20     // R20
        if (Keybuf == In20sw) {
#ifdef RESET_MASTER
#endif     
          Outbuf[RCU_OUTBUF_LAMP3] ^= Out20;
        }
#endif

#ifdef _INPUT21     // R21
        if (Keybuf == In21sw) {
#ifdef RESET_MASTER
#endif     
          Outbuf[RCU_OUTBUF_LAMP3] ^= Out21;
        }
#endif 

#ifdef _INPUT22     // R22
        if (Keybuf == In22sw) {
#ifdef RESET_MASTER
#endif     
          Outbuf[RCU_OUTBUF_LAMP3] ^= Out22;
        }
#endif 

#ifdef _INPUT23     // R23
        if (Keybuf == In23sw) {
#ifdef RESET_MASTER
#endif     
          Outbuf[RCU_OUTBUF_LAMP3] ^= Out23;
        }
#endif         

#ifdef _INPUT24     // R24
        if (Keybuf == In24sw) {
#ifdef RESET_MASTER
#endif     
          Outbuf[RCU_OUTBUF_LAMP3] ^= Out24;
        }
#endif

#ifdef _INPUT25     // R25
        if (Keybuf == In25sw) {
#ifdef RESET_MASTER
#endif     
          Outbuf[RCU_OUTBUF_LAMP4] ^= Out25;
        }
#endif

#ifdef _INPUT26     // R26
        if (Keybuf == In26sw) {
#ifdef RESET_MASTER
#endif     
          Outbuf[RCU_OUTBUF_LAMP4] ^= Out26;
        }
#endif

#ifdef _INPUT27     // R27
        if (Keybuf == In27sw) {
#ifdef RESET_MASTER
#endif     
          Outbuf[RCU_OUTBUF_LAMP4] ^= Out27;
        }
#endif
#ifdef _INPUT28     // R28
        if (Keybuf == In28sw) {
#ifdef RESET_MASTER
#endif     
          Outbuf[RCU_OUTBUF_LAMP4] ^= Out28;
        }
#endif

      } else { // Power Off
        /******** Setup time ********/
#ifdef _SETTIME
        if ((Keybuf == Hoursw) || (Keybuf == Minsw)) {
          if (Keybuf == Hoursw) {
            Outbuf[RCU_OUTBUF_HOUR] = UpBCD(Outbuf[RCU_OUTBUF_HOUR]); //RTC Hour
            if (Outbuf[RCU_OUTBUF_HOUR] >= 0x24) {
              Outbuf[RCU_OUTBUF_HOUR] = 0x00;
            }
#ifdef USE_RTC
            WriteRTC(HSRADD, Outbuf[RCU_OUTBUF_HOUR]);
#endif
          } else {
            Outbuf[RCU_OUTBUF_MINUTE] = UpBCD(Outbuf[RCU_OUTBUF_MINUTE]); //RTC Minute
            if (Outbuf[RCU_OUTBUF_MINUTE] >= 0x60) {
              Outbuf[RCU_OUTBUF_MINUTE] = 0x00;
            }
#ifdef USE_RTC
            WriteRTC(MINADD, Outbuf[RCU_OUTBUF_MINUTE]);
#endif
          }

          Timeflag = true;
          Timer = 0;
        }
#endif
      }
	    }
#endif

#if defined(_SWITCH)
    SwitchLed_SyncFromOutputs();
#endif
#ifdef _THERMOSTAT
    ThermostatPanel_SyncCommandFromAirOutput(false);
#ifdef _SWITCH
    SwitchThermostat_ProcessBus();
#endif
#endif
	
	    /* Half Timer */
	    if (Halfflag) {
      Halfflag = false;

      // Led Monitor
#ifdef USE_DOOR
#ifdef USE_CCS
      if ((Inpbuf[0] & Ccsin) && (Inpbuf[0] & Doorin) && (Inpbuf[0] & Guein)) {
#else
      if ((Inpbuf[0] & Doorin) && (Inpbuf[0] & Guein)) {
#endif
#else
#ifdef USE_CCS
      if ((Inpbuf[0] & Ccsin) && (Inpbuf[0] & Guein)) {
#else
      if ((Boxbuf == GUEST) || (Inpbuf[0] & Guein)) {
#endif
#endif
        Powerled = true; //On power Led
        if (Timeflag) {
          Relbuf[1] ^= POWLED; //Blink Monitor
        }
      } else {
        if (Outbuf[RCU_OUTBUF_CONTROL] & Guest) {
          Powerled ^= true; //Blink power led
          Relbuf[1] ^= POWLED; //Fast Blink Monitor
        } else if (Timeflag) {
          Powerled ^= true; //Normal Blink power led
          Relbuf[1] ^= POWLED; //Blink Monitor
        }
      }

    }

    /******** Timer ********/
    if (Timeflag) {
      Timeflag = false;

      // Power Up Delay
      if (++PowUpcnt >= POWERUP) {
        if (!PowUpflag) {
          PowUpflag = true;
          Txdflag = true;
        }
      }

      // Read RTC     
#ifdef USE_RTC
      if (Outbuf[RCU_OUTBUF_MINUTE] != ReadRTC(MINADD)) {
        Outbuf[RCU_OUTBUF_MINUTE] = ReadRTC(MINADD);
        Outbuf[RCU_OUTBUF_HOUR] = ReadRTC(HSRADD);
        Txdflag = true;
      }
#endif
    }

    /* Timer2 */
    if (TMR2_HasOverflowOccured()) {
#ifdef _SWITCH
      SwitchLed_TimerTick();
#endif
#ifdef _THERMOSTAT
      ThermostatPanel_TimerTick();
#endif
      Inpflag = true;
      Inpcnt++; //Key delay

      if (++Timer == (MAIN_HALF_TIMER_MS / MAIN_TIMER_TICK_MS)) { //250 mSec
        Halfflag = true;
      } else if (Timer >= (MAIN_TIME_TIMER_MS / MAIN_TIMER_TICK_MS)) { //500 mSec
        Halfflag = true;
        Timeflag = true;
        Timer = 0;
      }

#if defined(_SERIAL)
      if (++Rcvdly >= (30 / 10)) { //Uart delay
        Rcvflag = false;
      }
#endif

      if (++upDateCnt1 >= (300 / 10)) { //Fan Delay
        upDateFlag1 = true;
      }

      if (Dndflag) { //Dnd flag
        if (++Blkcnt >= (100 / 10)) { //Dnd delay
          Blkcnt = 0;
          if (++Dndcnt <= (3 * 2)) { //Blink counter
            Outbuf[RCU_OUTBUF_CONTROL] ^= Dnd;
          } else Dndflag = false;
        }
      }
    }// Timer 2

  }//while loop
}
/**
 End of File
 */
