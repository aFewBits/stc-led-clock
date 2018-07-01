
#ifndef _GLOBAL_H
#define _GLOBAL_H

#define TRUE    1                   // for general use, bit setters too
#define FALSE   0

#define FOSC 22118400L              // clock speed in mhz
#define T0TICKS 20000               // Timer 0 tick rate (t) (1/t = 50us)
#define BAUD 115200                 // debug serial baud rate
#define RxD2 P3_6                   // UART only in 15W408AS
#define TxD2 P3_7                   // use for debugging
#define DEBUG FALSE                 // set true for serial debugging

#define TICKS_MS    20              // This is set by Timer 1 tick rate
#define MAX_BRIGHT  63              // maximum tick count for brightness
#define MIN_BRIGHT   1              // minimum tick count for brightness

//---------------------------------------------------------------------------
// Begin Hardware Option configuration
//---------------------------------------------------------------------------

#define BOARD_TALKING      TRUE
#define BOARD_BLUE_6       FALSE
#define BOARD_BLUE_5_RELAY FALSE
#define BOARD_YELLOW_5     FALSE
#define BOARD_YELLOW_SMALL FALSE
#define BOARD_WHITE_SMALL  FALSE
#define BOARD_GREEN_SMALL  FALSE

#define COMMON_ANODE   TRUE
#define COMMON_CATHODE FALSE

#define PROC_IS_15W408AS TRUE
#define PROC_IS_15W404AS FALSE
#define PROC_IS_15F204EA FALSE

#define HAS_LDR TRUE
#define HAS_THERMISTOR TRUE
#define HAS_RELAY FALSE

#define DIGIT_3_FLIP TRUE

// When setting TEST_DEFAULTS to TRUE,
// ensure that all display options are TRUE as well
// OPT_TEMP_DSP OPT_DATE_DSP OPT_DAY_DSP
// All three must be set to TRUE otherwise some code will fail
// Just ensure that any mode bit that is ON has
// has corresponding logic enabled in codeto support it.

#define TEST_DEFAULTS FALSE

//---------------------------------------------------------------------------
// End Hardware Option configuration
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Begin Software Option configuration
//---------------------------------------------------------------------------

#define OPT_ALARM       TRUE    // TRUE to implement
#define OPT_CHIME       TRUE    // FALSE removes
#define OPT_TEMP_DSP    TRUE
#define OPT_DATE_DSP    TRUE
#define OPT_DAY_DSP     TRUE
#define OPT_UNITS_GROUP FALSE    // use 12/F/MD or 24/C/DM groups

// Set the default units for the clock
// Use only one each of these groups of two

#define SET_12HR_FORMAT TRUE
#define SET_24HR_FORMAT FALSE

#define SET_MMDD_FORMAT TRUE
#define SET_DDMM_FORMAT FALSE

#define SET_DEGF_FORMAT TRUE
#define SET_DEGC_FORMAT FALSE

//---------------------------------------------------------------------------
// End Software Option configuration
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// Begin board hardware defines
//---------------------------------------------------------------------------
//
#if BOARD_TALKING

#define HAS_NY3P_SPEECH TRUE
#define NY3P_RST    P3_6
#define NY3P_DAT    P3_7
#define NY3P_BZY    P1_3

// Pushbutton port pins

#define S1 P3_1                     // push button input pin aliases
#define S2 P3_0
#define S3 P1_4

// DS1302 pin to port mapping

#define CE_HI P1_0 = 1;             // pin 5
#define CE_LO P1_0 = 0;
#define IO   P1_1                   // pin 6
#define SCLK P1_2                   // pin 7

// adc channels for sensors
#if HAS_LDR
#define ADC_LDR   6
#define SET_LDR_PORT  P1M1 |= (1 << ADC_LDR); P1M0 |= (1 << ADC_LDR);
#else
#define SET_LDR_PORT ;
#endif

