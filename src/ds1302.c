
#include <stdint.h>
#include "stc15.h"
#include "global.h"
#include "timer.h"
#include "adc.h"
#include "ds1302.h"
#if DEBUG
#include "serial.h"
#include "stdio.h"  // used for debug only
#endif

struct Clock clockRam;

// Declare this byte absolute 0x2F which memory maps it in the 8051's bit space
// begining at 0x78. Then declare the bits used in that byte so they can be accessed
// without having to mask and shift...

uint8_t __at 0x2F configBitReg;
__bit __at 0x78 AlarmOn;    // alarm On/Off status
__bit __at 0x79 ChimeOn;    // chime On/Off status
__bit __at 0x7A TempOn;     // Temperature Display On/Off status
__bit __at 0x7B DateOn;     // Date display On/Off status
__bit __at 0x7C DowOn;      // Day of Week On/Off status
__bit __at 0x7D Select_FC;  // select degrees F or C
__bit __at 0x7E Select_MD;  // select month:day display format MM:DD or DD:MM
__bit __at 0x7F Select_12;  // = 1 when 12 hr mode

void wait500()
{
    // the call overhead + 4 nops = 500ns
    __asm__ ("\tnop\n\tnop\n\tnop\n");
}

// Reset and enable the 3-wire interface
// Exits with SCLK LOW and CE HIGH (selected)

void reset_3w()
{
    SCLK = 0;
    CE = 0;
    wait500();
    CE = 1;
}

// Write one byte to the DS1302
//
//  Exits with SCLK HIGH, IO in last state

void wbyte_3w(uint8_t W_Byte)
{
    uint8_t i;

    for(i = 0; i < 8; ++i){
        IO = 0;                 // do not remove!
        IO = (W_Byte & 0x01);
        wait500();
        SCLK = 0;
        wait500();
        SCLK = 1;               // write occurs on 0->1
        W_Byte >>= 1;
    }
}

// Read one byte from the DS1302
//
//  Exit with SCLK LOW

uint8_t	rbyte_3w()
{
    uint8_t i;
    uint8_t R_Byte;
    uint8_t TmpByte;

    R_Byte = 0x00;
    for(i = 0; i < 8; i++){
        SCLK = 1;               // read occurs on 1->0
        wait500();
        SCLK = 0;
        wait500();              // wait for I/O pin to settle
        TmpByte = (uint8_t)IO;  // get new bit
        TmpByte <<= 7;
        R_Byte >>= 1;
        R_Byte |= TmpByte;      // save parital byte
    }
    return R_Byte;
}

// Burst mode clock data registers from DS1302 and install in struct
// Process "Hours" and convert from 24hr format if in 12hr mode
// Return pointer to struct for use by the caller

void getClock()
{
    reset_3w();
    wbyte_3w(kClockBurstRead);
    clockRam.sec  = rbyte_3w();
    clockRam.min  = rbyte_3w();
    clockRam.hr   = rbyte_3w();
    clockRam.date = rbyte_3w();
    clockRam.mon  = rbyte_3w();
    clockRam.day  = rbyte_3w();
    clockRam.yr   = rbyte_3w();
    reset_3w();
}

// Burst mode ram struct to DS1302 clock data registers

void putClock()
{
    reset_3w();
    wbyte_3w(kClockBurstWrite);
    wbyte_3w(clockRam.sec);
    wbyte_3w(clockRam.min);
    wbyte_3w(clockRam.hr);
    wbyte_3w(clockRam.date);
    wbyte_3w(clockRam.mon);
    wbyte_3w(clockRam.day);
    wbyte_3w(clockRam.yr);
    wbyte_3w(0);                // must write 8 bytes in burst mode
    reset_3w();
}

// Refresh only time from RTC
// Used before puts that didn't affect time

void refreshTime()
{
    reset_3w();
    wbyte_3w(0x81);             // read seconds value
    clockRam.sec = rbyte_3w();
    reset_3w();
    wbyte_3w(0x83);             // minutes v
    clockRam.min = rbyte_3w();
    reset_3w();
    wbyte_3w(0x85);             // and hours
    clockRam.hr = rbyte_3w();
    reset_3w();
}

