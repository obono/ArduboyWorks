#include "common.h"
#include "data.h"

/*  Defines  */

#define PILLARS_NUM     16
#define PILLAR_X_MIN    -128
#define PILLAR_X_MAX    128
#define PILLAR_Z_MAX    255
#define CAM_F           64
#define SCORE_MAX       59999
#define ACCEL_INT_BASE  900

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

static void     handlePlaying(void);
static void     handleOver(void);

static void     initPillars(void);
static bool     updatePillars(int8_t vx, uint8_t speed);

static void     onContinue(void);
static void     onConfirmRetry(void);
static void     onRetry(void);
static void     onConfirmQuit(void);
static void     onQuit(void);

static void     drawPlaying(void);
static void     drawOver(void);
static void     drawPillars(void);
static void     fillDitheredRect(int16_t x, int16_t y, uint8_t w, int8_t h, uint8_t dither);
static void     drawPlayer(void);
static void     drawMenu(void);

/*  Local Functions (macros)  */

#define callHandlerFunc(n)  ((void (*)(void)) pgm_read_ptr(handlerFuncTable + n))()
#define callDrawerFunc(n)   ((void (*)(void)) pgm_read_ptr(drawerFuncTable + n))()

/*  Local Variables  */

PROGMEM static const uint8_t ditherPatterns[] = { // 8 grades
    0x11, 0x00, 0x55, 0x00, 0x55, 0x22, 0x55, 0xAA, 0x77, 0xAA, 0xFF, 0xAA, 0xFF, 0xBB, 0xFF, 0xFF
};

PROGMEM static void(*const handlerFuncTable[])(void) = {
    NULL, handlePlaying, handleMenu, handleOver
};

PROGMEM static void(*const drawerFuncTable[])(void) = {
    NULL, drawPlaying, drawMenu, drawOver
};

static STATE_T  state = STATE_INIT;
static PILLAR_T pillars[PILLARS_NUM];
static uint16_t score, accelInt;
static uint8_t  pillarsTopIdx, pillarsSpeed, pillarsZOdd;
static int8_t   playerVx, nextDistance;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initGame(void)
{
    dprintln(F("Initalize game"))
    arduboy.playScore(soundStart, SND_PRIO_START);
    initPillars();
    score = 0;
    record.playCount++;
    isRecordDirty = true;
    writeRecord();
    state = STATE_PLAYING;
    isInvalid = true;
}

MODE_T updateGame(void)
{
    callHandlerFunc(state);
    return (state == STATE_LEAVE) ? MODE_TITLE : MODE_GAME;
}

