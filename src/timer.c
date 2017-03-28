
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

volatile uint8_t    _10msTimer;         // rolls over every 10ms
volatile uint8_t    _100msTimer;        // increments, blinks display at 5hz
volatile uint8_t    _500msTimer;        // increments, blinks ":" seperator at 1hz
volatile uint8_t    userTimer100;       // user delay in 100ms ticks (25.5 seconds max)
volatile uint8_t    userTimer3;         // user delay in 3ms ticks 765ms max (3*255)

volatile uint16_t   stateQueueS1;      // holds 16 key states
volatile uint8_t    stateQS1;          // 8 bit state value (for detecting state changes)
volatile __bit      pressedS1;         // true when pressed, user clears

volatile uint16_t   stateQueueS2;      // pressed, released, etc.
volatile uint8_t    stateQS2;          // 8 bit state value (for detecting state changes)
volatile uint8_t    stateLS2;          // 8 bit last state
volatile __bit      pressedS2;         // true when pressed, user clears

volatile uint8_t    keyRepeatTimer;    // sets repeat time (in 10ms ticks)
volatile uint8_t    keyDebounceTimer;  // in timer 0 ticks (max 12.8ms at 50us)

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

void timer0_isr() __interrupt 1
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

    if (!keyDebounceTimer--){
        keyDebounceTimer = 3 * TICKS_MS;
        debounceSwitches();
        if (userTimer3) userTimer3--;
    }
    if (!_10msTimer--) {
        _10msTimer = 10 * TICKS_MS;
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
    // Update pushbutton state tables every 3ms.
    // Uses negative logic (pressed = 0)
    // State consists of 16 past events and when key is true for 39ms (13*3)
    // the current state will = F000.

    // sw1
    stateQueueS1 = stateQueueS1<<1 | S1 | Q_HELD;
    stateQS1 = state16to8(stateQueueS1);
    if (stateQS1 == K_PRESSED) pressedS1 = TRUE;

    // sw2
    stateQueueS2 = stateQueueS2<<1 | S2 | Q_HELD;
    stateQS2 = state16to8(stateQueueS2);
    if (((stateQS2 == K_HELD)&(stateLS2 == K_HELD)&(!keyRepeatTimer))|(stateQS2 == K_PRESSED)){
        // key down or key repeat occured
        // in case of repeat, reset state to reload timer
        stateQS2 = K_PRESSED;
        pressedS2 = TRUE;
    }
    if ((stateQS2 == K_HELD) & (stateLS2 != K_HELD))
        // restart timer when held down last pass and this one
        keyRepeatTimer = KEY_REPEAT;        // 200ms (1 tick = 10ms)
    // save last state for timer logic
    stateLS2 = stateQS2;
}

// Used at power up to detect user reset
// S1 and S2 must be held down to effect reset

__bit checkForReset()
{

    if ((stateQS1 == K_HELD) & (stateQS2 == K_HELD))
        return TRUE;
    else
        return FALSE;
}

// Used at power up to detect key release after reset

__bit checkForRelease()
{
    pressedS1 = FALSE;
    pressedS2 = FALSE;
    if ((stateQS1 == K_RELEASED) & (stateQS2 == K_RELEASED))
        return FALSE;   // use neg logic to avoid negation of value
    else
        return TRUE;    // got it?
}


// state16to8 - return 8bit state value from the 16bit state table
// This makes state comparisions faster and smaller
//
uint8_t state16to8(uint16_t queue16)
{
    uint8_t que8,rv;

    que8 = queue16 >> 8;
    if ( que8 == 0xE0 )
        rv = K_HELD;
    else if ( que8 == 0xF0 )
        rv = K_PRESSED;
    else if ( que8 == 0xFF )
        rv = K_RELEASED;
    else
        rv = K_OTHER;
    return rv;
}

uint8_t getStateS1()
{
    return state16to8(stateQueueS1);
}

uint8_t getStateS2()
{
    return state16to8(stateQueueS2);
}

// return the current queue back to caller
// used for detecting S2 auto-repeat to
// prevent constant flashing of display while changing

__bit getStateS2Flasher()
{
    if ( state16to8(stateQueueS2) == K_HELD )
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
            setText2(text2);
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
            setText2(text2);
    }
}
