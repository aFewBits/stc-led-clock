//
// STC15W408AS LED Clock
//
#include <stdint.h>
#include "stc15.h"
#include "global.h"
#include "timer.h"
#include "ds1302.h"
#include "adc.h"
#include "display.h"
#include "utility.h"
#if DEBUG
#include "serial.h"
#include "stdio.h"  // used for debug only
#endif

// Alarm delay routine
// Call with number of 3ms ticks to wait
// Max is 3ms * 255 = 7.65 seconds delay

void alarmDelay(uint8_t ticks)
{
    userTimer3 = ticks;
    while (userTimer3)
        updateClock();
}

uint8_t i;
__bit soundAlarm;

int main()
{
#if DEBUG
    initSerial();           // use P3.6 & 3.7 serial port for debug!
    printf_tiny("STC15W408AS awake\n");
#endif

    SET_PORT_DRIVE;
    // set photoresistor & ntc pins to open-drain output
    P1M1 |= (1 << ADC_LDR) | (1 << ADC_TEMP);
    P1M0 |= (1 << ADC_LDR) | (1 << ADC_TEMP);
    //
    blankDisplay();         // turn everything off for startup
    initRtc();              // setup DS1302 and read config ram
    initTimer0();           // start timers and display scan
    delay3(33);             // wait 100ms - else the reset doesn't work
    if (checkForReset()){
        initColdStart();    // reset clock when both switches down @ powerup
        while (checkForRelease()) ;
    }
    getClock();

// soundAlarm - Toggle the buzzer on and off at an anoying rate!
// The buzzer has its own native frequency when +5 volts is applied.
// The polarity is usually low to sound buzzer so the constants
// BZR_ON and BZR_OFF are usually inverted (ON = 0, OFF = 1)
//
// Routine waits until S1 is pressed to exit (and dumps keybuf)

    while(TRUE) {
        //check for Chime
        if (ChimeOn)
                if (clockRam.min == 0)
                    if (clockRam.sec == 0)
                        checkChime();
        // check for Alarm
        if (AlarmOn)
            if (clockRam.hr == clockRam.almHour)
                if (clockRam.min == clockRam.almMin)
                    if (clockRam.sec == 1){
                        displayState = stClock;
                        soundAlarm = TRUE;
                    }
        while(soundAlarm){
            BZR_ON;
            alarmDelay(66);         // 200ms
            BZR_OFF;
            alarmDelay(33);         // 100ms
            BZR_ON;
            alarmDelay(66);         // 200ms
            BZR_OFF;
            alarmDelay(225);        // 675ms
            if (checkAndClearS1()){
                soundAlarm = FALSE;
                break;
            }
        }
        // call state machine, everthing done from there
        displayFSM();
    }
}
