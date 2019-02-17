
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

// Insert static tables into the code segement here.
// Doing this allows this modules statements to contain
// references to this tables without having to prototype the
// variables beforehand. This gets messy since prototyping a
// an array variable before later redeclaring it with a
// static initialization it just doesn't work.
// This is all done to save code space.
// Ugly but seemingly necessary.

#include "codeTables.c"

// These are global to this module.
// Not a great practice but when you've only got
// 80 bytes of directly addressable ram...
// Most are used in several places at different times.

uint8_t h,m,d;                  // (h)ours and (m)inutes or (m)onth and (d)ay
uint8_t displayState;           // the main control variable

__bit   dp0,dp1;                // decimal points for each digit
__bit   dp2,dp3;                // numbers match Cathode[] array index
__bit   AM_PM;                  // used to set decimal point on/off
__bit   readyRead;              // used to read temperature once per second
__bit   timeChanged;            // write time to clock when set
#if OPT_BLANK_ZERO
__bit   blankZero;              // set true to suppress leading zero in clock
#endif

uint8_t maskOnOff;              // set to 0x7F or 0xFF to blink decimal point
uint8_t maskDp0;                // set to 0x7F or 0xFF with dp0 (=dp on or off)
uint8_t maskDp1;                // set to 0x7F or 0xFF with dp1 (=dp on or off)
uint8_t maskDp2;                // set to 0x7F or 0xFF with dp2 (=dp on or off)
uint8_t maskDp3;                // set to 0x7F or 0xFF with dp3 (=dp on or off)

uint8_t brightLevel;            // result of LDR and config settings
uint8_t actualTemp;             // result of ADC and transform

// declared static - in routine that is called from from an ISR

uint8_t CathodeBuf[4];          // holds each digits segments (0 = on)
volatile uint8_t aPos;                // cycles through 0-3 anode positions
volatile uint8_t aOnTicks;
volatile uint8_t aOffTicks;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Update the LED display based on four variables.
//
// These are:
//
//      CathodeBuf[i] = 7 segment pattern to display, i = AnodePos
//      aPos          = Current digit, 0.1.2.3 where 0 = LHS of display
//      aOnTicks      = Number of Timer 0 ticks for the Anode to be low (active /on)
//      aOffTicks     = Number of Timer 0 ticks for the Anode to be high (inactive/off)
//
// This routine is called from inside the ISR so be quick!!
//

