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

enum ALIGN_T {
    ALIGN_LEFT = 0,
    ALIGN_CENTER,
    ALIGN_RIGHT,
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
static void onContinue(void);
static void onQuit(void);

static void drawPlayer(void);
static void drawLasers(void);
static void drawRay(LASER_T *p);
static void drawLaser(LASER_T *p);
static void drawStrings(void);
static int8_t drawFigure(int16_t x, int16_t y, int value, ALIGN_T align);

/*  Local Variables  */

static STATE_T  state = STATE_INIT;
static LASER_T  lasers[LASERS_MAX];
static uint16_t score, gameFrames;
static int16_t  power, playerX, playerY;
static int16_t  laserCount, laserWait;
static int8_t   playerVx, playerVy, playerMoving, colorBias;
static int8_t   scoreX, scoreY, powerY;
static bool     isPlayerWhite, isHiscore;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initGame(void)
{
    record.playCount++;
    isRecordDirty = true;
    writeRecord();

    score = 0;
    scoreX = 0;
    scoreY = -9;
    power = POWER_MAX;
    powerY = -9;
    gameFrames = 0;

    playerX = WIDTH * SCALE / 2;
    playerY = HEIGHT * SCALE / 2;
    playerMoving = 0;
    isPlayerWhite = true;
    colorBias = 4;

    for (LASER_T *p = &lasers[0]; p < &lasers[LASERS_MAX]; p++) {
        p->vx = 0;
    }
    laserCount = 0;
    laserWait = FPS * 2;

    state = STATE_PLAYING;
    arduboy.playScore2(soundStart, SND_PRIO_START);
}

MODE_T updateGame(void)
{
    switch (state) {
    case STATE_PLAYING:
        if (laserCount > 0) {
            record.playFrames++;
            if ((++gameFrames & 7) == 0) score++;
            if (scoreY < 0) scoreY++;
            isRecordDirty = true;
        }
        if (power < POWER_MAX) power++;
        movePlayer();
        moveLasers();
        if (power <= 0) {
            isHiscore = enterScore(score);
            writeRecord();
            arduboy.playScore2(soundOver, SND_PRIO_OVER);
            gameFrames = FPS * 10;
            state = STATE_OVER;
            dprintln(F("Game over"));
        } else if (laserCount > 0 && arduboy.buttonDown(A_BUTTON)) {
            writeRecord();
            clearMenuItems();
            addMenuItem(F("CONTINUE"), onContinue);
            addMenuItem(F("QUIT"), onQuit);
            setMenuCoords(34, 26, 59, 17);
            setMenuItemPos(0);
            state = STATE_MENU;
            dprintln(F("Pause"));
        }
        break;
    case STATE_MENU:
        handleMenu();
        break;
    case STATE_OVER:
        gameFrames--;
        if (scoreY < 0) scoreY++;
        moveLasers();
        if (arduboy.buttonDown(A_BUTTON) || gameFrames == 0) state = STATE_LEAVE;
        if (arduboy.buttonDown(B_BUTTON) && gameFrames <= FPS * 8) initGame();
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
    if (state == STATE_MENU) drawMenuItems();
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

    /*  Others  */
    if (scoreX == 0 && playerX <= 36 * SCALE) {
        scoreX = 120;
        scoreY = -8;
    } else if (scoreX > 0 && playerX >= 91 * SCALE) {
        scoreX = 0;
        scoreY = -8;
    }
    if (playerY >= 53 * SCALE || power == POWER_MAX) powerY = -9;
    if (playerY <= 10 * SCALE) powerY = 6;
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
    p->isWhite = (random(8) >= colorBias);
    colorBias += p->isWhite * 2 - 1; 
}

static int16_t calcLaserY(LASER_T *p)
{
    return (descale(p->x) * p->a >> 3) + (p->b << 4);
}

static void onContinue(void)
{
    playSoundClick();
    state = STATE_PLAYING;
    dprintln(F("Continue"));
}

static void onQuit(void)
{
    playSoundClick();
    lastScore = 0;
    state = STATE_LEAVE;
    dprintln(F("Quit"));
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
        drawNumber(x - 5 + (power < 100) * 3, y + powerY, power / 10);
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
    drawFigure(scoreX, scoreY, score, (scoreX == 0) ? ALIGN_LEFT : ALIGN_RIGHT);
    if (laserCount == 0) drawBitmapBW(40, 8, imgReady, 49, 15);
    if (state == STATE_OVER) {
        drawBitmapBW(25, 26, imgGameOver, 77, 11);
        if (isHiscore) {
            uint8_t c = (gameFrames & 8) >> 3; 
            arduboy.setTextColor(c, c);
            arduboy.printEx(31, 40, F("NEW RECORD!"));
            arduboy.setTextColor(WHITE, WHITE);
        }
    }
}

static int8_t drawFigure(int16_t x, int16_t y, int value, ALIGN_T align)
{
    int8_t offset = (value > 9) ? drawFigure(x - align * 4, y, value / 10, align) : 0;
    drawBitmapBW(x + offset, y, imgFigures[value % 10], 8, 8);
    return offset + 8 - align * 4;
}
