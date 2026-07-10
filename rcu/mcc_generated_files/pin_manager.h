/**
  @Generated Pin Manager Header File

  @Company:
    Microchip Technology Inc.

  @File Name:
    pin_manager.h

  @Summary:
    This is the Pin Manager file generated using PIC10 / PIC12 / PIC16 / PIC18 MCUs

  @Description
    This header file provides APIs for driver for .
    Generation Information :
        Product Revision  :  PIC10 / PIC12 / PIC16 / PIC18 MCUs - 1.81.8
        Device            :  PIC16F18325
        Driver Version    :  2.11
    The generated drivers are tested against the following:
        Compiler          :  XC8 2.36 and above
        MPLAB 	          :  MPLAB X 6.00	
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

#ifndef PIN_MANAGER_H
#define PIN_MANAGER_H

/**
  Section: Included Files
*/

#include <xc.h>

#define INPUT   1
#define OUTPUT  0

#define HIGH    1
#define LOW     0

#define ANALOG      1
#define DIGITAL     0

#define PULL_UP_ENABLED      1
#define PULL_UP_DISABLED     0

// get/set RA0 procedures
#define RA0_SetHigh()            do { LATAbits.LATA0 = 1; } while(0)
#define RA0_SetLow()             do { LATAbits.LATA0 = 0; } while(0)
#define RA0_Toggle()             do { LATAbits.LATA0 = ~LATAbits.LATA0; } while(0)
#define RA0_GetValue()              PORTAbits.RA0
#define RA0_SetDigitalInput()    do { TRISAbits.TRISA0 = 1; } while(0)
#define RA0_SetDigitalOutput()   do { TRISAbits.TRISA0 = 0; } while(0)
#define RA0_SetPullup()             do { WPUAbits.WPUA0 = 1; } while(0)
#define RA0_ResetPullup()           do { WPUAbits.WPUA0 = 0; } while(0)
#define RA0_SetAnalogMode()         do { ANSELAbits.ANSA0 = 1; } while(0)
#define RA0_SetDigitalMode()        do { ANSELAbits.ANSA0 = 0; } while(0)

// get/set RA1 procedures
#define RA1_SetHigh()            do { LATAbits.LATA1 = 1; } while(0)
#define RA1_SetLow()             do { LATAbits.LATA1 = 0; } while(0)
#define RA1_Toggle()             do { LATAbits.LATA1 = ~LATAbits.LATA1; } while(0)
#define RA1_GetValue()              PORTAbits.RA1
#define RA1_SetDigitalInput()    do { TRISAbits.TRISA1 = 1; } while(0)
#define RA1_SetDigitalOutput()   do { TRISAbits.TRISA1 = 0; } while(0)
#define RA1_SetPullup()             do { WPUAbits.WPUA1 = 1; } while(0)
#define RA1_ResetPullup()           do { WPUAbits.WPUA1 = 0; } while(0)
#define RA1_SetAnalogMode()         do { ANSELAbits.ANSA1 = 1; } while(0)
#define RA1_SetDigitalMode()        do { ANSELAbits.ANSA1 = 0; } while(0)

// get/set RA2 procedures
#define RA2_SetHigh()            do { LATAbits.LATA2 = 1; } while(0)
#define RA2_SetLow()             do { LATAbits.LATA2 = 0; } while(0)
#define RA2_Toggle()             do { LATAbits.LATA2 = ~LATAbits.LATA2; } while(0)
#define RA2_GetValue()              PORTAbits.RA2
#define RA2_SetDigitalInput()    do { TRISAbits.TRISA2 = 1; } while(0)
#define RA2_SetDigitalOutput()   do { TRISAbits.TRISA2 = 0; } while(0)
#define RA2_SetPullup()             do { WPUAbits.WPUA2 = 1; } while(0)
#define RA2_ResetPullup()           do { WPUAbits.WPUA2 = 0; } while(0)
#define RA2_SetAnalogMode()         do { ANSELAbits.ANSA2 = 1; } while(0)
#define RA2_SetDigitalMode()        do { ANSELAbits.ANSA2 = 0; } while(0)

