
#ifndef _SOUND_H
#define _SOUND_H

#if HAS_NY3P_SPEECH

enum soundsIndex {
    sndOne = 1,
    sndTwo,
    sndThree,
    sndFour,
    sndFive,
    sndSix,
    sndSeven,
    sndEight,
    sndNine,
    sndTen,
    sndEleven,
    sndTwelve,
    sndThirteen,
    sndFourteen,
    sndFifteen,
    sndSixteen,
    sndSeventeen,
    sndEighteen,
    sndNineteen,
    sndTwenty,
    sndThirty,
    sndFourty,
    sndFifty,
    sndOhClock,
    sndAh,
    sndOh,
    sndAM,
    sndPM,
    sndChime,
    sndJingle,
    sndRing
};

void speakTime();
void speakItem(uint8_t bitCount);
void speakAM_PM();
void sendOneBit();
void resetSound();
void waitS1Clk();

extern volatile uint8_t soundTimer;     // decremented every timer 0 tick if set
extern volatile uint8_t stateQS3;       // 8 bit state value (for detecting state changes)
extern volatile __bit   pressedS3;      // true when pressed, user clears

extern __bit   AM_PM;

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
#endif
