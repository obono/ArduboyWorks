#pragma once

#include "MyArduboy.h"

/*  Defines  */

//#define DEBUG
#define FPS             60
#define APP_TITLE       "LASERS"
#define APP_INFO        "OBN-Y07 VER 0.04"
#define APP_RELEASED    "FEBRUARY 2019"

enum MODE_T {
    MODE_LOGO = 0,
    MODE_WARN,
    MODE_TITLE,
    MODE_GAME,
};

/*  Typedefs  */

typedef struct {
    uint8_t x:3;
    uint8_t y:3;
} POS_T;

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

void    drawFrame(int8_t x, int8_t y, int8_t w, int8_t h);
void    drawNumber(int16_t x, int16_t y, int32_t value);
void    drawTime(int16_t x, int16_t y, uint32_t frames);
void    drawBitmapBW(int16_t x, int16_t y, const uint8_t *bitmap, uint8_t w, uint8_t h);
void    clearScreenGray(void);

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
void    setMenuCoords(int8_t x, int8_t y, int8_t w, int8_t h);
void    setMenuItemPos(int8_t pos);
void    handleMenu(void);
void    drawMenuItems(void);

/*  Global Functions (Each Mode)  */

void    initLogo(void);
MODE_T  updateLogo(void);
void    drawLogo(void);

void    initWarn(void);
MODE_T  updateWarn(void);
void    drawWarn(void);

void    initTitle(void);
MODE_T  updateTitle(void);
void    drawTitle(void);

void    initGame(void);
MODE_T  updateGame(void);
void    drawGame(void);

/*  Global Variables  */

extern MyArduboy    arduboy;
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
