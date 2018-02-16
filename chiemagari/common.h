#ifndef COMMON_H
#define COMMON_H

#include "MyArduboy.h"

/*  Defines  */

//#define DEBUG
#define APP_TITLE       "CHIE MAGARI ITA"
#define APP_INFO        "OBN-Y04 VER 0.01"
#define APP_RELEASED    "MARCH 2018"

/*  Global Functions  */

void initLogo(void);
bool updateLogo(void);
void drawLogo(void);

void initTitle(void);
bool updateTitle(void);
void drawTitle(void);
uint8_t setLastScore(int score, uint32_t time);

void initGame(void);
bool updateGame(void);
void drawGame(void);

/*  Global Variables  */

extern MyArduboy        arduboy;

/*  For Debugging  */

#ifdef DEBUG
extern bool             dbgPrintEnabled;
extern char             dbgRecvChar;
#define dprint(...)     (!dbgPrintEnabled || Serial.print(__VA_ARGS__))
#define dprintln(...)   (!dbgPrintEnabled || Serial.println(__VA_ARGS__))
#else
#define dprint(...)
#define dprintln(...)
#endif

#endif // COMMON_H
