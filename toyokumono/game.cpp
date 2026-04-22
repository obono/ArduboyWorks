#include "common.h"
#include "data.h"

/*  Defines  */

enum STATE_T : uint8_t {
    STATE_INIT = 0,
    STATE_START,
    STATE_PLAYING,
    STATE_OVER,
    STATE_MENU,
    STATE_INSTRUCTION,
    STATE_LEAVE,
};

#define CLOUDS_MAX          16
#define GROUNDS_MAX         16
#define FLOWERS_MAX         32

#define PLAYER_Y_MAX        24
#define PLAYER_ACCEL        (1 << (DECIMAL_BITS - 2))
#define ELEMENT_INITIAL     100
#define CLOUD_LIFE_MAX      180  // 3 seconds
#define GROUND_WET_MAX      4095
#define PLANT_LIFE_MAX      1800 // 30 seconds
#define PLANT_POWER_MAX     360
#define PLANT_POWER_FINE    240
#define PLANT_POWER_SPROUT  2
#define PLANT_POWER_RECOVER 6
#define PLANT_DECAY         256
#define PLANT_HEIGHT_SHIFT  8
#define CALM_FRAMES         1800 // 30 seconds
#define ENDURE_FRAMES_MAX   180  // 3 seconds
#define INSTRUCTION_FRAMES  600  // 10 seconds

/*  Typedefs  */

typedef struct {
    int16_t     x;      // 0~8191
    int8_t      y;      // -3~21
    uint8_t     life;   // 0~180
} CLOUD_T;

typedef struct {
    uint16_t    wet;    // 0~4095
    uint16_t    life;   // 0~1800
    uint16_t    growth; // 0~7200
    uint16_t    power;  // 0~360
} GROUND_T;

typedef struct {
    uint8_t     r : 4;
    uint8_t     g : 4;
    uint8_t     b : 4;
    uint8_t     v : 4;
} LED_T;

/*  Local Functions  */

static void     handleStart(void);
static void     handlePlaying(void);
static void     handleOver(void);
static void     handleInstruction(void);
static uint8_t  updateClouds(void);
static uint8_t  updateGrounds(void);
static void     newCloud(int16_t x, int8_t y);
static void     wetGround(uint8_t groundIndex, uint8_t n);
static void     updateFlowers(void);
static void     newFlower(int8_t x, int8_t y);
static void     updateDots(void);
static void     newDot(int8_t x, int8_t y, int8_t vx, int8_t vy);
static void     updatePlayer(void);
static void     updateWind(void);
static void     setupInstruction(void);
static void     flashLed(uint8_t r, uint8_t g, uint8_t b);

static void     onContinue(void);
static void     onConfirmRetry(void);
static void     onRetry(void);
static void     onConfirmQuit(void);
static void     onQuit(void);

static void     drawStart(void);
static void     drawPlaying(void);
static void     drawOver(void);
static void     drawInstruction(void);
static void     drawScore(void);
static void     drawPlayer(void);
static void     drawClouds(void);
static void     drawGrounds(void);
static void     drawFlowers(void);
static void     drawDots(void);

/*  Local Functions (macros)  */

#define callHandlerFunc(n)  ((void (*)(void)) pgm_read_ptr(handlerFuncTable - 1 + n))()
#define callDrawerFunc(n)   ((void (*)(void)) pgm_read_ptr(drawerFuncTable - 1 + n))()
#define loopWithinWidth(x)  ((x) & ((WIDTH << DECIMAL_BITS) - 1))

/*  Local Constants  */

PROGMEM static void(*const handlerFuncTable[])(void) = {
    handleStart, handlePlaying, handleOver, handleMenu, handleInstruction
};

PROGMEM static void(*const drawerFuncTable[])(void) = {
    drawStart, drawPlaying, drawOver, drawPlaying, drawInstruction
};

PROGMEM static const char instructionText1[] =
        "YOU MOVE BY D-PAD\0AND SUMMON CLOUDS\0BY PRESSING B OR\0AUTOMATICALLY.\0\e";
