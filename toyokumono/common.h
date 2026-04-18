#pragma once

#include "MyArduboy2.h"

/*  Defines  */

#define FPS             60
#define APP_TITLE       "TOYOKUMONO"
#define APP_CODE        "OBN-Y18"
#define APP_VERSION     "0.03"
#define APP_RELEASED    "APRIL 2026"

enum MODE_T : uint8_t {
    MODE_LOGO = 0,
    MODE_TITLE,
    MODE_GAME,
};

/*  Typedefs  */

typedef struct {
    uint16_t    hiscore[10];
    uint32_t    playFrames;
    uint16_t    playCount;
} RECORD_T; // sizeof(RECORD_T) is 26

/*  Global Functions (Common)  */

void    readRecord(void);
void    writeRecord(void);
void    clearRecord(void);
bool    enterScore(uint16_t score);
void    handleDPad(void);
void    drawTime(int16_t x, int16_t y, uint32_t frames);
void    drawText(const char *p, int16_t y);

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

/*  Global Functions (macros)  */

#define clamp(n, nMin, nMax)    max(min((n), (nMax)), (nMin))      
#define circulate(n, v, m)      (((n) + (v) + (m)) % (m))
#define addWithLimit(n, v, lim) (n = (n < (lim) - (v)) ? n + (v) : (lim))
#define subWithLimit(n, v, lim) (n = (n > (lim) + (v)) ? n - (v) : (lim))

/*  Global Variables  */

extern MyArduboy2   ab;
extern RECORD_T     record;
extern uint16_t     lastScore;
extern uint8_t      counter;
extern int8_t       padX, padY, padRepeatCount;
extern bool         isInvalid, isRecordDirty, isInstruction;
