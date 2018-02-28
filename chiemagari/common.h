#ifndef COMMON_H
#define COMMON_H

#include "MyArduboy.h"

/*  Defines  */

//#define DEBUG
#define APP_TITLE       "CHIE MAGARI ITA"
#define APP_INFO        "OBN-Y04 VER 0.10"
#define APP_RELEASED    "MARCH 2018"

#define PIECES  10

#define HELP_W  32
#define HELP_H  11
#define HELP_TOP_LIMIT  (HEIGHT - HELP_H)
#define HELP_RIGHT_POS  (WIDTH - HELP_W)

enum MODE_T {
    MODE_LOGO = 0,
    MODE_MENU,
    MODE_PUZZLE,
    MODE_GALLERY,
};

enum HELP_T
{
    HELP_FREE = 0,
    HELP_PICK,
    HELP_HOLD,
    HELP_GALLERY,
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
void    readPieces(void);
void    readEncodedPieces(uint8_t idx, CODE_T *pCode);
void    verifyEncodedPieces(void);
void    writeRecord(void);
void    writeEncodedPieces(uint8_t idx, CODE_T *pCode);
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
void    drawBoard(int16_t offsetX);
void    drawPiece(int8_t idx);
void    drawHelp(HELP_T idx, int16_t x, int16_t y);
void    decodePieces(CODE_T *pCode);

void    initGallery(void);
MODE_T  updateGallery(void);
void    drawGallery(void);
void    setGalleryIndex(uint8_t idx);

/*  Global Variables  */

extern MyArduboy arduboy;
extern bool     isDirty;
extern PIECE_T  pieceAry[PIECES];
extern bool     isHelpVisible;
extern uint8_t  clearCount;
extern uint32_t playFrames;
extern int8_t   padX, padY, padRepeatCount;
extern int8_t   helpX, helpY;

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
