#include "common.h"
#include "data.h"

/*  Defines  */

enum STATE_T : uint8_t {
    STATE_INIT = 0,
    STATE_START,
    STATE_PLAYING,
    STATE_OVER,
    STATE_MENU,
    STATE_LEAVE,
};

#define CLOUDS_MAX      16
#define GROUNDS_MAX     16
#define FLOWERS_MAX     32

#define DECIMAL_BITS    6
#define PLAYER_ACCEL    (1 << (DECIMAL_BITS - 2))

/*  Typedefs  */

typedef struct {
    int16_t     x;
    int8_t      y;
    uint8_t     life;
} CLOUD_T;

typedef struct {
    uint16_t    wet;
    uint16_t    life;
    uint16_t    growth;
    uint16_t    power;
} GROUND_T;

typedef struct {
    int8_t      x;
    int8_t      y;
} FLOWER_T;

/*  Local Functions  */

static void     handleStart(void);
static void     handlePlaying(void);
static void     handleOver(void);
static uint8_t  updateClouds(void);
static uint8_t  updateGrounds(void);
static void     wetGround(uint8_t groundIndex, uint8_t n);
static void     updateFlowers(void);
static void     movePlayer(void);

static void     onContinue(void);
static void     onConfirmRetry(void);
static void     onRetry(void);
static void     onConfirmQuit(void);
static void     onQuit(void);

static void     drawStart(void);
static void     drawPlaying(void);
static void     drawOver(void);
static void     drawPlayer(void);
static void     drawClouds(void);
static void     drawGrounds(void);
static void     drawFlowers(void);

/*  Local Functions (macros)  */

#define callHandlerFunc(n)  ((void (*)(void)) pgm_read_ptr(handlerFuncTable - 1 + n))()
#define callDrawerFunc(n)   ((void (*)(void)) pgm_read_ptr(drawerFuncTable - 1 + n))()

/*  Local Constants  */

PROGMEM static void(*const handlerFuncTable[])(void) = {
    handleStart, handlePlaying, handleOver, handleMenu
};

PROGMEM static void(*const drawerFuncTable[])(void) = {
    drawStart, drawPlaying, drawOver, drawPlaying
};

/*  Local Variables  */

static STATE_T  state = STATE_INIT;
static CLOUD_T  clouds[CLOUDS_MAX];
static GROUND_T grounds[GROUNDS_MAX];
static FLOWER_T flowers[FLOWERS_MAX];
static int16_t  playerX, playerY, playerVx, playerVy;
static int16_t  currentWind, targetWind, currentWindGap, targetWindGap;
static uint16_t score, cloudSource, windKeepFrames;
static uint8_t  cloudIndex, flowerIndex;
static bool     isHiscore;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initGame(void)
{
    ab.playScore(soundStart, SND_PRIO_START);
    playerX = (WIDTH / 2) << DECIMAL_BITS;
    playerY = 12 << DECIMAL_BITS;
    playerVx = playerVy = 0;
    currentWind = targetWind = currentWindGap = targetWindGap = 0;
    lastScore = 0;
    score = 0;
    cloudSource = 100;
    windKeepFrames = 1800; // 30 seconds
    counter = 2 * FPS;
    state = STATE_START;
    isInvalid = true;

    for (uint8_t i = 0; i < CLOUDS_MAX; i++) {
        clouds[i].life = 0;
    }
    for (uint8_t i = 0; i < GROUNDS_MAX; i++) {
        grounds[i].wet = 0;
        grounds[i].growth = 0;
        grounds[i].power = 0;
    }
    for (uint8_t i = 0; i < FLOWERS_MAX; i++) {
        flowers[i].y = -IMG_FLOWER_H;
    }
    cloudIndex = 0;
    flowerIndex = 0;
}

MODE_T updateGame(void)
{
    callHandlerFunc(state);
    return (state == STATE_LEAVE) ? MODE_TITLE : MODE_GAME;
}

