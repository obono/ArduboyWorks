#include "common.h"
#include "data.h"

/*  Defines  */

#define LASERS_MAX  8

#define LASER_X_MAX 2048
#define LASER_W     48

#define RAY_VD_COEF (DEG_TO_RAD / 8.0)
#define RAY_D_MAX   360
#define RAY_R_MAX   144
#define RAY_R_DEC   4

enum STATE_T {
    STATE_INIT = 0,
    STATE_PLAYING,
    STATE_MENU,
    STATE_OVER,
    STATE_LEAVE,
};

/*  Typedefs  */

typedef struct {
    int16_t x;
    int8_t  vx;
    int8_t  a;
    int8_t  b;
    float   d;
    int8_t  vd;
    uint8_t r;
    bool    isWhite;
} LASER_T;

/*  Local Functions  */

static void movePlayer(void);
static void moveLasers(void);
static void moveLaser(LASER_T *p);
static void newLaser(LASER_T *p);
static int16_t calcLaserY(LASER_T *p);

static void clearScreen(bool isBlink);
static void drawPlayer(void);
static void drawLasers(void);
static void drawRay(LASER_T *p);
static void drawLaser(LASER_T *p);
static void drawStrings(void);

/*  Local Variables  */

static STATE_T  state = STATE_INIT;
static LASER_T  lasers[LASERS_MAX];
static uint16_t score;
static int16_t  playerX, playerY;
static int8_t   playerVx, playerVy, playerMoving;
static bool     isPlayerWhite;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initGame(void)
{
    record.playCount++;
    isRecordDirty = true;
    writeRecord();

    for (LASER_T *p = &lasers[0]; p < &lasers[LASERS_MAX]; p++) {
        newLaser(p);
    }
    score = 0;
    playerX = 1024;
    playerY = 512;
    playerMoving = 0;
    isPlayerWhite = true;
    state = STATE_PLAYING;
    arduboy.playScore2(soundStart, 0);
}

MODE_T updateGame(void)
{
    switch (state) {
    case STATE_PLAYING:
        record.playFrames++;
        score++;
        isRecordDirty = true;
        movePlayer();
        moveLasers();
        if (arduboy.buttonDown(A_BUTTON)) state = STATE_LEAVE;
        break;
    case STATE_MENU:
        handleMenu();
        break;
    default:
        break;
    }
    return (state == STATE_LEAVE) ? MODE_TITLE : MODE_GAME;
}

void drawGame(void)
{
    clearScreenGray();
    if (state == STATE_LEAVE) return;
    drawLasers();
    drawPlayer();
    drawStrings();
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void movePlayer(void)
{
    playerVx = 0;
    playerVy = 0;
    if (arduboy.buttonPressed(LEFT_BUTTON))  playerVx--;
    if (arduboy.buttonPressed(RIGHT_BUTTON)) playerVx++;
    if (arduboy.buttonPressed(UP_BUTTON))    playerVy--;
    if (arduboy.buttonPressed(DOWN_BUTTON))  playerVy++;
    int8_t vr = (playerVx * playerVy == 0) ? 8 : 6;
    playerX += playerVx * vr;
    playerY += playerVy * vr;
    playerMoving = (playerMoving + 1) & 15;
    if (playerVx == 0 && playerVy == 0) playerMoving = 0;
    if (arduboy.buttonDown(B_BUTTON)) isPlayerWhite = !isPlayerWhite;
}

static void moveLasers(void)
{
    for (LASER_T *p = &lasers[0]; p < &lasers[LASERS_MAX]; p++) {
        if (p->vx) moveLaser(p);
    }
}

static void moveLaser(LASER_T *p)
{
    p->x += p->vx;
    p->d += p->vd * RAY_VD_COEF;
    if (p->d < 0)      p->d += TWO_PI;
    if (p->d > TWO_PI) p->d -= TWO_PI;
    if (p->x < 0 && p->vx < 0 || p->x >= LASER_X_MAX && p->vx > 0) {
        if (p->r < RAY_R_DEC) {
            newLaser(p);
        } else {
            p->r -= RAY_R_DEC;
        }
    } else {
        if (p->r < RAY_R_MAX) p->r++;
    }
}

static void newLaser(LASER_T *p)
{
    p->x = random(2) * (LASER_X_MAX + LASER_W * 2) - LASER_W;
    p->vx = random(4, 13) * ((p->x <= 0) * 2 - 1);
    p->b = random(HEIGHT);
    p->a = random(HEIGHT) - p->b;
    p->d = random(RAY_D_MAX) * DEG_TO_RAD;
    p->vd = random(8, 17) * (random(2) * 2 - 1);
    p->r = 0;
    p->isWhite = random(2);
}

static int16_t calcLaserY(LASER_T *p)
{
    return ((p->x >> 4) * p->a >> 3) + (p->b << 4);
}

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void drawPlayer(void)
{
    int16_t x = playerX >> 4;
    int16_t y = (playerY >> 4) - (playerMoving >= 8);
    arduboy.drawBitmap(x - 4, y - 3, imgPlayerFrame, 9, 8, !isPlayerWhite);
    arduboy.drawBitmap(x - 3, y - 2, imgPlayerBody, 7, 6, isPlayerWhite);
    arduboy.drawBitmap(x - 1 + playerVx, y - 1 + playerVy, imgPlayerFace, 3, 4, !isPlayerWhite);
}

static void drawLasers(void)
{
    for (LASER_T *p = &lasers[0]; p < &lasers[LASERS_MAX]; p++) {
        if (p->vx) drawRay(p);
    }
    for (LASER_T *p = &lasers[0]; p < &lasers[LASERS_MAX]; p++) {
        if (p->vx) drawLaser(p);
    }
}

static void drawRay(LASER_T *p)
{
    int16_t x = (p->x >> 4);
    int16_t y = (calcLaserY(p) >> 4);
    arduboy.drawLine(x, y, x + cos(p->d) * p->r, y - sin(p->d) * p->r, p->isWhite);
}

static void drawLaser(LASER_T *p)
{
    int16_t x = (p->x >> 4) - 3;
    int16_t y = (calcLaserY(p) >> 4) - 3;
    uint8_t idx = (uint8_t) (p->d * RAD_TO_DEG / 15.0 + 0.5) % 24;
    arduboy.drawBitmap(x, y, imgLaserBase[idx], 7, 7, p->isWhite);
    arduboy.drawBitmap(x + 2, y, imgLaserNeck[idx], 3, 7, !p->isWhite);
}

static void drawStrings(void)
{
    arduboy.printEx(0, 0, F("SCORE"));
    drawNumber(0, 6, score);
}
