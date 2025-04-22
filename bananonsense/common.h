#pragma once

#include "MyArduboy2.h"

/*  Defines  */

#define FPS             60
#define APP_TITLE       "BANANONSENSE"
#define APP_CODE        "OBN-Y17"
#define APP_VERSION     "0.02"
#define APP_RELEASED    "APRIL 2025"

enum MODE_T : uint8_t {
    MODE_LOGO = 0,
    MODE_TITLE,
    MODE_GAME,
};

/*  Typedefs  */

typedef struct {
    uint16_t    banana;
} RECORD_T; // sizeof(RECORD_T) is 2 bytes

/*  Global Functions (Common)  */

void    readRecord(void);
void    writeRecord(void);
void    clearRecord(void);
void    handleDPad(void);
void    drawTime(int16_t x, int16_t y, uint32_t frames);

void    setSound(bool on);
void    playSoundTick(void);
void    playSoundClick(void);

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

#define IMPORT_BIN_FILE(file, sym) asm (    \
    ".global " #sym "\n"                    \
    #sym ":\n"                              \
    ".incbin \"" file "\"\n"                \
    ".byte 0\n"                             \
    ".global _sizeof_" #sym "\n"            \
    ".set _sizeof_" #sym ", . - " #sym "\n" \
    ".balign 2\n")

#define circulate(n, v, m)      (((n) + (v) + (m)) % (m))
#define circulateOne(n, v, m)   (((n) + (v) + (m) - 1) % (m) + 1)
#define roundup(n, div)         (((n) + (div) - 1) / (div))
#define maskbits(bits)          ((1 << (bits)) - 1)

/*  Global Variables  */

extern MyArduboy2   ab;
extern RECORD_T     record;
extern uint8_t      counter;
extern int8_t       padX, padY, padRepeatCount;
extern bool         isInvalid, isRecordDirty;
