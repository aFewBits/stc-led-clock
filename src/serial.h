
#ifndef _SERIAL_H
#define _SERIAL_H

void Uart() __interrupt 4 __using 1;

void initSerial();
void sendString(char *s);
void sendData(uint8_t dat);
void putchar(uint8_t dat);

#endif