#if HAS_THERMISTOR
#define ADC_TEMP  7
#define SET_THERMISTOR_PORT P1M1 |= (1 << ADC_TEMP); P1M0 |= (1 << ADC_TEMP);
#endif

// buzzer port pins and active state set
// This is an LED on the talking model

#define BZR_ON  P1_5 = 0            // direct port assigments
#define BZR_OFF P1_5 = 1            // with logic

#if COMMON_ANODE
#define SET_PORT_DRIVE ;
#define LED_SET_ANODES   P3 &= ~(0b00000100 << aPos)
#define LED_RESET_ANODES P3 |=   0b00111100
#define LED_SET_CATHODES P2 = CathodeBuf[aPos]  // seg A = P.0 assumed
#endif

#endif  // end board_talking

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#if BOARD_BLUE_6

// Pushbutton port pins

#define S2 P3_0                     // push button input pin aliases
#define S1 P3_1

#if HAS_RELAY                       // not suuplied (user can)
#define RELAY P1_4                  // port/pin for relay
#endif

// DS1302 pin to port mapping

#define CE_HI P1_0 = 1;             // pin 5
#define CE_LO P1_0 = 0;
#define IO   P1_1                   // pin 6
#define SCLK P1_2                   // pin 7

// adc channels for sensors
#if HAS_LDR
#define ADC_LDR   6
#define SET_LDR_PORT  P1M1 |= (1 << ADC_LDR); P1M0 |= (1 << ADC_LDR);
#else
#define SET_LDR_PORT ;
#endif

#if HAS_THERMISTOR
#define ADC_TEMP  7
#define SET_THERMISTOR_PORT P1M1 |= (1 << ADC_TEMP); P1M0 |= (1 << ADC_TEMP);
#endif

// buzzer port pins and active state set

#define BZR_ON  P1_5 = 0            // direct port assigments
#define BZR_OFF P1_5 = 1            // with logic

#if COMMON_ANODE
#define SET_PORT_DRIVE ;
#define LED_SET_ANODES   P3 &= ~(0b00000100 << aPos)
#define LED_RESET_ANODES P3 |=   0b00111100
#define LED_SET_CATHODES P2 = CathodeBuf[aPos]  // seg A = P.0 assumed
#endif

#endif  // end board_blue_6

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#if BOARD_BLUE_5_RELAY

// Pushbutton port pins

#define S1 P3_0
#define S2 P3_1                     // push button input pin aliases

#if HAS_RELAY
#define RELAY P1_4                  // port/pin for relay
#endif

// DS1302 pin to port mapping
#if PROC_IS_15W408AS || PROC_IS_15W404AS
#define CE_HI   P5 |= 0b00010000;       // P5.4 DS1302 pin 5 set
#define CE_LO   P5 &= 0b11101111;       //                   unset
#define IO_LO P5 &= 0b11011111;         // P5.5 DS1302 pin 6
#define IO_WR P5|=((W_Byte&0x01)<<5 )   // write I/O pin
#define IO_RD (P5 & 0b00100000)>>5      // read I/O pin
#define SCLK P3_2                       // P3.2 DS1302 pin 7
#elif PROC_IS_15F204EA
#define CE_HI  P0 |= 0b00000001;       // P5.4 DS1302 pin 5 set
#define CE_LO  P0 &= 0b11111110;       //                   unset
#define IO_LO  P0 &= 0b11111101;       // P5.5 DS1302 pin 6
#define IO_WR  P0 |= ((W_Byte&0x01)<<1 ) // write I/O pin
#define IO_RD  (P0 & 0b00000010)>>1      // read I/O pin
#define SCLK P3_2                       // P3.2 DS1302 pin 7
#endif

// adc channels for sensors
#if HAS_LDR
#define ADC_LDR   6
#define SET_LDR_PORT  P1M1 |= (1 << ADC_LDR); P1M0 |= (1 << ADC_LDR);
#else
#define SET_LDR_PORT ;
#endif

#if HAS_THERMISTOR
#define ADC_TEMP  3
#define SET_THERMISTOR_PORT P1M1 |= (1 << ADC_TEMP); P1M0 |= (1 << ADC_TEMP);
#endif

