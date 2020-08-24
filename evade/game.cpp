#include "common.h"
#include "data.h"

/*  Defines  */

#define PILLARS_NUM 16
#define SPEED       2       // 1 - 5
#define THICKNESS   3       // 1 - 5
#define DENSITY     3       // 1 - 5

enum STATE_T : uint8_t {
    STATE_INIT = 0,
    STATE_PLAYING,
    STATE_MENU,
    STATE_OVER,
    STATE_LEAVE,
};

/*  Typedefs  */

typedef struct {
    int8_t  x;
    uint8_t z, w;
} PILLAR_T;

/*  Local Functions  */

static void     initPillars(void);
static bool     updatePillars(int8_t vx, int8_t vz);

static void     onContinue(void);
static void     onConfirmRetry(void);
static void     onRetry(void);
static void     onConfirmQuit(void);
static void     onQuit(void);

static void     drawPillars(void);
static void     fillDitheredRect(int16_t x, int16_t y, uint8_t w, int8_t h, uint8_t dither);
static void     drawPlayer(void);

/*  Local Variables  */

PROGMEM static const uint8_t ditherPatterns[] = { // 8 grades
    0x11, 0x00, 0x55, 0x00, 0x55, 0x22, 0x55, 0xAA, 0x77, 0xAA, 0xFF, 0xAA, 0xFF, 0xBB, 0xFF, 0xFF
};

static STATE_T  state = STATE_INIT;
static PILLAR_T pillars[PILLARS_NUM];
static uint32_t score;
static uint8_t  pillarsTopIdx;
static int8_t   playerVx, pillarAAA;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initGame(void)
{
    arduboy.playScore(soundStart, SND_PRIO_START);
    initPillars();
    score = 0;
    counter = 0;
    record.playCount++;
    isRecordDirty = true;
    writeRecord();
    state = STATE_PLAYING;
    isInvalid = true;
}

MODE_T updateGame(void)
{
    counter++;
    handleDPad();
    switch (state) {
    case STATE_PLAYING:
        record.playFrames++;
        isRecordDirty = true;
        playerVx = 0;
        if (arduboy.buttonPressed(LEFT_BUTTON))  playerVx--;
        if (arduboy.buttonPressed(RIGHT_BUTTON)) playerVx++;
        if (updatePillars(playerVx, SPEED + 1)) {
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
            state = STATE_MENU;
        }
        isInvalid = true;
        break;
    case STATE_OVER:
        if (arduboy.buttonDown(A_BUTTON)) {
            playSoundClick();
            clearMenuItems();
            addMenuItem(F("RETRY GAME"), onRetry);
            addMenuItem(F("BACK TO TITLE"), onQuit);
            setMenuCoords(19, 26, 89, 11, true, false);
            setMenuItemPos(0);
            state = STATE_MENU;
            isInvalid = true;
        } else if (arduboy.buttonDown(B_BUTTON)) {
            initGame();
        }
        break;
    case STATE_MENU:
        handleMenu();
        break;
    }
    return (state == STATE_LEAVE) ? MODE_TITLE : MODE_GAME;
}

void drawGame(void)
{
    if (state == STATE_LEAVE) return;
    if (state == STATE_MENU) {
        drawMenuItems(isInvalid);
        isInvalid = false;
        return;
    }
    if (isInvalid) {
        arduboy.clear();
        drawPillars();
        drawPlayer();
        if (state == STATE_OVER) {
            arduboy.printEx(37, 29, F("GAME OVER"));
        }
    }
    isInvalid = false;
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void initPillars(void)
{
    memset(pillars, 0, sizeof(pillars));
    pillarsTopIdx = 0;
    pillarAAA = 0;
}

static bool updatePillars(int8_t vx, int8_t vz)
{
    bool ret = false;
    for (PILLAR_T *p = pillars; p < pillars + PILLARS_NUM; p++) {
        if (!p->w) continue;
        p->x -= vx * 2; // TODO
        if (p->z < 256 - vz) {
            p->z += vz;
        } else if (p->x >= -p->w && p->x < p->w) {
            p->z = 255;
            ret = true; // game over !!
        } else {
            p->w = 0; // remove
        }
    }
    pillarAAA -= vz;
    if (pillarAAA <= 0 && !ret) {
        PILLAR_T *p = &pillars[pillarsTopIdx];
        p->x = random(-128, 128);
        p->z = -pillarAAA;
        p->w = random(THICKNESS) + THICKNESS + 3;
        pillarsTopIdx = (pillarsTopIdx + 1) % PILLARS_NUM;
        pillarAAA += 36 - DENSITY * 4;
    }
    return ret;
}

/*---------------------------------------------------------------------------*/
/*                               Menu Handlers                               */
/*---------------------------------------------------------------------------*/

static void onContinue(void)
{
    playSoundClick();
    state = STATE_PLAYING;
    isInvalid = true;
    dprintln(F("Menu: continue"));
}

static void onConfirmRetry(void)
{
    int16_t y = getMenuItemPos() * 6 + 25;
    setConfirmMenu(y, onRetry, onContinue);
    dprintln(F("Menu: confirm retry"));
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
    state = STATE_LEAVE;
    dprintln(F("Menu: quit"));
}

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void drawPillars(void)
{
    for (PILLAR_T *p = pillars; p < pillars + PILLARS_NUM; p++) {
        if (!p->w) continue;
        int16_t h = 4096 / (319 - p->z);
        int16_t x = WIDTH / 2 + ((p->x - p->w) * h >> 6);
        int16_t w = p->w * h >> 5;
        fillDitheredRect(x, (HEIGHT - h) >> 1, w, h, (p->z < 128) ? p->z >> 4 : 7);
    }
}

static void fillDitheredRect(int16_t x, int16_t y, uint8_t w, int8_t h, uint8_t dither)
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

    /*  Draw a vertical line  */
    uint8_t yOdd = y & 7;
    uint8_t d = 0xFF << yOdd;
    const uint8_t *pDitherPattern = ditherPatterns + (dither << 1);
    y -= yOdd;
    h += yOdd;
    for (uint8_t *p = arduboy.getBuffer() + x + (y / 8) * WIDTH; h > 0; h -= 8, p += WIDTH - w) {
        if (h < 8) d &= 0xFF >> (8 - h);
        for (uint8_t i = w; i > 0; i--) {
            *p++ |= pgm_read_byte(pDitherPattern + ((uint16_t)p & 1)) & d;
        }
        d = 0xFF;
    }
}

static void drawPlayer(void)
{
    uint8_t idx = (playerVx + 1) * 3;
    uint8_t anim = counter >> 2 & 3;
    if (anim & 1) idx += (anim >> 1) + 1;
    arduboy.drawBitmap(56, 48, imgPlayerMask[idx], IMG_PLAYER_W, IMG_PLAYER_H, BLACK);
    arduboy.drawBitmap(56, 48, imgPlayer[idx], IMG_PLAYER_W, IMG_PLAYER_H);
}
