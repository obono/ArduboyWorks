#ifndef COMMON_H
#define COMMON_H

#include "MyArduboy.h"

/*  Defines  */

#define APP_INFO "OBN-Y01 VER 0.17"

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

void initGame();
bool updateGame();
void drawGame();

/*  Global Variables  */

extern MyArduboy arduboy;

#endif // COMMON_H
