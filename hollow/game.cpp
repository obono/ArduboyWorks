#include "common.h"

/*  Defines  */

#define PI 3.14159

#define rnd(val)        (rand() % (val))
#define mod(value, div) (((value) + div) % div)

/*  Typedefs  */

typedef struct {
    uchar   top;
    uchar   bottom;
} COLUMN;

/*  Local Functions  */

static bool isHollow();
static void setColumn(COLUMN &newColumn, uchar top, uchar bottom);
static void growColumn(COLUMN &newColumn, bool isSpace, COLUMN &lastColumn);

/*  Local Variables  */

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

static COLUMN   column[18];
static bool     isStart;
static bool     isOver;
static int      score;
static uchar    counter;
static uchar    caveOffset;  // 0...143
static int      cavePhase;   // 0...383
static char     caveHollowCnt;
static uchar    caveMaxGap;
static int      caveGap;
static uchar    playerX;
static uchar    playerColumn;
static uchar    playerMove;  // 0...8
static uchar    playerDir;   // 0:left / 1:right
static char     playerJump;  // -8...8

/*---------------------------------------------------------------------------*/

void initGame()
{
    for (int i = 0; i < 9 ; i++) {
        setColumn(column[i], 32, 40);
    }
    for (int i = 9; i < 18; i++) {
        growColumn(column[i], isHollow(), column[i - 1]);
    }
    isStart = true;
    isOver = false;
    counter = 120; // 2 secs
    score = 0;
    caveOffset = 0;
    cavePhase = 0;
    caveHollowCnt = 0;
    caveMaxGap = 16;
    playerX = 0;
    playerColumn = 1;
    playerDir = 1;
    playerMove = 0;
    playerJump = 0;
}

bool updateGame()
{
    if (isStart || isOver) {
        if (--counter == 0) isStart = false;
    }
    if (!isStart) cavePhase = mod(cavePhase + 1, 384);
    caveGap = (1.0 - cos(cavePhase * PI / 192.0)) * caveMaxGap;

    /*  Key handling  */
    if (!isOver && playerMove == 0) {
        int vx = 0;
        if (arduboy.pressed(LEFT_BUTTON)) vx--;
        if (arduboy.pressed(RIGHT_BUTTON) || isStart) vx++;
        if (vx < 0) {
            playerDir = 0;
            if (playerX == 0) vx = 0;
        } else if (vx > 0) {
            playerDir = 1;
        }
        if (vx != 0) {
            int nextCol = mod(playerColumn + vx, 18);
            int nextGap = min(column[playerColumn].bottom - column[nextCol].top,
                    column[nextCol].bottom - column[playerColumn].top);
            if (nextGap + caveGap >= 8) {
                playerJump = column[playerColumn].bottom - column[nextCol].bottom;
                playerColumn = nextCol;
                playerMove = 8;
                if (playerX + vx > 56) {
                    int growCol = caveOffset / 8;
                    growColumn(column[growCol], isHollow(), column[mod(growCol - 1, 18)]);
                    score++;
                }
            }
        }
    }

    /*  Move and scroll  */
    if (playerMove > 0) {
        playerMove--;
        playerX += playerDir * 2 - 1;
        if (playerX > 56) {
            playerX--;
            caveOffset = mod(caveOffset + 1, 144);
        }
    }

    /*  Judge game over  */
    if (!isStart && cavePhase == 0) {
        if (column[playerColumn].bottom - column[playerColumn].top < 4) {
            isOver = true;
            counter = 240; // 4 secs
        }
        caveMaxGap++;
    }

    return (isOver && counter == 0);
}

void drawGame()
{
    arduboy.clear();

    /*  Cave  */
    int shake = (cavePhase > 374) ? cavePhase % 2 : 0;
    int offsetTop = -(caveGap + 1) / 2 + shake;
    int offsetBottom = caveGap / 2 + shake;
    for (int i = 0; i < 18; i++) {
        int x = i * 8 - caveOffset - 8;
        if (x <= -8) x += 144;
        arduboy.drawRect(x, offsetTop, 8, column[i].top, WHITE);
        arduboy.drawRect(x, column[i].bottom + offsetBottom, 8, 64 - column[i].bottom, WHITE);
    }

    /*  Player  */
    int playerY = column[playerColumn].bottom + playerJump * playerMove / 8 - 8;
    playerY = max(playerY + offsetBottom, column[playerColumn].top + offsetTop);
    if (playerY > 56) playerY = 56;
    if (isOver) playerY = column[playerColumn].top + offsetBottom;
    arduboy.drawBitmap(playerX, playerY, imgPlayerAry[playerDir * 4 + playerMove / 2], 8, 8, WHITE);

    /*  Score  */
    if (!isStart) {
        arduboy.setCursor(0, 0);
        arduboy.print(score);
    }

    /*  Message  */
    if (isStart) {
        arduboy.setCursor(46, 24);
        arduboy.print(F("Ready?"));
    } else if (isOver) {
        arduboy.setCursor(37, 24);
        arduboy.print(F("Game Over"));
    }
}

/*---------------------------------------------------------------------------*/

static bool isHollow() {
    if (--caveHollowCnt < 0) {
        caveHollowCnt = ((rand() + 32768) * score >> 22) + 1;
        return true;
    }
    return false;
}

static void setColumn(COLUMN &newColumn, uchar top, uchar bottom)
{
    newColumn.top = top;
    newColumn.bottom = bottom;
}

static void growColumn(COLUMN &newColumn, bool isSpace, COLUMN &lastColumn)
{
    int lastDiff = lastColumn.bottom - lastColumn.top;
    int curDiff = rnd(3);

    if (isSpace) curDiff = 8 - curDiff;
    int adjust = (lastColumn.bottom - 34) / 5;
    int change = rnd(17 - abs(curDiff - lastDiff) - abs(adjust)) - 8;
    if (curDiff > lastDiff) change += curDiff - lastDiff;
    if (adjust < 0) change -= adjust;
    int bottom = lastColumn.bottom + change;
    if (bottom < 16) bottom = 16;
    if (bottom > 56) bottom = 56;
    newColumn.top = bottom - curDiff;
    newColumn.bottom = bottom; 
}
