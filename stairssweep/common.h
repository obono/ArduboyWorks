#pragma once

#include "MyArduboyV.h"

/*  Defines  */

//#define DEBUG
#define FPS             60
#define APP_TITLE       "STAIRS SWEEP"
#define APP_CODE        "OBN-Y09"
#define APP_VERSION     "VER 0.02"
#define APP_RELEASED    "NOVEMBER 2019"

#if 0
#define LEFT_BUTTON_V   DOWN_BUTTON
#define RIGHT_BUTTON_V  UP_BUTTON
#define UP_BUTTON_V     LEFT_BUTTON
#define DOWN_BUTTON_V   RIGHT_BUTTON
#else
#define LEFT_BUTTON_V   LEFT_BUTTON
#define RIGHT_BUTTON_V  RIGHT_BUTTON
#define UP_BUTTON_V     UP_BUTTON
#define DOWN_BUTTON_V   DOWN_BUTTON
#endif

enum MODE_T {
    MODE_LOGO = 0,
    MODE_TITLE,
    MODE_GAME,
};

/*  Typedefs  */

typedef struct {
    uint16_t    hiscore[10];
    uint32_t    playFrames;
    uint16_t    playCount;
} RECORD_T; // sizeof(RECORD_T) must be 26 bytes

/*  Global Functions (Common)  */

void    readRecord(void);
void    writeRecord(void);
void    clearRecord(void);
bool    enterScore(uint16_t score);

void    drawNumber(int16_t x, int16_t y, int16_t digits, int32_t value);
void    drawTime(int16_t x, int16_t y, uint32_t frames);

void    setSound(bool on);
void    playSoundTick(void);
void    playSoundClick(void);

void    eepSeek(int addr);
uint8_t eepRead8(void);
uint16_t eepRead16(void);
uint32_t eepRead32(void);
void    eepReadBlock(void *p, size_t n);
void    eepWrite8(uint8_t val);
void    eepWrite16(uint16_t val);
void    eepWrite32(uint32_t val);
void    eepWriteBlock(const void *p, size_t n);

/*  Global Functions (Menu)  */

void    clearMenuItems(void);
void    addMenuItem(const __FlashStringHelper *label, void (*func)(void));
int8_t  getMenuItemPos(void);
int8_t  getMenuItemCount(void);
void    setMenuCoords(int8_t x, int8_t y, int8_t w, int8_t h, bool f, bool s);
void    setMenuItemPos(int8_t pos);
void    handleMenu(void);
void    drawMenuItems(bool isForced);
void    drawSoundEnabled(void);

/*  Global Functions (Each Mode)  */

void    initLogo(void);
MODE_T  updateLogo(void);
void    drawLogo(void);

void    initTitle(void);
MODE_T  updateTitle(void);
void    drawTitle(void);

void    initGame(void);
MODE_T  updateGame(void);
void    drawGame(void);

/*  Global Variables  */

extern MyArduboyV   arduboy;
extern RECORD_T     record;

extern bool     isRecordDirty;
extern uint16_t lastScore;
extern bool     isInvalid;

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