PROGMEM static const char instructionText2[] =
    "RAIN WETS GROUND.\0PLANTS SPROUT IF\0GROUND IS IN BEST\0CONDITION.\0\e";
PROGMEM static const char instructionText3[] =
    "PLANTS GROW WHILE\0GROUND IS IN GOOD\0CONDITION. FINALLY\0THEY BLOOM FLOWERS.\0\e";
PROGMEM static const char instructionText4[] =
    "FLOWERS RESTORE\0CLOUD SUMMON USES.\0GOOD LUCK!\0\e";
PROGMEM static const char * const instructionTextList[] = {
    instructionText1, instructionText2, instructionText3, instructionText4
};

/*  Local Variables  */

static STATE_T  state = STATE_INIT;
static CLOUD_T  clouds[CLOUDS_MAX];
static GROUND_T grounds[GROUNDS_MAX];
static FLYING_T flowers[FLOWERS_MAX];
static LED_T    led;
static int16_t  playerX, playerY, playerVx, playerVy, windKeepFrames;
static uint16_t score, elements, gameFrames, overFrames;
static int8_t   currentWind, targetWind, currentWindGap, targetWindGap, scoreY;
static uint8_t  endureFrames, cloudIndex, flowerIndex, playerImgIndex, instructionPage;
static bool     isFast, isHiscore;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initGame(void)
{
    gameFrames = 0;
    memset(clouds, 0, sizeof(clouds));
    memset(grounds, 0, sizeof(grounds));
    memset(flowers, 0, sizeof(flowers));
    memset(dots, 0, sizeof(dots));
    cloudIndex = 0;
    flowerIndex = 0;
    currentWind = targetWind = currentWindGap = targetWindGap = 0;
    windKeepFrames = CALM_FRAMES;

    if (isInstruction) {
        instructionPage = 0;
        state = STATE_INSTRUCTION;
    } else {
        lastScore = 0;
        score = 0;
        scoreY = 0;
        if (state != STATE_OVER) {
            playerX = (WIDTH / 2) << DECIMAL_BITS;
            playerY = 12 << DECIMAL_BITS;
            playerVx = playerVy = 0;
        }
        elements = 100;
        endureFrames = ENDURE_FRAMES_MAX;
        counter = 2 * FPS;
        state = STATE_START;
        ab.playScore(soundStart, SND_PRIO_START);
    }
    led.v = 0;
    isInvalid = true;
}

MODE_T updateGame(void)
{
    callHandlerFunc(state);
    if (state != STATE_MENU) {
        int8_t dx = playerX >> DECIMAL_BITS;
        int8_t dy = playerY >> DECIMAL_BITS;
        if (state == STATE_PLAYING && dy < 16 && (dx < 36 || dx >= 92)) {
            if (scoreY > -6) scoreY--;
        } else {
            if (scoreY < 0) scoreY++;
        }
    }
    if (state != STATE_MENU && state != STATE_LEAVE && led.v > 0) {
        ab.setRGBled(led.r * led.v, led.g * led.v, led.b * led.v);
        led.v--;
    } else {
        ab.setRGBled(0, 0, 0);
    }
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
    updateDots();
    updatePlayer();
    if (--counter == 0) {
        record.playCount++;
        isRecordDirty = true;
        state = STATE_PLAYING;
    }
    isInvalid = true;
}

static void handlePlaying(void)
{
    gameFrames++;
    record.playFrames++;
    isRecordDirty = true;
    uint8_t cloudsCount = updateClouds();
    uint8_t plantsCount;
    isFast = false;
    for (uint8_t i = 0; i < 8; i++) {
        plantsCount = updateGrounds();
        if (elements > 0 || cloudsCount > 0) break;
        isFast = true;
    }
    updateFlowers();
    updateDots();
    updatePlayer();
    updateWind();

    if (elements == 0 && cloudsCount == 0 && plantsCount == 0) {
        /*  Game over  */
        isHiscore = enterScore(score);
        writeRecord();
        flashLed(8, 8, 8);
        overFrames = 0;
        state = STATE_OVER;
        ab.playScore(soundOver, SND_PRIO_OVER);
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
        state = STATE_MENU;
    }
    isInvalid = true;
}

