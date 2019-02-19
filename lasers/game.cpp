#include "common.h"
#include "data.h"

/*  Defines  */

#define LASERS_MAX      12
#define SPARKS_MAX      64

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

#define DEG_MAX         (360 * SCALE)
#define DEG_COEF        (DEG_TO_RAD / SCALEF)

#define RAY_VD_MIN      (1 * SCALE)
#define RAY_VD_MAX      (3 * SCALE)
#define RAY_R_MAX       144
#define RAY_R_INC       2
#define RAY_R_DEC       8

#define SPARKS_HIT      3
#define SPARK_R_MIN     3
#define SPARK_D_DEVI    (45 * SCALE)
#define SPARK_S_MIN     6
#define SPARK_S_MIN_EX  11

#define POWER_MAX       1000
#define DAMAGE_LASER    60
#define IMG_LASER_DIV   (DEG_MAX / IMG_LASER_ID_MAX)

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

enum {
    HIT_SOUND_NONE = 0,
    HIT_SOUND_ABSORB = 1,
    HIT_SOUND_DAMAGE = 3,
};

/*  Typedefs  */

typedef struct {
    int16_t x;
    int8_t  vx;
    int8_t  a, b;
    int16_t d;
    int8_t  vd;
    uint8_t r;
    bool    isWhite;
} LASER_T;

typedef struct {
    int16_t d;
    uint8_t r, s;
    bool    isWhite;
} SPARK_T;

/*  Local Functions  */

static void tuneParams(void);
static void movePlayer(void);
static void moveLasers(void);
static void moveLaser(LASER_T *p);
static void newLaser(LASER_T *p);
static int16_t calcLaserY(LASER_T *p);
static void moveSparks(void);
static void newSpark(int16_t d, uint8_t s, bool isWhite);
static void controlSound(void);
static void finishgame(void);
static void pauseGame(void);

static void onContinue(void);
static void onQuit(void);

static void drawPlayer(void);
static void drawLasers(void);
static void drawRay(LASER_T *p);
static void drawLaser(LASER_T *p);
static void drawSparks(void);
static void drawStrings(void);
static int8_t drawFigure(int16_t x, int16_t y, int value, ALIGN_T align);
static void controlLed(void);

/*  Local Variables  */

static STATE_T  state = STATE_INIT;
static LASER_T  lasers[LASERS_MAX];
static SPARK_T  sparks[SPARKS_MAX];
static uint16_t score, gameFrames;
static int16_t  power, playerX, playerY;
static int16_t  laserCount, laserWait;
static int8_t   playerVx, playerVy, playerMoving, colorBias;
static int8_t   scoreX, scoreY, powerY;
static int8_t   hitSound, lastHitSound, sparkIdx;
static bool     isPlayerWhite, isHiscore;
static const byte *pSound;

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

    for (SPARK_T *p = &sparks[0]; p < &sparks[SPARKS_MAX]; p++) {
        p->s = 0;
    }
    sparkIdx = 0;

    state = STATE_PLAYING;
    arduboy.playScore2(soundStart, SND_PRIO_START);
    lastHitSound = HIT_SOUND_NONE;
}

MODE_T updateGame(void)
{
    switch (state) {
    case STATE_PLAYING:
        tuneParams();
        movePlayer();
        moveLasers();
        moveSparks();
        controlSound();
        if (power <= 0) {
            finishgame();
        } else if (laserCount > 0 && arduboy.buttonDown(A_BUTTON)) {
            pauseGame();
        }
        break;
    case STATE_MENU:
        handleMenu();
        break;
    case STATE_OVER:
        gameFrames++;
        if (scoreY < 0) scoreY++;
        moveLasers();
        moveSparks();
        if (gameFrames >= FPS) {
            if (arduboy.buttonDown(A_BUTTON) || gameFrames >= FPS * 10) state = STATE_LEAVE;
            if (arduboy.buttonDown(B_BUTTON)) initGame();
        }
        break;
    default:
        break;
    }
    return (state == STATE_LEAVE) ? MODE_TITLE : MODE_GAME;
}

void drawGame(void)
{
    clearScreenGray();
    controlLed();
    if (state == STATE_LEAVE) return;
    drawSparks();
    drawLasers();
    drawPlayer();
    drawStrings();
    if (state == STATE_MENU) drawMenuItems();
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void tuneParams(void)
{
    hitSound = HIT_SOUND_NONE;
    pSound = NULL;
    if (laserCount > 0) {
        record.playFrames++;
        if ((++gameFrames & 7) == 0) score++;
        if (scoreY < 0) scoreY++;
        isRecordDirty = true;
    }
    if (power < POWER_MAX) power++;
}

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
        pSound = soundToggle;
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
    if (p->d < 0) p->d += DEG_MAX;
    if (p->d >= DEG_MAX) p->d -= DEG_MAX;
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
    float deg = p->d * DEG_COEF;
    float vx = cos(deg), vy = -sin(deg);
    float lx = dx * vx + dy * vy, ly = dx * vy - dy * vx;
    if (lx >= 0 && lx < p->r * SCALEF && abs(ly) < PLAYER_DIAM) {
        p->r = lx / SCALEF;
        if (isPlayerWhite == p->isWhite) {
            score++;
            hitSound |= HIT_SOUND_ABSORB;
        } else {
            power -= DAMAGE_LASER;
            hitSound |= HIT_SOUND_DAMAGE;
        }
        for (int i = 0; i < SPARKS_HIT; i++) {
            newSpark(p->d + DEG_MAX / 2, SPARK_S_MIN, p->isWhite);
        }
    }
}

