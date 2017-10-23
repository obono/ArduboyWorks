#include "common.h"

/*  Defines  */

#define FLOORS_MANAGE   10
#define FLOORS_LEVEL    50

#define FLRTYPE_NONE    0
#define FLRTYPE_NORMAL  1
#define FLRTYPE_MOVE    2
#define FLRTYPE_VANISH  3
#define FLRTYPE_SMALL   4
#define FLRTYPE_GOAL    5

#define CENTER_X        64
#define CENTER_Y        32
#define PI              3.141592653589793

#define STATE_START 0
#define STATE_GAME  1
#define STATE_CLEAR 2
#define STATE_OVER  3

#define nextFloorIdx(i) ((i + 1) % FLOORS_MANAGE)

/*  Typedefs  */

typedef struct {
    uint8_t type;
    uint8_t pos;
    int16_t x, y, z;
    int16_t vx, vy;
    uint8_t size, sizeBack;
} FLOOR;

/*  Local Functions  */

static void initLevel(bool isStart);
static void movePlayer(void);
static void moveFloors(int scrollX, int scrollY, int scrollZ);
static void drawFloors(void);
static void addFloor(int z, uint8_t pos);
static void drawPlayer(void);
static void drawStrings(void);
static void fillPatternedRect(int16_t x, int16_t y, uint8_t w, int8_t h, const byte *ptn);

/*  Local Variables  */

PROGMEM static const byte floorPtn[][4] = {
    { 0x55, 0xAA, 0x55, 0xAA },
    { 0x33, 0x66, 0xCC, 0x99 },
    { 0x00, 0xAA, 0x00, 0xAA },
    { 0x55, 0xFF, 0x55, 0xFF }
};

PROGMEM static const byte soundStart[] = {
    0x90, 72, 0, 100, 0x80, 0, 25,
    0x90, 74, 0, 100, 0x80, 0, 25,
    0x90, 76, 0, 100, 0x80, 0, 25,
    0x90, 77, 0, 100, 0x80, 0, 25,
    0x90, 79, 0, 200, 0x80, 0xF0
};

PROGMEM static const byte soundGameOver[] = {
    0x90, 55, 0, 120, 0x80, 0, 10,
    0x90, 54, 0, 140, 0x80, 0, 20,
    0x90, 53, 0, 160, 0x80, 0, 30,
    0x90, 52, 0, 180, 0x80, 0, 40,
    0x90, 51, 0, 200, 0x80, 0, 50,
    0x90, 50, 0, 220, 0x80, 0, 60,
    0x90, 49, 0, 240, 0x80, 0, 70,
    0x90, 48, 0, 260, 0x80, 0xF0
};

static uint8_t  state;
static uint32_t gameFrames;
static int      counter;
static uint     score;
static uint     level;
static bool     isHiscore;
static int      playerX, playerY;
static int8_t   playerJump;
static FLOOR    floorDto[FLOORS_MANAGE];
static uint8_t  floorIdxFirst, floorIdxLast;
static uint8_t  flrTypes[4], flrTypesNum;

/*---------------------------------------------------------------------------*/

void initGame(void)
{
    playerX = 0;
    playerY = 0;
    playerJump = 0;
    initLevel(true);
}

bool updateGame(void)
{
    int scrollX = -playerX / 4;
    int scrollY = -playerY / 4;

    playerX += scrollX;
    playerY += scrollY;
    if (playerJump > -64) {
        playerJump--;
        if (state == STATE_CLEAR && playerJump == 0) {
            initLevel(false);
        }
    }
    if (state != STATE_OVER) {
        moveFloors(scrollX, scrollY, playerJump);
    }

    if (state == STATE_GAME || state == STATE_OVER) {
        movePlayer();
        if (state == STATE_GAME) {
            gameFrames++;
            if (floorDto[floorIdxFirst].z < 0) {
                state = STATE_OVER;
                isHiscore = (setLastScore(score, gameFrames) == 0);
                counter = 480; // 8 secs
                arduboy.playScore2(soundGameOver, 1);
            }
        } else {
            counter--;
            if (arduboy.buttonDown(A_BUTTON | B_BUTTON)) {
                initLevel(true);
            }
        }
    }

    return (state == STATE_OVER && counter == 0);
}

