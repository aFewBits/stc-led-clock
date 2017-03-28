
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

uint8_t h,m,d;                    // (h)ours and (m)inutes or (m)onth and (d)ay
uint8_t displayState;           // the main control variable

__bit   dp0,dp1;                // decimal points for each digit
__bit   dp2,dp3;                // numbers match Cathode[] array index
__bit   AM_PM;                  // used to set decimal point on/off
__bit   readyRead;              // used to read temperature once per second
__bit   timeChanged;            // write time to clock when set

uint8_t maskOnOff;              // set to 0x7F or 0xFF to blink decimal point
uint8_t maskDp0;                // set to 0x7F or 0xFF with dp0 (=dp on or off)
uint8_t maskDp1;                // set to 0x7F or 0xFF with dp1 (=dp on or off)
uint8_t maskDp2;                // set to 0x7F or 0xFF with dp2 (=dp on or off)
uint8_t maskDp3;                // set to 0x7F or 0xFF with dp3 (=dp on or off)

uint8_t brightLevel;             // result of LDR and config settings
uint8_t actualTemp;              // result of ADC and transform

// declared static - in routine that is called from from an ISR

uint8_t CathodeBuf[4];          // holds each digits segments (0 = on)
static uint8_t aPos;                // cycles through 0-3 anode positions
static uint8_t aOnTicks;
static uint8_t aOffTicks;

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
            if (aOnTicks == MAX_BRIGHT)
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
#ifdef DP1_IS_COLON
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
    displayHours(ON);
    displayMinutes(ON);
}

//----------------------------------------------------------------------------------------------//
//-------   The monster state machine. All mode changing and display is done from here     -----//
//-------  Rather than adding the code overhead of subroutines, all code is inline in this -----//
//-------  area. Big and clunky, yes. Code efficent, more so. If there is code space left  -----//
//-------  over when eveything else is complete, I'll go back and rework the monster.      -----//
//----------------------------------------------------------------------------------------------//