// Burst mode read DS1302 battery-backed ram into STC ram
// This is the user's clock configuration data

void getConfigRam()
{
    uint8_t i, *p;

    reset_3w();
    wbyte_3w(kRamBurstRead);
    p = &clockRam.check0;
    for (i = 0; i < configSize; i++){
        *p++ = rbyte_3w();
    }
    reset_3w();
    // set all 8 bits in one whack
    configBitReg = clockRam.statusBits;
}

// Burst mode write user ram back to the DS1302 ram
// This is the user's clock configuration data

void putConfigRam()
{
    uint8_t	i, *p;

    // push the user bits back into ram memory
    clockRam.statusBits = configBitReg;
    p = &clockRam.check0;
    reset_3w();
    wbyte_3w(kRamBurstWrite);
    for (i = 0; i < configSize; ++i){
        wbyte_3w(*p++);
    }
    reset_3w();
}

// --- Power up initization of the DS1302 RTC chip
// --- initialize time & date from user entries ---

void initRtc()
{
    uint8_t	t;

    reset_3w();
    wbyte_3w(0x8E);	    // control register
    wbyte_3w(0x00);		// disable write protect
    wbyte_3w(0x90);	    // trickle charger register
    wbyte_3w(0x00);	    // everything off!!
    wbyte_3w(0x81);     // read seconds value
    t = rbyte_3w();
    t &= 0x7f;          // mask off clock halt bit
    wbyte_3w(0x80);	    // and write it back to seconds reg
    wbyte_3w(t);        // less CH bit

    reset_3w();
    getConfigRam();
    t  = clockRam.check0;
    t ^= clockRam.check1;
    // if ram bad, initialize everything
    if (t != 0xff)
        initColdStart();
}

// The coldstart initialization table.
// Here you can set the reset at power up defaults to your liking.
// Be sure to declare times in the proper DS1302 12/24 bit format
// (that is for all hour values only) and remember that
// everthing is in BCD format for the DS1302.
//
// Declaring invalid values will usually result in just strange
// displays - but may cause crashes since the data is not validated.

#ifdef TEST_DEFAULTS
const uint8_t iniTable[] = {
        0x55,0x59,0xA7,                     // 07:59:55 pm
        0x12,0x31,                          // date,month
        0x01,                               // day of week
        0x16,                               // year (unused)
        0x55,0xAA,                          // checksum bytes
        kSelect_12+kSelect_MD+kSelect_FC+ \
        kAlarmOn+kChimeOn+kTempOn+kDowOn,   // mode bits
        0xA8,0x00,                          // 8:00pm alarm
        0x88,                               // 8:00am chime start
        0xA9,                               // 9:00pm chime stop
        0x63,0x62,                          // brightness max.min (0x63 max)
        0x00                                // temp offset
};
#else
const uint8_t iniTable[] = {
        0x55,0x59,0xA7,                     // 07:59:55 pm
        0x12,0x31,                          // date,month
        0x01,                               // day of week
        0x16,                               // year (unused)
        0x55,0xAA,                          // checksum bytes
        kSelect_12+kSelect_MD+kSelect_FC,   // mode bits
        0x88,0x00,                          // 8:00am alarm
        0x88,                               // 8:00am chime start
        0xA5,                               // 5:00pm chime stop
        0x50,0x01,                          // brightness max.min (0x63 max)
        0x00                                // temp offset
};
#endif

// Sample 24 hour setup:
//
//        0x55,0x59,0x19,                   // 19:59:55 pm
//        0x12,0x31,                        // date,month
//        0x01,                             // day of week
//        0x16,                             // year (unused)
//        0x55,0xAA,                        // checksum bytes
//        0x00,                             //
//        0x08,0x00,                        // 08:00 alarm
//        0x08,                             // 08:00 chime start
//        0x17,                             // 17:00 chime stop
//        0x50,0x05,                        // brightness max.min (0x63 max)
//        0x00                              // temp offset

void initColdStart()
{
    uint8_t	i, *p;

    p = &clockRam.sec;
    for (i = 0; i < clockSize+configSize; i++){
        *p++ = iniTable[i];
    }
    putClock();
    configBitReg = clockRam.statusBits;
    putConfigRam();
}