void drawGame(void)
{
    if (state == STATE_LEAVE) return;
    if (isInvalid) {
        ab.clear();
        callDrawerFunc(state);
    }
    if (state == STATE_MENU) drawMenuItems(isInvalid);
    isInvalid = false;
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void handleStart(void)
{
    movePlayer();
    if (--counter == 0) {
        record.playCount++;
        isRecordDirty = true;
        state = STATE_PLAYING;
    }
    isInvalid = true;
}

static void handlePlaying(void)
{
    record.playFrames++;
    isRecordDirty = true;
    uint8_t coundsCount = updateClouds();
    uint8_t plantsCount = updateGrounds();
    updateFlowers();
    movePlayer();

    /*  Manage the wind  */
    int16_t diffWind = targetWind - currentWind;
    if (diffWind != 0) currentWind += ((diffWind > 0) ? 1 : -1);
    int16_t diffWindGap = targetWindGap - currentWindGap;
    if (diffWindGap != 0) currentWindGap += (diffWindGap > 0) ? 1 : -1;
    if (--windKeepFrames == 0) {
        uint16_t range = min((record.playFrames - 1500) >> 8, 128);
        targetWind = random(range * 2 + 1) - range;
        if (range > 16) {
            range -= 16;
            targetWindGap = random(range + 1) - range / 2;
        } else {
            targetWindGap = 0;
        }
        uint16_t minFrames = max(360 - (record.playFrames >> 6), 60);
        windKeepFrames = random(600 - minFrames) + minFrames;
    }

    /*  Generate a cloud  */
    if (cloudSource > 0 && ab.buttonDown(B_BUTTON) && clouds[cloudIndex].life == 0) {
        CLOUD_T *p = &clouds[cloudIndex];
        p->x = (playerX - (IMG_CLOUD_W / 2 << DECIMAL_BITS)) & ((WIDTH << DECIMAL_BITS) - 1);
        p->y = (playerY >> DECIMAL_BITS) - IMG_CLOUD_H / 2;
        p->life = 180; // 3 seconds
        cloudSource--;
        cloudIndex = circulate(cloudIndex, 1, CLOUDS_MAX);
    }

    if (cloudSource == 0 && coundsCount == 0 && plantsCount == 0) {
        /*  Game over  */
        ab.playScore(soundOver, SND_PRIO_OVER);
        isHiscore = enterScore(score);
        writeRecord();
        counter = FPS;
        state = STATE_OVER;
    } else if (ab.buttonDown(A_BUTTON)) {
        /*  Pause  */
        playSoundClick();
        clearMenuItems();
        addMenuItem(F("CONTINUE"), onContinue);
        addMenuItem(F("RESTART GAME"), onConfirmRetry);
        addMenuItem(F("BACK TO TITLE"), onConfirmQuit);
        setMenuCoords(19, 23, 89, 17, true, true);
        setMenuItemPos(0);
        writeRecord();
        ab.setRGBled(0, 0, 0);
        state = STATE_MENU;
    }
    isInvalid = true;
}

static void handleOver(void)
{
    updateGrounds();
    updateFlowers();
    movePlayer();
    if (counter > 0) {
        counter--;
    } else if (ab.buttonDown(A_BUTTON)) {
        onQuit();
    } else if (ab.buttonDown(B_BUTTON)) {
        onRetry();
    }
    isInvalid = true;
}

static uint8_t updateClouds(void)
{
    uint8_t ret = 0;
    for (CLOUD_T *p = clouds; p < &clouds[CLOUDS_MAX]; p++) {
        if (p->life == 0) continue;
        int16_t wind = currentWind + currentWindGap * (p->y - 12) / 12;
        p->x = (p->x + wind) & ((WIDTH << DECIMAL_BITS) - 1);
        p->life--;
        if (p->life < 120) {
            uint8_t gx = p->x >> DECIMAL_BITS;
            uint8_t groundIndex = gx >> 3, odd = gx & 7;
            wetGround(groundIndex, 8 - odd);
            if (odd > 0) wetGround(circulate(groundIndex, 1, GROUNDS_MAX), odd);
        }
        ret++;
    }
    return ret;
}

static void wetGround(uint8_t groundIndex, uint8_t n)
{
    GROUND_T *p = &grounds[groundIndex];
    p->wet = (p->wet <= 4095 - n) ? p->wet + n : 4095;
}

static uint8_t updateGrounds(void)
{
    uint8_t ret = 0;
    for (uint8_t i = 0; i < GROUNDS_MAX; i++) {
        GROUND_T *p = &grounds[i];
        p->wet = (p->wet >= 2) ? p->wet - 2 : 0;
        uint8_t wetLevel = p->wet / 512; // 0~7
        if (p->growth == 0) {
            if (wetLevel == 3 || wetLevel == 4) {
                p->power += 2;
                if (p->power >= 360) {
                    // Sprout
                    p->growth = 1;
                    p->life = 1800; // 30 seconds
                }
            } else {
                p->power = 0;
            }
        } else if (p->power == 0) {
            // Decay
            p->growth = (p->growth >= 256) ? p->growth -= 256 : 0;
        } else {
            if (wetLevel == 0 || wetLevel == 7) {
                // Damage
                p->power--;
            } else {
                if (p->power < 360) {
                    // Recover
                    p->power += 6;
                    if (p->power > 360) p->power = 360;
                } else {
                    // Grow
                    p->growth += (wetLevel == 3 || wetLevel == 4) ? 4 : 1;
                }
                p->life--;
                if (p->life == 0) {
                    if (p->power >= 300) {
                        // Scatter flowers
                        uint8_t points = (p->growth >> 10) + 1;
                        score += points;
                        cloudSource += points * 2;
                        int8_t x = i * IMG_GROUND_W;
                        int8_t y = 56 - (p->growth >> 8);
                        for (uint8_t i = 0; i < points; i++) {
                            FLOWER_T *p = &flowers[flowerIndex];
                            p->x = x - (i & 1) * 4 - !i * 2 + 2 + random(2);
                            p->y = y;
                            y += 4;
                            flowerIndex = circulate(flowerIndex, 1, FLOWERS_MAX);
                        }
                    }
                    // Wither
                    p->power = 0;
                }
            }
        }
        if (p->growth > 0 && p->power > 0) ret++;
    }
    return ret;
}

static void updateFlowers(void)
{
    for (FLOWER_T *p = flowers; p < &flowers[FLOWERS_MAX]; p++) {
        if (p->y <= -IMG_FLOWER_H) continue;
        if (random(3)) {
            p->y--;
        } else {
            p->x += random(2) * 2 - 1;
        }
    }
}

static void movePlayer(void)
{
    playerVx = playerVx * 7 / 8;
    playerVy = playerVy * 7 / 8;
    if (ab.buttonPressed(LEFT_BUTTON))  playerVx -= PLAYER_ACCEL;
    if (ab.buttonPressed(RIGHT_BUTTON)) playerVx += PLAYER_ACCEL;
    if (ab.buttonPressed(UP_BUTTON))    playerVy -= PLAYER_ACCEL;
    if (ab.buttonPressed(DOWN_BUTTON))  playerVy += PLAYER_ACCEL;
    playerX = (playerX + playerVx) & ((WIDTH << DECIMAL_BITS) - 1);
    playerY += playerVy;
    playerY = clamp(playerY, 0, (25 << DECIMAL_BITS) - 1);
}

/*---------------------------------------------------------------------------*/
/*                               Menu Handlers                               */
/*---------------------------------------------------------------------------*/

static void onContinue(void)
{
    playSoundClick();
    state = STATE_PLAYING;
    isInvalid = true;
}

static void onConfirmRetry(void)
{
    int16_t y = getMenuItemPos() * 6 + 25;
    setConfirmMenu(y, onRetry, onContinue);
}

static void onRetry(void)
{
    initGame();
}

static void onConfirmQuit(void)
{
    int16_t y = getMenuItemPos() * 6 + 25;
    setConfirmMenu(y, onQuit, onContinue);
}

static void onQuit(void)
{
    playSoundClick();
    state = STATE_LEAVE;
}

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void drawStart(void)
{
    drawPlayer();
    drawGrounds();
    ab.printEx(46, 29, F("READY?"));
    ab.printEx(0, 0, score);
}

static void drawPlaying(void)
{
    drawPlayer();
    drawClouds();
    drawGrounds();
    drawFlowers();
    ab.printEx(0, 0, score);
    ab.printEx(110, 0, cloudSource);
    //ab.printEx(0, 6, currentWind);
    //ab.printEx(0, 12, currentWindGap);
}

static void drawOver(void)
{
    drawPlayer();
    drawGrounds();
    drawFlowers();
    ab.printEx(0, 0, score);
    ab.printEx(37, 29, F("GAME OVER"));
    if (isHiscore) ab.printEx(31, 40, F("NEW RECORD!"));
}

static void drawPlayer(void)
{
    uint8_t dx = playerX >> DECIMAL_BITS;
    uint8_t dy = playerY >> DECIMAL_BITS;
    ab.drawBitmap(dx - 6, dy - 4, imgPlayer, IMG_PLAYER_W, IMG_PLAYER_H, WHITE);
}

static void drawClouds(void)
{
    for (CLOUD_T *p = clouds; p < &clouds[CLOUDS_MAX]; p++) {
        if (p->life == 0) continue;
        uint8_t dx = p->x >> DECIMAL_BITS;
        uint8_t imgIndex = (dx + p->y + record.playFrames) & 1;
        if (p->life < 30)  imgIndex += (30 - p->life) / 4 * 2;
        if (p->life > 150) imgIndex += (p->life - 150) / 4 * 2;
        ab.drawBitmap(dx, p->y, imgCloud[imgIndex], IMG_CLOUD_W, IMG_CLOUD_H, WHITE);
        if (dx > WIDTH - IMG_CLOUD_W) {
            ab.drawBitmap(dx - WIDTH, p->y, imgCloud[imgIndex], IMG_CLOUD_W, IMG_CLOUD_H, WHITE);
        }
        if (p->life < 120) {
            uint8_t h = (60 - abs(p->life - 60)) / 4;
            for (uint8_t i = 0; i < 8; i++) {
                uint8_t rx = (dx + random(IMG_CLOUD_W)) & (WIDTH - 1);
                uint8_t ry = p->y + IMG_CLOUD_H + random(60 - h);
                ab.drawFastVLine(rx, ry, h, WHITE);
            }
        }
    }
}

static void drawGrounds(void)
{
    for (uint8_t i = 0; i < GROUNDS_MAX; i++) {
        GROUND_T *p = &grounds[i];
        uint8_t x = i * IMG_GROUND_W;
        if (p->growth > 0) {
            uint8_t y = 60 - (p->growth >> 8);
            uint8_t imgIndex = 0;
            if (p->power < 360) {
                imgIndex = 2;
            } else if (p->life <= 180) {
                imgIndex = 4 + !(record.playFrames & 3) * 2;
            }
            ab.drawBitmap(x, y - IMG_PLANT_H, imgPlant[imgIndex], IMG_PLANT_W, IMG_PLANT_H, WHITE);
            imgIndex++;
            while (y < 60) {
                ab.drawBitmap(x, y, imgPlant[imgIndex], IMG_PLANT_W, IMG_PLANT_H, WHITE);
                y += IMG_PLANT_H;
            }
        }
    }
    ab.fillRect(0, 60, WIDTH, 4, BLACK);
    for (uint8_t i = 0; i < GROUNDS_MAX; i++) {
        uint8_t imgIndex = (grounds[i].wet + 512) >> 10;
        ab.drawBitmap(i * IMG_GROUND_W, HEIGHT - IMG_GROUND_H, imgGround[imgIndex], IMG_GROUND_W, IMG_GROUND_H, WHITE);
    }
}

static void drawFlowers(void)
{
    uint8_t imgIndex = record.playFrames % 3;
    for (FLOWER_T *p = flowers; p < &flowers[FLOWERS_MAX]; p++) {
        if (p->y <= -IMG_FLOWER_H) continue;
        ab.drawBitmap(p->x, p->y, imgFlower[imgIndex], IMG_FLOWER_W, IMG_FLOWER_H, WHITE);
    }
}