// buzzer port pins and active state set

#define BZR_ON  P3_3 = 0            // direct port assigments
#define BZR_OFF P3_3 = 1            // with logic

#if COMMON_ANODE
#define SET_PORT_DRIVE ;
#define LED_SET_ANODES   P3 &= ~(0b00010000 << aPos)
#define LED_RESET_ANODES P3 |=   0b11110000
#define LED_SET_CATHODES P2 = CathodeBuf[aPos]  // seg A = P.0 assumed
#endif

#endif  // end board_blue_5_relay

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#if BOARD_YELLOW_5

// Pushbutton port pins

#define S2 P3_0                     // push button input pin aliases
#define S1 P3_1

// DS1302 pin to port mapping
#if PROC_IS_15W408AS || PROC_IS_15W404AS
#define CE_HI   P5 |= 0b00010000;       // P5.4 DS1302 pin 5 set
#define CE_LO   P5 &= 0b11101111;       //                   unset
#define IO_LO   P5 &= 0b11011111;       // P5.1 DS1302 pin 6
#define IO_WR  P5|=((W_Byte&0x01)<<5)   // write I/O pin
#define IO_RD (P5 & 0b00100000)>>5      // read I/O pin
#define SCLK P3_2                       // P3.2 DS1302 pin 7
#elif PROC_IS_15F204EA
// not setup yet
#endif


// adc channels for sensors
#if HAS_LDR
#define ADC_LDR   7
#define SET_LDR_PORT  P1M1 |= (1 << ADC_LDR); P1M0 |= (1 << ADC_LDR);
#else
#define SET_LDR_PORT ;
#endif

#if HAS_THERMISTOR
#define ADC_TEMP  6
#define SET_THERMISTOR_PORT P1M1 |= (1 << ADC_TEMP); P1M0 |= (1 << ADC_TEMP);
#endif

// buzzer port pins and active state set

#define BZR_ON  P3_3 = 0            // direct port assigments
#define BZR_OFF P3_3 = 1            // with logic

#if COMMON_ANODE
#define SET_PORT_DRIVE ;
#define LED_SET_ANODES   P3 &= ~(0b00010000 << aPos)
#define LED_RESET_ANODES P3 |=   0b11110000
#define LED_SET_CATHODES P2 = CathodeBuf[aPos]  // seg A = P.0 assumed
#endif

#endif  // end board_yellow_5

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#if BOARD_WHITE_SMALL

#define DP1_IS_COLON TRUE

// Pushbutton port pins

#define S2 P3_0                     // push button input pin aliases
#define S1 P3_1

// DS1302 pin to port mapping

#define CE_HI P1_0 = 1;             // pin 5
#define CE_LO P1_0 = 0;
#define IO   P1_1                   // pin 6
#define SCLK P1_2                   // pin 7

// adc channels for sensors
#if HAS_LDR
#define ADC_LDR   6
#define SET_LDR_PORT  P1M1 |= (1 << ADC_LDR); P1M0 |= (1 << ADC_LDR);
#endif

#if HAS_THERMISTOR
#define ADC_TEMP  7
#define SET_THERMISTOR_PORT P1M1 |= (1 << ADC_TEMP); P1M0 |= (1 << ADC_TEMP);
#endif

// buzzer port pins and active state set

#define BZR_ON  P1_5 = 0            // direct port assigments
#define BZR_OFF P1_5 = 1            // with logic

#if COMMON_ANODE
#define SET_PORT_DRIVE P3M0 = 0x3C;
#define LED_SET_ANODES P3   |= (0b00000100 << aPos)
#define LED_RESET_ANODES P3 &=  0b11000011
#define LED_SET_CATHODES P2 = CathodeBuf[aPos]  // seg A = P.0 assumed
#elif  COMMON_CATHODE
#define SET_PORT_DRIVE P2M0 = 0xFF;
#define LED_SET_ANODES P3 &= ~(0b00000100 << aPos)
#define LED_RESET_ANODES P3 |=  0b00111100
#define LED_SET_CATHODES P2 = ~CathodeBuf[aPos]  // seg A = P.0 assumed
#endif

