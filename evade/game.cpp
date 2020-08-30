#include "common.h"
#include "data.h"

/*  Defines  */

#define PILLARS_NUM     16
#define DOTS_NUM        64

#define OBJECT_X_MIN    -128
#define OBJECT_X_MAX    128
#define PILLAR_Z_MAX    255
#define PLAYER_DX       ((WIDTH - IMG_PLAYER_W) / 2)
#define PLAYER_DY       (HEIGHT - IMG_PLAYER_H)

#define CAM_F           64
#define SCORE_MAX       59999
#define ACCEL_INT_BASE  900
#define JUMP_FRAMES     20

enum STATE_T : uint8_t {
    STATE_INIT = 0,
    STATE_START,
    STATE_PLAYING,
    STATE_DEATH,
    STATE_OVER,
    STATE_MENU,
    STATE_LEAVE,
};

/*  Typedefs  */

typedef struct {
    int8_t  x;
    uint8_t z, w;
} PILLAR_T;

typedef union {
    struct {
        int8_t x, y;
    } rect;
    struct {
        uint16_t r:11;
        uint16_t v:5;
    } polar;
} DOT_T;

/*  Local Functions  */

static void     handleStart(void);
static void     handlePlaying(void);
static void     handleDeath(void);
static void     handleOver(void);

static void     initObjects(void);
static bool     updateObjects(int8_t vx, uint8_t speed);
static void     initDotsDeath(void);
static void     updateDotsDeath(void);

static void     onContinue(void);
static void     onConfirmRetry(void);
static void     onRetry(void);
static void     onConfirmQuit(void);
static void     onQuit(void);

static void     drawStart(void);
static void     drawPlaying(void);
static void     drawDeath(void);
static void     drawOver(void);
static void     drawMenu(void);

static void     drawPillars(void);
static void     drawPlayer(int16_t x, int16_t y, uint8_t idx);
static void     drawDotsPlaying(void);
static void     drawDotsDeath(void);
static void     drawScore(void);
static void     fillDitheredRect(int16_t x, int16_t y, uint8_t w, int8_t h, uint8_t dither);

/*  Local Functions (macros)  */

#define callHandlerFunc(n)  ((void (*)(void)) pgm_read_ptr(handlerFuncTable - 1 + n))()
#define callDrawerFunc(n)   ((void (*)(void)) pgm_read_ptr(drawerFuncTable - 1 + n))()

/*  Local Variables  */

PROGMEM static void(*const handlerFuncTable[])(void) = {
    handleStart, handlePlaying, handleDeath, handleOver, handleMenu
};

PROGMEM static void(*const drawerFuncTable[])(void) = {
    drawStart, drawPlaying, drawDeath, drawOver, drawMenu
};

