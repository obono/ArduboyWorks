#include "common.h"
#include "data.h"

/*  Defines  */

#define FIELD_W     6
#define FIELD_H     13
#define SET_MAX     3
#define ROT_SINGLE  5

enum STATE_T {
    STATE_INIT = 0,
    STATE_START,
    STATE_PLAYING,
    STATE_MENU,
    STATE_OVER,
    STATE_LEAVE,
};

enum GAME_MODE_T {
    GAME_MODE_WAIT = 0,
    GAME_MODE_CONTROL,
    GAME_MODE_FALL,
    GAME_MODE_ERASE,
    GAME_MODE_BALL,
    GAME_MODE_STAR,
};

enum OBJECT_T {
    OBJECT_EMPTY = 0,
    OBJECT_BOX,
    OBJECT_BALL_L,
    OBJECT_BALL_R,
    OBJECT_STAR,
    OBJECT_ENEMY,
};

/*  Typedefs  */

/*  Local Functions  */

static void     initField(void);
static boolean  isMoveable(int8_t x, int8_t y, uint8_t r);
static void     forwardGame(void);
static bool     fallObject(void);
static uint16_t checkFilledLines(void);
static void     onContinue(void);
static void     onRestart(void);
static void     onQuit(void);

/*  Local Variables  */

static STATE_T      state = STATE_INIT;
static GAME_MODE_T  gameMode = GAME_MODE_WAIT;
static OBJECT_T     field[FIELD_H][FIELD_W];
static OBJECT_T     currentSet[SET_MAX], nextSet[SET_MAX];
static int8_t       currentX, currentY, currentR;
static int8_t       lastX, lastY, lastR;
static uint16_t     filledLines;
static uint32_t     score;
static uint16_t     level, gameFrames;
static int16_t      gameCounter;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initGame(void)
{
    score = 0;
    level = 0;
    gameFrames = FPS * 2;
    initField();

    state = STATE_START;
    isInvalid = true;
    arduboy.playScore2(soundStart, SND_PRIO_START);
}

