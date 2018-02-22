#ifndef COMMON_H
#define COMMON_H

#include "MyArduboy.h"

/*  Defines  */

//#define DEBUG
#define APP_TITLE       "CHIE MAGARI ITA"
#define APP_INFO        "OBN-Y04 VER 0.03"
#define APP_RELEASED    "MARCH 2018"

/*  Global Functions  */

void readRecord(void);
bool readRecordPieces(uint16_t *pPiece);
void saveRecord(uint16_t *pPiece);
void clearRecord(void);

void setSound(bool on);
void playSoundTick(void);
void playSoundClick(void);

void eepSeek(int addr);
uint8_t eepRead8(void);
uint16_t eepRead16(void);
uint32_t eepRead32(void);
void eepReadBlock(void *p, size_t n);
void eepWrite8(uint8_t val);
void eepWrite16(uint16_t val);
void eepWrite32(uint32_t val);
void eepWriteBlock(const void *p, size_t n);

void initLogo(void);
bool updateLogo(void);
void drawLogo(void);

void initMenu(void);
bool updateMenu(void);
void drawMenu(void);

void initGame(void);
bool updateGame(void);
void drawGame(void);
void resetPieces(void);

/*  Global Variables  */

extern MyArduboy arduboy;
extern bool     isSoundEnable;
extern bool     isHelpVisible;
extern uint8_t  clearCount;
extern uint32_t playFrames;

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
