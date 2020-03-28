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
#define BULLET_SPD_MAX  31
#define BULLET_SPD_MIN  1
#define BULLET_BASE_SPD 4
#define BULLET_SCALE    12
#define BULLET_CYC_BASE 3
#define BULLET_GAP_BASE 8
#define GROUP_MAX       3
#define GROUP_TOTAL     12
#define GROUP_INT       (FPS * 5)
#define ENEMY_UNITY     5
#define ENEMY_MAX       (GROUP_MAX * ENEMY_UNITY)
#define ENEMY_LIFE_INIT 15
#define ENEMY_ACTIVE    (FPS * 12)
#define ENEMY_FADE      6
#define START_OMIT      FPS
#define START_DURATION  (FPS * 2)
#define GAME_DURATION   (GROUP_INT * GROUP_TOTAL)
#define OVER_OMIT       FPS
#define EDGE_THRESHOLD  12

enum STATE_T : uint8_t {
    STATE_START = 0,
    STATE_PLAYING,
    STATE_OVER,
    STATE_LEAVE,
};

enum {
    ENEMY_TYPE_NEEDLE = 0,
    ENEMY_TYPE_FUNWISE,
    ENEMY_TYPE_RAPIDFIRE,
    ENEMY_TYPE_WHIP,
    ENEMY_TYPE_MOWING,
    ENEMY_TYPE_MAX,
};

/*  Typedefs  */

typedef struct {
    uint8_t     x;
    uint8_t     y;
} SHOT_T; // sizeof(SHOT_T) is 2 bytes

typedef struct {
    uint16_t    spd:5;
    uint16_t    rad:11;
    uint8_t     bx;
    uint8_t     by;
    int8_t      deg;
} BULLET_T; // sizeof(BULLET_T) is 5 bytes

typedef struct {
    uint16_t    type:4;
    uint16_t    xRad:6;
    uint16_t    yRad:5;
    uint16_t    yFlip:1;
    uint16_t    xCoef:6;
    uint16_t    yCoef:6;
    uint16_t    fireGap:4;
    uint8_t     fireTimes:4;
    uint8_t     fireNum:4;
    uint8_t     fireCycle:3;
    uint8_t     fireExSpd:2;
    uint8_t     entryInt:3;
    uint8_t     fireInt;
} GROUP_T; // sizeof(GROUP_T) is 7 bytes

typedef struct {
    uint16_t    bx:6;
    uint16_t    by:6;
    uint16_t    life:4;
    uint8_t     framesGap;
    int8_t      fireDeg;
} ENEMY_T; // sizeof(ENEMY_T) is 4 bytes

typedef struct {
    uint16_t    x:7;
    uint16_t    y:6;
    uint16_t    a:3;
} EXPLO_T; // sizeof(EXPLO_T) is 2 bytes

/*  Local Functions  */

static void     initGameParams(void);
static void     initShots(void);
static void     initBullets(void);
static void     initEnemies(void);

static void     handleStart(void);
static void     handlePlaying(void);
static void     handleOver(void);
static void     openGameMenu(void);
static void     sparkleLed(uint8_t idx);

static void     updatePlayer(void);
static void     newShot(uint8_t g);
static void     updateShots(void);
static void     updateExplo(void);

static void     newGroup(uint8_t groupIdx);
static bool     updateEnemies(void);
static Point    getEnemyCoords(GROUP_T *pG, ENEMY_T *pE, uint16_t frames);

static void     newBullet(int16_t x, int16_t y, int8_t deg, int8_t spd);
static void     newNeedleBullets(int16_t x, int16_t y, int8_t deg, int8_t spd, uint8_t num);
static void     newFanwiseBullets(int16_t x, int16_t y, int8_t deg, int8_t spd, uint8_t num, int8_t gap);
static bool     updateBullets(void);
static Point    getBulletCoords(BULLET_T *pB);

static void     onContinue(void);
static void     onRetry(void);
static void     onConfirmQuit(void);
static void     onQuit(void);

