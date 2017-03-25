
#ifndef _UTILITY_H
#define _UTILITY_H

uint8_t mapLDR(int16_t x);
uint8_t mapTemp(int16_t x);
uint8_t map();

uint8_t bcdToDec(uint8_t bcd);
uint8_t decToBcd(uint8_t dec);

uint8_t toFormat12(uint8_t);
uint8_t toFormat24(uint8_t);
void changeTimeFormat(__bit);

void checkChime();
void soundChime();

#endif
