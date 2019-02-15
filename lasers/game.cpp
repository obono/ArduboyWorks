#include "common.h"
#include "data.h"

/*  Defines  */

#define LASERS_MAX      12

#define SCALE           16
#define SCALEF          16.0

#define PLAYER_X_MIN    (4 * SCALE)
#define PLAYER_X_MAX    ((WIDTH - 5) * SCALE)
#define PLAYER_Y_MIN    (4 * SCALE)
#define PLAYER_Y_MAX    ((HEIGHT - 5) * SCALE)
#define PLAYER_DIAM     (4 * SCALEF)

#define LASER_X_MAX     (WIDTH * SCALE)
#define LASER_DIAM      (3 * SCALE)
#define LASER_WAIT_MAX  90
#define LASER_WAIT_MIN  30

#define RAY_D_MAX       (360 * SCALE)
#define RAY_D_COEF      (DEG_TO_RAD / SCALEF)
#define RAY_VD_MIN      (1 * SCALE)
#define RAY_VD_MAX      (3 * SCALE)
#define RAY_R_MAX       144
#define RAY_R_INC       2
#define RAY_R_DEC       8

#define POWER_MAX       1000
#define DAMAGE_LASER    60
#define IMG_LASER_DIV   (RAY_D_MAX / IMG_LASER_ID_MAX)

#define descale(x)      ((x) >> 4)

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
    int16_t d;
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
static int16_t  power;
static int16_t  playerX, playerY;
static int16_t  laserCount, laserWait;
static uint8_t  gameFrames;
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

    score = 0;
    power = POWER_MAX;
    playerX = WIDTH * SCALE / 2;
    playerY = HEIGHT * SCALE / 2;
    playerMoving = 0;
    isPlayerWhite = true;
    gameFrames = 0;

    for (LASER_T *p = &lasers[0]; p < &lasers[LASERS_MAX]; p++) {
        p->vx = 0;
    }
    laserCount = 0;
    laserWait = 120;

    state = STATE_PLAYING;
    arduboy.playScore2(soundStart, SND_PRIO_START);
}

