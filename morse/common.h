#pragma once

#include "MyArduboy2.h"

/*  Defines  */

//#define DEBUG
#define FPS             60
#define APP_TITLE       "MORSE CODE TRAINER"
#define APP_CODE        "OBN-Y15"
#define APP_VERSION     "0.03"
#define APP_RELEASED    "OCTOBER 2022"

#define UNIT_FRAMES_MAX 13
#define LED_COLOR_MAX   13
#define TONE_FREQ_MAX   121

enum MODE_T : uint8_t {
    MODE_LOGO = 0,
    MODE_CONSOLE,
    MODE_LIST,
    MODE_SETTING,
    MODE_CREDIT,
};

enum : uint8_t {
    DECODE_MODE_EN = 0,
    DECODE_MODE_JA,
    DECODE_MODE_MAX,
};

enum : uint8_t {
    LED_OFF = 0,
    LED_LOW,
    LED_HIGH,
    LED_MAX,
};

enum : uint8_t {
    KEYBOARD_NONE = 0,
    KEYBOARD_US,
    KEYBOARD_JP,
    KEYBOARD_MAX,
};

enum : uint8_t {
    IME_MODE_ROMAN = 0,
    IME_MODE_KANA,
    IME_MODE_MAX,
};

/*  Typedefs  */

typedef struct {
    uint32_t    playFrames;
    uint32_t    madeLetters:24;
    uint32_t    unitFrames:4;
    uint32_t    decodeMode:1;
    uint32_t    keyboard:2;
    uint32_t    imeMode:1;
    uint8_t     led:2;
    uint8_t     ledColor:4;
    uint8_t     toneFreq;
} RECORD_T; // sizeof(RECORD_T) is 10 bytes

/*  Global Functions (Common)  */

void    readRecord(void);
void    writeRecord(void);
void    clearRecord(void);
void    handleDPad(void);
void    drawTime(int16_t x, int16_t y, uint32_t frames);
void    drawButtonIcon(int16_t x, int16_t y, bool isB);
uint8_t drawMorseCode(int16_t x, int16_t y, uint16_t code);
void    indicateSignalOn(void);
void    indicateSignalOff(void);
void    checkUSBStatus(void);
bool    isUSBConfigured(void);

void    setSound(bool on);
void    playSoundTick(void);
void    playSoundClick(void);

/*  Global Functions (Menu)  */

void    clearMenuItems(void);
void    addMenuItem(const __FlashStringHelper *label, void (*func)(void));
int8_t  getMenuItemPos(void);
int8_t  getMenuItemCount(void);
void    setMenuCoords(int8_t x, int8_t y, int8_t w, int8_t h, bool f);
void    setMenuItemPos(int8_t pos);
void    setConfirmMenu(int8_t y, void (*funcOk)(), void (*funcCancel)());
void    handleMenu(void);
void    drawMenuItems(bool isForced);

/*  Global Functions (Each Mode)  */

void    initLogo(void);
MODE_T  updateLogo(void);
void    drawLogo(void);

void    initConsole(void);
MODE_T  updateConsole(void);
void    drawConsole(void);

void    initList(void);
MODE_T  updateList(void);
void    drawList(void);

void    initSetting(void);
MODE_T  updateSetting(void);
void    drawSetting(void);

void    initCredit(void);
MODE_T  updateCredit(void);
void    drawCredit(void);

/*  Global Functions (macros)  */

#define circulate(n, v, m)  (((n) + (v) + (m)) % (m))

#define isDigit(letter)     ((letter) >= '0' && letter <= '9')
#define isAlpha(letter)     ((letter) >= 'A' && letter <= 'Z')
#define isAlnum(letter)     (isDigit(letter) || isAlpha(letter))
#define isKana(letter)      ((uint8_t)(letter) >= 0x60 && (uint8_t)(letter) <= 0x93)
#define isPrint(letter)     ((letter) >= ' ' && (letter) <= '_')
#define isGraph(letter)     ((letter) > ' ' && (letter) <= '_')
#define isNonGraph(letter)  ((letter) >= '\0' && (letter) <= ' ')

/*  Global Variables  */

extern MyArduboy2   arduboy;
extern RECORD_T     record;

extern uint8_t  counter;
extern int8_t   padX, padY, padRepeatCount;
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
