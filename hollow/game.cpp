#include "common.h"

typedef struct {
    uchar top;
    uchar bottom;
} COLUMN;

PROGMEM static const uchar imgPlayerLeft0[] = {
    0x10, 0x86, 0x89, 0x39, 0x3F, 0xBF, 0x86, 0x10
};
PROGMEM static const uchar imgPlayerLeft1[] = {
    0x00, 0x16, 0x49, 0x39, 0xBF, 0xBF, 0x06, 0x20
};
PROGMEM static const uchar imgPlayerLeft2[] = {
    0x00, 0x26, 0x09, 0xB9, 0xBF, 0x3F, 0x06, 0x20
};
PROGMEM static const uchar imgPlayerLeft3[] = {
    0x20, 0x0C, 0x92, 0xB2, 0x3E, 0x3E, 0xCC, 0x10
};
PROGMEM static const uchar imgPlayerRight0[] = {
    0x10, 0x86, 0xBF, 0x3F, 0x39, 0x89, 0x86, 0x10
};
PROGMEM static const uchar imgPlayerRight1[] = {
    0x20, 0x06, 0xBF, 0xBF, 0x39, 0x49, 0x16, 0x00
};
PROGMEM static const uchar imgPlayerRight2[] = {
    0x20, 0x06, 0x3F, 0xBF, 0xB9, 0x09, 0x26, 0x00
};
PROGMEM static const uchar imgPlayerRight3[] = {
    0x10, 0xCC, 0x3E, 0x3E, 0xB2, 0x92, 0x0C, 0x20
};

static const uchar *imgPlayerAry[] = {
    imgPlayerLeft0, imgPlayerLeft1, imgPlayerLeft2, imgPlayerLeft3,
    imgPlayerRight0, imgPlayerRight1, imgPlayerRight2, imgPlayerRight3,
};

static bool isHollow();
static void set(COLUMN &newColumn, uchar top, uchar bottom);
static void grow(COLUMN &newColumn, bool isSpace, COLUMN &lastColumn);

static COLUMN column[18];
static int columnOffset;
static int wavePhase;
static int caveMaxGap;
static int caveGap;
static int score;
static int px, pc, pa, pd, pj;
static int cm;

void initGame()
{
    for (int i = 0; i < 9 ; i++) {
        set(column[i], 32, 40);
    }
    for (int i = 9; i < 18; i++) {
        grow(column[i], isHollow(), column[i - 1]);
    }
    columnOffset = 0;
    wavePhase = 0;
    caveMaxGap = 16;
    score = 0;
    px = 0;
    pc = 1;
    pd = 1;
    pa = 0;
    pj = 0;
    cm = 0;
}

bool updateGame()
{
    wavePhase = (wavePhase + 1) % 384;
    caveGap = (1.0 - cos(wavePhase * 3.14159 / 192.0)) * 16;

    if (pa == 0) {
        int vx = 0;
        if (arduboy.pressed(LEFT_BUTTON)) vx--;
        if (arduboy.pressed(RIGHT_BUTTON)) vx++;
        if (vx < 0) {
            pd = 0;
            if (px == 0) vx = 0;
        } else if (vx > 0) {
            pd = 1;
        }
        if (vx != 0) {
            int nextCol = (pc + vx + 18) % 18;
            int gap = min(column[pc].bottom - column[nextCol].top, column[nextCol].bottom - column[pc].top);
            if (gap + caveGap >= 8) {
                pj = column[pc].bottom - column[nextCol].bottom;
                pc = nextCol;
                pa = 8;
                if (px + vx > 56) {
                    int growCol = columnOffset / 8;
                    grow(column[growCol], isHollow(), column[(growCol > 0) ? growCol - 1 : 17]);
                    score++;
                }
            }
        }
    }

    if (pa > 0) {
        pa--;
        px += pd * 2 - 1;
        if (px > 56) {
            px--;
            columnOffset = (columnOffset + 1) % 144;
        }
    }

    if (wavePhase == 0) {
        if (column[pc].bottom - column[pc].top < 4) {
            return true;
        }
        caveMaxGap++;
    }
    return false;
}

void drawGame()
{
    arduboy.clear();

    /*  Cave  */
    int sh = (wavePhase > 374) ? wavePhase % 2 : 0;
    for (int i = 0; i < 18; i++) {
        int x = i * 8 - columnOffset - 8;
        if (x <= -8) x += 144;
        arduboy.drawRect(x, -(caveGap + 1) / 2 + sh, 8, column[i].top, WHITE);
        arduboy.drawRect(x, column[i].bottom + caveGap / 2 + sh, 8, 64 - column[i].bottom, WHITE);
    }

    /*  Player  */
    int y = max(column[pc].bottom + pj * pa / 8 + caveGap / 2 - 8, column[pc].top -(caveGap + 1) / 2);
    if (y > 56) y = 56;
    arduboy.drawBitmap(px, y + sh, imgPlayerAry[pd * 4 + pa / 2], 8, 8, WHITE);

    /*  Score  */
    arduboy.setCursor(0, 0);
    arduboy.print(score);
}

static bool isHollow() {
    if (--cm < 0) {
        cm = ((rand() + 32768) * score >> 22) + 1;
        return true;
    }
    return false;
}

static void set(COLUMN &newColumn, uchar top, uchar bottom)
{
    newColumn.top = top;
    newColumn.bottom = bottom;
}

static void grow(COLUMN &newColumn, bool isSpace, COLUMN &lastColumn)
{
    int lastDiff = lastColumn.bottom - lastColumn.top;
    int curDiff = rand() % 3;
    if (isSpace) curDiff = 8 - curDiff;
    int adjust = (lastColumn.bottom - 34) / 5;
    int change = rand() % (17 - abs(curDiff - lastDiff) - abs(adjust)) - 8;
    if (curDiff > lastDiff) change += curDiff - lastDiff;
    if (adjust < 0) change -= adjust;
    int bottom = lastColumn.bottom + change;
    if (bottom < 16) bottom = 16;
    if (bottom > 56) bottom = 56;
    newColumn.top = bottom - curDiff;
    newColumn.bottom = bottom; 
}

