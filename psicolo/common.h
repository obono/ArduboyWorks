#pragma once

#include "MyArduboy.h"

/*  Defines  */

//#define DEBUG
#define FPS             40
#define APP_TITLE       "PSI COLO"
#define APP_INFO        "OBN-Y05 VER 0.10"
#define APP_RELEASED    "JULY 2018"

enum MODE_T {
    MODE_LOGO = 0,
    MODE_TITLE,
    MODE_GAME,
};

enum GAME_MODE_T {
    GAME_MODE_ENDLESS = 0,
    GAME_MODE_LIMITED,
    GAME_MODE_PUZZLE,
};

/*  Typedefs  */

typedef struct {
    uint32_t    endlessHiscore : 24;
    uint32_t    endlessMaxLevel : 8;
    uint32_t    limitedHiscore : 24;
    uint32_t    limitedMaxChain : 8;
    uint32_t    playFrames;
    uint32_t    erasedDice;
    uint8_t     puzzleClearFlag[5];
    uint8_t     puzzleClearCount;
} RECORD_T; // sizeof(RECORD_T) is 22 bytes

/*  Global Functions (Common)  */

void    readRecord(void);
void    writeRecord(void);
void    clearRecord(void);

void    handleDPad(void);
void    drawNumber(int16_t x, int16_t y, int32_t value);
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

extern MyArduboy arduboy;
extern RECORD_T record;
extern bool     isRecordDirty;
extern int8_t   padX, padY, padRepeatCount;
extern GAME_MODE_T gameMode;
extern bool     isInvalid;

extern const uint8_t imgLabelScore[];
extern const uint8_t imgLabelChain[];
extern const uint8_t imgLabelLevel[];
extern const uint8_t imgLabelTime[];
extern const uint8_t imgLabelInfo[];
extern const uint8_t imgLabelStep[];
extern const uint8_t imgLabelIssue[];

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