static void handleOver(void)
{
    gameFrames++;
    updateGrounds();
    updateFlowers();
    updateDots();
    updatePlayer();
    overFrames++;
    if (overFrames >= FPS * 8) {
        isTitleAnimation = true;
        state = STATE_LEAVE;
    } else if (overFrames >= FPS) {
        if (ab.buttonDown(A_BUTTON)) {
            onQuit();
        } else if (ab.buttonDown(B_BUTTON)) {
            onRetry();
        }
    }
    isInvalid = true;
}

static void handleInstruction(void)
{
    if (gameFrames == 0) setupInstruction();
    if (instructionPage == 0) {
        updateClouds();
    } else {
        updateGrounds();
    }
    updateFlowers();
    updateDots();
    if (instructionPage == 0 && gameFrames % 45 == 30) {
        newCloud(random(WIDTH << DECIMAL_BITS), random(32) + 28);
    }
    if (++gameFrames == INSTRUCTION_FRAMES) gameFrames = 0;
    if (ab.buttonDown(A_BUTTON)) {
        onQuit();
    } else if (ab.buttonDown(B_BUTTON)) {
        if (++instructionPage >= 4) {
            onQuit();
        } else {
            playSoundTick();
            gameFrames = 0;
        }
    } else {
        int8_t v = ab.buttonDown(RIGHT_BUTTON) - ab.buttonDown(LEFT_BUTTON);
        if (v != 0 && instructionPage + v >= 0 && instructionPage + v < 4) {
            playSoundTick();
            instructionPage += v;
            gameFrames = 0;
        }
    }
    isInvalid = true;
}

/*---------------------------------------------------------------------------*/