#endif  // endif board_white_small

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#if BOARD_YELLOW_SMALL

#define DP1_IS_COLON TRUE

// Pushbutton port pins

#define S2 P2_6                     // push button input pin aliases
#define S1 P2_7

// DS1302 pin to port mapping

#define CE_HI P1_1 = 1;             // pin 5
#define CE_LO P1_1 = 0;
#define IO   P1_2                   // pin 6
#define SCLK P1_0                   // pin 7

// adc channels for sensors
#if HAS_LDR
#define ADC_LDR   6
#define SET_LDR_PORT  P1M1 |= (1 << ADC_LDR); P1M0 |= (1 << ADC_LDR);
#endif

#if HAS_THERMISTOR
#define ADC_TEMP  7
#define SET_THERMISTOR_PORT P1M1 |= (1 << ADC_TEMP); P1M0 |= (1 << ADC_TEMP);
#endif

// buzzer port pins and active state set
// Port 0.1 is 5.5 on the 15W408AS!!!!
#define BZR_ON  P5 = 0x00         // direct port assigments
#define BZR_OFF P5 = 0x20         // with logic

#if COMMON_ANODE
#define SET_PORT_DRIVE P2M0 = 0x0F;
#define LED_SET_ANODES P2 |= (0b00001000 >> aPos)
#define LED_RESET_ANODES P2 &= 0b11110000
#define LED_SET_CATHODES P3 = CathodeBuf[aPos]  // seg A = P.0 assumed
#elif COMMON_CATHODE
#define SET_PORT_DRIVE P3M0 = 0xFF;
#define LED_SET_ANODES P2 &= ~(0b00001000 >> aPos)
#define LED_RESET_ANODES P2 |= 0b00001111
#define LED_SET_CATHODES P3 = ~CathodeBuf[aPos]  // seg A = P.0 assumed
#endif

#endif  // endif board_yellow_small

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#if BOARD_GREEN_SMALL

#define DP1_IS_COLON TRUE

// Pushbutton port pins

#define S2 P1_3                     // push button input pin aliases
#define S1 P1_4

// DS1302 pin to port mapping
// Port 0.1 is 5.5 on the 15W408AS!!!!

#if PROC_IS_15W408AS || PROC_IS_15W404AS
#define CE_HI   P5 |= 0b00100000;       // P5.5 DS1302 pin 5 set
#define CE_LO   P5 &= 0b11011111;       //                   unset
#define IO   P3_1                       // pin 6
#define SCLK P3_2                       // P3.2 DS1302 pin 7
#elif PROC_IS_15F204EA
#define CE   P1_0                   // pin 5
#define IO   P3_1                   // pin 6
#define SCLK P3_2                   // pin 7
#endif

// adc channels for sensors
#if HAS_LDR
#define ADC_LDR   4
#define SET_LDR_PORT  P1M1 |= (1 << ADC_LDR); P1M0 |= (1 << ADC_LDR);
#endif

#if HAS_THERMISTOR
#define ADC_TEMP  3
#define SET_THERMISTOR_PORT P1M1 |= (1 << ADC_TEMP); P1M0 |= (1 << ADC_TEMP);
#endif

// buzzer port pins and active state set

#define BZR_ON  P3_0 = 0         // direct port assigments
#define BZR_OFF P3_1 = 1         // with logic

#if COMMON_CATHODE
#define SET_PORT_DRIVE P2M0 = 0xFF;
#define LED_SET_ANODES   P3 &= ~(0b00001000 << aPos)
#define LED_RESET_ANODES P3 |=  0b01111000
#define LED_SET_CATHODES P2 = ~CathodeBuf[aPos]  // seg A = P.0 assumed
#endif

#endif  // endif board_green_small

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#endif