static STATE_T  state = STATE_INIT;
static PILLAR_T pillars[PILLARS_NUM];
static DOT_T    dots[DOTS_NUM];
static uint16_t score, accelInt;
static uint8_t  pillarsTopIdx, pillarsSpeed, pillarsZOdd, dotsBaseX, dotsBaseZ;
static int8_t   playerVx, nextDistance, shakeX, shakeY;
static bool     isHiscore;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initGame(void)
{
    dprintln(F("Initalize game"));
    arduboy.playScore(soundStart, SND_PRIO_START);
    initObjects();
    score = 0;
    counter = 0;
    state = STATE_START;
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

static void handleStart(void)
{
    if (++counter >= FPS) {
        record.playCount++;
        isRecordDirty = true;
        state = STATE_PLAYING;
    } else if (!record.simpleMode && arduboy.buttonDown(A_BUTTON)) {
        playSoundClick();
        clearMenuItems();
        addMenuItem(F("CONTINUE"), onContinue);
        addMenuItem(F("BACK TO TITLE"), onQuit);
        setMenuCoords(19, 26, 89, 11, true, true);
        setMenuItemPos(0);
        state = STATE_MENU;
    }
    isInvalid = true;
}

static void handlePlaying(void)
{
    record.playFrames++;
    isRecordDirty = true;
    if (score <= SCORE_MAX) {
        if ((++score & 7) == 0) arduboy.playTone(384, 8, SND_PRIO_STEP, 5);
    }
    if (record.acceleration > 0 && score % accelInt == 0) {
        pillarsSpeed++;
        dprint(F("Speed="));
        dprintln(pillarsSpeed);
    }
    playerVx = 0;
    if (arduboy.buttonPressed(LEFT_BUTTON))  playerVx--;
    if (arduboy.buttonPressed(RIGHT_BUTTON)) playerVx++;
    if (updateObjects(playerVx, pillarsSpeed)) {
        arduboy.playScore(soundOver, SND_PRIO_OVER);
        isHiscore = (record.hiscore < score);
        if (isHiscore) record.hiscore = score;
        writeRecord();
        initDotsDeath();
        counter = 0;
        state = STATE_DEATH;
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

static void handleDeath(void)
{
    uint8_t shake = 5 - counter / 10;
    shakeX = random(-shake, shake + 1);
    shakeY = random(-shake, shake + 1);
    updateDotsDeath();
    if (++counter >= FPS) {
        counter = 0;
        state = STATE_OVER;
    }
    isInvalid = true;
}

static void handleOver(void)
{
    updateObjects(0, 8);
    if (record.simpleMode) {
        SIMPLE_OP_T op = handleSimpleMode();
        if (op == SIMPLE_OP_START) initGame();
        if (op == SIMPLE_OP_SETTINGS) state = STATE_LEAVE;
    } else {
        if (arduboy.buttonDown(A_BUTTON)) {
            playSoundClick();
            clearMenuItems();
            addMenuItem(F("RETRY GAME"), onRetry);
            addMenuItem(F("BACK TO TITLE"), onQuit);
            setMenuCoords(19, 26, 89, 11, true, true);
            setMenuItemPos(0);
            state = STATE_MENU;
        } else if (arduboy.buttonDown(B_BUTTON)) {
            initGame();
        }
    }
    isInvalid = true;
}

/*---------------------------------------------------------------------------*/

static void initObjects(void)
{
    /*  Init pillars  */
    memset(pillars, 0, sizeof(pillars));
    pillarsTopIdx = 0;
    pillarsSpeed = (record.speed + 3) << 3;
    pillarsZOdd = 0;
    nextDistance = 0;
    accelInt = ACCEL_INT_BASE >> record.acceleration;
    shakeX = shakeY = 0;
    dprint(F("Speed="));
    dprintln(pillarsSpeed);

    /*  Init dots  */
    for (DOT_T *p = dots; p < dots + DOTS_NUM; p++) {
        p->rect.x = random(OBJECT_X_MIN, OBJECT_X_MAX);
        p->rect.y = random(-HEIGHT / 2, HEIGHT / 2);
    }
    dotsBaseX = 0;
    dotsBaseZ = 0;
}

static bool updateObjects(int8_t vx, uint8_t speed)
{
    bool ret = false;
    pillarsZOdd += speed;
    uint8_t vz = pillarsZOdd >> 4;
    pillarsZOdd &= 0xF;

    /*  Scroll pillars  */
    vx = vx * ((speed >> 5) + 1);
    for (PILLAR_T *p = pillars; p < pillars + PILLARS_NUM; p++) {
        if (!p->w) continue;
        p->x -= vx;
        if (p->z <= PILLAR_Z_MAX - vz) {
            p->z += vz;
        } else if (state == STATE_PLAYING && p->x >= -p->w && p->x < p->w) {
            p->z = PILLAR_Z_MAX;
            ret = true; // Game over!!
        } else {
            p->w = 0; // Remove a pillar
        }
    }
    nextDistance -= vz;
    dotsBaseX -= vx;
    dotsBaseZ = (dotsBaseZ - vz) & (DOTS_NUM - 1); // % DOTS_NUM

    /*  Append a pillar  */
    if (nextDistance <= 0 && !ret) {
        PILLAR_T *p = &pillars[pillarsTopIdx];
        p->x = random(OBJECT_X_MIN, OBJECT_X_MAX);
        p->z = -nextDistance;
        p->w = random(record.thickness) + record.thickness + 3;
        pillarsTopIdx = (pillarsTopIdx + 1) & (PILLARS_NUM - 1); // % PILLARS_NUM;
        nextDistance += 36 - record.density * 4;
    }
    return ret;
}

static void initDotsDeath(void)
{
    for (DOT_T *p = dots; p < dots + DOTS_NUM; p++) {
        p->polar.r = 0;
        p->polar.v = random(16, 45);
    }
}

static void updateDotsDeath(void)
{
    for (DOT_T *p = dots; p < dots + DOTS_NUM; p++) {
        if (p->polar.v > 0) {
            p->polar.r += p->polar.v;
            p->polar.v--;
        }
    }
}

/*---------------------------------------------------------------------------*/
/*                               Menu Handlers                               */
/*---------------------------------------------------------------------------*/

static void onContinue(void)
{
    playSoundClick();
    state = (score == 0) ? STATE_START : STATE_PLAYING;
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

static void drawStart(void)
{
    arduboy.drawBitmap((WIDTH - IMG_READY_W) / 2, 16, imgReady, IMG_READY_W, IMG_READY_H); 
    int16_t y = (counter * counter >> 5) - IMG_PLAYER_H;
    if (y > PLAYER_DY) y = PLAYER_DY;
    drawPlayer(PLAYER_DX, y, (y == PLAYER_DY));
}

static void drawPlaying(void)
{
    drawPillars();
    drawDotsPlaying();
    uint8_t idx = 5 + playerVx * 3;
    uint8_t anim = score >> 2 & 3;
    if (anim & 1) idx += (anim >> 1) + 1;
    drawPlayer(PLAYER_DX, PLAYER_DY, idx);
    drawScore();
}

static void drawDeath(void)
{
    drawPillars();
    drawDotsDeath();
    int16_t tmp = counter - JUMP_FRAMES;
    int16_t y = (tmp * tmp >> 4) + 23;
    drawPlayer(PLAYER_DX + shakeX, y + shakeY, 11 + (counter >= JUMP_FRAMES));
    drawScore();
}

static void drawOver(void)
{
    drawPillars();
    drawScore();
    if (isHiscore) {
        arduboy.printEx(0, 14, "NEW RECORD!");
    } else {
        arduboy.printEx(0, 14, "HI:");
        arduboy.print(record.hiscore / 6);
    }
    drawBitmapBordered((WIDTH - IMG_GAMEOVER_W) / 2, 16, imgGameOver,
            IMG_GAMEOVER_W, IMG_GAMEOVER_H); 
    if (record.simpleMode) drawSimpleModeInstruction();
}

static void drawMenu(void)
{
    drawMenuItems(isInvalid);
}

/*---------------------------------------------------------------------------*/

static void drawPillars(void)
{
    for (PILLAR_T *p = pillars; p < pillars + PILLARS_NUM; p++) {
        if (!p->w) continue;
        int8_t h = (HEIGHT * CAM_F) / ((CAM_F + PILLAR_Z_MAX) - p->z);
        int16_t x = WIDTH / 2 + ((p->x - p->w) * h >> 6) + shakeX;
        int16_t y = ((HEIGHT - h) >> 1) + shakeY;
        uint8_t w = p->w * h >> 5;
        uint8_t dither = (p->z <= PILLAR_Z_MAX / 2) ? p->z >> 4 : 7;
        fillDitheredRect(x, y, w, h, dither);
    }
}

static void drawPlayer(int16_t x, int16_t y, uint8_t idx)
{
    drawBitmapWithMask(x, y, imgPlayer[idx][0], IMG_PLAYER_W, IMG_PLAYER_H);
}

static void drawDotsPlaying(void)
{
    DOT_T *p = dots;
    for (int i = 0; i < DOTS_NUM; i++, p++) {
        int8_t s = (HEIGHT * CAM_F) / (CAM_F + ((i + dotsBaseZ) & (DOTS_NUM - 1))); // % DOTS_NUM
        int16_t x = WIDTH / 2 + ((int8_t)(p->rect.x + dotsBaseX) * s >> 6);
        int16_t y = HEIGHT / 2 + (p->rect.y * s >> 6);
        if (y < 24 || x < 48 || x >= 80) arduboy.drawPixel(x, y);
    }
}

static void drawDotsDeath(void)
{
    DOT_T *p = dots;
    for (int i = 0; i < DOTS_NUM; i++, p++) {
        if (p->polar.v == 0) continue;
        double deg = i * PI / (double)DOTS_NUM;
        double vx = cos(deg), vy = -sin(deg);
        double r1 = p->polar.r / 4.0, r2 = (p->polar.r - p->polar.v - 1) / 4.0;
        int16_t x = WIDTH / 2 + shakeX, y = HEIGHT - IMG_PLAYER_H / 2 + shakeY;
        arduboy.drawLine(x + vx * r1, y + vy * r1, x + vx * r2, y + vy * r2);
    }
}

static void drawScore(void)
{
    uint16_t tmp = score / 6;
    uint8_t figures[4], digit = 0;
    do {
        figures[digit++] = tmp % 10;
        tmp /= 10;
    } while (tmp > 0);
    for (int16_t x = 0; digit > 0; x += IMG_FIGURE_W) {
        drawBitmapWithMask(x, 0, imgFigure[figures[--digit]][0], IMG_FIGURE_W, IMG_FIGURE_H);
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
