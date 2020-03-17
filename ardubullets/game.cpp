#include "common.h"
#include "data.h"

/*  Defines  */

#define SHOT_MAX        18
#define SHOT_SPD        5
#define PLAYER_SCALE    4
#define PLAYER_SLOW_MAX 8
#define PLAYER_FIRE_MAX 12
#define PLAYER_FIRE_INT 3
#define BULLET_MAX      64
#define BULLET_CYC_BASE 4
#define BULLET_SPD_MAX  31
#define BULLET_SPD_MIN  1
#define BULLET_SPD_BASE 4
#define BULLET_SCALE    12
#define GROUP_MAX       3
#define GROUP_INTERVAL  (FPS * 5)
#define ENEMY_UNITY     5
#define ENEMY_MAX       (GROUP_MAX * ENEMY_UNITY)
#define ENEMY_LIFE_INIT 15

enum STATE_T : uint8_t {
    STATE_PLAYING = 0,
    STATE_OVER,
    STATE_LEAVE,
};

/*  Typedefs  */

typedef struct {
    uint8_t     x;
    uint8_t     y;
} SHOT_T;

typedef struct {
    uint8_t     bx;
    uint8_t     by;
    int8_t      deg;
    uint16_t    spd:5;
    uint16_t    rad:11;
} BULLET_T;

typedef struct {
    uint8_t     xCoef:4;
    uint8_t     yCoef:4;
    uint16_t    xRad:5;
    uint16_t    yRad:5;
    uint16_t    type:4;
    uint16_t    fireCycle:2;
    uint8_t     fireTimes:4;
    uint8_t     fireNum:4;
    uint8_t     fireBaseSpeed:2;
    uint8_t     fireGap:6;
    uint8_t     fireInterval;
} GROUP_T;

typedef struct {
    uint16_t    bx:6;
    uint16_t    by:6;
    uint16_t    life:4;
    uint8_t     cycleGap;
    int8_t      fireDeg;
} ENEMY_T;

typedef struct {
    uint16_t    x:7;
    uint16_t    y:6;
    uint16_t    a:3;
} EXPLO_T;

/*  Local Functions  */

static void     initGameParams(void);
static void     initShots(void);
static void     initBullets(void);
static void     initEnemies(void);

static void     handlePlaying(void);
static void     handleOver(void);

static void     updatePlayer(void);
static void     newShot(uint8_t g);
static void     updateShots(void);

static void     newGroup(uint8_t groupIdx);
static bool     updateEnemies(void);
static Point    getEnemyCoords(GROUP_T *pG, ENEMY_T *pB);

static void     newBullet(int16_t x, int16_t y, int8_t deg, int8_t spd);
static void     newNeedleBullets(int16_t x, int16_t y, int8_t deg, int8_t spd, uint8_t num);
static void     newFanwiseBullets(int16_t x, int16_t y, int8_t deg, int8_t spd, uint8_t num, int8_t gap);
static bool     updateBullets(void);
static Point    getBulletCoords(BULLET_T *pB);

static void     onContinue(void);
static void     onConfirmRetry(void);
static void     onRetry(void);
static void     onConfirmQuit(void);
static void     onQuit(void);

static void     drawPlayer(void);
static void     drawShots(void);
static void     drawBullets(void);
static void     drawEnemies(void);
static void     drawExplosion(void);

/*  Local Functions (macros)  */

#define circulate(n, v, m)  (((n) + (v) + (m)) % (m))
#define callStateHandler(n) ((void (*)(void)) pgm_read_ptr(handlerFuncTable + n))()

/*  Local Variables  */

PROGMEM static void(*const handlerFuncTable[])(void) = {
    handlePlaying, handleOver
};

static const Rect screenRect = Rect(0, 0, WIDTH, HEIGHT);

static STATE_T  state;
static SHOT_T   shots[SHOT_MAX];
static BULLET_T bullets[BULLET_MAX];
static GROUP_T  groups[GROUP_MAX];
static ENEMY_T  enemies[ENEMY_MAX];
static EXPLO_T  explo;
static uint16_t score, gameFrames;
static uint8_t  playerX, playerY, playerFire, playerSlow;
static uint8_t  counter, gameRank, bulletsNum;
static uint8_t  ledLevel, ledRed, ledGreen, ledBlue;
static bool     isMenu;
//static const __FlashStringHelper *evalLabel;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initGame(void)
{
    arduboy.playScore(soundStart, SND_PRIO_START);
    record.playCount++;
    initGameParams();
    isMenu = false;
    state = STATE_PLAYING;
    isInvalid = true;
}

