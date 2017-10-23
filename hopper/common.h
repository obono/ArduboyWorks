#ifndef COMMON_H
#define COMMON_H

#include "MyArduboy.h"

/*  Defines  */

#define APP_INFO        "OBN-Y02 VER 0.11"

#define rnd(val)        (rand() % (val))
#define mod(value, div) (((value) + div) % div)

/*  Typedefs  */

typedef unsigned char   uchar;
typedef unsigned int    uint;

/*  Global Functions  */

void initLogo(void);
bool updateLogo(void);
void drawLogo(void);

void initTitle(void);
bool updateTitle(void);
void drawTitle(void);
uint8_t setLastScore(int score, uint32_t time);

void initGame(void);
bool updateGame(void);
void drawGame(void);

/*  Global Variables  */

extern MyArduboy arduboy;

#endif // COMMON_H
