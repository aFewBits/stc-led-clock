
#include <stdint.h>
#include "global.h"
#include "stc15.h"
#include "ds1302.h"
#include "adc.h"
#include "timer.h"
#include "display.h"
#if DEBUG
#include "serial.h"
#include "stdio.h"  // used for debug only
#endif

volatile uint8_t    _3msTimer;          // in timer 0 ticks (max 12.8ms at 50us)
volatile uint8_t    _10msTimer;         // rolls over every 10ms
volatile uint8_t    _100msTimer;        // increments, blinks display at 5hz
volatile uint8_t    _500msTimer;        // increments, blinks ":" seperator at 1hz
volatile uint8_t    userTimer100;       // user delay in 100ms ticks (25.5 seconds max)
volatile uint8_t    userTimer3;         // user delay in 3ms ticks 765ms max (3*255)

volatile uint8_t    stateQueueS1;       // holds 5 key states
volatile __bit      pressedS1;          // true when pressed, user clears

volatile uint8_t    stateQueueS2;       // pressed, released, etc.
volatile __bit      pressedS2;          // true when pressed, user clears

volatile uint8_t    keyRepeatTimer;     // sets repeat time (in 10ms ticks)

#if HAS_NY3P_SPEECH
volatile uint8_t    stateQueueS3;       // holds 5 key states
volatile __bit      pressedS3;          // true when pressed, user clears
volatile uint8_t    soundTimer;         // typically set to 10 ticks (500us)
#endif

__bit _1hzToggle;   // 500 ms pulse width (blinks : in display)
__bit _5hzToggle;   // 100 ms used to blink digits being set

void initTimer0(void)       // 50us @ 22.1184mhz
{
    AUXR |= 0x80;   // set to clock div 1 without trashing other bits
    TH0 = (65536-(FOSC/T0TICKS)) >> 8;
    TL0 = (65536-(FOSC/T0TICKS)) & 0xFF;
    TF0 = 0;        // Clear TF0 flag
    TR0 = 1;        // Timer0 start run
    ET0 = 1;        // enable timer0 interrupt
    EA  = 1;        // global interrupt enable
}

void timer0_isr() __interrupt (1)
{
    // Here on every timer roll-over (50us).

    // Tell display code a timer tick has occurred.
    // Response is:
    // Just increment counter and return if during anode ON time
    // If time on period has expired, turn off anode driver and contiune counting
    // unitl at end of cycle (MaxOnTime)

    displayUpdateISR();

    // handle various time based tasks
    // User key press handler, call every 3ms

    if (!_3msTimer--){
        _3msTimer = 3 * TICKS_MS;
        if (userTimer3) userTimer3--;
    }
    if (!_10msTimer--) {
        _10msTimer = 10 * TICKS_MS;
        debounceSwitches();
        if (keyRepeatTimer) keyRepeatTimer--;
        if (!_100msTimer--) {
            _100msTimer = 10;
            _5hzToggle =! _5hzToggle;
            if (userTimer100) userTimer100--;
        }
        if (!_500msTimer--) {
            _500msTimer = 50;
            _1hzToggle =! _1hzToggle;
        }
    }
  #if HAS_NY3P_SPEECH
    if (soundTimer) soundTimer--;
  #endif

}

// User delay routine
// Call with number of 10ms ticks to wait
// Max is 10ms * 255 = 2.55 seconds delay

void delay3(uint8_t ticks)
{
    userTimer3 = ticks;
    while (userTimer3)
    ;
}

void debounceSwitches(void)
{
    // Called from Timer ISR code.
    // Update pushbutton state tables every 10ms.
    // Uses negative logic (pressed = 0)
    // State consists of 5 past events and when key is true for 50ms (5*10)
    // the current state will = F0.

    // sw1
    stateQueueS1 = stateQueueS1<<1 | S1 | K_HELD;
    if (stateQueueS1 == K_PRESSED) pressedS1 = TRUE;

    // sw2
    stateQueueS2 = stateQueueS2<<1 | S2 | K_HELD;
    if ((stateQueueS2 == K_HELD) & (!keyRepeatTimer)){
        keyRepeatTimer = KEY_REPEAT;        // 150ms (1 tick = 10ms)
        pressedS2 = TRUE;
    }

#if HAS_NY3P_SPEECH
    // sw3
    stateQueueS3 = stateQueueS3<<1 | S3 | K_HELD;
    if (stateQueueS3 == K_PRESSED) pressedS3 = TRUE;
#endif
}

// Used at power up to detect user reset
// S1 and S2 must be held down to effect reset

__bit checkForReset()
{

    if ((stateQueueS1 == K_HELD) & (stateQueueS2 == K_HELD))
        return TRUE;
    else
        return FALSE;
}

// Used at power up to detect key release after reset

__bit checkForRelease()
{
    pressedS1 = FALSE;
    pressedS2 = FALSE;
    if ((stateQueueS1 == K_RELEASED) & (stateQueueS2 == K_RELEASED))
        return FALSE;   // use neg logic to avoid negation of value
    else
        return TRUE;    // got it?
}


// return the current queue back to caller
// used for detecting S2 auto-repeat to
// prevent constant flashing of display while changing

__bit getStateS2Flasher()
{
    if ( stateQueueS2 == K_HELD )
        return TRUE;
    else
        return _5hzToggle;
}

// Check if S1 pressed. Return TRUE and clear key events if so.
// Return FALSE if no key pressed.

__bit checkAndClearS1()
{
    if (pressedS1){
        pressedS1 = FALSE;
        return TRUE;
    }
    else
        return FALSE;
}

// Check if S2 pressed. Return TRUE and dump key events if so.
// Return FALSE if no key pressed.

__bit checkAndClearS2()
{
    if (pressedS2){
        pressedS2 = FALSE;
        return TRUE;
    }
    else
        return FALSE;
}

// Check if S3 pressed. Return TRUE and dump key events if so.
// Return FALSE if no key pressed.

#ifdef S3

__bit checkAndClearS3()
{
    if (pressedS3){
        pressedS3 = FALSE;
        return TRUE;
    }
    else
        return FALSE;
}

#endif

// Check if S1 pressed. Set new state if true.

void stateSwitchWithS1(uint8_t newState)
{
    if (pressedS1){
        pressedS1 = FALSE;
        displayState = newState;
    }
}

void stateSwitchExtendedWithS1(uint8_t newState, uint8_t text2, __bit flag)
{
    if (pressedS1){
        pressedS1 = FALSE;
        displayState = newState;
        // if the flag is off, goto OFF state (-1)
        if (!flag)
            displayState--;
        if ( text2 != NoText2 )
            setHourDigits(text2);
    }
}

// Check if S2 pressed. Set new state if true.

void stateSwitchWithS2(uint8_t newState)
{
    if (pressedS2){
        pressedS2 = FALSE;
        displayState = newState;
    }
}

void stateSwitchExtendedWithS2(uint8_t newState, uint8_t text2, __bit flag)
{
    if (pressedS2){
        pressedS2 = FALSE;
        displayState = newState;
        // if the flag is off, goto OFF state (-1)
        if (!flag)
            displayState--;
        if ( text2 != NoText2 )
            setHourDigits(text2);
    }
}
