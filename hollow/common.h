#ifndef COMMON_H
#define COMMON_H

#include "MyArduboy.h"

/*  Defines  */

#define APP_INFO        "OBN-Y01 VER 0.30"

#define rnd(val)        (rand() % (val))
#define mod(value, div) (((value) + div) % div)

/*  Typedefs  */

typedef unsigned char   uchar;
typedef unsigned int    uint;

/*  Global Functions  */

void initLogo();
bool updateLogo();
void drawLogo();

void initTitle();
bool updateTitle();
void drawTitle();
uint8_t setLastScore(int score, uint32_t time);

void initGame();
bool updateGame();
void drawGame();

/*  Global Variables  */

extern MyArduboy arduboy;

#endif // COMMON_H