// get/set SPK aliases
#define SPK_TRIS                 TRISAbits.TRISA4
#define SPK_LAT                  LATAbits.LATA4
#define SPK_PORT                 PORTAbits.RA4
#define SPK_WPU                  WPUAbits.WPUA4
#define SPK_OD                   ODCONAbits.ODCA4
#define SPK_ANS                  ANSELAbits.ANSA4
#define SPK_SetHigh()            do { LATAbits.LATA4 = 1; } while(0)
#define SPK_SetLow()             do { LATAbits.LATA4 = 0; } while(0)
#define SPK_Toggle()             do { LATAbits.LATA4 = ~LATAbits.LATA4; } while(0)
#define SPK_GetValue()           PORTAbits.RA4
#define SPK_SetDigitalInput()    do { TRISAbits.TRISA4 = 1; } while(0)
#define SPK_SetDigitalOutput()   do { TRISAbits.TRISA4 = 0; } while(0)
#define SPK_SetPullup()          do { WPUAbits.WPUA4 = 1; } while(0)
#define SPK_ResetPullup()        do { WPUAbits.WPUA4 = 0; } while(0)
#define SPK_SetPushPull()        do { ODCONAbits.ODCA4 = 0; } while(0)
#define SPK_SetOpenDrain()       do { ODCONAbits.ODCA4 = 1; } while(0)
#define SPK_SetAnalogMode()      do { ANSELAbits.ANSA4 = 1; } while(0)
#define SPK_SetDigitalMode()     do { ANSELAbits.ANSA4 = 0; } while(0)

// get/set RTS aliases
#define RTS_TRIS                 TRISAbits.TRISA5
#define RTS_LAT                  LATAbits.LATA5
#define RTS_PORT                 PORTAbits.RA5
#define RTS_WPU                  WPUAbits.WPUA5
#define RTS_OD                   ODCONAbits.ODCA5
#define RTS_ANS                  ANSELAbits.ANSA5
#define RTS_SetHigh()            do { LATAbits.LATA5 = 1; } while(0)
#define RTS_SetLow()             do { LATAbits.LATA5 = 0; } while(0)
#define RTS_Toggle()             do { LATAbits.LATA5 = ~LATAbits.LATA5; } while(0)
#define RTS_GetValue()           PORTAbits.RA5
#define RTS_SetDigitalInput()    do { TRISAbits.TRISA5 = 1; } while(0)
#define RTS_SetDigitalOutput()   do { TRISAbits.TRISA5 = 0; } while(0)
#define RTS_SetPullup()          do { WPUAbits.WPUA5 = 1; } while(0)
#define RTS_ResetPullup()        do { WPUAbits.WPUA5 = 0; } while(0)
#define RTS_SetPushPull()        do { ODCONAbits.ODCA5 = 0; } while(0)
#define RTS_SetOpenDrain()       do { ODCONAbits.ODCA5 = 1; } while(0)
#define RTS_SetAnalogMode()      do { ANSELAbits.ANSA5 = 1; } while(0)
#define RTS_SetDigitalMode()     do { ANSELAbits.ANSA5 = 0; } while(0)

