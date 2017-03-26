
#include <stdint.h>
#include "global.h"
#include "stc15.h"
#include "serial.h"

#if DEBUG

#define S1_S0 0x40              //P_SW1.6
#define S1_S1 0x80              //P_SW1.7

volatile __bit txBusy;

void initSerial()
{
    ACC = P_SW1;
    ACC &= ~(S1_S0 | S1_S1);        // S1_S0=1 S1_S1=0
    ACC |= S1_S0;                   // P3.6/RxD_2, P3.7/TxD_2
    P_SW1 = ACC;
    SCON  = 0x40;                	// xmit only & 8 bit variable baud rate uart
    T2H   = (65536-(FOSC/BAUD/4) >> 8);		// baud rate high
    T2L   = (65536-(FOSC/BAUD/4) & 0xFF);	// baud rate low
    ACC = AUXR;
    ACC |= 0x15;                    // T2 is 1T mode, start timer 2, T2 = uart clock
    AUXR = ACC;                     // Select Timer 2 as the baud rate generator for serial port 1
    ES = 1;                         // Enable serial port 1 interrupt
    PS = 1;                         // set serial int priority
    EA = 1;                         // insure interrupts are on
}

// UART Interrupt service routine

void Uart() __interrupt 4 __using 1
{
    if (RI){
        RI = 0;                 // Clear the RI bit
    }
    if (TI){
        TI = 0;                 // Clear the TI bit
        txBusy = 0;             // Busy sign
    }
}

// Send serial data

void putchar(uint8_t dat)
{
    while (txBusy)
    __asm__ ("\tnop\n");   // Waiting for the previous data to be sent
    txBusy = 1;
    SBUF = dat;                 // Write data to the UART data register
}

// Send a string

void sendString(char *s)
{
    while (*s)
    {                  // Detect the string end flag
        putchar(*s++);          // Send the current character
    }
}

#endif