static void     drawPlayer(void);
static void     drawShots(void);
static void     drawBullets(void);
static void     drawEnemies(void);
static void     drawExplosion(void);
static void     drawBackground(void);
static void     drawLetters(void);
static void     drawLettersBigLabel(int16_t x, int16_t y, const uint8_t *bitmap, uint8_t w, uint8_t h);
static void     drawLettersGameSeed(void);

/*  Local Functions (macros)  */

#define circulate(n, v, m)  (((n) + (v) + (m)) % (m))
#define callHandlerFunc(n)  ((void (*)(void)) pgm_read_ptr(handlerFuncTable + n))()

/*  Local Variables  */

PROGMEM static void(*const handlerFuncTable[])(void) = {
    handleStart, handlePlaying, handleOver
};

static const Rect screenRect = Rect(0, 0, WIDTH, HEIGHT);

static STATE_T  state;
static SHOT_T   shots[SHOT_MAX];
static BULLET_T bullets[BULLET_MAX];
static GROUP_T  groups[GROUP_MAX];
static ENEMY_T  enemies[ENEMY_MAX];
static EXPLO_T  explo;
static uint16_t gameFrames;
static uint8_t  gameRank, bulletsNum;
static uint8_t  playerX, playerY, playerFire, playerSlow;
static uint8_t  ledLevel, ledRed, ledGreen, ledBlue;
static bool     isDefeated, isMenu;
#ifdef DEBUG
static bool     isMuteki = false;
#endif

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initGame(void)
{
    arduboy.playScore(soundStart, SND_PRIO_START);
    initGameParams();
    counter = 0;
    ledLevel = 0;
    isMenu = false;
    state = STATE_START;
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
        callHandlerFunc(state);
    }
    return (state == STATE_LEAVE) ? MODE_TITLE : MODE_GAME;
}