MODE_T updateGame(void)
{
    switch (state) {
    case STATE_START:
        if (--gameFrames == 0) {
            record.playCount++;
            state = STATE_PLAYING;
            isInvalid = true;
        }
        break;
    case STATE_PLAYING:
        record.playFrames++;
        isRecordDirty = true;
        if (arduboy.buttonDown(B_BUTTON)) {
            writeRecord();
            state = STATE_OVER;
            isInvalid = true;
        } else if (arduboy.buttonDown(A_BUTTON)) {
            clearMenuItems();
            addMenuItem(F("CONTINUE"), onContinue);
            addMenuItem(F("RESTART"), onRestart);
            addMenuItem(F("QUIT"), onQuit);
            setMenuCoords(55, 61, 17, 59, true, true);
            setMenuItemPos(0);
            writeRecord();
            state = STATE_MENU;
        }
        break;
    case STATE_MENU:
        handleMenu();
        break;
    case STATE_OVER:
        gameFrames++;
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
    if (isInvalid) {
        arduboy.clear();
        if (state == STATE_START) {
            arduboy.printEx(61, 49, F("READY?"));
        } else if (state == STATE_OVER) {
            arduboy.printEx(61, 58, F("GAME OVER"));
        }
        drawNumber(0, 63, 0, state);
        isInvalid = false;
    }
    if (state == STATE_MENU) drawMenuItems(false);
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void initField(void)
{
    for (int i = 0; i < FIELD_H; i++) {
        for (int j = 0; j < FIELD_W; j++) {
            field[i][j] = OBJECT_EMPTY;
        }
    }
}

static boolean isMoveable(int8_t x, int8_t y, uint8_t r)
{
    for (int i = 0; i < SET_MAX; i++) {
        int8_t dx = x + (r + i & 2) * 4;
        int8_t dy = y + (r + i + 1 & 2) * 4;
        if (dx < 0 || dx >= FIELD_W || dy < 0 || field[dy][dx] != OBJECT_EMPTY) return false;
        if (r == 5) break;
    }
    return true;
}

static void forwardGame(void)
{
    int8_t vx, vy;
    switch (gameMode) {
    case GAME_MODE_WAIT:
        if (--gameCounter <= 0) {
            currentX = 2;
            currentY = FIELD_H * 8;
            currentR = 0;
            gameMode = GAME_MODE_CONTROL;
        }
        break;
    case GAME_MODE_CONTROL:
        vx = arduboy.buttonDown(RIGHT_BUTTON_V) - arduboy.buttonDown(LEFT_BUTTON_V);
        if (isMoveable(currentX + vx, currentY, currentR)) currentX += vx;
        if (arduboy.buttonDown(UP_BUTTON_V | B_BUTTON) && currentR != ROT_SINGLE &&
                isMoveable(currentX, currentY, (currentR + 1) & 3)) {
            currentR = (currentR + 1) & 3;
        }
        if (isMoveable(currentX, currentY - 1, currentR)) {
            currentY--;
            gameCounter = 0;
        } else if (++gameCounter >= 30) {
            gameMode = GAME_MODE_FALL;
        }
        break;
    case GAME_MODE_FALL:
        if (fallObject()) {
            isInvalid = true;
        } else {
            filledLines = checkFilledLines();
            if (filledLines) {
                gameCounter = 60;
                gameMode = GAME_MODE_ERASE;
            } else {
                gameCounter = 30;
                gameMode = GAME_MODE_WAIT;
            }
        }
        break;
    case GAME_MODE_ERASE:
        gameCounter--;
        if (gameCounter < 0) gameMode = GAME_MODE_FALL;
        break;
    case GAME_MODE_BALL:
        break;
    case GAME_MODE_STAR:
        break;
    }
}

static bool fallObject(void)
{
    bool isFalled = false;
    for (int i = 0; i < FIELD_H - 1; i++) {
        for (int j = 0; j < FIELD_W; j++) {
            if (field[i][j] == 0 && field[i + 1][j] != 0) {
                field[i][j] = field[i + 1][j];
                field[i + 1][j] = OBJECT_EMPTY;
                isFalled = true;
            }
        }
    }
    return isFalled;
}

static uint16_t checkFilledLines(void)
{
    uint16_t ret = 0;
    for (int i = 0; i < FIELD_H; i++) {
        bool isFilled = true;
        for (int j = 0; j < FIELD_W; isFilled &= (field[i][j] == 1), j++) { ; }
        if (isFilled) {
            bitSet(ret, i);
            for (int j = 0; j < FIELD_W; field[i][j] = OBJECT_EMPTY, j++) { ; }
        }
    }
    return ret;
}

static void onContinue(void)
{
    playSoundClick();
    state = STATE_PLAYING;
    isInvalid = true;
    dprintln(F("Continue"));
}

static void onRestart(void)
{
    initGame();
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

static void drawField(void)
{
    arduboy.drawRect2(16, 7, FIELD_H * 8 + 1, FIELD_W * 8 + 2, WHITE);
    for (int i = 0; i < FIELD_H; i++) {
        int16_t dx = 112 - i * 8;
        for (int j = 0; j < FIELD_W; j++) {
            int16_t dy = 56 - j * 8;
            arduboy.drawBitmap(dx, dy, imgObject[field[i][j]], IMG_OBJECT_W, IMG_OBJECT_H, WHITE); 
        }
    }
}

static void clearSet(int8_t x, int8_t y, uint8_t r)
{
    for (int i = 0; i < SET_MAX; i++) {
        int16_t dx = x + (r + i & 2) * 4;
        int16_t dy = y + (r + i + 1 & 2) * 4;
        arduboy.fillRect2(dx, dy, IMG_OBJECT_W, IMG_OBJECT_H, BLACK);
        if (r == ROT_SINGLE) break;
    }
}

static void drawSet(int8_t x, int8_t y, uint8_t r, OBJECT_T* pSet)
{
    for (int i = 0; i < SET_MAX; i++) {
        int16_t dx = x + (r + i & 2) * 4;
        int16_t dy = y + (r + i + 1 & 2) * 4;
            arduboy.drawBitmap(dx, dy, imgObject[pSet[i]], IMG_OBJECT_W, IMG_OBJECT_H, WHITE); 
        if (r == ROT_SINGLE) break;
    }
}