MODE_T updateGame(void)
{
    handleDPad();
    if (isMenu) {
        handleMenu();
    } else {
        counter++;
        if (ledLevel > 0) ledLevel--;
        callStateHandler(state);
    }
    return (state == STATE_LEAVE) ? MODE_TITLE : MODE_GAME;
}

void drawGame(void)
{
    if (isMenu) {
        drawMenuItems(isInvalid);
        ledLevel = 0;
    } else if (isInvalid) {
        arduboy.clear();
        drawTime(0, 0, gameFrames);
        drawPlayer();
        drawShots();
        drawBullets();
        drawEnemies();
        drawExplosion();
    }
    //arduboy.setRGBled(ledRed * ledLevel, ledGreen * ledLevel, ledBlue * ledLevel);
    isInvalid = false;
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void initGameParams(void)
{
    playerX = WIDTH / 8 * PLAYER_SCALE;
    playerY = HEIGHT / 2 * PLAYER_SCALE;
    score = 0;
    gameRank = 5; // TODO
    gameFrames = 0;
    playerFire = 0;
    playerSlow = 0;
    initShots();
    initBullets();
    initEnemies();
    explo.a = IMG_EXPLOSION_ID_MAX;
    randomSeed(record.gameSeed + 1);
}

static void initShots(void)
{
    for (SHOT_T *pS = shots; pS < &shots[SHOT_MAX]; pS++) pS->x = WIDTH;
}

static void initBullets(void)
{
    for (BULLET_T *pB = bullets; pB < &bullets[BULLET_MAX]; pB++) pB->spd = 0;
    bulletsNum = 0;
}

static void initEnemies(void)
{
    for (ENEMY_T *pE = enemies; pE < &enemies[ENEMY_MAX]; pE++) pE->life = 0;
}

/*-----------------------------------------------------------------------------------------------*/

static void handlePlaying()
{
    updateShots();
    updatePlayer();
    if (explo.a < IMG_EXPLOSION_ID_MAX) explo.a++;
    bool isHit = updateBullets();
    bool isClear = updateEnemies();
    gameFrames++;
    record.playFrames++;
    isRecordDirty = true;

    if (isHit) {
        arduboy.playScore(soundOver, SND_PRIO_OVER);
        writeRecord();
        state = STATE_OVER;
    } else if (arduboy.buttonDown(A_BUTTON)) {
        playSoundClick();
        clearMenuItems();
        addMenuItem(F("CONTINUE"), onContinue);
        addMenuItem(F("RESTART GAME"), onConfirmRetry);
        addMenuItem(F("BACK TO TITLE"), onConfirmQuit);
        setMenuCoords(19, 23, 89, 17, true, true);
        setMenuItemPos(0);
        isMenu = true;
    }
    isInvalid = true;
}

static void handleOver()
{
    if (arduboy.buttonDown(A_BUTTON)) {
        playSoundClick();
        clearMenuItems();
        addMenuItem(F("RETRY GAME"), onRetry);
        addMenuItem(F("BACK TO TITLE"), onQuit);
        setMenuCoords(19, 26, 89, 11, true, false);
        setMenuItemPos(0);
        isMenu = true;
        isInvalid = true;
    }
}

/*-----------------------------------------------------------------------------------------------*/

static void updatePlayer(void)
{
    int8_t vx = arduboy.buttonPressed(RIGHT_BUTTON) - arduboy.buttonPressed(LEFT_BUTTON);
    int8_t vy = arduboy.buttonPressed(DOWN_BUTTON) - arduboy.buttonPressed(UP_BUTTON);
    int8_t vr = (playerSlow == PLAYER_SLOW_MAX) ? 2 : ((vx != 0 && vy != 0) ? 3 : 4);
    playerX += vx * vr;
    playerX = constrain(playerX, 3 * PLAYER_SCALE, (WIDTH / 2 - 3) * PLAYER_SCALE - 1);
    playerY += vy * vr;
    playerY = constrain(playerY, 3 * PLAYER_SCALE, (HEIGHT - 3) * PLAYER_SCALE - 1);

    if (arduboy.buttonPressed(B_BUTTON)) {
        if (playerFire == 0) playerFire = PLAYER_FIRE_MAX;
        if (playerSlow < PLAYER_SLOW_MAX) playerSlow++;
    } else {
        if (playerSlow > 0) playerSlow--;
    }
    if (playerFire > 0 && playerFire-- % PLAYER_FIRE_INT == 0) {
        newShot((PLAYER_FIRE_MAX / PLAYER_FIRE_INT) - playerFire / PLAYER_FIRE_INT);
    }
}

static void newShot(uint8_t g)
{
    static SHOT_T *pS = shots;
    uint8_t x = playerX / PLAYER_SCALE + SHOT_SPD, y = playerY / PLAYER_SCALE;
    pS->x = x;
    pS->y = y - g;
    pS++;
    pS->x = x;
    pS->y = y + g;
    if (++pS >= &shots[SHOT_MAX]) pS = shots;
}

static void updateShots(void)
{
    for (SHOT_T *pS = shots; pS < &shots[SHOT_MAX]; pS++) {
        if (pS->x < WIDTH) pS->x += SHOT_SPD;
    }
}

/*-----------------------------------------------------------------------------------------------*/

static void newGroup(uint8_t groupIdx)
{
    int idx = groupIdx % GROUP_MAX;
    GROUP_T *pG = &groups[idx];

    /*  Moving path design  */
    pG->xRad = random(4, 32);
    pG->yRad = random(4, 25);
    pG->xCoef = random(32, 64) / pG->xRad;
    pG->yCoef = random(24, 64) / pG->yRad;

    /*  Bullets design  */
    pG->type = random(5);
    pG->fireInterval = random(FPS, FPS * 3 + 1);
    pG->fireCycle = random(4);
    pG->fireBaseSpeed = random(4);
    uint8_t fires = gameRank * pG->fireInterval / FPS / 2;
    if (fires == 0) fires = 1;
    uint8_t fireDiv = (random(4)) ? 1 : random(1, sqrt(fires) + 1);
    if (pG->type < 2) {
        pG->fireTimes = fireDiv;
        pG->fireNum = fires / fireDiv;
    } else {
        pG->fireTimes = fires / fireDiv;
        pG->fireNum = fireDiv;
    }
    if (pG->type == 4 && pG->fireNum > 1) {
        pG->fireGap = random(32, 64) / (pG->fireNum - 1);
    } else {
        pG->fireGap = random(2, 8);
    }

    /*  Position of each enemy  */
    ENEMY_T *pE = &enemies[idx * ENEMY_UNITY];
    int8_t x = playerX / PLAYER_SCALE, y = playerY / PLAYER_SCALE;
    for (int i = 0; i < ENEMY_UNITY; i++, pE++) {
        pE->bx = random(pG->xRad / 2, WIDTH / 2 - 3 - pG->xRad);
        pE->by = random(pG->yRad + 3, HEIGHT - 3 - pG->yRad);
        pE->life = ENEMY_LIFE_INIT;
        pE->cycleGap = random(256);
        Point enemyPoint = getEnemyCoords(pG, pE);
        pE->fireDeg = myAtan2f(y - enemyPoint.y, x - enemyPoint.x);
    }
}

static bool updateEnemies(void)
{
    /*  New group  */
    if (gameFrames < FPS * 60 && gameFrames % GROUP_INTERVAL == 0) {
        newGroup(gameFrames / GROUP_INTERVAL);
        arduboy.playScore(soundEntry, SND_PRIO_EFFECT);
    }

    GROUP_T *pG = groups;
    int enemyCounter = 0, enemyLives = 0;
    bool isDamaged = false;
    for (ENEMY_T *pE = enemies; pE < &enemies[ENEMY_MAX]; pE++) {
        if (enemyCounter++ == ENEMY_UNITY) pG++, enemyCounter = 1;
        if (pE->life == 0) continue;

        /*  Hit judgement  */
        Point enemyPoint = getEnemyCoords(pG, pE);
        Rect enemyRect = Rect(enemyPoint.x - 3, enemyPoint.y - 3, 7, 7);
        for (SHOT_T *pS = shots; pE->life > 0 && pS < &shots[SHOT_MAX]; pS++) {
            if (pS->x < WIDTH && arduboy.collide(Point(pS->x, pS->y), enemyRect)) {
                pS->x = WIDTH;
                pE->life--;
                isDamaged = true;
            }
        }
        if (pE->life == 0) { // Defeat!
            arduboy.playScore(soundDefeat, SND_PRIO_EFFECT);
            score++;
            explo.x = enemyPoint.x;
            explo.y = enemyPoint.y;
            explo.a = 0;
            continue;
        }
        enemyLives++;

        uint16_t firePhase = (gameFrames + pE->cycleGap) % pG->fireInterval;
        uint8_t fireCycle = pG->fireCycle + BULLET_CYC_BASE;
        if (firePhase % fireCycle > 0) continue;
        uint8_t fireCnt = firePhase / fireCycle;
        if (fireCnt >= pG->fireTimes) continue;

        /*  Fire enemy's bullets  */
        int8_t spd = gameRank + pG->fireBaseSpeed + BULLET_SPD_BASE;
        int16_t x = enemyPoint.x, y = enemyPoint.y;
        int8_t deg = myAtan2f(playerY / PLAYER_SCALE - y, playerX / PLAYER_SCALE - x);
        if (fireCnt == 0) {
            pE->fireDeg = deg;
            if (pG->type == 4 && pG->fireTimes > 1) {
                pE->fireDeg -= pG->fireGap * (pG->fireTimes - 1) / 2;
            }
        }
        switch (pG->type) {
        case 0: // needle
            spd = spd * 3 / 2 - pG->fireNum / 2;
            newNeedleBullets(x, y, deg, spd, pG->fireNum);
            break;
        case 1: // funwise
            newFanwiseBullets(x, y, deg, spd, pG->fireNum - (fireCnt & 1), pG->fireGap);
            break;
        case 2: // rapid-fire
            spd = spd * 3 / 2;
            newFanwiseBullets(x, y, pE->fireDeg, spd, pG->fireNum, pG->fireGap);
            break;
        case 3: // whip
            spd += fireCnt * 2 - pG->fireNum;
            newFanwiseBullets(x, y, deg, spd, pG->fireNum, pG->fireGap);
            break;
        case 4: // mowing
            newNeedleBullets(x, y, pE->fireDeg, spd, pG->fireNum);
            pE->fireDeg += pG->fireGap;
            break;
        default:
            break;
        }
    }
    if (isDamaged) arduboy.playTone(200, 10);
    return (enemyLives == 0 && gameFrames >= FPS * 60);
}

static Point getEnemyCoords(GROUP_T *pG, ENEMY_T *pE)
{
    uint16_t seed = gameFrames + pE->cycleGap;
    int8_t xDeg = seed * pG->xCoef;
    int8_t yDeg = seed * pG->yCoef;
    return Point(pE->bx + WIDTH / 2 + myCos(xDeg, pG->xRad), pE->by + mySin(yDeg, pG->yRad));
}

/*-----------------------------------------------------------------------------------------------*/

static void newBullet(int16_t x, int16_t y, int8_t deg, int8_t spd)
{
    if (bulletsNum == BULLET_MAX) return;
    //if (!arduboy.collide(Point(x, y), screenRect)) return;

    static BULLET_T *pB = bullets;
    while (pB->spd) {
        if (++pB >= &bullets[BULLET_MAX]) pB = bullets;
    }
    pB->bx = x;
    pB->by = y;
    pB->deg = deg;
    pB->spd = constrain(spd, BULLET_SPD_MIN, BULLET_SPD_MAX);
    pB->rad = 0;
    bulletsNum++;
}

static void newNeedleBullets(int16_t x, int16_t y, int8_t deg, int8_t spd, uint8_t num)
{
    for (int i = 0; i < num && spd <= BULLET_SPD_MAX; i++, spd++) {
        if (spd >= BULLET_SPD_MIN) newBullet(x, y, deg, spd);
    }
}

static void newFanwiseBullets(int16_t x, int16_t y, int8_t deg, int8_t spd, uint8_t num,  int8_t gap)
{
    if (num > 1) deg -= gap * (num - 1) / 2;
    for (int i = 0; i < num; i++, deg += gap) {
        newBullet(x, y, deg, spd);
    }
}

static bool updateBullets(void)
{
    bool ret = false;
    Rect playerRect = Rect(playerX / PLAYER_SCALE - 1, playerY / PLAYER_SCALE - 1, 3, 3);
    for (BULLET_T *pB = bullets; pB < &bullets[BULLET_MAX]; pB++) {
        if (pB->spd) {
            pB->rad += pB->spd;
            Point bulletPoint = getBulletCoords(pB);
            if (!arduboy.collide(bulletPoint, screenRect)) {
                pB->spd = 0;
                bulletsNum--;
            } else if (arduboy.collide(bulletPoint, playerRect)) { // Be defeated...
                ret = true;
            }
        }
    }
    return ret;
}

static Point getBulletCoords(BULLET_T *pB)
{
    int16_t rad = pB->rad / BULLET_SCALE;
    return Point(pB->bx + myCos(pB->deg, rad), pB->by + mySin(pB->deg, rad));
}

/*---------------------------------------------------------------------------*/
/*                               Menu Handlers                               */
/*---------------------------------------------------------------------------*/

static void onContinue(void)
{
    playSoundClick();
    isMenu = false;
    isInvalid = true;
    dprintln(F("Menu: continue"));
}

static void onConfirmRetry(void)
{
    int16_t y = getMenuItemPos() * 6 + 25;
    setConfirmMenu(y, onRetry, onContinue);
    dprintln(F("Menu: confirm quit"));
}

static void onRetry(void)
{
    dprintln(F("Menu: retry"));
    initGame();
}

static void onConfirmQuit(void)
{
    int16_t y = getMenuItemPos() * 6 + 25;
    setConfirmMenu(y, onQuit, onContinue);
    dprintln(F("Menu: confirm quit"));
}

static void onQuit(void)
{
    playSoundClick();
    writeRecord();
    isMenu = false;
    state = STATE_LEAVE;
    dprintln(F("Menu: quit"));
}

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void drawPlayer(void)
{
    arduboy.drawBitmap(playerX / PLAYER_SCALE - 3, playerY / PLAYER_SCALE - 3,
            imgPlayer, IMG_SHIP_W, IMG_SHIP_H);
}

static void drawShots(void)
{
    for (SHOT_T *pS = shots; pS < &shots[SHOT_MAX]; pS++) {
        if (pS->x < WIDTH) {
            arduboy.drawFastHLine(pS->x - SHOT_SPD + 1, pS->y, SHOT_SPD, WHITE);
        }
    }
}

static void drawBullets(void)
{
    for (BULLET_T *pB = bullets; pB < &bullets[BULLET_MAX]; pB++) {
        if (pB->spd > 0) {
            Point bulletPoint = getBulletCoords(pB);
            arduboy.drawFastHLine(bulletPoint.x, bulletPoint.y, 2, WHITE);
        }
    }
}

static void drawEnemies(void)
{
    GROUP_T *pG = groups;
    int enemyCounter = 0;
    for (ENEMY_T *pE = enemies; pE < &enemies[ENEMY_MAX]; pE++) {
        if (pE->life > 0) {
            Point enemyPoint = getEnemyCoords(pG, pE);
            arduboy.drawBitmap(enemyPoint.x - 3, enemyPoint.y - 3,
                    imgEnemy[pG->type], IMG_SHIP_W, IMG_SHIP_H);
        }
        if (++enemyCounter == ENEMY_UNITY) pG++, enemyCounter = 0;
    }
}

static void drawExplosion(void)
{
    if (explo.a < IMG_EXPLOSION_ID_MAX) {
        arduboy.drawBitmap(explo.x - 7, explo.y - 7,
                imgExplosion[explo.a], IMG_EXPLOSION_W, IMG_EXPLOSION_H);
    }
}