void displayFSM()
{
    brightLevel = mapLDR(getADCResult8(ADC_LDR) >> 2);
    if ( readyRead & _1hzToggle ){
        actualTemp = mapTemp(getADCResult(ADC_TEMP));
        readyRead = FALSE;
    }
    else if (!_1hzToggle)
        readyRead = TRUE;

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
            if( TempOn )
                displayState = stOptTemp;
            else if( DateOn )
                displayState = stOptDate;
            else if( DowOn )
                displayState = stOptDay;
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
        displayHours(ON);
        displayMinutes(ON);
        stateSwitchWithS1(stClock);
        if (checkAndClearS2()){
            clockRam.sec = 0;       // reset seconds on S2 press
            putClock();             // save change
            userTimer100 = 200;     // and refresh time to keep us here
        }
        // go back to clock on time out
        if ( !userTimer100 ) displayState = stClock;
        break;

    case stOptTemp:
        displayTemperature();
        stateSwitchWithS1(scSet);
        checkAndClearS2();
        if (!userTimer100){
            userTimer100 = 30;
            if( DateOn )
                displayState = stOptDate;
            else if( DowOn )
                displayState = stOptDay;
            else
                displayState = stClock;
        }
        break;

    case stOptDate:
        displayDate();
        stateSwitchWithS1(scSet);
        checkAndClearS2();
        if (!userTimer100){
            userTimer100 = 30;
            if( DowOn )
                displayState = stOptDay;
            else
                displayState = stClock;
        }
        break;

    case stOptDay:
        displayDayOfWeek();
        if (!userTimer100)
            displayState = stClock;
        stateSwitchWithS1(scSet);
        checkAndClearS2();
        break;

// ###################################################################################
// Setup / Config
// ###################################################################################

// scSet,scBeep,scDsp,scCfg

    case scSet:
        setText4(txSet);
        stateSwitchWithS1(scBeep);
        stateSwitchWithS2(msClock);
        break;

    case scBeep:
        setText4(txBeep);
        stateSwitchWithS1(scDsp);
        stateSwitchExtendedWithS2(msAlarmOn,txAlarm,AlarmOn);
        break;

    case scDsp:
        setText4(txDsp);
        stateSwitchWithS1(scCfg);
        stateSwitchExtendedWithS2(msDspTempOn,txTemp,TempOn);
        break;

    case scCfg:
        setText4(txCfg);
        stateSwitchWithS1(msExit);
        stateSwitchWithS2(mcFormatTime);
        break;

// ###################################################################################
// Set Menu
// ###################################################################################

// msClock,msClockHour,msClockMinute

    case msClock:
        setText2(txClock);
        stateSwitchWithS1(msAlarm);
        stateSwitchWithS2(msClockHour);
        break;

    case msClockHour:
        #ifdef DP1_IS_COLON
            dp1 = ON;
            dp2 = OFF;
        #else
            dp1 = ON;
            dp2 = ON;
        #endif
        dp3 = AM_PM;
        displayHours(getStateS2Flasher());
        displayMinutes(ON);
        stateSwitchWithS1(msClockMinute);
        if (checkAndClearS2()){
            clockRam.hr = incrementHours(clockRam.hr);
            setupHour(clockRam.hr);
            timeChanged = TRUE;
        }
        break;

    case msClockMinute:
        displayHours(ON);
        displayMinutes(getStateS2Flasher());
        stateSwitchWithS1(msAlarm);
        if (checkAndClearS2()){
            m = incrementMinutes(m);
            clockRam.min = m;
            timeChanged = TRUE;
        }
        break;

// msAlarm,msAlarmHour,msAlarmMinute

    case msAlarm:
        setText2(txAlarm);
        stateSwitchWithS1(msChime);
        stateSwitchWithS2(msAlarmHour);
        break;

    case msAlarmHour:
        #ifdef DP1_IS_COLON
            dp1 = ON;
            dp2 = OFF;
        #else
            dp1 = ON;
            dp2 = ON;
        #endif
        dp3 = AM_PM;
        setupHour(clockRam.almHour);
        m = clockRam.almMin;
        displayHours(getStateS2Flasher());
        displayMinutes(ON);
        stateSwitchWithS1(msAlarmMinute);
        if (checkAndClearS2())
            clockRam.almHour = incrementHours(clockRam.almHour);
        break;

    case msAlarmMinute:
        displayHours(ON);
        displayMinutes(getStateS2Flasher());
        stateSwitchWithS1(msChime);
        if (checkAndClearS2()){
            m = incrementMinutes(m);
            clockRam.almMin = m;
            }
        break;

// msChime,msChimeStartHour,msChimeStopHour

    case msChime:
        setText2(txChime);
        stateSwitchWithS1(msDate);
        stateSwitchWithS2(msChimeStartHour);
        break;

    case msChimeStartHour:
        setChimeVars();
        displayHours( getStateS2Flasher() );
        displayMinutes(ON);
        stateSwitchWithS1(msChimeStopHour);
        if (checkAndClearS2())
            clockRam.chimeStartHour = incrementHours(clockRam.chimeStartHour);
        break;

    case msChimeStopHour:
        setChimeVars();
        displayHours(ON);
        displayMinutes(getStateS2Flasher());
        stateSwitchWithS1(msDate);
        if (checkAndClearS2())
            clockRam.chimeStopHour = incrementHours(clockRam.chimeStopHour);
        break;

// msDate,msDateMonth,msDateDay,

    case msDate:
        dp1 = ON;
        dp2 = OFF;
        dp3 = OFF;
        setText2(txDate);
        stateSwitchWithS1(msDay);
        stateSwitchWithS2(msDateMonth);
        break;

    case msDateMonth:
        getDateVars();
        displayHours(getStateS2Flasher());
        displayMinutes(ON);
        stateSwitchWithS1(msDateDay);
        if (checkAndClearS2())
            h = incrementDate(h,H10);
        break;

    case msDateDay:
        getDateVars();
        displayHours(ON);
        displayMinutes(getStateS2Flasher());
        stateSwitchWithS1(msDay);
        if (checkAndClearS2())
            m = incrementDate(m,M10);
        break;

    case msDay:
        setText2(txDay);
        stateSwitchWithS1(msExit);
        stateSwitchWithS2(msDayOfWeek);
        break;

    case msDayOfWeek:
        m = clockRam.day;
        dp1 = OFF;              // hold-over from date display
        displayHours(OFF);
        displayMinutes( getStateS2Flasher() );
        stateSwitchWithS1(msExit);
        if (checkAndClearS2())
            clockRam.day = incrementDay(clockRam.day);
        break;

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

// msDateOff,msDateOn,

    case msDspTempOff:
        setMsgOff();
        TempOn = FALSE;
        stateSwitchExtendedWithS1(msDateOn,txDate,DateOn);
        stateSwitchWithS2(msDspTempOn);
        break;

    case msDspTempOn:
        setMsgOn();
        TempOn = TRUE;
        stateSwitchExtendedWithS1(msDateOn,txDate,DateOn);
        stateSwitchWithS2(msDspTempOff);
        break;

// msDspTempOff,msDspTempOn,

    case msDateOff:
        setMsgOff();
        DateOn = FALSE;
        stateSwitchExtendedWithS1(msDayOn,txDay,DowOn);
        stateSwitchWithS2(msDateOn);
        break;

    case msDateOn:
        setMsgOn();
        DateOn = TRUE;
        stateSwitchExtendedWithS1(msDayOn,txDay,DowOn);
        stateSwitchWithS2(msDateOff);
        break;

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

// ###################################################################################
// end of menu routines
// ###################################################################################

// ###################################################################################
// begin configuration routines
// ###################################################################################

    case mcFormatTime:
        setText4(tx1224);
        stateSwitchWithS1(mcTempUnits);
        stateSwitchExtendedWithS2(mc12,NoText2,Select_12);
        break;

    case mc12:
        setText2A(tx12);
        changeTimeFormat(TRUE);
        stateSwitchWithS1(mcTempUnits);
        stateSwitchWithS2(mc24);
        break;

    case mc24:
        setText2A(tx24);
        changeTimeFormat(FALSE);
        stateSwitchWithS1(mcTempUnits);
        stateSwitchWithS2(mc12);
        break;

    case mcTempUnits:
        setText4(txTemp4);
        stateSwitchWithS1(mcFormatDate);
        stateSwitchExtendedWithS2(mcF,NoText2,Select_FC);
        break;

    case mcF:
        setText2A(txF);
        Select_FC = TRUE;
        stateSwitchWithS1(mcFormatDate);
        stateSwitchWithS2(mcC);
        break;

    case mcC:
        setText2A(txC);
        Select_FC = FALSE;
        stateSwitchWithS1(mcFormatDate);
        stateSwitchWithS2(mcF);
        break;

    case mcFormatDate:
        setText4(txDate4);
        stateSwitchWithS1(mcBrightness);
        stateSwitchExtendedWithS2(mc1231,NoText2,Select_MD);
        break;

    case mc1231:
        setText4(tx1231);
        Select_MD = TRUE;
        stateSwitchWithS1(mcBrightness);
        stateSwitchWithS2(mc3112);
        break;

    case mc3112:
        setText4(tx3112);
        Select_MD = FALSE;
        stateSwitchWithS1(mcBrightness);
        stateSwitchWithS2(mc1231);
        break;

    case mcBrightness:
        setText4(txBrt);
        stateSwitchWithS1(mcTempCal);
        stateSwitchWithS2(mcBrtMax);
        break;

    case mcBrtMax:
        dp1 = ON;
        dp2 = OFF;
        dp3 = OFF;
        h = clockRam.brightMax;
        m = clockRam.brightMin;
        displayHours(getStateS2Flasher());
        displayMinutes(ON);
        stateSwitchWithS1(mcBrtMin);
        if (checkAndClearS2()){
            d = incrementBrightness(h);
            if (d < clockRam.brightMin)
                d = clockRam.brightMin;
            clockRam.brightMax = d;
        }
        break;

    case mcBrtMin:
        m = clockRam.brightMin;
        displayHours(ON);
        displayMinutes(getStateS2Flasher());
        stateSwitchWithS1(mcTempCal);
        if (checkAndClearS2()){
            d = incrementBrightness(m);
            if ( d > clockRam.brightMax ) d = 1;
            clockRam.brightMin = d;
        }
        break;

    case mcTempCal:
        // fix bright from previous set and go back if needed
        // until min <= max
        if (clockRam.brightMin > clockRam.brightMax){
            clockRam.brightMin = clockRam.brightMax;
            displayState = mcBrtMax;
            break;
        }
        setText4(txCal);
        stateSwitchWithS1(msExit);
        stateSwitchWithS2(mcSetTemp);
        break;

    case mcSetTemp:
        displayTemperature();
        stateSwitchWithS1(msExit);
        if (checkAndClearS2()){
            if (clockRam.tempOffset == 15)
                clockRam.tempOffset = -15;
            else
                clockRam.tempOffset += 1;
        }
        break;

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
        displayState = stClock;         // goto clock display
        break;

    default:
        displayState = stClock;
    }
}

