
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

#define TICKS_MS   20               // This is set by Timer 1 tick rate

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

#define LED_SET_MASK P3 &= ~(0b00000100 << aPos)
#define LED_RESET_MASK P3 |= 0b00111100
#define LED_CATHODE_PORT P2         // seg A = P.0 assumed

#endif
