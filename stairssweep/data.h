#pragma once

#include "common.h"

/*---------------------------------------------------------------------------*/
/*                                Image Data                                 */
/*---------------------------------------------------------------------------*/

#define IMG_OBJECT_W        8
#define IMG_OBJECT_H        8
#define IMG_OBJECT_ID_MAX   6

PROGMEM static const uint8_t imgObject[IMG_OBJECT_ID_MAX][8] = { // 8x8 x6
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0xFF, 0xFF, 0xAA, 0xD5, 0xAA, 0xD5, 0xAA, 0x80 },
    { 0x3C, 0x76, 0xE7, 0xC7, 0xE7, 0xF7, 0x7E, 0x3C },
    { 0x3C, 0x6E, 0xE7, 0xE3, 0xE7, 0xEF, 0x7E, 0x3C },
    { 0x18, 0x18, 0xFF, 0x7E, 0x3C, 0x3C, 0x7E, 0x66 },
    { 0x24, 0x5A, 0x66, 0x81, 0x81, 0x42, 0x99, 0xE7 },
};

/*---------------------------------------------------------------------------*/
/*                                Sound Data                                 */
/*---------------------------------------------------------------------------*/

enum {
    SND_PRIO_START = 0,
    SND_PRIO_OVER,
};

PROGMEM static const byte soundStart[] = {
    0x90, 72, 0, 100, 0x90, 74, 0, 100, 0x90, 76, 0, 100,
    0x90, 77, 0, 100, 0x90, 79, 0, 200, 0x80, 0xF0
};

PROGMEM static const byte soundOver[] = {
    0x90, 55, 0, 120, 0x90, 54, 0, 140, 0x90, 53, 0, 160, 0x90, 52, 0, 180,
    0x90, 51, 0, 200, 0x90, 50, 0, 220, 0x90, 49, 0, 240, 0x90, 48, 0, 260, 0x80, 0xF0
};