// ###################################################################################
// End state machine
// ###################################################################################

void setText4(uint8_t index)
{
    segs[H10] = textDesc4[(index << 2)];
    segs[H01] = textDesc4[(index << 2)+1];
    segs[M10] = textDesc4[(index << 2)+2];
    segs[M01] = textDesc4[(index << 2)+3];
}

void setText2(uint8_t index)
{
    segs[H10] = textDesc2[(index << 1)];
    segs[H01] = textDesc2[(index << 1)+1];
    segs[M10] = 0xFF;
    segs[M01] = 0xFF;
}

void setText2A(uint8_t index)
{
    segs[H10] = 0xFF;
    segs[H01] = 0xFF;
    segs[M10] = textDesc2[(index << 1)];
    segs[M01] = textDesc2[(index << 1)+1];
}

void setMsgOn()
{
#ifdef NO_DIGIT_3_FLIP
    segs[M10] = 0XA3; // o
    segs[M01] = 0xAB; // n
#else
    segs[M10] = 0x9C; // o
    segs[M01] = 0xAB; // n
#endif
}

void setMsgOff()
{
#ifdef NO_DIGIT_3_FLIP
    segs[M10] = 0xC0; // o
    segs[M01] = 0x8E; // F
#else
    segs[M10] = 0XC0; // o
    segs[M01] = 0x8E; // F
#endif
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

void displayHours(__bit blink)
{
    maskOnOff = blink ? 0x00:0xFF;
    maskDp0   = dp0?0x7F:0xFF;
    maskDp1   = dp1?0x7F:0xFF;
    segs[H10] = (ledSegTB[(h & 0xF0) >> 4] | maskOnOff) & maskDp0;
    segs[H01] = (ledSegTB[(h & 0x0F)]      | maskOnOff) & maskDp1;
}

void displayMinutes(__bit blink)
{
    maskOnOff = blink ? 0x00:0xFF;
    maskDp2   = dp2?0x7F:0xFF;
    maskDp3   = dp3?0x7F:0xFF;
    segs[M10] = (ledSegBT[(m & 0xF0) >> 4] | maskOnOff) & maskDp2;
    segs[M01] = (ledSegTB[(m & 0x0F)]      | maskOnOff) & maskDp3;
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
#ifdef NO_DIGIT_3_FLIP
    if ( Select_FC)
        segs[M10] = 0x8E;   // F only
    else
        segs[M10] = 0xC6;   // C only
#else
    if ( Select_FC)
        segs[M10] = 0x31;  // F & dp on
    else
        segs[M10] = 0x70;  // C & dp on
#endif
    segs[M01] = 0xFF;
}

void displayDate()
{
    dp1 = ON;
    dp2 = OFF;
    dp3 = OFF;
    getDateVars();
    displayHours(ON);
    displayMinutes(ON);
}

void displayDayOfWeek()
{
    dp1 = OFF;
    dp2 = OFF;
    dp3 = OFF;
    segs[H10] = 0xFF;
    segs[H01] = 0xFF;
    segs[M10] = 0xFF;
    segs[M01] = ledSegTB[clockRam.day];
}

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