// get/set STR aliases
#define STR_TRIS                 TRISCbits.TRISC0
#define STR_LAT                  LATCbits.LATC0
#define STR_PORT                 PORTCbits.RC0
#define STR_WPU                  WPUCbits.WPUC0
#define STR_OD                   ODCONCbits.ODCC0
#define STR_ANS                  ANSELCbits.ANSC0
#define STR_SetHigh()            do { LATCbits.LATC0 = 1; } while(0)
#define STR_SetLow()             do { LATCbits.LATC0 = 0; } while(0)
#define STR_Toggle()             do { LATCbits.LATC0 = ~LATCbits.LATC0; } while(0)
#define STR_GetValue()           PORTCbits.RC0
#define STR_SetDigitalInput()    do { TRISCbits.TRISC0 = 1; } while(0)
#define STR_SetDigitalOutput()   do { TRISCbits.TRISC0 = 0; } while(0)
#define STR_SetPullup()          do { WPUCbits.WPUC0 = 1; } while(0)
#define STR_ResetPullup()        do { WPUCbits.WPUC0 = 0; } while(0)
#define STR_SetPushPull()        do { ODCONCbits.ODCC0 = 0; } while(0)
#define STR_SetOpenDrain()       do { ODCONCbits.ODCC0 = 1; } while(0)
#define STR_SetAnalogMode()      do { ANSELCbits.ANSC0 = 1; } while(0)
#define STR_SetDigitalMode()     do { ANSELCbits.ANSC0 = 0; } while(0)

// get/set SCL aliases
#define SCL_TRIS                 TRISCbits.TRISC1
#define SCL_LAT                  LATCbits.LATC1
#define SCL_PORT                 PORTCbits.RC1
#define SCL_WPU                  WPUCbits.WPUC1
#define SCL_OD                   ODCONCbits.ODCC1
#define SCL_ANS                  ANSELCbits.ANSC1
#define SCL_SetHigh()            do { LATCbits.LATC1 = 1; } while(0)
#define SCL_SetLow()             do { LATCbits.LATC1 = 0; } while(0)
#define SCL_Toggle()             do { LATCbits.LATC1 = ~LATCbits.LATC1; } while(0)
#define SCL_GetValue()           PORTCbits.RC1
#define SCL_SetDigitalInput()    do { TRISCbits.TRISC1 = 1; } while(0)
#define SCL_SetDigitalOutput()   do { TRISCbits.TRISC1 = 0; } while(0)
#define SCL_SetPullup()          do { WPUCbits.WPUC1 = 1; } while(0)
#define SCL_ResetPullup()        do { WPUCbits.WPUC1 = 0; } while(0)
#define SCL_SetPushPull()        do { ODCONCbits.ODCC1 = 0; } while(0)
#define SCL_SetOpenDrain()       do { ODCONCbits.ODCC1 = 1; } while(0)
#define SCL_SetAnalogMode()      do { ANSELCbits.ANSC1 = 1; } while(0)
#define SCL_SetDigitalMode()     do { ANSELCbits.ANSC1 = 0; } while(0)

// get/set SDA aliases
#define SDA_TRIS                 TRISCbits.TRISC2
#define SDA_LAT                  LATCbits.LATC2
#define SDA_PORT                 PORTCbits.RC2
#define SDA_WPU                  WPUCbits.WPUC2
#define SDA_OD                   ODCONCbits.ODCC2
#define SDA_ANS                  ANSELCbits.ANSC2
#define SDA_SetHigh()            do { LATCbits.LATC2 = 1; } while(0)
#define SDA_SetLow()             do { LATCbits.LATC2 = 0; } while(0)
#define SDA_Toggle()             do { LATCbits.LATC2 = ~LATCbits.LATC2; } while(0)
#define SDA_GetValue()           PORTCbits.RC2
#define SDA_SetDigitalInput()    do { TRISCbits.TRISC2 = 1; } while(0)
#define SDA_SetDigitalOutput()   do { TRISCbits.TRISC2 = 0; } while(0)
#define SDA_SetPullup()          do { WPUCbits.WPUC2 = 1; } while(0)
#define SDA_ResetPullup()        do { WPUCbits.WPUC2 = 0; } while(0)
#define SDA_SetPushPull()        do { ODCONCbits.ODCC2 = 0; } while(0)
#define SDA_SetOpenDrain()       do { ODCONCbits.ODCC2 = 1; } while(0)
#define SDA_SetAnalogMode()      do { ANSELCbits.ANSC2 = 1; } while(0)
#define SDA_SetDigitalMode()     do { ANSELCbits.ANSC2 = 0; } while(0)