MODE_T updateGame(void)
{
    switch (state) {
    case STATE_PLAYING:
        record.playFrames++;
        isRecordDirty = true;
        gameFrames++;
        if (laserCount > 0 && (gameFrames & 7) == 0) score++;
        if (power < POWER_MAX) power++;
        movePlayer();
        moveLasers();
        if (power <= 0) {
            enterScore(score);
            writeRecord();
            state = STATE_OVER;
            arduboy.playScore2(soundOver, SND_PRIO_OVER);
            dprintln(F("Game over"));
        }
        break;
    case STATE_MENU:
        handleMenu();
        break;
    case STATE_OVER:
        moveLasers();
        if (arduboy.buttonDown(A_BUTTON)) state = STATE_LEAVE;
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
    /*  Direction  */
    playerVx = 0;
    playerVy = 0;
    if (arduboy.buttonPressed(LEFT_BUTTON))  playerVx--;
    if (arduboy.buttonPressed(RIGHT_BUTTON)) playerVx++;
    if (arduboy.buttonPressed(UP_BUTTON))    playerVy--;
    if (arduboy.buttonPressed(DOWN_BUTTON))  playerVy++;

    /*  Move & Boundary check  */
    int8_t vr = (playerVx * playerVy == 0) ? 8 : 6;
    playerX += playerVx * vr;
    if (playerX < PLAYER_X_MIN) playerX = PLAYER_X_MIN;
    if (playerX > PLAYER_X_MAX) playerX = PLAYER_X_MAX;
    playerY += playerVy * vr;
    if (playerY < PLAYER_Y_MIN) playerY = PLAYER_Y_MIN;
    if (playerY > PLAYER_Y_MAX) playerY = PLAYER_Y_MAX;

    /*  Animation  */  
    playerMoving = (playerMoving + 1) & 15;
    if (playerVx == 0 && playerVy == 0) playerMoving = 0;

    /*  Toggle color  */
    if (arduboy.buttonDown(B_BUTTON)) {
        isPlayerWhite = !isPlayerWhite;
    }
}

static void moveLasers(void)
{
    for (LASER_T *p = &lasers[0]; p < &lasers[LASERS_MAX]; p++) {
        if (p->vx) moveLaser(p);
    }
    if (--laserWait <= 0) {
        newLaser(&lasers[laserCount % LASERS_MAX]);
        laserCount++;
        laserWait = LASER_WAIT_MAX - laserCount / 4;
        if (laserWait < LASER_WAIT_MIN) laserWait = LASER_WAIT_MIN;
        dprint(F("New laser: "));
        dprintln(laserCount);
    }
}

static void moveLaser(LASER_T *p)
{
    /*  Move & Adjust ray length  */
    p->x += p->vx;
    p->d += p->vd;
    if (p->d < 0) p->d += RAY_D_MAX;
    if (p->d >= RAY_D_MAX) p->d -= RAY_D_MAX;
    if (p->x < 0 && p->vx < 0 || p->x >= LASER_X_MAX && p->vx > 0) {
        if (p->r < RAY_R_DEC) {
            p->vx = 0;
            p->r = 0;
        } else {
            p->r -= RAY_R_DEC;
        }
    } else {
        p->r += RAY_R_INC;
        if (p->r > RAY_R_MAX) p->r = RAY_R_MAX;
    }
    if (state == STATE_OVER) return;

    /*  Collision detection  */
    int16_t dx = playerX - p->x;
    int16_t dy = playerY - calcLaserY(p);
    float deg = p->d * RAY_D_COEF;
    float vx = cos(deg), vy = -sin(deg);
    float lx = dx * vx + dy * vy, ly = dx * vy - dy * vx;
    if (lx >= 0 && lx < p->r * SCALEF && abs(ly) < PLAYER_DIAM) {
        p->r = lx / SCALEF;
        if (isPlayerWhite == p->isWhite) {
            score++;
        } else {
            power -= DAMAGE_LASER;
        }
    }
}

static void newLaser(LASER_T *p)
{
    p->x = random(2) * (LASER_X_MAX + LASER_DIAM * 2) - LASER_DIAM;
    p->vx = random(6, 11) * ((p->x <= 0) * 2 - 1);
    p->b = random(HEIGHT);
    p->a = random(HEIGHT) - p->b;
    p->d = random(RAY_D_MAX) * DEG_TO_RAD;
    p->vd = random(RAY_VD_MIN, RAY_VD_MAX + 1) * (random(2) * 2 - 1);
    p->r = 0;
    p->isWhite = random(2);
}

static int16_t calcLaserY(LASER_T *p)
{
    return (descale(p->x) * p->a >> 3) + (p->b << 4);
}

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void drawPlayer(void)
{
    if (power <= 0) return;
    int16_t x = descale(playerX);
    int16_t y = descale(playerY) - (playerMoving >= 8);
    arduboy.drawBitmap(x - 4, y - 3, imgPlayerFrame, 9, 8, !isPlayerWhite);
    arduboy.drawBitmap(x - 3, y - 2, imgPlayerBody, 7, 6, isPlayerWhite);
    arduboy.drawBitmap(x - 1 + playerVx, y - 1 + playerVy, imgPlayerFace, 3, 4, !isPlayerWhite);
    if (power < POWER_MAX) {
        drawNumber(x - 5, y - 9, power / 10);
    }
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
    int16_t x = descale(p->x);
    int16_t y = descale(calcLaserY(p));
    float deg = p->d * RAY_D_COEF;
    arduboy.drawLine(x, y, x + cos(deg) * p->r, y - sin(deg) * p->r, p->isWhite);
}

static void drawLaser(LASER_T *p)
{
    int16_t x = descale(p->x);
    int16_t y = descale(calcLaserY(p));
    uint8_t idx = (p->d + IMG_LASER_DIV / 2) / IMG_LASER_DIV % IMG_LASER_ID_MAX;
    arduboy.drawBitmap(x - 3, y - 3, imgLaserBase[idx], 7, 7, p->isWhite);
    arduboy.drawBitmap(x - 1, y - 1, imgLaserNeck[idx], 3, 3, !p->isWhite);
}

static void drawStrings(void)
{
    if (laserCount == 0) {
        arduboy.printEx(46, 16, F("READY?"));
    } else {
        arduboy.printEx(0, 0, F("SCORE"));
        drawNumber(0, 6, score);
    }
    if (state == STATE_OVER) {
        arduboy.printEx(37, 29, F("GAME OVER"));
    }
}
