#pragma once

#include "MyArduboy.h"

/*  Defines  */

//#define DEBUG
#define FPS             40
#define APP_TITLE       "PSI-COLO"
#define APP_INFO        "OBN-Y05 VER 0.01"
#define APP_RELEASED    "JULY 2018"

enum MODE_T {
    MODE_LOGO = 0,
    MODE_TITLE,
    MODE_GAME,
    MODE_GALLERY,
};

/*  Typedefs  */

typedef struct {
    uint16_t    x : 6;
    uint16_t    y : 6;
    uint16_t    rot : 4;
} PIECE_T;

typedef struct {
    uint8_t xy : 5;
    uint8_t rot : 3;
} CODE_T;

/*  Global Functions (Common)  */

void    readRecord(void);
void    writeRecord(void);
void    clearRecord(void);
void    handleDPad(void);

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
extern bool     isDirty;
extern uint32_t playFrames;
extern uint32_t shotCount;
extern uint32_t bombCount;
extern uint32_t killCount;
extern int8_t   padX, padY, padRepeatCount;

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