// get/set DIR aliases
#define DIR_TRIS                 TRISCbits.TRISC3
#define DIR_LAT                  LATCbits.LATC3
#define DIR_PORT                 PORTCbits.RC3
#define DIR_WPU                  WPUCbits.WPUC3
#define DIR_OD                   ODCONCbits.ODCC3
#define DIR_ANS                  ANSELCbits.ANSC3
#define DIR_SetHigh()            do { LATCbits.LATC3 = 1; } while(0)
#define DIR_SetLow()             do { LATCbits.LATC3 = 0; } while(0)
#define DIR_Toggle()             do { LATCbits.LATC3 = ~LATCbits.LATC3; } while(0)
#define DIR_GetValue()           PORTCbits.RC3
#define DIR_SetDigitalInput()    do { TRISCbits.TRISC3 = 1; } while(0)
#define DIR_SetDigitalOutput()   do { TRISCbits.TRISC3 = 0; } while(0)
#define DIR_SetPullup()          do { WPUCbits.WPUC3 = 1; } while(0)
#define DIR_ResetPullup()        do { WPUCbits.WPUC3 = 0; } while(0)
#define DIR_SetPushPull()        do { ODCONCbits.ODCC3 = 0; } while(0)
#define DIR_SetOpenDrain()       do { ODCONCbits.ODCC3 = 1; } while(0)
#define DIR_SetAnalogMode()      do { ANSELCbits.ANSC3 = 1; } while(0)
#define DIR_SetDigitalMode()     do { ANSELCbits.ANSC3 = 0; } while(0)

// get/set RC4 procedures
#define RC4_SetHigh()            do { LATCbits.LATC4 = 1; } while(0)
#define RC4_SetLow()             do { LATCbits.LATC4 = 0; } while(0)
#define RC4_Toggle()             do { LATCbits.LATC4 = ~LATCbits.LATC4; } while(0)
#define RC4_GetValue()              PORTCbits.RC4
#define RC4_SetDigitalInput()    do { TRISCbits.TRISC4 = 1; } while(0)
#define RC4_SetDigitalOutput()   do { TRISCbits.TRISC4 = 0; } while(0)
#define RC4_SetPullup()             do { WPUCbits.WPUC4 = 1; } while(0)
#define RC4_ResetPullup()           do { WPUCbits.WPUC4 = 0; } while(0)
#define RC4_SetAnalogMode()         do { ANSELCbits.ANSC4 = 1; } while(0)
#define RC4_SetDigitalMode()        do { ANSELCbits.ANSC4 = 0; } while(0)

// get/set RC5 procedures
#define RC5_SetHigh()            do { LATCbits.LATC5 = 1; } while(0)
#define RC5_SetLow()             do { LATCbits.LATC5 = 0; } while(0)
#define RC5_Toggle()             do { LATCbits.LATC5 = ~LATCbits.LATC5; } while(0)
#define RC5_GetValue()              PORTCbits.RC5
#define RC5_SetDigitalInput()    do { TRISCbits.TRISC5 = 1; } while(0)
#define RC5_SetDigitalOutput()   do { TRISCbits.TRISC5 = 0; } while(0)
#define RC5_SetPullup()             do { WPUCbits.WPUC5 = 1; } while(0)
#define RC5_ResetPullup()           do { WPUCbits.WPUC5 = 0; } while(0)
#define RC5_SetAnalogMode()         do { ANSELCbits.ANSC5 = 1; } while(0)
#define RC5_SetDigitalMode()        do { ANSELCbits.ANSC5 = 0; } while(0)

/**
   @Param
    none
   @Returns
    none
   @Description
    GPIO and peripheral I/O initialization
   @Example
    PIN_MANAGER_Initialize();
 */
void PIN_MANAGER_Initialize (void);

/**
 * @Param
    none
 * @Returns
    none
 * @Description
    Interrupt on Change Handling routine
 * @Example
    PIN_MANAGER_IOC();
 */
void PIN_MANAGER_IOC(void);



#endif // PIN_MANAGER_H
/**
 End of File
*/