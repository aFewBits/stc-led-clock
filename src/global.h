
#ifndef _GLOBAL_H
#define _GLOBAL_H

#define TRUE    1                   // for general use, bit setters too
#define FALSE   0

#define FOSC 22118400L              // clock speed in mhz
#define T0TICKS 20000				// Timer 0 tick rate in hz (50us)
#define BAUD 115200                 // debug serial baud rate
#define RxD2 P3_6                   // UART only in 15W408AS
#define TxD2 P3_7                   // use for debugging
#define DEBUG FALSE                 // set true for serial debugging

#define TICKS_MS   20               // This is set by Timer 1 tick rate

#define BZR_ON  P1_5 = 0            // direct port assigments
#define BZR_OFF P1_5 = 1            // with negative logic

#define S2 P3_0                     // push button input pin aliases
#define S1 P3_1

// adc channels for sensors
#define ADC_LDR   6
#define ADC_TEMP  7

#endif
