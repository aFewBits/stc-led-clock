
#ifndef _DISPLAY_H
#define _DISPLAY_H

enum stateTable {

// Regular display:
    stClock,stClockSeconds,stOptTemp,stOptDate,stOptDay,
// setup/config:
    scSet,scBeep,scDsp,scCfg,
// set:
    msClock,msClockHour,msClockMinute,
    msAlarm,msAlarmHour,msAlarmMinute,
    msChime,msChimeStartHour,msChimeStopHour,
    msDate,msDateMonth,msDateDay,
    msDay,msDayOfWeek,
// beep:
    msAlarmOff,msAlarmOn,           // the next 5 states:
    msChimeOff,msChimeOn,           // OFF must preceed ON state
// dsp:
    msDspTempOff,msDspTempOn,       // due to assumuptions in
    msDateOff,msDateOn,             // in state change code
    msDayOff,msDayOn,
// cfg:
    mcFormatTime,mc24,mc12,
    mcTempUnits,mcC,mcF,
    mcFormatDate,mc3112,mc1231,
    mcBrightness,mcBrtMax,mcBrtMin,
    mcTempCal,mcSetTemp,
// exit for all
    msExit
};

enum text2Entry {
    txClock,
    txAlarm,
    txChime,
    txDate,
    txDay,
    txTemp,
    tx12,
    tx24,
    txF,
    txC,
    NoText2
    };

enum text4Entry {
// set stuff
    txSet,
    txBeep,
    txDsp,
    txCfg,
// config stuff
    txBrt,
    txCal,
    txDate4,
    txTemp4,
    tx1224,
    tx1231,
    tx3112,
    };

enum led_position { H10,H01,M10,M01 };
enum decimal_status { OFF, ON };

void updateClock();
void displayFSM();
void setupHour(uint8_t);
void displayHours(__bit);
void displayMinutes(__bit);

void displayTemperature();
void displayDate();
void displayDayOfWeek();

void blankDisplay();
void displayUpdateISR();
void setChimeVars();
void getDateVars();
void putDateVars();
void setText2(uint8_t);
void setText2A(uint8_t);
void setText4(uint8_t);
void setMsgOn();
void setMsgOff();

uint8_t incrementHours(uint8_t);
uint8_t incrementMinutes(uint8_t);
uint8_t incrementDate(uint8_t,uint8_t);
uint8_t incrementDay(uint8_t);
uint8_t incrementBrightness(uint8_t);
uint8_t inc12(uint8_t);
uint8_t inc31(uint8_t);

#define segs CathodeBuf

// these two bits used to flash various things

extern __bit _1hzToggle;
extern __bit _5hzToggle;

extern volatile uint8_t userTimer3;         // decremented every 3ms by timer 0 task
extern volatile uint8_t userTimer100;       // decremented every 100ms by timer 0 task

// and access to the main data source

extern struct Clock clockRam;

// these are saved in DS1302 battery backed ram

extern __bit AlarmOn;     // alarm On/Off status
extern __bit ChimeOn;     // chime On/Off status
extern __bit TempOn;      // Temperature Display On/Off status
extern __bit DateOn;      // Date display On/Off status
extern __bit DowOn;       // Day of Week On/Off status
extern __bit Select_FC;   // select degrees F or C
extern __bit Select_MD;   // select month:day display format MM:DD or DD:MM
extern __bit Select_12;   // = 1 when 12 hr mode

#endif
