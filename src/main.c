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
#if HAS_NY3P_SPEECH
#include "sound.h"
#endif
#if DEBUG
#include "serial.h"
#include "stdio.h"  // used for debug only
#endif

void checkAlarm();
void checkChime();
void soundChime();
void alarmDelay(uint8_t ticks);

uint8_t i;
__bit soundAlarm;

int main()
{
#if DEBUG
    initSerial();           // use P3.6 & 3.7 serial port for debug!
    printf_tiny("STC15W408AS awake\n");
#endif
    // setup 20ma source for direct drive clocks
    SET_PORT_DRIVE;
    // set photoresistor & ntc pins to open-drain output
    SET_LDR_PORT;
    SET_THERMISTOR_PORT;
    // done setup of ports
    blankDisplay();         // turn everything off for startup
    initRtc();              // setup DS1302 and read config ram
    initTimer0();           // start timers and display scan
    delay3(33);             // wait 100ms - else the reset doesn't work
    if (checkForReset()){
        initColdStart();    // reset clock when both switches down @ powerup
        while (checkForRelease()) ;
    }
#if TEST_DEFAULTS
    initColdStart();        // reset everything everytime for testing on powerup
#endif
    getClock();
#if HAS_NY3P_SPEECH         // first item spoken is one less than sent!!
    speakItem(sndOh);       // must say (anything/something) to clear the chip
#endif

// soundAlarm - Toggle the buzzer on and off at an anoying rate!
// The buzzer has its own native frequency when +5 volts is applied.
// The polarity is usually low to sound buzzer so the constants
// BZR_ON and BZR_OFF are usually inverted (ON = 0, OFF = 1)
//
// Routine waits until S1 is pressed to exit (and dumps keybuf)

    while(TRUE) {
#if OPT_CHIME
        //check for Chime
        if (ChimeOn)
                if (clockRam.min == 0)
                    if (clockRam.sec == 0)
                        checkChime();
#endif
#if OPT_ALARM
        // check for Alarm
        if (AlarmOn)
            if (clockRam.hr == clockRam.almHour)
                if (clockRam.min == clockRam.almMin)
                    if (clockRam.sec == 3){
                        displayState = stClock;
                        soundAlarm = TRUE;
                    }
#endif
        checkAlarm();               // sound alarm if needed
#if HAS_NY3P_SPEECH
    if ( displayState == stClock && pressedS3 && !soundAlarm ){
        pressedS3 = FALSE;
        speakTime();                // load buffer with sounds
    }
#endif
        // call state machine, everthing done from there
        displayFSM();
    }
}

// ###################################################################################
// End Main()
// ###################################################################################

// Alarm delay routine
// Call with number of 3ms ticks to wait
// Max is 3ms * 255 = 7.65 seconds delay

void alarmDelay(uint8_t ticks)
{
    userTimer3 = ticks;
    while (userTimer3)
        updateClock();
}

void checkAlarm()
{
#if OPT_ALARM
    while(soundAlarm){
        BZR_ON;
        alarmDelay(66);         // 200ms
        BZR_OFF;
        alarmDelay(33);         // 100ms
        BZR_ON;
        alarmDelay(66);         // 200ms
        BZR_OFF;
#if HAS_NY3P_SPEECH
        speakItem(sndRing);
        if (checkAndClearS3()){
#else
        alarmDelay(225);        // 675ms
        if (checkAndClearS1()){
#endif
            soundAlarm = FALSE;
            break;
        }
    }
#endif
}

// checkChime - convert current time and chime S/S to 24 hour format
// then setup proper start/stop order and compare to see if chime
// should sound

void checkChime()
{
    uint8_t tCurrent,tStart,tStop;
    __bit tFormat;
    tFormat = Select_12;
    changeTimeFormat(FALSE);    // convert to 24 hour (maybe)
    tCurrent = clockRam.hr;
    tStart = clockRam.chimeStartHour;
    tStop = clockRam.chimeStopHour;
    changeTimeFormat(tFormat);    // convert back to entry state
    if (tStop >= tStart) {
        if (tCurrent >= tStart && tCurrent <= tStop)
            soundChime();
    } else {
        if (tCurrent >= tStart || tCurrent <= tStop)
            soundChime();
    }
}

// soundChime - Toggle the buzzer on and off at alarm rate only once.
// The buzzer has its own native frequency when +5 volts is applied.
// The polarity is usually low to sound buzzer so the constants
// BZR_ON and BZR_OFF are usually inverted (ON = 0, OFF = 1)
// This routine must consume more than 1000ms else it will refire
// when it returns since time will still be 00:00

void soundChime()
{

#if OPT_CHIME && !HAS_NY3P_SPEECH
    BZR_ON;
    delay3(66);         // 200ms
    BZR_OFF;
    delay3(33);         // 100ms
    BZR_ON;
    delay3(66);         // 200ms
    BZR_OFF;
    delay3(250);        // 750ms for 1050ms total
#elif OPT_CHIME && HAS_NY3P_SPEECH
    speakItem(sndChime);        // updates display, returns when finished
#endif

}

