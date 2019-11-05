#pragma once

#include "MyArduboy.h"

/*  Defines  */

//#define DEBUG
#define FPS             60
#define APP_TITLE       "PI 24K"
#define APP_CODE        "OBN-Y10"
#define APP_VERSION     "0.02"
#define APP_RELEASED    "NOVEMBER 2019"

enum MODE_T {
    MODE_LOGO = 0,
    MODE_MAIN,
};

/*  Typedefs  */

/*  Global Functions (Common)  */

void    drawNumber(int16_t x, int16_t y, int32_t value);
void    setSound(bool on);
void    playSoundTick(void);
void    playSoundClick(void);
int16_t getKetaMax(void);
int8_t  getPiNumber(int16_t pos);

/*  Global Functions (Each Mode)  */

void    initLogo(void);
MODE_T  updateLogo(void);
void    drawLogo(void);

void    initMain(void);
MODE_T  updateMain(void);
void    drawMain(void);

/*  Global Variables  */

extern MyArduboy    arduboy;
extern bool         isInvalid;

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
