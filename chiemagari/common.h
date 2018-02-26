#ifndef COMMON_H
#define COMMON_H

#include "MyArduboy.h"

/*  Defines  */

//#define DEBUG
#define APP_TITLE       "CHIE MAGARI ITA"
#define APP_INFO        "OBN-Y04 VER 0.04"
#define APP_RELEASED    "MARCH 2018"

enum MODE_T {
    MODE_LOGO = 0,
    MODE_MENU,
    MODE_PUZZLE,
    MODE_GALLERY,
};

/*  Global Functions (Common)  */

void    readRecord(void);
bool    readRecordPieces(uint16_t *pPiece);
void    saveRecord(uint16_t *pPiece);
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

void    initMenu(void);
MODE_T  updateMenu(void);
void    drawMenu(void);

void    initPuzzle(void);
MODE_T  updatePuzzle(void);
void    drawPuzzle(void);
void    resetPieces(void);

void    initGallery(void);
MODE_T  updateGallery(void);
void    drawGallery(void);

/*  Global Variables  */

extern MyArduboy arduboy;
extern bool     isHelpVisible;
extern uint8_t  clearCount;
extern uint32_t playFrames;
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

#endif // COMMON_H
