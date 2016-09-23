#ifndef __COMMON_H__
#define __COMMON_H__

#include <Arduboy.h>

/*  Defines  */

#define APP_INFO "OBN-Y01 VER 0.13"

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

extern Arduboy arduboy;

#endif // __COMMON_H__
