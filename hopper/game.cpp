#include "common.h"

/*  Defines  */

#define FLOORS_MANAGE   10
#define FLOORS_LEVEL    50
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
    int8_t  vx, vy;
    uint8_t size;
    uint8_t count;
} FLOOR;

/*  Local Functions  */

static void initLevel(bool isStart);
static void movePlayer(void);
static void moveFloors(int scrollX, int scrollY, int scrollZ);
static void drawFloors(void);
static void drawPlayer(void);
static void drawStrings(void);

/*  Local Variables  */

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
    }

    for (int i = 0; i < FLOORS_MANAGE; i++) {
        floorDto[i].type = 0;
    }

    floorIdxFirst = 0;
    floorIdxLast = 0;
    FLOOR *pFloor = &floorDto[0];
    pFloor->type = 1;
    pFloor->pos = 0;
    pFloor->x = playerX;
    pFloor->y = playerY;
    pFloor->z = 4096;
    pFloor->size = 64 - level * 3;
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
        if (pFloor->type > 0) {
            pFloor->x += scrollX;
            pFloor->y += scrollY;
            pFloor->z += scrollZ;
            int z = pFloor->z;
            int diff = max(abs(pFloor->x - playerX), abs(pFloor->y - playerY));
            if (playerJump <= 0 && z >= 0 && z + playerJump - 1 < 0 && diff < pFloor->size * 128) {
                uint8_t pos = pFloor->pos;
                score = max(score, level * FLOORS_LEVEL + pos);
                if (pos == FLOORS_LEVEL) {
                    playerJump = 96;
                    state = STATE_CLEAR;
                } else {
                    playerJump = 64;
                    if (state == STATE_START || state == STATE_CLEAR) {
                        if (state == STATE_CLEAR) level++;
                        state = STATE_GAME;
                    }
                }
            }
        }
    }

    pFloor = &floorDto[floorIdxFirst];
    if (pFloor->z > 4096) {
        pFloor->type = 0;
        floorIdxFirst = nextFloorIdx(floorIdxFirst);
    }

    int     z = floorDto[floorIdxLast].z;
    uint8_t pos = floorDto[floorIdxLast].pos;
    if (state == STATE_GAME && pos < FLOORS_LEVEL && z >= 0) {
        floorIdxLast = nextFloorIdx(floorIdxLast);
        pFloor = &floorDto[floorIdxLast];
        pFloor->type = 1;
        pFloor->pos = pos + 1;
        pFloor->x = rand() * 2;
        pFloor->y = rand() * 2;
        pFloor->z = z - 512 - level * 32;
        pFloor->size = 64 - level * 3;
    }
}

/*---------------------------------------------------------------------------*/

void drawFloors(void)
{
    for (int idx = floorIdxFirst; idx != nextFloorIdx(floorIdxLast); idx = nextFloorIdx(idx)) {
        FLOOR *pFloor = &floorDto[idx];
        if (pFloor->type == 0 || pFloor->z < 0) continue;
        int16_t q = 256 + pFloor->z;
        uint8_t s = pFloor->size * 256 / q;
        int16_t x = pFloor->x / q + CENTER_X - s / 2;
        int16_t y = pFloor->y / q + CENTER_Y - s / 2;
        arduboy.fillRect2(x, y, s, s, BLACK);
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
        arduboy.printEx(0, 0, F("FLOOR "));
        arduboy.print(score);
        arduboy.setCursor(122, 0);
        arduboy.printEx(86, 0, F("LEVEL "));
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
