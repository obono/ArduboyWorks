#pragma once

#include "MyArduboyV.h"

/*  Defines  */

//#define FLIP_SCREEN
#define ROTATE_DPAD

//#define DEBUG
#define FPS             60
#define APP_TITLE       "STAIRS SWEEP"
#define APP_CODE        "OBN-Y09"
#define APP_VERSION     "0.10"
#define APP_RELEASED    "OCTOBER 2019"

#define PAD_REPEAT_DELAY    (FPS / 6)
#define PAD_REPEAT_INTERVAL 1

#define HILEVEL3    60
#define HISCORE3    200000UL
#define HILEVEL4    90
#define HISCORE4    500000UL

#ifdef ROTATE_DPAD
    #ifdef FLIP_SCREEN
        #define LEFT_BUTTON_V   UP_BUTTON
        #define RIGHT_BUTTON_V  DOWN_BUTTON
        #define UP_BUTTON_V     RIGHT_BUTTON
        #define DOWN_BUTTON_V   LEFT_BUTTON
    #else
        #define LEFT_BUTTON_V   DOWN_BUTTON
        #define RIGHT_BUTTON_V  UP_BUTTON
        #define UP_BUTTON_V     LEFT_BUTTON
        #define DOWN_BUTTON_V   RIGHT_BUTTON
    #endif
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

enum ALIGN_T {
    ALIGN_LEFT = 0,
    ALIGN_CENTER,
    ALIGN_RIGHT,
};

/*  Typedefs  */

typedef struct {
    uint32_t    score : 24;
    uint32_t    level : 8;
} HISCORE_T;

typedef struct {
    HISCORE_T   hiscore[5];
    uint32_t    playFrames;
    uint16_t    playCount;
} RECORD_T; // sizeof(RECORD_T) must be 26 bytes

/*  Global Functions (Common)  */

void    readRecord(void);
void    writeRecord(void);
void    clearRecord(void);
bool    enterScore(uint32_t score, uint8_t level);
void    handleDPadV(void);
void    drawNumberV(int16_t x, int16_t y, int32_t value, ALIGN_T align);
void    drawTime(int16_t x, int16_t y, uint32_t frames, ALIGN_T align);
void    drawLabelLevel(int16_t x, int16_t y);
void    drawLabelScore(int16_t x, int16_t y);

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
extern int8_t   padX, padY, padRepeatCount;
extern uint8_t  startLevel;
extern uint32_t lastScore;
extern bool     isInvalid, isRecordDirty;

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
