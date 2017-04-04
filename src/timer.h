
#ifndef _TIMER_H
#define _TIMER_H

enum key_states {
    K_HELD      = 0xE0,
    K_PRESSED   = 0xF0,
    K_RELEASED  = 0xFF,
    K_OTHER = 0
};

enum queue_states {
    Q_HELD      = 0xE000,
    Q_RELEASING = 0xE001,   // >= 0xE001 & <= 0xEFFF
    Q_PRESSED   = 0xF000,
    Q_PRESSING  = 0xF001,   // >= 0xF001 & <= 0xFFFE
    Q_RELEASED  = 0xFFFF,
    Q_ERROR = 0
};

#define KEY_REPEAT 25;

void timer0_isr() __interrupt (1) __using (1);
void initTimer0(void);
void delay3(uint8_t ticks);
void debounceSwitches();
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
uint8_t state16to8(uint16_t);

extern uint8_t displayState;

#endif