void drawGame(void)
{
    arduboy.clear();
    drawFloors();
    drawPlayer();
    drawStrings();
}

/*---------------------------------------------------------------------------*/

void initLevel(bool isStart) {
    if (isStart) {
        state = STATE_START;
        gameFrames = 0;
        score = 0;
        level = 0;
        arduboy.playScore2(soundStart, 0);
    } else {
        level++;
    }

    flrTypesNum = 0;
    if (level < 6) flrTypes[flrTypesNum++] = FLRTYPE_NORMAL;
    if (level & 1) flrTypes[flrTypesNum++] = FLRTYPE_MOVE;
    if (level & 2) flrTypes[flrTypesNum++] = FLRTYPE_VANISH;
    if (level & 4 || level == 8) flrTypes[flrTypesNum++] = FLRTYPE_SMALL;

    for (int i = 0; i < FLOORS_MANAGE; i++) {
        floorDto[i].type = FLRTYPE_NONE;
    }

    floorIdxFirst = 0;
    floorIdxLast = 0;
    FLOOR *pFloor = &floorDto[0];
    pFloor->type = FLRTYPE_NORMAL;
    pFloor->pos = 0;
    pFloor->x = playerX;
    pFloor->y = playerY;
    pFloor->z = 4096;
    pFloor->vx = 0;
    pFloor->vy = 0;
    pFloor->size = max(64 - level * 3, 16);
    pFloor->sizeBack = pFloor->size;
}


/*---------------------------------------------------------------------------*/

void movePlayer(void)
{
    int vx = 0, vy = 0, vr;
    if (arduboy.buttonPressed(LEFT_BUTTON)) vx--;
    if (arduboy.buttonPressed(RIGHT_BUTTON)) vx++;
    if (arduboy.buttonPressed(UP_BUTTON)) vy--;
    if (arduboy.buttonPressed(DOWN_BUTTON)) vy++;
    vr = (vx != 0 && vy != 0) ? 384 : 512;
    playerX += vx * vr;
    playerY += vy * vr;
}

void moveFloors(int scrollX, int scrollY, int scrollZ)
{
    FLOOR *pFloor;
    for (int i = 0; i < FLOORS_MANAGE; i++) {
        pFloor = &floorDto[i];
        if (pFloor->type != FLRTYPE_NONE) {
            pFloor->x += scrollX + pFloor->vx;
            pFloor->y += scrollY + pFloor->vy;
            pFloor->z += scrollZ;
            int z = pFloor->z;
            if (pFloor->type == FLRTYPE_VANISH) {
                if (z < 0) {
                    pFloor->size = pFloor->sizeBack;
                } else if (pFloor->size != pFloor->sizeBack && pFloor->size > 0) {
                    pFloor->size--;
                }
            }
            if (scrollZ <= 0 && z >= 0 && z + scrollZ - 1 < 0) {
                int diff = max(abs(pFloor->x - playerX), abs(pFloor->y - playerY));
                if (diff < pFloor->size * 128) {
                    score = max(score, level * FLOORS_LEVEL + pFloor->pos);
                    if (pFloor->type == FLRTYPE_GOAL) {
                        playerJump = 96;
                        state = STATE_CLEAR;
                    } else {
                        playerJump = 64;
                        if (pFloor->type == FLRTYPE_VANISH) {
                            pFloor->size--;
                        }
                        if (state == STATE_START || state == STATE_CLEAR) {
                            state = STATE_GAME;
                        }
                    }
                }
            }
        }
    }

    pFloor = &floorDto[floorIdxFirst];
    if (pFloor->z > 4096) {
        pFloor->type = FLRTYPE_NONE;
        floorIdxFirst = nextFloorIdx(floorIdxFirst);
    }

    int z = floorDto[floorIdxLast].z;
    if (state == STATE_GAME && z >= 0 && floorDto[floorIdxLast].type != FLRTYPE_GOAL) {
        addFloor(z, floorDto[floorIdxLast].pos + 1);
    }
}