void displayUpdateISR(void){

    if(aOnTicks){
        LED_SET_ANODES;             // turn on selected anode
        LED_SET_CATHODES;           // and output segment values
        aOnTicks--;
    }
    else if(aOffTicks){
        LED_RESET_ANODES;           // all four bits back to 1
        aOffTicks--;
    }
    else {
        if (++aPos >= 4)            // Done with this digit so move to next
            aPos = 0;               // and reset timers so next time in turn on new anode
            aOnTicks = brightLevel;
            if (aOnTicks >= MAX_BRIGHT)
               aOffTicks = 1;      // don't allow Off to goto zero!!
            else
               aOffTicks = MAX_BRIGHT - aOnTicks;
        }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// updateClock routine used by alarm. Must update the display while
// in alarm mode, beeping and waiting for the user to reset.
// Maybe need to add snooze too...

void updateClock()
{
    dp0 = OFF;
#if DP1_IS_COLON
    dp1 = _1hzToggle;           // toggle clock ':'
    dp2 = OFF;
#else
    dp1 = _1hzToggle;           // toggle clock ':'
    dp2 = _1hzToggle;           // two dp's for 1" LED's
#endif
    dp3 = AM_PM;
    getClock();
    setupHour(clockRam.hr);     // possibly convert to 12 format with AM/PM
    m = clockRam.min;
#if OPT_BLANK_ZERO
 blankZero = TRUE;
    displayHoursOn();
 blankZero = FALSE;
#else
    displayHoursOn();
#endif
    displayMinutesOn();
}

//----------------------------------------------------------------------------------------------//
//-------   The monster state machine. All mode changing and display is done from here     -----//
//-------  Rather than adding the code overhead of subroutines, all code is inline in this -----//
//-------  area. Big and clunky, yes. Code efficent, more so. If there is code space left  -----//
//-------  over when eveything else is complete, I'll go back and rework the monster.      -----//
//----------------------------------------------------------------------------------------------//

void displayFSM()
{
  #if HAS_LDR
    brightLevel = mapLDR(getADCResult8(ADC_LDR) >> 2);
  #else
    brightLevel = mapLDR(MIN_BRIGHT);   // not max since anlog is inverted
  #endif

  #if HAS_LDR
    if ( readyRead & _1hzToggle ){
        actualTemp = mapTemp(getADCResult(ADC_TEMP));
        readyRead = FALSE;
    }
    else if (!_1hzToggle)
        readyRead = TRUE;
  #endif

    switch (displayState) {

// ###################################################################################
// Home menu, Regular display items
// ###################################################################################

// stClock,stOptTemp,stOptDate,stOptDay

    case stClock:
        // break update away from switch check for alarming...
        updateClock();
        stateSwitchWithS1(scSet);
        if (checkAndClearS2() ){
            userTimer100 = 200;
            displayState = stClockSeconds;
        }
        if (clockRam.sec == 0x30){
            userTimer100 = 30;
    // do not change the order of these three if statements...
    #if OPT_DAY_DSP
            if( DowOn )
                displayState = stOptDay;
    #endif
    #if OPT_DATE_DSP
            if( DateOn )
                displayState = stOptDate;
    #endif
    #if OPT_TEMP_DSP
            if( TempOn )
                displayState = stOptTemp;
    #endif
        }
        break;

    case stClockSeconds:
        dp0 = OFF;
        dp1 = _1hzToggle;
        dp2 = OFF;
        dp3 = OFF;
        getClock();
        h = clockRam.min;
        m = clockRam.sec;
        displayHoursOn();
        displayMinutesOn();
        stateSwitchWithS1(stClock);
        if (checkAndClearS2()){
            clockRam.sec = 0;       // reset seconds on S2 press
            putClock();             // save change
            userTimer100 = 200;     // and refresh time to keep us here
        }
        // go back to clock on time out
        if ( !userTimer100 ) displayState = stClock;
        break;

#if OPT_TEMP_DSP
    case stOptTemp:
        displayTemperature();
        stateSwitchWithS1(scSet);
        checkAndClearS2();
        if (!userTimer100){
            userTimer100 = 30;
    #if OPT_DAY_DSP
            if( DowOn )
                displayState = stOptDay;
    #endif
    #if OPT_DATE_DSP
            if( DateOn )
                displayState = stOptDate;
    #endif
            if ( !DateOn && !DowOn )
                displayState = stClock;
        }
        break;
#endif

#if OPT_DATE_DSP
    case stOptDate:
        displayDate();
        stateSwitchWithS1(scSet);
        checkAndClearS2();
        if (!userTimer100){
            userTimer100 = 30;
    #if OPT_DAY_DSP
            if( DowOn )
                displayState = stOptDay;
            else
                displayState = stClock;
    #else
           displayState = stClock;
    #endif
        }
        break;
#endif

#if OPT_DAY_DSP
    case stOptDay:
        displayDayOfWeek();
        if (!userTimer100)
            displayState = stClock;
        stateSwitchWithS1(scSet);
        checkAndClearS2();
        break;
#endif

// ###################################################################################
// Setup / Config
// ###################################################################################

// scSet,scBeep,scDsp,scCfg

    case scSet:
        setAllDigits(txSet);
        stateSwitchWithS1(scBeep);
        stateSwitchWithS2(msClock);
        break;

    case scBeep:
        setAllDigits(txBeep);
        stateSwitchWithS1(scDsp);
        stateSwitchExtendedWithS2(msAlarmOn,txAlarm,AlarmOn);
        break;

#if OPT_TEMP_DSP | OPT_DATE_DSP | OPT_DAY_DSP
    case scDsp:
        setAllDigits(txDsp);
        stateSwitchWithS1(scCfg);
  #if OPT_TEMP_DSP
        stateSwitchExtendedWithS2(msDspTempOn,txTemp,TempOn);
  #else
        stateSwitchExtendedWithS2(msDateOn,txDate,DateOn);
  #endif


        break;
#endif

    case scCfg:
        setAllDigits(txCfg);
        stateSwitchWithS1(msExit);
#if OPT_UNITS_GROUP
        stateSwitchWithS2(msSetUnits);
#else
        stateSwitchWithS2(msFormatTime);
#endif

        break;

// ###################################################################################
// Set Menu
// ###################################################################################

// msClock,msClockHour,msClockMinute

    case msClock:
        setHourDigits(txClock);
        stateSwitchWithS1(msAlarm);
        stateSwitchWithS2(msClockHour);
        break;

    case msClockHour:
        #if DP1_IS_COLON
            dp1 = ON;
            dp2 = OFF;
        #else
            dp1 = ON;
            dp2 = ON;
        #endif
        dp3 = AM_PM;
        displayHoursFlash();
        displayMinutesOn();
        stateSwitchWithS1(msClockMinute);
        if (checkAndClearS2()){
            clockRam.hr = incrementHours(clockRam.hr);
            setupHour(clockRam.hr);
            timeChanged = TRUE;
        }
        break;

    case msClockMinute:
        displayHoursOn();
        displayMinutesFlash();
        stateSwitchWithS1(msAlarm);
        if (checkAndClearS2()){
            m = incrementMinutes(m);
            clockRam.min = m;
            timeChanged = TRUE;
        }
        break;

// msAlarm,msAlarmHour,msAlarmMinute

    case msAlarm:
        setHourDigits(txAlarm);
        stateSwitchWithS1(msChime);
        stateSwitchWithS2(msAlarmHour);
        break;

    case msAlarmHour:
        #if DP1_IS_COLON
            dp1 = ON;
            dp2 = OFF;
        #else
            dp1 = ON;
            dp2 = ON;
        #endif
        dp3 = AM_PM;
        setupHour(clockRam.almHour);
        m = clockRam.almMin;
        displayHoursFlash();
        displayMinutesOn();
        stateSwitchWithS1(msAlarmMinute);
        if (checkAndClearS2())
            clockRam.almHour = incrementHours(clockRam.almHour);
        break;

    case msAlarmMinute:
        displayHoursOn();
        displayMinutesFlash();
        stateSwitchWithS1(msChime);
        if (checkAndClearS2()){
            m = incrementMinutes(m);
            clockRam.almMin = m;
            }
        break;

// msChime,msChimeStartHour,msChimeStopHour

    case msChime:
        setHourDigits(txChime);
#if OPT_DATE_DSP
        stateSwitchWithS1(msDate);
#elif OPT_DAY_DSP
        stateSwitchWithS1(msDay);
#else
        stateSwitchWithS1(msExit);
#endif
        stateSwitchWithS2(msChimeStartHour);
        break;

    case msChimeStartHour:
        setChimeVars();
        displayHoursFlash();
        displayMinutesOn();
        stateSwitchWithS1(msChimeStopHour);
        if (checkAndClearS2())
            clockRam.chimeStartHour = incrementHours(clockRam.chimeStartHour);
        break;

    case msChimeStopHour:
        setChimeVars();
        displayHoursOn();
        displayMinutesFlash();
#if OPT_DATE_DSP
        stateSwitchWithS1(msDate);
#elif OPT_DAY_DSP
        stateSwitchWithS1(msDay);
#else
        stateSwitchWithS1(msExit);
#endif
        if (checkAndClearS2())
            clockRam.chimeStopHour = incrementHours(clockRam.chimeStopHour);
        break;

// msDate,msDateMonth,msDateDay,

#if OPT_DATE_DSP
    case msDate:
        dp1 = ON;
        dp2 = OFF;
        dp3 = OFF;
        setHourDigits(txDate);
  #if OPT_DAY_DSP
        stateSwitchWithS1(msDay);
  #else
        stateSwitchWithS1(msExit);
  #endif
        stateSwitchWithS2(msDateMonth);
        break;

    case msDateMonth:
        getDateVars();
        displayHoursFlash();
        displayMinutesOn();
        stateSwitchWithS1(msDateDay);
        stateSwitchWithS1(msExit);
        if (checkAndClearS2())
            h = incrementDate(h,H10);
        break;

    case msDateDay:
        getDateVars();
        displayHoursOn();
        displayMinutesFlash();
  #if OPT_DAY_DSP
        stateSwitchWithS1(msDay);
  #else
        stateSwitchWithS1(msExit);
  #endif
        if (checkAndClearS2())
            m = incrementDate(m,M10);
        break;
#endif

// msDay,msDayOfWeek

#if OPT_DAY_DSP
    case msDay:
        setHourDigits(txDay);
        stateSwitchWithS1(msExit);
        stateSwitchWithS2(msDayOfWeek);
        break;

    case msDayOfWeek:
        setDayOfWeek();
        stateSwitchWithS1(msExit);
        if (checkAndClearS2())
            clockRam.day = incrementDay(clockRam.day);
        break;
#endif

// ###################################################################################
// Beep Menu
// ###################################################################################

// msAlarmOff,msAlarmOn

    case msAlarmOff:
        setMsgOff();
        AlarmOn = FALSE;
        stateSwitchExtendedWithS1(msChimeOn,txChime,ChimeOn);
        stateSwitchWithS2(msAlarmOn);
        break;

    case msAlarmOn:
        setMsgOn();
        AlarmOn = TRUE;
        stateSwitchExtendedWithS1(msChimeOn,txChime,ChimeOn);
        stateSwitchWithS2(msAlarmOff);
        break;

// ChimeOff,ChimeOn

    case msChimeOff:
        setMsgOff();
        ChimeOn = FALSE;
        stateSwitchWithS1(msExit);
        stateSwitchWithS2(msChimeOn);
        break;

    case msChimeOn:
        setMsgOn();
        ChimeOn = TRUE;
        stateSwitchWithS1(msExit);
        stateSwitchWithS2(msChimeOff);
        break;

// ###################################################################################
// Display Menu
// ###################################################################################

// msDspTempOff,msDspTempOn

#if OPT_TEMP_DSP
    case msDspTempOff:
        setMsgOff();
        TempOn = FALSE;
#if OPT_DATE_DSP
        stateSwitchExtendedWithS1(msDateOn,txDate,DateOn);
#else
        stateSwitchWithS1(msExit);
#endif
        stateSwitchWithS2(msDspTempOn);
        break;

    case msDspTempOn:
        setMsgOn();
        TempOn = TRUE;
#if OPT_DATE_DSP
        stateSwitchExtendedWithS1(msDateOn,txDate,DateOn);
#else
        stateSwitchWithS1(msExit);
#endif
        stateSwitchWithS2(msDspTempOff);
        break;
#endif

// msDateOff,msDateOn,

#if OPT_DATE_DSP
    case msDateOff:
        setMsgOff();
        DateOn = FALSE;
  #if OPT_DAY_DSP
        stateSwitchExtendedWithS1(msDayOn,txDay,DowOn);
  #else
        stateSwitchWithS1(msExit);
  #endif
        stateSwitchWithS2(msDateOn);
        break;

    case msDateOn:
        setMsgOn();
        DateOn = TRUE;
  #if OPT_DAY_DSP
        stateSwitchExtendedWithS1(msDayOn,txDay,DowOn);
  #else
        stateSwitchWithS1(msExit);
  #endif
        stateSwitchWithS2(msDateOff);
        break;
#endif

// msDayOff,msDayOn,

#if OPT_DAY_DSP
    case msDayOff:
        setMsgOff();
        DowOn = FALSE;
        stateSwitchWithS1(msExit);
        stateSwitchWithS2(msDayOn);
        break;

    case msDayOn:
        setMsgOn();
        DowOn = TRUE;
        stateSwitchWithS1(msExit);
        stateSwitchWithS2(msDayOff);
        break;
#endif

// ###################################################################################
// end of menu routines
// ###################################################################################

// ###################################################################################
// begin configuration routines
// ###################################################################################

#if OPT_UNITS_GROUP
    case msSetUnits:
        setAllDigits(txUnit);
        stateSwitchWithS1(msBrightness);
        stateSwitchExtendedWithS2(msUS,NoText2,Select_12);
        break;

    case msEU:
        setMinuteDigits(txEU);
        changeTimeFormat(FALSE);
        stateSwitchWithS1(msBrightness);
        stateSwitchWithS2(msUS);
        break;

    case msUS:
        setMinuteDigits(txUS);
        changeTimeFormat(TRUE);
        stateSwitchWithS1(msBrightness);
        stateSwitchWithS2(msEU);
        break;
#else
    case msFormatTime:
        setAllDigits(tx1224);
        stateSwitchWithS1(msTempUnits);
        stateSwitchExtendedWithS2(ms12,NoText2,Select_12);
        break;

    case ms12:
        setMinuteDigits(tx12);
        changeTimeFormat(TRUE);
        stateSwitchWithS1(msTempUnits);
        stateSwitchWithS2(ms24);
        break;

    case ms24:
        setMinuteDigits(tx24);
        changeTimeFormat(FALSE);
        stateSwitchWithS1(msTempUnits);
        stateSwitchWithS2(ms12);
        break;

    case msTempUnits:
        setAllDigits(txTemp4);
        stateSwitchWithS1(msFormatDate);
        stateSwitchExtendedWithS2(msF,NoText2,Select_FC);
        break;

    case msF:
        setMinuteDigits(txF);
        Select_FC = TRUE;
        stateSwitchWithS1(msFormatDate);
        stateSwitchWithS2(msC);
        break;

    case msC:
        setMinuteDigits(txC);
        Select_FC = FALSE;
        stateSwitchWithS1(msFormatDate);
        stateSwitchWithS2(msF);
        break;

    case msFormatDate:
        setAllDigits(txDate4);
        stateSwitchWithS1(msBrightness);
        stateSwitchExtendedWithS2(ms1231,NoText2,Select_MD);
        break;

    case ms1231:
        setAllDigits(tx1231);
        Select_MD = TRUE;
        stateSwitchWithS1(msBrightness);
        stateSwitchWithS2(ms3112);
        break;

    case ms3112:
        setAllDigits(tx3112);
        Select_MD = FALSE;
        stateSwitchWithS1(msBrightness);
        stateSwitchWithS2(ms1231);
        break;
#endif
    case msBrightness:
        setAllDigits(txBrt);
#if OPT_TEMP_DSP
        stateSwitchWithS1(msTempCal);
#else
        stateSwitchWithS1(msExit);
#endif
        stateSwitchWithS2(msBrtMax);
        break;

    case msBrtMax:
        dp1 = ON;
        dp2 = OFF;
        dp3 = OFF;
        h = clockRam.brightMax;
        m = clockRam.brightMin;
        displayHoursFlash();
        displayMinutesOn();
        stateSwitchWithS1(msBrtMin);
        if (checkAndClearS2()){
            d = incrementBrightness(h);
            clockRam.brightMax = d;
        }
        break;

    case msBrtMin:
        m = clockRam.brightMin;
        displayHoursOn();
        displayMinutesFlash();
#if OPT_TEMP_DSP
        stateSwitchWithS1(msTempCal);
#else
        stateSwitchWithS1(msExit);
#endif
        if (checkAndClearS2()){
            d = incrementBrightness(m);
            if ( d > clockRam.brightMax ) d = 1;
            clockRam.brightMin = d;
        }
        break;

#if OPT_TEMP_DSP
    case msTempCal:
        // fix bright from previous set and go back if needed
        // until min <= max
        if (clockRam.brightMin > clockRam.brightMax){
            clockRam.brightMin = clockRam.brightMax;
            displayState = msBrtMax;
            break;
        }
        setAllDigits(txCal);
        stateSwitchWithS1(msExit);
        stateSwitchWithS2(msSetTemp);
        break;

    case msSetTemp:
        displayTemperature();
        stateSwitchWithS1(msExit);
        if (checkAndClearS2()){
            if (clockRam.tempOffset == 15)
                clockRam.tempOffset = -15;
            else
                clockRam.tempOffset += 1;
        }
        break;
#endif

// ###################################################################################
// end configuration routines
// ###################################################################################

// ###################################################################################
// all states that set something must exit through here
// ###################################################################################

    case msExit:
        blankDisplay();                 // go blank at end of cycle
        if (!timeChanged){
            refreshTime();              // time didn't change so refresh get current
        }
        else {                          //  time before writing everything back
            clockRam.sec = 0;           // reset seconds when changing time
            timeChanged = FALSE;
        }
        putClock();                     // save changes
        putConfigRam();                 // from any set routine...
        userTimer100 = 6;               // wait for ~.5 second
        while(userTimer100) ;           // just delay...
        checkAndClearS1();              // dump keybuf on the way out the door...
        checkAndClearS2();              // dump keybuf on the way out the door...
#ifdef S3
        checkAndClearS3();              // dump keybuf on the way out the door...
#endif
        displayState = stClock;         // goto clock display
        break;

    default:
        displayState = stClock;
    }
}

// ###################################################################################
// End state machine
// ###################################################################################

void setAllDigits(uint8_t index)
{
    segs[H10] = textDesc4[(index << 2)];
    segs[H01] = textDesc4[(index << 2)+1];
    segs[M10] = textDesc4[(index << 2)+2];
    segs[M01] = textDesc4[(index << 2)+3];
}

void setHourDigits(uint8_t index)
{
    segs[H10] = textDesc2[(index << 1)];
    segs[H01] = textDesc2[(index << 1)+1];
    segs[M10] = 0xFF;
    segs[M01] = 0xFF;
}

void setMinuteDigits(uint8_t index)
{
    segs[H10] = 0xFF;
    segs[H01] = 0xFF;
    segs[M10] = textDesc2[(index << 1)];
    segs[M01] = textDesc2[(index << 1)+1];
}

void setMsgOn()
{
    segs[M10] = textDesc2[(txOn << 1)];
    segs[M01] = textDesc2[(txOn << 1)+1];
}

void setMsgOff()
{
    segs[M10] = textDesc2[(txOff << 1)];
    segs[M01] = textDesc2[(txOff << 1)+1];
}


// Setup hour display if 12 hour format.
// Set the module level global var "h" used thoughout the state machine

void setupHour(uint8_t hour)
{
    if (Select_12){
        h = hour & 0x1F;
        AM_PM = (hour & 0x20)?1:0;
    }
    else {
        h = hour;
        AM_PM = FALSE;
    }
}

void setChimeVars()
{
    if (Select_12){
        h = clockRam.chimeStartHour & 0x1F;
        dp1 = (clockRam.chimeStartHour & 0x20)?1:0;
        m = clockRam.chimeStopHour  & 0x1F;
        dp3 = (clockRam.chimeStopHour  & 0x20)?1:0;
    }
    else {
        h = clockRam.chimeStartHour;
        dp1 = FALSE;
        m = clockRam.chimeStopHour;
        dp3 = FALSE;
    }
    dp2 = OFF;
}

void getDateVars()
{
    if (Select_MD){
        h = clockRam.mon;
        m = clockRam.date;
    }
    else {
        h = clockRam.date;
        m = clockRam.mon;
    }
    dp1 = ON;
}

void putDateVars()
{
    if (Select_MD){
        clockRam.mon = h;
        clockRam.date = m;
    }
    else {
        clockRam.date = h;
        clockRam.mon = m;
    }
}

void displayTemperature()
{
    h = decToBcd(actualTemp + clockRam.tempOffset);
    segs[H10] = ledSegTB[(h & 0xF0) >> 4];
    segs[H01] = ledSegTB[(h & 0x0F)];
#if DIGIT_3_FLIP
    if ( Select_FC)
        segs[M10] = 0x31;  // F & dp on
    else
        segs[M10] = 0x70;  // C & dp on
#else
    if ( Select_FC)
        segs[M10] = 0x8E;   // F only
    else
        segs[M10] = 0xC6;   // C only
#endif
    segs[M01] = 0xFF;
}

void displayDate()
{
    dp1 = ON;
    dp2 = OFF;
    dp3 = OFF;
    getDateVars();
    displayHoursOn();
    displayMinutesOn();
}

#if OPT_DAY_DSP
void setDayOfWeek()
{
  #if OPT_DAY_ALPHA
	if (getStateS2Flasher()){
		displayDayOfWeek();
	}
	else {
        segs[H10] = BLANK_CHAR;
        segs[H01] = BLANK_CHAR;
        segs[M10] = BLANK_CHAR;
    	segs[M01] = BLANK_CHAR;
	}
  #else
        dp1 = OFF;              // hold-over from date display
        m = clockRam.day;
        displayHoursOff();
        displayMinutesFlash();
		//displayDayOfWeek();
  #endif
}
#endif

#if OPT_DAY_DSP

void displayDayOfWeek()
{
    dp1 = OFF;
    dp2 = OFF;
    dp3 = OFF;
  #if OPT_DAY_ALPHA
    h = clockRam.day;               // use hours (unused right now)
    __asm
    mov     _CathodeBuf,#0xFF	    ; blank first digit
    mov     r0,#_CathodeBuf+1	    ; ram destination address
    mov     r1,_h         		    ; get day of week and make it zero origin
    dec     r1
    mov     a,r1
    add     a,r1
    add     a,r1                  ; *3 bytes per text entry
    mov     r1,a
    mov     r2,#3                 ; number of bytes to move
    mov     dptr,#_ledDOW         ; source table address
    jnb     _Select_MD,pLp1
    inc     dptr                  ; adjust offset for US DOW table
    inc     dptr                  ; adjust offset for US DOW table
    inc     dptr                  ; adjust offset for US DOW table
pLp1:
    mov     a,r1                  ; offset into code seg source
    movc    a,@a+dptr             ; get byte in code seg from @(r1+dpr)
    mov     @r0,a                 ; put byte to ram @r0
    inc     r0
    inc     r1
    djnz    r2,pLp1	              ; loop for r2 bytes
    __endasm;
  #else
    segs[H10] = BLANK_CHAR;
    segs[H01] = 0xBF;
    segs[M10] = ledSegBT[clockRam.day];
    segs[M01] = 0xBF;
  #endif
}

#endif

void blankDisplay(void)
{
    CathodeBuf[0] = 0xFF;           // turn off all segments
    CathodeBuf[1] = 0xFF;           // in all digits
    CathodeBuf[2] = 0xFF;           // to eliminate 88:88 display
    CathodeBuf[3] = 0xFF;           // at power up
}

uint8_t incrementHours(uint8_t hour)
{
    if (Select_12) hour = toFormat24(hour);     // change to 24hr format for increment
    __asm
    mov     a,r7;
    add     a,#1;
    da      a;
    mov     r7,a;
    __endasm;
    if ( hour == 0x24 ) hour = 0;
    if (Select_12) hour = toFormat12(hour);     // if mode is 12hr, convert back
    return hour;
}

uint8_t incrementMinutes(uint8_t min)
{
    __asm
    mov     a,r7;
    add     a,#1;
    da      a;
    mov     r7,a;
    __endasm;
    if ( min == 0x60 ) min = 0;
    return min;
}

uint8_t incrementDate(uint8_t value, uint8_t position)
{
    uint8_t r;
    // test for lhs digits
    if (position == H10){
        if (Select_MD){
           r = inc12(value);
        }
        else {
          r = inc31(value);
        }
    }
    else {
        // doing rhs digits
        if (Select_MD)
          r = inc31(value);
        else
          r = inc12(value);
    }
    return r;
}

uint8_t inc12(uint8_t month)
{
    __asm
    mov     a,r7;
    add     a,#1;
    da      a;
    mov     r7,a;
    __endasm;
    if ( month == 0x13 ) month = 1;
    clockRam.mon = month;
    return month;
}

uint8_t inc31(uint8_t day)
{
    __asm
    mov     a,r7;
    add     a,#1;
    da      a;
    mov     r7,a;
    __endasm;
    if ( day == 0x32 ) day = 1;
    clockRam.date = day;
    return day;
}

uint8_t incrementDay(uint8_t day)
{
    __asm
    mov     a,r7;
    add     a,#1;
    da      a;
    mov     r7,a;
    __endasm;
    if ( day == 0x8 ) day = 1;
    return day;
}

uint8_t incrementBrightness(uint8_t brt)
{
    __asm
    mov     a,r7;
    add     a,#1;
    da      a;
    mov     r7,a;
    __endasm;
    if ( bcdToDec(brt) > MAX_BRIGHT ) brt = MIN_BRIGHT;
    return brt;
}

/*
void displayHours(__bit blink)
{
    maskOnOff = blink ? 0x00:0xFF;
    maskDp0   = dp0?0x7F:0xFF;
    maskDp1   = dp1?0x7F:0xFF;
    segs[H10] = (ledSegTB[(h & 0xF0) >> 4] | maskOnOff) & maskDp0;
    segs[H01] = (ledSegTB[(h & 0x0F)]      | maskOnOff) & maskDp1;
}
*/

void displayHours()
{
    __asm
_displayHoursOn:
    setb    c
    sjmp    L001                ; mask =00 when on

_displayHoursOff:
    clr     c
    sjmp    L001                ; mask =FF when off

_displayHoursFlash:
    lcall	_getStateS2Flasher  ; get CY bit for on/off
L001:
    mov     a,#0xFF
    addc    a,#0
    mov     _maskOnOff,a        ; =0 when cy set, =ff when not

    mov     a,#BLANK_CHAR
    mov     c,_dp0
    cpl     c                   ; 0x7f = turn it on
    rrc     a                   ; 0xFF = turn it off
    mov     _maskDp0,a

    mov     a,#BLANK_CHAR
    mov     c,_dp1
    cpl     c
    rrc     a
    mov     _maskDp1,a

    mov     dptr,#_ledSegTB
    mov     a,_h                ; get hours
    swap    a                   ; cheap way to shift right 4 places
    anl     a,#0x0f
    jnb     _blankZero,L0011    ; not blanking, leave
    jnz     L0011               ; blanking but not zero, leave
    mov     a,#0xFF             ; A was 0 on entry
    sjmp    L0012               ; write FF to display to blank digit
L0011:
    movc    a,@a+dptr
    orl     a,_maskOnOff
    anl     a,_maskDp0
L0012:
    mov     _CathodeBuf,a

    mov     a,#0x0f
    anl     a,_h
    movc	a,@a+dptr
    orl     a,_maskOnOff
    anl     a,_maskDp1
    mov     (_CathodeBuf + 1),a
    __endasm;
}

/*
void displayMinutes(__bit blink)
{
    maskOnOff = blink ? 0x00:0xFF;
    maskDp2   = dp2?0x7F:0xFF;
    maskDp3   = dp3?0x7F:0xFF;
    segs[M10] = (ledSegBT[(m & 0xF0) >> 4] | maskOnOff) & maskDp2;
    segs[M01] = (ledSegTB[(m & 0x0F)]      | maskOnOff) & maskDp3;
}
*/

void displayMinutes()
{
    __asm
_displayMinutesOn:
    setb    c
    sjmp    L002

_displayMinutesOff:
    clr     c
    sjmp    L002

_displayMinutesFlash:
    lcall	_getStateS2Flasher
L002:
    mov     a,#0xFF
    addc    a,#0
	mov     _maskOnOff,a        ; =0 when cy set, =ff when not

    mov     a,#0xFF
    mov     c,_dp2
    cpl     c                   ; 0x7f = turn it on
    rrc     a                   ; 0xFF = turn it off
	mov     _maskDp2,a

    mov     a,#0xFF
    mov     c,_dp3
    cpl     c
    rrc     a
	mov     _maskDp3,a

	mov	    a,_m                ; get minutes
	swap	a                   ; cheap way to shift right 4 places
	anl     a,#0x0f
	mov     dptr,#_ledSegBT     ; special digit LED pattern
	movc	a,@a+dptr
	orl 	a,_maskOnOff
	anl 	a,_maskDp2
	mov 	(_CathodeBuf+2),a

	mov     a,#0x0f
	anl     a,_m
	mov     dptr,#_ledSegTB     ; use reg ular pattern
	movc	a,@a+dptr
	orl     a,_maskOnOff
	anl     a,_maskDp3
	mov     (_CathodeBuf+3),a
    __endasm;
}


