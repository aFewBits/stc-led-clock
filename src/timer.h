
#ifndef _TIMER_H
#define _TIMER_H

#define K_HELD      0xE0
#define K_PRESSED   0xF0
#define K_RELEASED  0xFF

#define KEY_REPEAT 25;

void timer0_isr() __interrupt (1);
void debounceSwitches();
uint8_t state16to8(uint16_t);

void initTimer0(void);
void delay3(uint8_t ticks);

void stateSwitchWithS1(uint8_t);
void stateSwitchExtendedWithS1(uint8_t,uint8_t,__bit);
void stateSwitchWithS2(uint8_t);
void stateSwitchExtendedWithS2(uint8_t,uint8_t,__bit);

__bit checkAndClearS1();
__bit checkAndClearS2();
__bit checkAndClearS3();

__bit checkForReset();
__bit checkForRelease();
__bit getStateS2Flasher();

uint8_t getStateS1();
uint8_t getStateS2();

extern uint8_t displayState;

#endif