void addFloor(int z, uint8_t pos)
{
    floorIdxLast = nextFloorIdx(floorIdxLast);
    FLOOR *pFloor = &floorDto[floorIdxLast];

    uint8_t type = (pos == FLOORS_LEVEL) ? FLRTYPE_GOAL : flrTypes[rnd(flrTypesNum)];
    pFloor->type = type;
    pFloor->pos = pos;
    pFloor->x = rand() * 2;
    pFloor->y = rand() * 2;
    pFloor->z = z - 512 - level * 32;

    double vd = rnd(256) * PI / 128.0;
    int vr = (type == FLRTYPE_MOVE) ? 256 + level * 16 : 0;
    pFloor->vx = cos(vd) * vr;
    pFloor->vy = sin(vd) * vr;

    uint8_t size = (type == FLRTYPE_SMALL) ? 36 - level : 64 - level * 3;
    pFloor->size = max(size, 16);
    pFloor->sizeBack = pFloor->size;
}

/*---------------------------------------------------------------------------*/

void drawFloors(void)
{
    for (int i = 0; i < FLOORS_MANAGE; i++) {
        FLOOR   *pFloor = &floorDto[(floorIdxFirst + i) % FLOORS_MANAGE];
        uint8_t type = pFloor->type;
        if (pFloor->type == FLRTYPE_NONE) break;
        if (pFloor->z < 0 || pFloor->size == 0) continue;
        int16_t q = 256 + pFloor->z;
        uint8_t s = pFloor->size * 256 / q;
        int16_t x = pFloor->x / q + CENTER_X - s / 2;
        int16_t y = pFloor->y / q + CENTER_Y - s / 2;
        if (type == FLRTYPE_GOAL) {
            arduboy.fillRect2(x, y, s, s, playerJump % 2);
        } else {
            fillPatternedRect(x, y, s, s, floorPtn[type - 1]);
        }
        arduboy.drawRect2(x, y, s, s, WHITE);
    }
}

void drawPlayer(void)
{
    arduboy.fillRect(playerX / 256 + CENTER_X - 3, playerY / 256 + CENTER_Y - 3, 7, 7, WHITE);
}

void drawStrings(void)
{
    if (state == STATE_START) {
        arduboy.printEx(46, 18, F("READY?"));
    } else {
        arduboy.printEx(0, 0, F("SCORE"));
        arduboy.setCursor(0, 6);
        arduboy.print(score);
        arduboy.printEx(98, 0, F("LEVEL"));
        arduboy.setCursor(122, 6);
        arduboy.print(level + 1);
        if (state == STATE_CLEAR) {
            arduboy.printEx(34, 18, F("EXCELLENT!"));
            arduboy.printEx(22, 40, F("TRY NEXT LEVEL"));
        } else if (state == STATE_OVER) {
            arduboy.printEx(37, 18, F("GAME OVER"));
            if (isHiscore && counter % 8 < 4) {
                arduboy.printEx(31, 40, F("NEW RECORD!"));
            }
        }
    }
}

void fillPatternedRect(int16_t x, int16_t y, uint8_t w, int8_t h, const byte *ptn)
{
    /*  Check parameters  */
    if (x < 0) {
        if (w <= -x) return;
        w += x;
        x = 0;
    }
    if (y < 0) {
        if (h <= -y) return;
        h += y;
        y = 0;
    }
    if (w <= 0 || x >= WIDTH || h <= 0 || y >= HEIGHT) return;
    if (x + w > WIDTH) w = WIDTH - x;
    if (y + h > HEIGHT) h = HEIGHT - y;

    /*  Draw a patterned rectangle  */
    uint8_t yOdd = y & 7;
    uchar d = 0xFF << yOdd;
    y -= yOdd;
    h += yOdd;
    for (uchar *p = arduboy.getBuffer() + x + (y / 8) * WIDTH; h > 0; h -= 8, p += WIDTH - w) {
        if (h < 8) d &= 0xFF >> (8 - h);
        for (uint8_t i = w; i > 0; i--, p++) {
            *p = *p & ~d | pgm_read_byte(ptn + (int) p % 4) & d;
        }
        d = 0xFF;
    }
}