void drawGame(void)
{
    if (state == STATE_LEAVE) {
        ledLevel = 0;
    } else if (isMenu) {
        drawMenuItems(isInvalid);
        ledLevel = 0;
    } else if (isInvalid) {
        arduboy.clear();
        drawPlayer();
        drawShots();
        drawBullets();
        drawEnemies();
        drawExplosion();
        drawBackground();
        drawLetters();
    }
    arduboy.setRGBled(ledRed * ledLevel, ledGreen * ledLevel, ledBlue * ledLevel);
    isInvalid = false;
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void initGameParams(void)
{
    gameRank = record.gameRank;
    gameFrames = 0;
    playerX = WIDTH / 8 * PLAYER_SCALE;
    playerY = HEIGHT / 2 * PLAYER_SCALE;
    playerFire = 0;
    playerSlow = 0;
    isDefeated = false;
    initShots();
    initBullets();
    initEnemies();
    explo.a = IMG_EXPLO_ID_MAX;
    randomSeed(record.gameSeed + 1);
    dprint(F("Game start: seed="));
    dprint(record.gameSeed);
    dprint(F(" rank="));
    dprintln(gameRank);
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

static void handleStart()
{
    if (counter >= START_OMIT) {
        updateShots();
        updatePlayer();
    }
    if (counter == START_DURATION) {
        record.playCount++;
        isRecordDirty = true;
        writeRecord();
        state = STATE_PLAYING;
    } else if (arduboy.buttonDown(A_BUTTON)) {
        openGameMenu();
    }
    isInvalid = true;
}

static void handlePlaying()
{
    updateShots();
    updatePlayer();
    updateExplo();
    isDefeated = updateBullets();
    bool isCleared = updateEnemies();
    gameFrames++;
    record.playFrames++;
    isRecordDirty = true;

#ifdef DEBUG
    if (gameFrames % FPS == 0) {
        dprint(F("Bullets="));
        dprintln(bulletsNum);
    }
    if (dbgRecvChar == 'a') {
        isMuteki = !isMuteki;
        dprint(F("Muteki="));
        dprintln(isMuteki);
    }
    if (dbgRecvChar == 'q') isCleared = true;
#endif

    if (isDefeated || isCleared) {
        if (isDefeated) {
            arduboy.playScore(soundOver, SND_PRIO_OVER);
            sparkleLed(LED_IDX_OVER);
            dprintln(F("Game over..."));
        } else {
            arduboy.playScore(soundClear, SND_PRIO_OVER);
            record.isCleared = true;
            dprintln(F("Clear!!"));
        }
        writeRecord();
        counter = 0;
        state = STATE_OVER;
    } else if (arduboy.buttonDown(A_BUTTON)) {
        openGameMenu();
    }
    isInvalid = true;
}

static void handleOver()
{
    if (!isDefeated || counter >= OVER_OMIT) {
        updateShots();
        updateExplo();
        updateBullets();
        updateEnemies();
        if (gameFrames < GAME_DURATION + ENEMY_ACTIVE) gameFrames++;
    }
    if (counter >= OVER_OMIT) {
        if (counter >= 128) counter -= 8;
        if (arduboy.buttonDown(B_BUTTON)) {
            (isDefeated) ? onRetry() : onQuit();
        } else if (arduboy.buttonDown(A_BUTTON)) {
            openGameMenu();
        }
    }
    isInvalid = true;
}

static void openGameMenu(void)
{
    playSoundClick();
    clearMenuItems();
    if (state <= STATE_PLAYING) addMenuItem(F("CONTINUE"), onContinue);
    if (state >= STATE_PLAYING) addMenuItem(F("RETRY"), onRetry);
    addMenuItem(F("BACK TO TITLE"), (state == STATE_PLAYING) ? onConfirmQuit : onQuit);
    uint8_t h = getMenuItemCount() * 6;
    setMenuCoords(19, 54 - h, 89, h - 1, true, true);
    setMenuItemPos((state == STATE_OVER && !isDefeated) ? 1 : 0);
    isMenu = true;
}

static void sparkleLed(uint8_t idx)
{
    if (!record.isBlinkLED) return;
    uint8_t ledValue = pgm_read_byte(ledValues + idx);
    ledRed = ledValue / 25;
    ledGreen = ledValue / 5 % 5;
    ledBlue = ledValue % 5;
    ledLevel = (idx == LED_IDX_OVER) ? LED_LEVEL_OVER : LED_LEVEL_NORMAL;
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

static void updateExplo(void)
{
    if (explo.a < IMG_EXPLO_ID_MAX) explo.a++;
}

/*-----------------------------------------------------------------------------------------------*/

static void newGroup(uint8_t groupIdx)
{
    dprint(F("New group "));
    dprintln(groupIdx);
    int idx = groupIdx % GROUP_MAX;
    GROUP_T *pG = &groups[idx];

    /*  Moving path design  */
    pG->xRad = random(4, 37);
    pG->yRad = random(3, 28);
    pG->yFlip = random(2);
    pG->xCoef = random(4, 64);
    pG->yCoef = random(4, 64);
    pG->entryInt = random(8);

    /*  Bullets design  */
    pG->type = random(ENEMY_TYPE_MAX);
    pG->fireInt = random(FPS, FPS * 3 + 1);
    pG->fireCycle = random(8);
    pG->fireExSpd = random(4);
    uint8_t fires = gameRank * pG->fireInt / FPS / 2;
    if (fires == 0) fires = 1;
    uint8_t fireDiv = (random(4)) ? 1 : random(1, sqrt(fires) + 1);
    uint8_t fireThick = fires / fireDiv;
    if (pG->type <= ENEMY_TYPE_FUNWISE) {
        pG->fireTimes = fireDiv;
        pG->fireNum = fireThick;
    } else {
        pG->fireTimes = fireThick;
        pG->fireNum = fireDiv;
    }
    uint16_t fireCycleMax = pG->fireInt / pG->fireTimes - BULLET_CYC_BASE;
    if (pG->fireCycle > fireCycleMax) pG->fireCycle = fireCycleMax;
    if ((pG->type == ENEMY_TYPE_FUNWISE || pG->type == ENEMY_TYPE_MOWING) && fireThick > 1) {
        uint8_t mowGap = random(32, 64) / (fireThick - 1);
        pG->fireGap = constrain(mowGap, BULLET_GAP_BASE, 23) - BULLET_GAP_BASE;
    } else {
        pG->fireGap = random(1, 5);
    }

    /*  Position of each enemy  */
    ENEMY_T *pE = &enemies[idx * ENEMY_UNITY];
    int8_t x = playerX / PLAYER_SCALE, y = playerY / PLAYER_SCALE;
    for (int i = 0; i < ENEMY_UNITY; i++, pE++) {
        pE->bx = random(pG->xRad / 2, WIDTH / 2 - 3 - pG->xRad);
        pE->by = random(pG->yRad + 3, HEIGHT - 3 - pG->yRad);
        pE->life = ENEMY_LIFE_INIT;
        pE->framesGap = random(256);
        Point enemyPoint = getEnemyCoords(pG, pE, 0);
        pE->fireDeg = myAtan2f(y - enemyPoint.y, x - enemyPoint.x);
    }
}

static bool updateEnemies(void)
{
    /*  New group  */
    if (gameFrames < GAME_DURATION && gameFrames % GROUP_INT == 0) {
        newGroup(gameFrames / GROUP_INT);
    }

    GROUP_T *pG = groups - 1;
    ENEMY_T *pE = enemies;
    uint8_t enemyLives = 0;
    int16_t enemyFrames;
    bool isDamaged = false;
    for (int i = 0; i < ENEMY_MAX; i++, pE++) {
        if (i % ENEMY_UNITY == 0) {
            pG++;
            enemyFrames = (gameFrames - (pG - groups) * GROUP_INT) % (GROUP_INT * GROUP_MAX);
        } else {
            enemyFrames -= (pG->entryInt + 1) * 4;
        }

        /*  Is it active?  */
        if (pE->life == 0) continue;
        if (enemyFrames > ENEMY_ACTIVE) pE->life = 0;
        if (state != STATE_PLAYING) continue;
        if (enemyFrames < ENEMY_FADE || enemyFrames > ENEMY_ACTIVE - ENEMY_FADE) {
            if (enemyFrames == 0) arduboy.playScore(soundEntry, SND_PRIO_ENTRY);
            continue;
        }

        /*  Hit judgement  */
        Point enemyPoint = getEnemyCoords(pG, pE, enemyFrames);
        Rect enemyRect = Rect(enemyPoint.x - 3, enemyPoint.y - 3, 7, 7);
        for (SHOT_T *pS = shots; pE->life > 0 && pS < &shots[SHOT_MAX]; pS++) {
            if (pS->x < WIDTH && arduboy.collide(Point(pS->x, pS->y), enemyRect)) {
                pS->x = WIDTH;
                pE->life--;
                isDamaged = true;
            }
        }
        if (pE->life == 0) { // Defeat!
            arduboy.playScore(soundDefeat, SND_PRIO_DEFEAT);
            sparkleLed(pG->type);
            explo.x = enemyPoint.x;
            explo.y = enemyPoint.y;
            explo.a = 0;
            dprintln(F("Defeat!"));
            continue;
        }
        enemyLives++;

        /*  Should it fire?  */
        uint16_t firePhase = (enemyFrames + pE->framesGap) % pG->fireInt;
        uint8_t fireCycle = pG->fireCycle + BULLET_CYC_BASE;
        if (firePhase % fireCycle > 0) continue;
        uint8_t fireCnt = firePhase / fireCycle;
        if (fireCnt >= pG->fireTimes) continue;

        /*  Fire enemy's bullets  */
        int8_t spd = gameRank + pG->fireExSpd + BULLET_BASE_SPD;
        int16_t x = enemyPoint.x, y = enemyPoint.y;
        int8_t deg = myAtan2f(playerY / PLAYER_SCALE - y, playerX / PLAYER_SCALE - x);
        int8_t mowGap = (pG->fireGap + BULLET_GAP_BASE) * ((pG->fireCycle & 1) * 2 - 1);
        if (fireCnt == 0) {
            pE->fireDeg = deg;
            if (pG->type == ENEMY_TYPE_MOWING && pG->fireTimes > 1) {
                pE->fireDeg -= mowGap * (pG->fireTimes - 1) / 2;
            }
        }
        switch (pG->type) {
        case ENEMY_TYPE_NEEDLE:
            spd = spd * 3 / 2 - pG->fireNum / 2;
            newNeedleBullets(x, y, deg, spd, pG->fireNum);
            break;
        case ENEMY_TYPE_FUNWISE:
            newFanwiseBullets(x, y, deg, spd, pG->fireNum - (fireCnt & 1), mowGap);
            break;
        case ENEMY_TYPE_RAPIDFIRE:
            spd = spd * 3 / 2;
            newFanwiseBullets(x, y, pE->fireDeg, spd, pG->fireNum, pG->fireGap);
            break;
        case ENEMY_TYPE_WHIP:
            spd += fireCnt * 2 - pG->fireNum;
            newFanwiseBullets(x, y, deg, spd, pG->fireNum, pG->fireGap);
            break;
        case ENEMY_TYPE_MOWING:
            newNeedleBullets(x, y, pE->fireDeg, spd, pG->fireNum);
            pE->fireDeg += mowGap;
            break;
        default:
            break;
        }
    }
    if (isDamaged) arduboy.playTone(200, 10);
    return (enemyLives == 0 && gameFrames >= GAME_DURATION);
}

static Point getEnemyCoords(GROUP_T *pG, ENEMY_T *pE, uint16_t frames)
{
    frames += pE->framesGap;
    int8_t xDeg = (frames * pG->xCoef) / pG->xRad;
    int8_t yDeg = (frames * pG->yCoef) / pG->yRad + pG->yFlip * 128;
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
            } else if (arduboy.collide(bulletPoint, playerRect)) {
                ret = true; // Game over...
#ifdef DEBUG
                ret = !isMuteki;
#endif
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

static void onRetry(void)
{
    dprintln(F("Menu: retry"));
    initGame();
}

static void onConfirmQuit(void)
{
    int16_t y = getMenuItemPos() * 6 + 38;
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
    int16_t dx = playerX / PLAYER_SCALE, dy = playerY / PLAYER_SCALE;
    if (isDefeated) {
        uint8_t a = counter / 4;
        if (a < IMG_EXPLO_ID_MAX) {
            arduboy.drawBitmap(dx - 7, dy - 7, imgExplo[a], IMG_EXPLO_W, IMG_EXPLO_H);
        }
    } else {
        if (state == STATE_START && counter < START_OMIT) dx -= (START_OMIT - counter) / 3;
        if (state == STATE_OVER) {
            int16_t a = counter - 16;
            dx += a * a / 32 - 8;
        }
        arduboy.drawBitmap(dx - 3, dy - 3, imgPlayer, IMG_CRAFT_W, IMG_CRAFT_H);
    }
}

static void drawShots(void)
{
    for (SHOT_T *pS = shots; pS < &shots[SHOT_MAX]; pS++) {
        if (pS->x < WIDTH) arduboy.drawFastHLine(pS->x - SHOT_SPD + 1, pS->y, SHOT_SPD, WHITE);
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
    GROUP_T *pG = groups - 1;
    ENEMY_T *pE = enemies;
    int16_t enemyFrames;
    for (int i = 0; i < ENEMY_MAX; i++, pE++) {
        if (i % ENEMY_UNITY == 0) {
            pG++;
            enemyFrames = (gameFrames - (pG - groups) * GROUP_INT) % (GROUP_INT * GROUP_MAX);
        } else {
            enemyFrames -= (pG->entryInt + 1) * 4;
        }
        if (pE->life > 0) {
            int16_t fade = enemyFrames - ENEMY_ACTIVE / 2;
            fade = abs(fade) - (ENEMY_ACTIVE / 2 - ENEMY_FADE);
            Point enemyPoint = getEnemyCoords(pG, pE, enemyFrames);
            if (fade <= 0) {
                arduboy.drawBitmap(enemyPoint.x - 3, enemyPoint.y - 3,
                        imgEnemy[pG->type], IMG_CRAFT_W, IMG_CRAFT_H);
            } else if (fade <= ENEMY_FADE) {
                int8_t z = min(fade, 3);
                arduboy.fillRect(enemyPoint.x - 3 + z, enemyPoint.y - fade * 2,
                        IMG_CRAFT_W - z * 2, IMG_CRAFT_H + fade * 4, WHITE);
            }
        }
    }
}

static void drawExplosion(void)
{
    if (explo.a < IMG_EXPLO_ID_MAX) {
        arduboy.drawBitmap(explo.x - 7, explo.y - 7, imgExplo[explo.a], IMG_EXPLO_W, IMG_EXPLO_H);
    }
}

static void drawBackground(void)
{
    /*  Starts  */
    uint8_t g = counter & 3;
    uint8_t d = g * 71;
    uint8_t *p = (uint8_t *)drawBackground + g * 3;
    int16_t s = (state == STATE_START) ? counter - START_DURATION : gameFrames;
    for (int16_t y = g * 2; y < HEIGHT; y += 8, p += 12) {
        d += pgm_read_byte(p) + 29;
        int16_t x = (d - (s * ((pgm_read_byte(p + 1) & 15) + 8) >> 6)) & (WIDTH - 1);
        arduboy.drawPixel(x, y);
    }

    /*  Edge detection  */
    if (!record.isDrawEdge || state >= STATE_OVER || bitRead(counter, 0)) return;
    int16_t dx = playerX / PLAYER_SCALE - (EDGE_THRESHOLD - 1);
    int16_t dy = playerY / PLAYER_SCALE;
    if (dx <= 0) {
        arduboy.drawFastVLine(0, dy - (EDGE_THRESHOLD - 1), EDGE_THRESHOLD * 2 - 1, WHITE);
    }
    if (dy < EDGE_THRESHOLD) {
        arduboy.drawFastHLine(dx, 0, EDGE_THRESHOLD * 2 - 1, WHITE);
    }
    if (dy >= HEIGHT - EDGE_THRESHOLD) {
        arduboy.drawFastHLine(dx, HEIGHT - 1, EDGE_THRESHOLD * 2 - 1, WHITE);
    }
}

static void drawLetters(void)
{
    switch (state) {
    case STATE_START:
        if (bitRead(counter, 3)) {
            drawLettersBigLabel((WIDTH - IMG_READY_W) / 2, 24, imgReady, IMG_READY_W, IMG_READY_H);
        }
        drawLettersGameSeed();
        break;
    case STATE_PLAYING:
        drawTime(104, 0, (gameFrames < GAME_DURATION) ? GAME_DURATION - 1 - gameFrames : 0);
        break;
    case STATE_OVER:
        if (isDefeated) {
            int16_t a = 30 - min(counter, 30);
            drawLettersBigLabel((WIDTH - IMG_FAILED_W) / 2 + a * a / 10, 24, imgFailed,
                    IMG_FAILED_W, IMG_FAILED_H);
        } else {
            int16_t dy = min(counter, 40) - 16;
            drawLettersBigLabel((WIDTH - IMG_COMPLETED_W) / 2, dy, imgCompleted,
                    IMG_COMPLETED_W, IMG_COMPLETED_H);
        }
        if (counter >= OVER_OMIT) {
           arduboy.fillRect(42, 48, 43, 7, BLACK);
            arduboy.printEx(55, 49, (isDefeated) ? F("RETRY") : F("TITLE"));
            drawButtonIcon(43, 48, true);
            drawLettersGameSeed();
        }
        break;
    default:
        break;
    }
}

static void drawLettersBigLabel(int16_t x, int16_t y, const uint8_t *bitmap, uint8_t w, uint8_t h)
{
    for (int i = 1; i <= 7; i += 2) {
        arduboy.drawBitmap(x + i % 3 - 1, y + i / 3 - 1, bitmap, w, h, BLACK);
    }
    arduboy.drawBitmap(x, y, bitmap, w, h);
}

static void drawLettersGameSeed(void)
{
    bool is2Digits = (gameRank >= 10);
    int16_t dx = 19 - is2Digits * 3;
    arduboy.fillRect(dx - 1, 16, 91 + is2Digits * 6, 7, BLACK);
    arduboy.printEx(dx, 17, F("PATTERN:"));
    printGameSeed(dx + 48, 17, record.gameSeed);
    arduboy.print('-');
    arduboy.print(gameRank);
}