static void newLaser(LASER_T *p)
{
    p->x = random(2) * (LASER_X_MAX + LASER_DIAM * 2) - LASER_DIAM;
    p->vx = random(6, 11) * ((p->x <= 0) * 2 - 1);
    p->b = random(HEIGHT);
    p->a = random(HEIGHT) - p->b;
    p->d = random(DEG_MAX) * DEG_TO_RAD;
    p->vd = random(RAY_VD_MIN, RAY_VD_MAX + 1) * (random(2) * 2 - 1);
    p->r = 0;
    p->isWhite = (random(8) >= colorBias);
    colorBias += p->isWhite * 2 - 1; 
    pSound = (p->isWhite) ? soundLaserWhite : soundLaserBlack;
}

static int16_t calcLaserY(LASER_T *p)
{
    return (descale(p->x) * p->a >> 3) + (p->b << 4);
}

static void moveSparks(void)
{
    for (SPARK_T *p = &sparks[0]; p < &sparks[SPARKS_MAX]; p++) {
        if (p->s) p->r += p->s--;
    }
}

static void newSpark(int16_t d, uint8_t s, bool isWhite)
{
    SPARK_T *p = &sparks[sparkIdx];
    p->d = d + random(-SPARK_D_DEVI, SPARK_D_DEVI);
    p->r = SPARK_R_MIN;
    p->s = s + random(s);
    p->isWhite = isWhite;
    sparkIdx = (sparkIdx + 1) % SPARKS_MAX;
}

static void controlSound(void)
{
    if (lastHitSound != hitSound) {
        switch (hitSound) {
        case HIT_SOUND_NONE:
            arduboy.stopScore2();
            break;
        case HIT_SOUND_ABSORB:
            arduboy.playScore2(soundAbsorb, SND_PRIO_HIT);
            break;
        case HIT_SOUND_DAMAGE:
            arduboy.playScore2(soundDamage, SND_PRIO_HIT);
            break;
        default:
            break;
        }
        lastHitSound = hitSound;
    }
    if (pSound) {
        arduboy.playScore2(pSound, (pSound == soundToggle) ? SND_PRIO_TOGGLE : SND_PRIO_LASER);
    }
}

static void finishgame(void)
{
    /*  Explosion  */
    for (int i = 0; i < SPARKS_MAX; i++) {
        newSpark(i * DEG_MAX / SPARKS_MAX, SPARK_S_MIN_EX, isPlayerWhite);
    }

    isHiscore = enterScore(score);
    writeRecord();
    gameFrames = 0;
    state = STATE_OVER;
    arduboy.playScore2(soundOver, SND_PRIO_OVER);
    dprintln(F("Game over"));
}

static void pauseGame(void)
{
    writeRecord();
    clearMenuItems();
    addMenuItem(F("CONTINUE"), onContinue);
    addMenuItem(F("QUIT"), onQuit);
    setMenuCoords(34, 26, 59, 17);
    setMenuItemPos(0);
    state = STATE_MENU;
    arduboy.stopScore2();
    playSoundClick();
    lastHitSound = HIT_SOUND_NONE;
    dprintln(F("Pause"));
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
    float deg = p->d * DEG_COEF;
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

static void drawSparks(void)
{
    int16_t x = descale(playerX);
    int16_t y = descale(playerY);
    for (SPARK_T *p = &sparks[0]; p < &sparks[SPARKS_MAX]; p++) {
        if (p->s) {
            float deg = p->d * DEG_COEF;
            float r = p->r / 4.0;
            arduboy.drawFastHLine(x + cos(deg) * r, y - sin(deg) * r, 2, p->isWhite);
        }
    }
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

static void controlLed(void)
{
    uint8_t r = 0, g = 0;
    if (state == STATE_PLAYING && power < 800) {
        uint8_t level = power / 200;
        uint8_t cycle = 1 << (level + 3);
        int8_t  tmp = cycle / 2 - (gameFrames & (cycle - 1));
        r = max((abs(tmp) << (3 - level)) - level * 8, 0);
    } else if (state == STATE_OVER && gameFrames < 16) {
        r = 64 - gameFrames * 4;
        g = r;
    }
    arduboy.setRGBled(r, g, 0);
}
