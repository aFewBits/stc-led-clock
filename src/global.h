
#ifndef _GLOBAL_H
#define _GLOBAL_H

#define TRUE    1                   // for general use, bit setters too
#define FALSE   0

#define FOSC 22118400L              // clock speed in mhz
#define T0TICKS 20000               // Timer 0 tick rate in hz (50us)
#define BAUD 115200                 // debug serial baud rate
#define RxD2 P3_6                   // UART only in 15W408AS
#define TxD2 P3_7                   // use for debugging
#define DEBUG FALSE                 // set true for serial debugging

#define TICKS_MS    20              // This is set by Timer 1 tick rate
#define MAX_BRIGHT  63              // maximum tick count for brightness
#define MIN_BRIGHT   1              // minimum tick count for brightness

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#define BOARD_BLUE_5
#define COMMON_ANODE

//#define BOARD_YELLOW_SMALL
//#define BOARD_WHITE_SMALL

//#define COMMON_CATHODE

//#define TEST_DEFAULTS

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#ifdef BOARD_BLUE_5

// Pushbutton port pins

#define S2 P3_0                     // push button input pin aliases
#define S1 P3_1

// DS1302 pin to port mapping

#define CE   P1_0                   // pin 5
#define IO   P1_1                   // pin 6
#define SCLK P1_2                   // pin 7

// adc channels for sensors
#define ADC_LDR   6                 // these are always Port 1
#define ADC_TEMP  7                 // just need the bit

// buzzer port pins and active state set

#define BZR_ON  P1_5 = 0            // direct port assigments
#define BZR_OFF P1_5 = 1            // with logic

#ifdef COMMON_ANODE
#define SET_PORT_DRIVE ;
#define LED_SET_ANODES   P3 &= ~(0b00000100 << aPos)
#define LED_RESET_ANODES P3 |=   0b00111100
#define LED_SET_CATHODES P2 = CathodeBuf[aPos]  // seg A = P.0 assumed
#endif

#endif  // end board_blue5

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#ifdef BOARD_WHITE_SMALL
#define NO_DIGIT_3_FLIP
#define DP1_IS_COLON

// Pushbutton port pins

#define S2 P3_0                     // push button input pin aliases
#define S1 P3_1

// DS1302 pin to port mapping

#define CE   P1_0                   // pin 5
#define IO   P1_1                   // pin 6
#define SCLK P1_2                   // pin 7

// adc channels for sensors

#define ADC_LDR   6                 // these are always Port 1
#define ADC_TEMP  7                 // just need the bit

// buzzer port pins and active state set

#define BZR_ON  P1_5 = 0            // direct port assigments
#define BZR_OFF P1_5 = 1            // with logic

#ifdef COMMON_ANODE
#define SET_PORT_DRIVE P3M0 = 0x3C;
#define LED_SET_ANODES P3   |= (0b00000100 << aPos)
#define LED_RESET_ANODES P3 &=  0b11000011
#define LED_SET_CATHODES P2 = CathodeBuf[aPos]  // seg A = P.0 assumed
#endif

#ifdef COMMON_CATHODE
#define SET_PORT_DRIVE P2M0 = 0xFF;
#define LED_SET_ANODES P3 &= ~(0b00000100 << aPos)
#define LED_RESET_ANODES P3 |=  0b00111100
#define LED_SET_CATHODES P2 = ~CathodeBuf[aPos]  // seg A = P.0 assumed
#endif

#endif  // endif board_white_small

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#ifdef BOARD_YELLOW_SMALL
#define NO_DIGIT_3_FLIP
#define DP1_IS_COLON

// Pushbutton port pins

#define S2 P2_6                     // push button input pin aliases
#define S1 P2_7

// DS1302 pin to port mapping

#define CE   P1_1                   // pin 5
#define IO   P1_2                   // pin 6
#define SCLK P1_0                   // pin 7

// adc channels for sensors
#define ADC_LDR   6                 // these are always Port 1
#define ADC_TEMP  7                 // just need the bit

// buzzer port pins and active state set
// Port 0.1 is 5.5 on the 15W408AS!!!!
#define BZR_ON  P5 = 0x00         // direct port assigments
#define BZR_OFF P5 = 0x20         // with logic

#ifdef COMMON_ANODE
#define SET_PORT_DRIVE P2M0 = 0x0F;
#define LED_SET_ANODES P2 |= (0b00001000 >> aPos)
#define LED_RESET_ANODES P2 &= 0b11110000
#define LED_SET_CATHODES P3 = CathodeBuf[aPos]  // seg A = P.0 assumed
#endif

#ifdef COMMON_CATHODE
#define SET_PORT_DRIVE P3M0 = 0xFF;
#define LED_SET_ANODES P2 &= ~(0b00001000 >> aPos)
#define LED_RESET_ANODES P2 |= 0b00001111
#define LED_SET_CATHODES P3 = ~CathodeBuf[aPos]  // seg A = P.0 assumed
#endif
#endif  // endif board_yellow_small

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#endif
