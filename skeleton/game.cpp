#include "common.h"
#include "data.h"

/*  Defines  */

enum STATE_T : uint8_t {
    STATE_INIT = 0,
    STATE_START,
    STATE_PLAYING,
    STATE_MENU,
    STATE_LEAVE,
};

/*  Typedefs  */

/*  Local Functions  */

static void     handleStart(void);
static void     handlePlaying(void);

static void     onContinue(void);
static void     onConfirmRetry(void);
static void     onRetry(void);
static void     onConfirmQuit(void);
static void     onQuit(void);

static void     drawStart(void);
static void     drawPlaying(void);
static void     drawMenu(void);

/*  Local Functions (macros)  */

#define callHandlerFunc(n)  ((void (*)(void)) pgm_read_ptr(handlerFuncTable - 1 + n))()
#define callDrawerFunc(n)   ((void (*)(void)) pgm_read_ptr(drawerFuncTable - 1 + n))()

/*  Local Constants  */

PROGMEM static void(*const handlerFuncTable[])(void) = {
    handleStart, handlePlaying, handleMenu
};

PROGMEM static void(*const drawerFuncTable[])(void) = {
    drawStart, drawPlaying, drawMenu
};

/*  Local Variables  */

static STATE_T  state = STATE_INIT;
static int8_t   playerX, playerY, score;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initGame(void)
{
    ab.playScore(soundStart, SND_PRIO_START);
    playerX = 64;
    playerY = 32;
    score = 0;
    counter = FPS;
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
    callDrawerFunc(state);
    isInvalid = false;
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void handleStart(void)
{
    if (--counter == 0) {
        record.playCount++;
        isRecordDirty = true;
        state = STATE_PLAYING;
        isInvalid = true;
    }
}

static void handlePlaying(void)
{
    record.playFrames++;
    isRecordDirty = true;

    int8_t vx = ab.buttonPressed(RIGHT_BUTTON) - ab.buttonPressed(LEFT_BUTTON);
    int8_t vy = ab.buttonPressed(DOWN_BUTTON) - ab.buttonPressed(UP_BUTTON);
    if (vx || vy) {
        playerX += vx;
        playerY += vy;
        isInvalid = true;
    }
    if (ab.buttonDown(B_BUTTON)) {
        score++;
        ab.playScore(soundCoin, SND_PRIO_COIN);
        isInvalid = true;
    }
    ab.setRGBled(playerY, 0, playerX);

    if (ab.buttonDown(A_BUTTON)) {
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
    if (isInvalid) {
        ab.clear();
        ab.printEx(46, 29, F("READY?"));
    }
}

static void drawPlaying(void)
{
    if (isInvalid) {
        ab.clear();
        ab.printEx(playerX, playerY, score);
    }
}

static void drawMenu(void)
{
    drawMenuItems(isInvalid);
}
