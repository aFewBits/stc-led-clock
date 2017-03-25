
#ifndef _DS1302_H
#define _DS1302_H

typedef enum {
    kSunday    = 1,
    kMonday    = 2,
    kTuesday   = 3,
    kWednesday = 4,
    kThursday  = 5,
    kFriday    = 6,
    kSaturday  = 7
} Day;

enum Register {
  kSecondReg       = 0,
  kMinuteReg       = 1,
  kHourReg         = 2,
  kDateReg         = 3,
  kMonthReg        = 4,
  kDayReg          = 5,
  kYearReg         = 6,
  kWriteProtectReg = 7,
};

enum Command {
  kClockBurstWrite = 0xBE,
  kClockBurstRead  = 0xBF,
  kRamBurstWrite   = 0xFE,
  kRamBurstRead    = 0xFF
};

// Time structure as used by DS1302
// This values are all BCD!

struct Clock {
    uint8_t sec;            // seconds. Range: {0...59}
    uint8_t min;            // minutes. Range: {0...59}
    uint8_t hr;             // hours.   Range: {0...23} or {1...12}
    uint8_t mon;            // month.   Range: {1...12}
    uint8_t date;           // dom.     Range: {1...31}
    uint8_t day;            // dow      Range: {1...7}  Sunday = 1;
    uint8_t yr;             // year.    Range: {00...99}
    uint8_t check0;         // 0xAA     check0 xor check1 = 0xFF
    uint8_t check1;         // 0x55
    uint8_t statusBits;     // Pack all into one byte (no bool's allowed)
    uint8_t almHour;        // {0...23} or {1...12} in bcd
    uint8_t almMin;         // {0...59} in bcd
    uint8_t chimeStartHour; // {0...23} or {1...12} in bcd
    uint8_t chimeStopHour;  // {0...23} or {1...12} in bcd
    uint8_t brightMax;      // {1...50} in bcd
    uint8_t brightMin;      // {1...50} in bcd
    int8_t  tempOffset;     // constrain range to +/- 15
};

#define clockSize   7
#define configSize 10
#define totalSize clockSize + configSize

// bit definitions used in status bits
//
#define kAlarmOn     0x01        // alarm On/Off status
#define kChimeOn     0x02        // chime On/Off status
#define kTempOn      0x04        // Temperature Display On/Off status
#define kDateOn      0x08        // Date display On/Off status
#define kDowOn       0x10        // Day of Week On/Off status
#define kSelect_FC   0x20        // select degrees F or C
#define kSelect_MD   0x40        // select month:day display format MM:DD or DD:MM
#define kSelect_12   0x80        // = 1 when 12 hr mode

// External module usage:
void    initRtc();
void    initColdStart();
void    getClock();
void    putClock();

// Internal module use only
void	getConfigRam();
void	putConfigRam();

#endif