static uint8_t updateClouds(void)
{
    uint8_t ret = 0;
    for (CLOUD_T *p = clouds; p < &clouds[CLOUDS_MAX]; p++) {
        if (p->life == 0) continue;
        int16_t wind = currentWind + currentWindGap * (p->y - 12) / 12;
        p->x = loopWithinWidth(p->x + wind);
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

static void newCloud(int16_t x, int8_t y)
{
    CLOUD_T *p = &clouds[cloudIndex];
    p->x = loopWithinWidth(x - (IMG_CLOUD_W / 2 << DECIMAL_BITS));
    p->y = y - IMG_CLOUD_H / 2;
    p->life = CLOUD_LIFE_MAX;
    cloudIndex = (cloudIndex + 1) % CLOUDS_MAX;
    ab.playScore(soundCloud, SND_PRIO_CLOUD);
}

static void wetGround(uint8_t groundIndex, uint8_t n)
{
    GROUND_T *p = &grounds[groundIndex];
    addWithLimit(p->wet, n, GROUND_WET_MAX);
}

static uint8_t updateGrounds(void)
{
    uint8_t ret = 0;
    uint8_t dry = (gameFrames >= 18000) ? gameFrames / 3600 - 2 : 2; 
    for (uint8_t i = 0; i < GROUNDS_MAX; i++) {
        GROUND_T *p = &grounds[i];
        if (!isInstruction) subWithLimit(p->wet, dry, 0);
        uint8_t wetLevel = p->wet / 512; // 0~7
        if (p->growth == 0) {
            if (wetLevel == 3 || wetLevel == 4) {
                addWithLimit(p->power, PLANT_POWER_SPROUT, PLANT_POWER_MAX);
                if (p->power == PLANT_POWER_MAX) {
                    // Sprout
                    p->growth = 1;
                    p->life = PLANT_LIFE_MAX;
                    flashLed(0, 4, 0);
                    for (uint8_t j = 0; j < 8; j++) {
                        newDot(i * IMG_GROUND_W + 3, 60, random(91) - 45, -48 - random(49));
                    }
                    ab.playScore(soundSprout, SND_PRIO_SPROUT);
                }
            } else {
                p->power = 0;
            }
        } else if (p->power == 0) {
            // Decay
            subWithLimit(p->growth, PLANT_DECAY, 0);
        } else {
            if (wetLevel == 0 || wetLevel == 7) {
                p->power--;
                if (p->power == PLANT_POWER_FINE) {
                    // Damage
                    flashLed(2, 1, 0);
                    ab.playScore(soundDamage, SND_PRIO_DAMAGE);
                } else if (p->power == 0) {
                    // Wither
                    flashLed(6, 0, 0);
                    ab.playScore(soundWither, SND_PRIO_WITHER);
                }
            } else {
                if (p->power < PLANT_POWER_MAX) {
                    // Recover
                    addWithLimit(p->power, PLANT_POWER_RECOVER, PLANT_POWER_MAX);
                } else {
                    // Grow
                    if (wetLevel == 3 || wetLevel == 4) {
                        p->growth += 4;
                    } else if (wetLevel == 2 || wetLevel == 5) {
                        p->growth += 2;
                    } else if (wetLevel == 1 || wetLevel == 6) {
                        p->growth++;
                    }
                }
                p->life--;
                if (p->life == 0) {
                    if (p->power > PLANT_POWER_FINE) {
                        // Scatter flowers
                        uint8_t points = (p->growth >> (PLANT_HEIGHT_SHIFT + 2)) + 1;
                        score += points;
                        elements += points * 2;
                        int8_t x = i * IMG_GROUND_W;
                        int8_t y = 60 - (p->growth >> PLANT_HEIGHT_SHIFT);
                        for (uint8_t i = 0; i < points; i++) {
                            newFlower(x + 6 - (i & 1) * 6 - !i * 3, y);
                            y += 4;
                            flowerIndex = (flowerIndex + 1) % FLOWERS_MAX;
                        }
                        flashLed(4, 0, 8);
                        ab.playScore(soundScatter, SND_PRIO_SCATTER);
                    } else {
                        // Wither
                        flashLed(6, 0, 0);
                        ab.playScore(soundWither, SND_PRIO_WITHER);
                    }
                    p->power = 0;
                }
            }
        }
        if (p->growth > 0 && p->power > 0 || wetLevel >= 3) ret++;
    }
    return ret;
}

static void updateFlowers(void)
{
    for (FLYING_T *p = flowers; p < &flowers[FLOWERS_MAX]; p++) {
        if (p->y <= 0) continue;
        p->x = loopWithinWidth(p->x + p->vx);
        p->y += p->vy;
        subWithLimit(p->vy, 1, -128);
    }
}

static void newFlower(int8_t x, int8_t y)
{
    FLYING_T *p = &flowers[flowerIndex];
    p->x = loopWithinWidth((x * 2 + 1) << (DECIMAL_BITS - 1));
    p->y = y << DECIMAL_BITS;
    p->vx = random(33) - 16;
    p->vy = random(17) - 8;
    flowerIndex = (flowerIndex + 1) % FLOWERS_MAX;
}

static void updateDots(void)
{
    for (FLYING_T *p = dots; p < &dots[DOTS_MAX]; p++) {
        if (p->y <= 0 || (p->y >> DECIMAL_BITS) >= HEIGHT) continue;
        p->x = loopWithinWidth(p->x + p->vx);
        p->y += p->vy;
        addWithLimit(p->vy, 3, 127);
    }
}

static void newDot(int8_t x, int8_t y, int8_t d, int8_t r)
{
    FLYING_T *p = &dots[dotIndex];
    p->x = loopWithinWidth(x << DECIMAL_BITS);
    p->y = y << DECIMAL_BITS;
    float deg = d * DEG_TO_RAD;
    p->vx = sin(deg) * r;
    p->vy = cos(deg) * r;
    dotIndex = (dotIndex + 1) % DOTS_MAX;
}

static void updatePlayer(void)
{
    /*  Move  */
    playerVx = playerVx * 7 / 8;
    playerVy = playerVy * 7 / 8;
    if (ab.buttonPressed(LEFT_BUTTON))  playerVx -= PLAYER_ACCEL;
    if (ab.buttonPressed(RIGHT_BUTTON)) playerVx += PLAYER_ACCEL;
    if (ab.buttonPressed(UP_BUTTON))    playerVy -= PLAYER_ACCEL;
    if (ab.buttonPressed(DOWN_BUTTON))  playerVy += PLAYER_ACCEL;
    playerX = loopWithinWidth(playerX + playerVx);
    playerY += playerVy;
    playerY = clamp(playerY, 0, ((PLAYER_Y_MAX + 1) << DECIMAL_BITS) - 1);
    playerImgIndex = 4 - (playerVy < -PLAYER_ACCEL) * 3 - (playerVx < -PLAYER_ACCEL)
                       + (playerVy >  PLAYER_ACCEL) * 3 + (playerVx >  PLAYER_ACCEL);
    if (elements > 0 && random(16) == 0) {
        newDot((playerX >> DECIMAL_BITS) + random(6) - 3,
               (playerY >> DECIMAL_BITS) + random(3) + 1, 90, currentWind);
    }
    if (state != STATE_PLAYING) return;

    /*  Summon a cloud  */
    if (endureFrames > 0) endureFrames--;
    if (elements > 0 && (ab.buttonDown(B_BUTTON) || endureFrames == 0) && clouds[cloudIndex].life == 0) {
        newCloud(playerX, playerY >> DECIMAL_BITS);
        elements--;
        uint8_t endureNext = (elements < 220) ? 120 - elements / 2 : 10;
        addWithLimit(endureFrames, endureNext, ENDURE_FRAMES_MAX);
    }
    if (elements <= 10 && gameFrames % (FPS / 2) == 0 || elements <= 20 && gameFrames % FPS == 0) {
        ab.playScore(soundWarn, SND_PRIO_WARN);
    }
}

static void updateWind(void)
{
    int16_t diff = targetWind - currentWind;
    if (diff != 0) currentWind += (diff > 0) ? 1 : -1;
    diff = targetWindGap - currentWindGap;
    if (diff != 0) currentWindGap += (diff > 0) ? 1 : -1;
    if (--windKeepFrames <= 0) {
        int8_t range = min((gameFrames - 1500) >> 8, 120);
        targetWind = random(range * 2 + 1) - range;
        if (range > 16) {
            range = (range - 16) / 2;
            targetWindGap = random(range * 2 + 1) - range;
        } else {
            targetWindGap = 0;
        }
        uint16_t minFrames = max(360 - (gameFrames >> 6), 60);
        windKeepFrames = random(600 - minFrames) + minFrames;
    }
}

static void setupInstruction(void)
{
    if (instructionPage == 0) {
        memset(clouds, 0, sizeof(clouds));
    } else {
        for (uint8_t i = 0; i < GROUNDS_MAX; i++) {
            GROUND_T *p = &grounds[i];
            if (i % 3 == 0) {
                memset(p, 0, sizeof(GROUND_T));
                continue;
            }
            p->wet = (i % 3 == 0) ? 0 : (i / 3) * 1023;
            switch (instructionPage) {
                case 1:
                    p->growth = p->life = 0;
                    p->power = random(60);
                    break;
                case 2:
                    p->growth = 1024 + random(512);
                    p->life = PLANT_LIFE_MAX;
                    p->power = PLANT_POWER_MAX - random(60); 
                    break;
                case 3:
                    if (i >= 5 && i <= 10) {
                        p->growth = ((i == 7 || i == 8) ? 3072 : 1024) + random(1024);
                        p->life = 420 + random(120);
                        p->power = PLANT_POWER_MAX; 
                    } else {
                        p->growth = p->life = p->power = 0;
                    }
                    break;
            }
        }
    }
}

static void flashLed(uint8_t r, uint8_t g, uint8_t b)
{
    led.r = r;
    led.g = g;
    led.b = b;
    led.v = 15;
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
    drawScore();
    drawPlayer();
    drawGrounds();
    drawDots();
    ab.drawBitmap(39, 24, imgReady, IMG_READY_W, IMG_READY_H);
    ab.drawBitmap(80, 40, imgReadyExtra, 4, 2);
}

static void drawPlaying(void)
{
    drawScore();
    drawPlayer();
    drawClouds();
    drawGrounds();
    drawFlowers();
    drawDots();
    if (isFast && (gameFrames & 4)) ab.drawBitmap(56, 0, imgFast, IMG_FAST_W, IMG_FAST_H);
}

static void drawOver(void)
{
    drawPlayer();
    drawGrounds();
    drawFlowers();
    drawScore();
    drawDots();
    ab.drawBitmap(18, 24, imgGameOver, IMG_GAMEOVER_W, IMG_GAMEOVER_H);
    ab.drawBitmap(53, 40, imgGameOverExtra, 2, 3);
    if (isHiscore && (gameFrames & 8)) ab.printEx(31, 44, F("NEW RECORD!"));
}

static void drawInstruction(void)
{
    const char *p = (const char *)pgm_read_ptr(&instructionTextList[instructionPage]);
    drawText(p, 0);
    ab.printEx(110, 24, instructionPage + 1);
    ab.print(F("/4"));
    if (gameFrames >= 20) {
        if (instructionPage == 0) {
            drawClouds();
        } else {
            if (instructionPage == 1) {
                ab.printEx(7, 33, F("TOO"));
                ab.printEx(7, 39, F("DRY"));
                ab.printEx(52, 36, F("BEST"));
                ab.printEx(103, 33, F("TOO"));
                ab.printEx(103, 39, F("WET"));
            }
            drawGrounds();
            drawDots();
        }
    }
    drawFlowers();
}

/*---------------------------------------------------------------------------*/

static void drawScore(void)
{
    if (scoreY > -5) {
        ab.printEx(0, scoreY, F("SCORE"));
        ab.printEx(98, scoreY, F("CLOUD"));
    }
    ab.printEx(0, scoreY + 6, score);
    if (state != STATE_PLAYING ||
            !(elements <= 10 && !(gameFrames & 4) || elements <= 20 && !(gameFrames & 12))) {
        ab.printEx(122 - ((elements >= 10) + (elements >= 100)) * 6, scoreY + 6, elements);
    }
}

static void drawPlayer(void)
{
    uint8_t dx = playerX >> DECIMAL_BITS;
    uint8_t dy = playerY >> DECIMAL_BITS;
    ab.drawBitmap(dx - 6, dy - 4, imgPlayer[playerImgIndex], IMG_PLAYER_W, IMG_PLAYER_H, WHITE);
}

static void drawClouds(void)
{
    for (CLOUD_T *p = clouds; p < &clouds[CLOUDS_MAX]; p++) {
        if (p->life == 0) continue;
        uint8_t dx = p->x >> DECIMAL_BITS;
        uint8_t imgIndex = (dx + p->y + gameFrames) & 1;
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
        if (isInstruction && i % 3 == 0) continue;
        GROUND_T *p = &grounds[i];
        uint8_t x = i * IMG_GROUND_W;
        if (p->growth > 0) {
            uint8_t y = 60 - (p->growth >> PLANT_HEIGHT_SHIFT);
            uint8_t imgIndex = 0;
            if (p->power <= PLANT_POWER_FINE) {
                imgIndex = 2;
            } else if (p->life <= PLANT_LIFE_MAX / 10) {
                imgIndex = 4 + (gameFrames & 2);
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
        if (isInstruction && i % 3 == 0) continue;
        uint8_t imgIndex = (grounds[i].wet + 512) >> 10;
        ab.drawBitmap(i * IMG_GROUND_W, HEIGHT - IMG_GROUND_H, imgGround[imgIndex], IMG_GROUND_W, IMG_GROUND_H, WHITE);
    }
}

static void drawFlowers(void)
{
    uint8_t imgIndex = gameFrames % 6 / 2;
    for (FLYING_T *p = flowers; p < &flowers[FLOWERS_MAX]; p++) {
        if (p->y <= 0) continue;
        int16_t dx = (p->x >> DECIMAL_BITS) - IMG_FLOWER_W / 2;
        int16_t dy = (p->y >> DECIMAL_BITS) - IMG_FLOWER_H;
        ab.drawBitmap(dx, dy, imgFlower[imgIndex], IMG_FLOWER_W, IMG_FLOWER_H, WHITE);
    }
}
