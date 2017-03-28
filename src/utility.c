
#include <stdint.h>
#include "global.h"
#include "stc15.h"
#include "ds1302.h"
#include "timer.h"
#include "display.h"
#include "utility.h"

#if DEBUG
#include "serial.h"
#include "stdio.h"  // used for debug only
#endif

int16_t in;
int16_t in_min;
int16_t in_max;
int16_t out_min;
int16_t out_max;

uint8_t map()
{
    return (in-in_min) * (out_max-out_min) / (in_max-in_min) + out_min;
}

uint8_t mapLDR(int16_t x)
{
    in = x;
    in_min =  MAX_BRIGHT;
    in_max =  MIN_BRIGHT;
    out_min = bcdToDec(clockRam.brightMin);
    out_max = bcdToDec(clockRam.brightMax);
    return map();
}

uint8_t mapTemp(int16_t x)
{
    in = x;
    in_min = 754;     // actual A/D counts at 32F
    in_max = 281;     // actual A/D counts at 123F
    if (Select_FC){
        out_min = 32;
        out_max = 123;
    }
    else {
        out_min = 0;
        out_max = 51;
    }
    return map();
}

// Returns the decoded decimal value from a binary-coded decimal (BCD) byte.
// Assumes 'bcd' is coded with 4-bits per digit, with the tens place digit in
// the upper 4 MSBs.

uint8_t bcdToDec(uint8_t bcd)
{
  return (10 * ((bcd & 0xF0) >> 4) + (bcd & 0x0F));
}

// Returns the binary-coded decimal of 'dec'. Inverse of bcdToDec.

uint8_t decToBcd(uint8_t dec)
{
  uint8_t tens = dec / 10;
  uint8_t ones = dec % 10;
  return (tens << 4) | ones;
}

// Table to convert directly from 24 hour time to 12 hour
// time format with one instruction. The reverse requires one
// for loop to iterate the array to find the matching 12 hour
// time and the 24 time is the resulting index into the array.
//
// This only done becase a lookup table with 24 bytes is far more
// compact than two seperate routines with the needed if/else logic
// to convert back and forth between the formats when changing modes.

const uint8_t _24To12Hr[]={
    0x92,   //  0 = 12 am
    0x81,   //  1 =  1 am
    0x82,   //  2 =  2 am
    0x83,   //  3 =  3 am
    0x84,   //  4 =  4 am
    0x85,   //  5 =  5 am
    0x86,   //  6 =  6 am
    0x87,   //  7 =  7 am
    0x88,   //  8 =  8 am
    0x89,   //  9 =  9 am
    0x90,   // 10 = 10 am
    0x91,   // 11 = 11 am
    0xB2,   // 12 = 12 pm
    0xA1,   // 13 =  1 pm
    0xA2,   // 14 =  2 pm
    0xA3,   // 15 =  3 pm
    0xA4,   // 16 =  4 pm
    0xA5,   // 17 =  5 pm
    0xA6,   // 18 =  6 pm
    0xA7,   // 19 =  7 pm
    0xA8,   // 20 =  8 pm
    0xA9,   // 21 =  9 pm
    0xB0,   // 22 = 10 pm
    0xB1    // 23 = 11 pm
};

// convert 24 hour format to DS1302 12 hour format

uint8_t toFormat12(uint8_t h24)
{
    return _24To12Hr[ bcdToDec(h24) ];
}

// convert DS1302 12 hour format to 24 hour format

uint8_t toFormat24(uint8_t h12)
{
    uint8_t i;

    for (i = 0; i <= 23; i++){
        if (_24To12Hr[i] == h12)
            break;
    }
    return decToBcd(i);
}
// Conditionally change the format US/EU.
// If new != old, change. Else leave everything
// as-is.

void changeTimeFormat(__bit newFormat)
{
    if ( newFormat != Select_12){
        if (newFormat){
            Select_12 = TRUE;
            Select_MD = TRUE;
            Select_FC = TRUE;
            clockRam.hr             = toFormat12(clockRam.hr);
            clockRam.almHour        = toFormat12(clockRam.almHour);
            clockRam.chimeStartHour = toFormat12(clockRam.chimeStartHour);
            clockRam.chimeStopHour  = toFormat12(clockRam.chimeStopHour);
        }
        else {
            Select_12 = FALSE;
            Select_MD = FALSE;
            Select_FC = FALSE;
            clockRam.hr             = toFormat24(clockRam.hr);
            clockRam.almHour        = toFormat24(clockRam.almHour);
            clockRam.chimeStartHour = toFormat24(clockRam.chimeStartHour);
            clockRam.chimeStopHour  = toFormat24(clockRam.chimeStopHour);
        }
    }
}

// checkChime - convert current time and chime S/S to 24 hour format
// then setup proper start/stop order and compare to see if chime
// should sound

void checkChime()
{
    uint8_t tCurrent,tStart,tStop,t;
    __bit tFormat;

//    printf_tiny("%x %x %x\n",clockRam.hr,clockRam.min,clockRam.sec);


    tFormat = Select_12;
    changeTimeFormat(FALSE);    // convert to 24 hour (maybe)
    tCurrent = clockRam.hr;
    tStart = clockRam.chimeStartHour;
    tStop = clockRam.chimeStopHour;
    changeTimeFormat(tFormat);    // convert back to entry state

//    printf_tiny("%x %x %x\n",tCurrent,tStart,tStop);


    if (tStart > tStop){
        t = tStart;
        tStart = tStop;
        tStop = t;
    }
    if ( tCurrent >= tStart )
        soundChime();
    else if ( tCurrent <= tStop )
        soundChime();
}

// soundChime - Toggle the buzzer on and off at alarm rate only once.
// The buzzer has its own native frequency when +5 volts is applied.
// The polarity is usually low to sound buzzer so the constants
// BZR_ON and BZR_OFF are usually inverted (ON = 0, OFF = 1)
// This routine must consume more than 1000ms else it will refire
// when it returns since time will still be 00:00

void soundChime()
{
    BZR_ON;
    delay3(66);         // 200ms
    BZR_OFF;
    delay3(33);         // 100ms
    BZR_ON;
    delay3(66);         // 200ms
    BZR_OFF;
    delay3(250);        // 750ms for 1050ms total
}

