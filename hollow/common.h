#ifndef __COMMON_H__
#define __COMMON_H__

#include <Arduboy.h>

/*  Defines  */

#define APP_INFO "OBN-Y01 VER 0.12"

/*  Typedefs  */

typedef uint8_t     uchar;
typedef uint16_t    uint;

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