void drawGame(void)
{
    if (state == STATE_LEAVE) return;
    if (isInvalid && state != STATE_MENU) arduboy.clear();
    callDrawerFunc(state);
    isInvalid = false;
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void handlePlaying(void)
{
    record.playFrames++;
    isRecordDirty = true;
    if (score <= SCORE_MAX) score++;
    if (record.acceleration > 0 && score % accelInt == 0) {
        pillarsSpeed++;
        dprint(F("Speed="));
        dprintln(pillarsSpeed);
    }
    playerVx = 0;
    if (arduboy.buttonPressed(LEFT_BUTTON))  playerVx--;
    if (arduboy.buttonPressed(RIGHT_BUTTON)) playerVx++;
    if (updatePillars(playerVx, pillarsSpeed)) {
        arduboy.playScore(soundOver, SND_PRIO_OVER);
        if (record.hiscore < score) {
            record.hiscore = score;
        }
        writeRecord();
        counter = 0;
        state = STATE_OVER;
        dprintln(F("Game over!!"));
    } else if (!record.simpleMode && arduboy.buttonDown(A_BUTTON)) {
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
}

static void handleOver(void)
{
    if (record.simpleMode) {
        if (arduboy.buttonPressed(DOWN_BUTTON)) {
            if (counter < FPS + 8) counter++;
        } else {
            counter = 0;
        }
        if (counter >= FPS) {
            if (arduboy.buttonDown(A_BUTTON)) {
                setSound(!arduboy.isAudioEnabled());
                playSoundClick();
            }
            if (arduboy.buttonDown(B_BUTTON)) state = STATE_LEAVE;
        } else {
            if (arduboy.buttonDown(A_BUTTON | B_BUTTON)) initGame();
        }
    } else {
        if (arduboy.buttonDown(A_BUTTON)) {
            playSoundClick();
            clearMenuItems();
            addMenuItem(F("RETRY GAME"), onRetry);
            addMenuItem(F("BACK TO TITLE"), onQuit);
            setMenuCoords(19, 26, 89, 11, true, false);
            setMenuItemPos(0);
            state = STATE_MENU;
        } else if (arduboy.buttonDown(B_BUTTON)) {
            initGame();
        }
    }
    isInvalid = true;
}

static void initPillars(void)
{
    memset(pillars, 0, sizeof(pillars));
    pillarsTopIdx = 0;
    pillarsSpeed = (record.speed + 3) << 3;
    pillarsZOdd = 0;
    nextDistance = 0;
    accelInt = ACCEL_INT_BASE >> record.acceleration;
    dprint(F("Speed="));
    dprintln(pillarsSpeed);
}

static bool updatePillars(int8_t vx, uint8_t speed)
{
    bool ret = false;
    pillarsZOdd += speed;
    uint8_t vz = pillarsZOdd >> 4;
    pillarsZOdd &= 0xF;

    /*  Scroll pillars  */
    for (PILLAR_T *p = pillars; p < pillars + PILLARS_NUM; p++) {
        if (!p->w) continue;
        p->x -= vx * ((speed >> 5) + 1);
        if (p->z <= PILLAR_Z_MAX - vz) {
            p->z += vz;
        } else if (p->x >= -p->w && p->x < p->w) {
            p->z = PILLAR_Z_MAX;
            ret = true; // Game over!!
        } else {
            p->w = 0; // Remove a pillar
        }
    }
    nextDistance -= vz;

    /*  Append a pillar  */
    if (nextDistance <= 0 && !ret) {
        PILLAR_T *p = &pillars[pillarsTopIdx];
        p->x = random(PILLAR_X_MIN, PILLAR_X_MAX);
        p->z = -nextDistance;
        p->w = random(record.thickness) + record.thickness + 3;
        pillarsTopIdx = (pillarsTopIdx + 1) % PILLARS_NUM;
        nextDistance += 36 - record.density * 4;
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

static void drawPlaying(void)
{
    drawPillars();
    drawPlayer();
    arduboy.printEx(0, 0, score / 6);
    arduboy.printEx(0, 8, pillarsSpeed);
}

static void drawOver(void)
{
    drawPillars();
    arduboy.printEx(37, 29, F("GAME OVER"));
    if (counter >= FPS) {
        drawSimpleModeInstruction(HEIGHT - (counter - FPS));
    }
    arduboy.printEx(0, 0, score / 6);
}

static void drawPillars(void)
{
    for (PILLAR_T *p = pillars; p < pillars + PILLARS_NUM; p++) {
        if (!p->w) continue;
        int8_t h = (HEIGHT * CAM_F) / ((CAM_F + PILLAR_Z_MAX) - p->z);
        int16_t x = WIDTH / 2 + ((p->x - p->w) * h >> 6);
        uint8_t w = p->w * h >> 5;
        uint8_t dither = (p->z <= PILLAR_Z_MAX / 2) ? p->z >> 4 : 7;
        fillDitheredRect(x, (HEIGHT - h) >> 1, w, h, dither);
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
    uint8_t anim = score >> 2 & 3;
    if (anim & 1) idx += (anim >> 1) + 1;
    arduboy.drawBitmap(56, 48, imgPlayerMask[idx], IMG_PLAYER_W, IMG_PLAYER_H, BLACK);
    arduboy.drawBitmap(56, 48, imgPlayer[idx], IMG_PLAYER_W, IMG_PLAYER_H);
}

static void drawMenu(void)
{
    drawMenuItems(isInvalid);
}
