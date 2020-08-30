#pragma once

#include "MyArduboy2.h"

/*  Defines  */

//#define DEBUG
#define FPS             60
#define APP_TITLE       "EVADE"
#define APP_CODE        "OBN-Y14"
#define APP_VERSION     "0.10"
#define APP_RELEASED    "AUGUST 2020"

enum MODE_T : uint8_t {
    MODE_LOGO = 0,
    MODE_TITLE,
    MODE_GAME,
};

enum SIMPLE_OP_T : uint8_t {
    SIMPLE_OP_NONE = 0,
    SIMPLE_OP_START,
    SIMPLE_OP_SOUND,
    SIMPLE_OP_SETTINGS,
};

/*  Typedefs  */

typedef struct {
    uint16_t    speed:4;
    uint16_t    acceleration:2;
    uint16_t    density:3;
    uint16_t    thickness:3;
    uint16_t    simpleMode:1;
    uint16_t    dummy:3;
    uint16_t    hiscore;
    uint32_t    playFrames;
    uint16_t    playCount;
} RECORD_T; // sizeof(RECORD_T) is 10 bytes

/*  Global Functions (Common)  */

void    readRecord(void);
void    writeRecord(void);
void    clearRecord(void);
void    handleDPad(void);
void    drawTime(int16_t x, int16_t y, uint32_t frames);
void    drawBitmapWithMask(int16_t x, int16_t y, const uint8_t *bitmap, uint8_t w, uint8_t h);
void    drawBitmapBordered(int16_t x, int16_t y, const uint8_t *bitmap, uint8_t w, uint8_t h);
SIMPLE_OP_T handleSimpleMode(void);
void    drawSimpleModeInstruction(void);

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
void    setConfirmMenu(int8_t y, void (*funcOk)(), void (*funcCancel)());
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

/*  Grobal Functions (macros)  */

#define circulate(n, v, m)      (((n) + (v) + (m)) % (m))
#define circulateOne(n, v, m)   (((n) + (v) + (m) - 1) % (m) + 1)

/*  Global Variables  */

extern MyArduboy2   arduboy;
extern RECORD_T     record;
extern uint8_t      counter;
extern int8_t       padX, padY, padRepeatCount;
extern bool         isInvalid, isRecordDirty;

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
