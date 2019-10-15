#include "common.h"
#include "data.h"

/*  Defines  */

#define FIELD_W     6
#define FIELD_H     13
#define BUNCH_MAX   3
#define ROT_SINGLE  5
#define GAP_MAX     64
#define FALLSPD_MAX 32 // = GAP_MAX / 2
#define BALLSPD_MAX 24
#define LEVEL_MAX   99
#define LEVEL_FREQ  15

#define getScreenX(x, y)    (112 - (y) * IMG_OBJECT_W)
#define getScreenY(x, y)    (48  - (x) * IMG_OBJECT_H)

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
    OBJECT_BALL,
    OBJECT_BALL_L,
    OBJECT_BALL_R,
    OBJECT_STAR,
    OBJECT_STAR_BLINK,
    OBJECT_ENEMY,
};

/*  Typedefs  */

/*  Local Functions  */

static void     initField(void);
static void     tuneParameters(void);
static void     forwardGameWait(void);
static void     forwardGameControl(void);
static void     forwardGameFall(void);
static void     forwardGameErase(void);
static void     forwardGameBall(void);
static void     forwardGameStar(void);
static void     setNextBunch(void);
static bool     isMoveable(int8_t x, int8_t y, uint8_t r);
static bool     placeCurrentBunch(void);
static bool     fallObject(void);
static uint16_t checkFilledLines(void);

static void     onContinue(void);
static void     onRestart(void);
static void     onQuit(void);

static void     drawField(void);
static void     drawBunch(int8_t x, int8_t y, int8_t r, int8_t g, OBJECT_T* pBunch);
static void     drawObjectFast(int8_t x, int8_t y, int8_t g, OBJECT_T obj);
static void     drawErasingEffect(void);
static void     drawBallEffect(void);
static void     drawStarEffect(void);
static void     drawStatus(void);

/*  Local Variables  */

PROGMEM static void (* const forwardGameFuncs[])(void) = {
    forwardGameWait,    forwardGameControl, forwardGameFall,
    forwardGameErase,   forwardGameBall,    forwardGameStar
};
#define forwardGame(mode) ((void (*)(void))pgm_read_ptr(forwardGameFuncs + (mode)))()

static STATE_T      state = STATE_INIT;
static GAME_MODE_T  gameMode = GAME_MODE_WAIT;
static OBJECT_T     field[FIELD_H][FIELD_W];
static OBJECT_T     currentBunch[BUNCH_MAX], nextBunch[BUNCH_MAX];
static bool         isFastFall, isHiscore;
static int8_t       bunchX, bunchY, bunchR, bunchG;
static int8_t       ballX, ballY, ballV, ballG, ballE;
static uint8_t      level, bunchCount, fallSpeed, ballSpeed, ballFreq, scoreCoef;
static uint8_t      nextBall, boxesCount, enemiesCount, killedEnemies;
static int16_t      gameCounter, filledLines, starGuage, starGuageMax;
static uint32_t     score;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initGame(void)
{
    initField();
    level = startLevel;
    score = 0; // TODO
    bunchCount = 0;
    starGuage = 0;
    starGuageMax = 180; // TODO
    tuneParameters();
    nextBall = ballFreq;
    setNextBunch();
    gameMode = GAME_MODE_WAIT;
    gameCounter = FPS * 2;

    state = STATE_START;
    isInvalid = true;
    arduboy.playScore2(soundStart, SND_PRIO_START);
    dprintln(F("Start Game!"));
}

MODE_T updateGame(void)
{
    switch (state) {
    case STATE_START:
        if (--gameCounter <= 0) {
            record.playCount++;
            state = STATE_PLAYING;
            isInvalid = true;
        }
        break;
    case STATE_PLAYING:
        record.playFrames++;
        isRecordDirty = true;
#ifdef DEBUG
        if (dbgRecvChar == 'l' && level < LEVEL_MAX) {
            level++;
            bunchCount = 0;
            tuneParameters();
        }
        if (dbgRecvChar == 's') {
            starGuage = starGuageMax;
        }
#endif
        handleDPadV();
        forwardGame(gameMode);
        if (state == STATE_OVER) {
            isHiscore = enterScore(score, level);
            writeRecord();
            gameCounter = FPS * 10;
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
            isInvalid = true;
        }
        break;
    case STATE_MENU:
        handleMenu();
        break;
    case STATE_OVER:
        if (--gameCounter <= FPS * 9) {
            if (arduboy.buttonDown(A_BUTTON) || gameCounter == 0) state = STATE_LEAVE;
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
    if (state == STATE_LEAVE) return;
    if (state == STATE_MENU) {
        drawMenuItems(isInvalid);
        isInvalid = false;
        return;
    }

    if (true /*isInvalid*/) {
        arduboy.clear();
        drawField();
        drawStatus();
    }
    drawBunch(2, FIELD_H + 1, (nextBunch[1] == OBJECT_EMPTY) ? ROT_SINGLE : 0, 0, nextBunch);
    switch (state) {
    case STATE_START:
        arduboy.drawBitmap(57, 12, imgReady, IMG_READY_W, IMG_READY_H, WHITE);
        break;
    case STATE_PLAYING:
        if (gameMode == GAME_MODE_CONTROL) {
            drawBunch(bunchX, bunchY, bunchR, bunchG, currentBunch);
        }
        if (gameMode == GAME_MODE_ERASE) drawErasingEffect();
        if (gameMode == GAME_MODE_BALL) drawBallEffect();
        if (gameMode == GAME_MODE_STAR) drawStarEffect();
        break;
    case STATE_OVER:
        arduboy.drawBitmap(60, 0, imgGameOver1, IMG_GAMEOVER1_W, IMG_GAMEOVER1_H, WHITE);
        arduboy.drawBitmap(60, 56, imgGameOver2, IMG_GAMEOVER2_W, IMG_GAMEOVER2_H, WHITE);
        break;
    }
    isInvalid = false;
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
    boxesCount = 0;
    enemiesCount = 0;
}

static void tuneParameters(void)
{
    uint8_t baseLevel = level / 10;
    uint8_t baseFallSpeed = max(baseLevel * baseLevel / 2, 1);
    fallSpeed = baseFallSpeed * (4 + level % 5 * ((level % 10 >= 5) + 1)) / 4;
    fallSpeed = min(fallSpeed, GAP_MAX);
    ballSpeed = (baseLevel == 0) ? 8 : (baseLevel + 3) * 2;
    ballSpeed = min(ballSpeed, BALLSPD_MAX);
    ballFreq = (baseLevel < 4) ? baseLevel / 2 + 3 : baseLevel + 1;
    scoreCoef = (baseLevel + 1) * 5;
    dprint(F("Level="));
    dprint(level);
    dprint(F(" fallSpd="));
    dprint(fallSpeed);
    dprint(F(" ballSpd="));
    dprint(ballSpeed);
    dprint(F(" ballFreq="));
    dprint(ballFreq);
    dprint(F(" scoreCoef="));
    dprintln(scoreCoef);
}

static void forwardGameWait(void)
{
    if (--gameCounter > 0) return;
    memcpy(currentBunch, nextBunch, sizeof(currentBunch));
    setNextBunch();
    bunchX = 2;
    bunchY = FIELD_H + 1;
    bunchR = (currentBunch[1] == OBJECT_EMPTY) ? ROT_SINGLE : 0;
    bunchG = 0;
    isFastFall = false;
    killedEnemies = 0;
    gameMode = GAME_MODE_CONTROL;
    gameCounter = FPS / 2;
    dprint(F("Boxes="));
    dprint(boxesCount);
    dprint(F(" Enemies="));
    dprintln(enemiesCount);
}

static void forwardGameControl(void)
{
    if (arduboy.buttonDown(DOWN_BUTTON_V)) isFastFall = true;
    if (!arduboy.buttonPressed(DOWN_BUTTON_V)) isFastFall = false;
    bunchG -= (isFastFall && fallSpeed < FALLSPD_MAX) ? FALLSPD_MAX : fallSpeed;
    while (bunchG < 0) {
        if (isMoveable(bunchX, bunchY - 1, bunchR)) {
            bunchY--;
            bunchG += GAP_MAX;
            gameCounter = FPS / 2;
        } else {
            bunchG = 0;
            if (gameCounter == FPS / 2) playSoundTick();
            gameCounter--;
            if (isFastFall) gameCounter = 0;
        }
    }
    if (gameCounter > 0) {
        if (padX != 0 && isMoveable(bunchX + padX, bunchY, bunchR)) bunchX += padX;
        if (arduboy.buttonDown(UP_BUTTON_V | B_BUTTON) && bunchR != ROT_SINGLE &&
                isMoveable(bunchX, bunchY, (bunchR + 1) & 3)) {
            bunchR = (bunchR + 1) & 3;
            playSoundClick(); // TODO
        }
    } else if (placeCurrentBunch()) {
        gameMode = GAME_MODE_FALL;
    } else {
        state = STATE_OVER;
        dprintln(F("Game over..."));
    }
}

static void forwardGameFall(void)
{
    if (fallObject()) {
        isInvalid = true;
        return;
    }

    filledLines = checkFilledLines();
    if (filledLines) {
        gameMode = GAME_MODE_ERASE;
        gameCounter = FPS;
    } else if (ballY < FIELD_H) {
        OBJECT_T obj = field[ballY][ballX];
        if (obj == OBJECT_STAR) {
            gameMode = GAME_MODE_STAR;
            ballG = 0;
            ballE = 0;
        } else {
            ballV = (obj == OBJECT_BALL_L) ? -1 : 1;
            ballG = 0;
            ballE = 0;
            gameMode = GAME_MODE_BALL;
        }
    } else if (field[FIELD_H - 1][2] == OBJECT_EMPTY && field[FIELD_H - 1][3] == OBJECT_EMPTY) {
        if (++bunchCount >= LEVEL_FREQ && level < LEVEL_MAX) {
            level++;
            bunchCount = 0;
            tuneParameters();
            if (level % 5 == 0) playSoundClick(); // TODO
        }
        gameMode = GAME_MODE_WAIT;
        gameCounter = FPS / 2;
    } else {
        state = STATE_OVER;
        dprintln(F("Game over..."));
    }
}

static void forwardGameErase(void)
{
    if (--gameCounter > 0) return;
    uint8_t lines = 0;
    for (int i = 0; i < FIELD_H; i++) {
        lines += bitRead(filledLines, i);
    }
    score += (lines * (lines - 1) + 1) * scoreCoef * ((killedEnemies > 0) + 1) * 20;
    boxesCount -= lines * FIELD_W;
    gameMode = GAME_MODE_FALL;
    dprint(F("Erase lines:"));
    dprintln(lines);
}

static void forwardGameBall(void)
{
    ballG -= ballSpeed;
    while (ballG < 0 && gameMode == GAME_MODE_BALL)  {
        ballG += GAP_MAX;
        if (field[ballY][ballX] == OBJECT_ENEMY) {
            enemiesCount--;
            killedEnemies++;
            starGuage++;
            score += (killedEnemies + 1) * scoreCoef;
        }
        field[ballY][ballX] = OBJECT_EMPTY;
        if (ballY > 0 && field[ballY - 1][ballX] != OBJECT_BOX) {
            ballY--;
            ballE = 0;
        } else {
            ballE |= (ballX == 0 || field[ballY][ballX - 1] == OBJECT_BOX) | 2;
            ballE |= (ballX == FIELD_W - 1 || field[ballY][ballX + 1] == OBJECT_BOX) * 4;
            if (ballE == 7) {
                gameMode = GAME_MODE_FALL;
                dprint(F("Killed enemies:"));
                dprintln(killedEnemies);
            } else {
                if (bitRead(ballE, ballV + 1)) ballV = -ballV;
                ballX += ballV;
            }
        }
    }
}

static void forwardGameStar(void)
{
    ballG -= ballSpeed;
    while (ballG < 0 && gameMode == GAME_MODE_STAR) {
        ballE++;
        int8_t y = ballY - ballE;
        if (y >= 0 && ballE <= 7) {
            ballG += GAP_MAX;
            for (int i = 0; i < FIELD_W; i++) {
                if (field[y][i] == OBJECT_ENEMY) {
                    field[y][i] = OBJECT_EMPTY;
                    enemiesCount--;
                    killedEnemies++;
                    score += (killedEnemies + 1) * scoreCoef;
                }
            }
        } else {
            field[ballY][ballX] = OBJECT_EMPTY;
            if (killedEnemies == 0) score += scoreCoef * 400;
            dprint(F("Star:"));
            dprintln(killedEnemies);
            gameMode = GAME_MODE_FALL;
            break;
        }
    }
}

static void setNextBunch(void)
{
    bool isBall = false, isSingle = false, isStar = false;
    if (--nextBall == 0) {
        if (starGuage >= starGuageMax) {
            isStar = true;
            isSingle = true;
            starGuage -= starGuageMax;
            starGuageMax = 180; // TODO
            dprint(F("starGuageMax="));
            dprintln(starGuageMax);
        } else {
            isBall = true;
            isSingle = (level < 10);
        }
        nextBall = ballFreq;
    }
    int objMax = (isSingle) ? 1 : BUNCH_MAX;
    for (int i = 0; i < BUNCH_MAX; i++) {
        if (i < objMax) {
            nextBunch[i] = random(2) ? OBJECT_BOX : OBJECT_ENEMY;
        } else {
            nextBunch[i] = OBJECT_EMPTY;
        }
    }
    if (isBall || isStar) {
        int pos = random(objMax);
        if (isStar) {
            nextBunch[pos] = OBJECT_STAR;
        } else {
            nextBunch[pos] = random(2) ? OBJECT_BALL_L : OBJECT_BALL_R;
        }
    }
}

static bool isMoveable(int8_t x, int8_t y, uint8_t r)
{
    for (int i = 0; i < BUNCH_MAX; i++) {
        int8_t bx = x + bitRead(r + i, 1);
        int8_t by = y - bitRead(r + i + 1,  1);
        if (bx < 0 || bx >= FIELD_W || by < 0 || by < FIELD_H && field[by][bx] != OBJECT_EMPTY) {
            return false;
        }
        if (r == ROT_SINGLE) break;
    }
    return true;
}

static bool placeCurrentBunch(void)
{
    bool ret = true;
    for (int i = 0; i < BUNCH_MAX; i++) {
        int8_t bx = bunchX + bitRead(bunchR + i, 1);
        int8_t by = bunchY - bitRead(bunchR + i + 1,  1);
        if (bx < 0 || bx >= FIELD_W || by < 0 || by >= FIELD_H) {
            ret = false;
        } else {
            OBJECT_T obj = currentBunch[i];
            field[by][bx] = obj;
            if (obj == OBJECT_BOX) boxesCount++;
            if (obj == OBJECT_ENEMY) enemiesCount++;
        }
        if (bunchR == ROT_SINGLE) break;
    }
    return ret;
}

static bool fallObject(void)
{
    bool isFalled = false;
    for (int i = 1; i < FIELD_H; i++) {
        for (int j = 0; j < FIELD_W; j++) {
            OBJECT_T obj = field[i][j];
            if (obj != OBJECT_EMPTY && field[i - 1][j] == OBJECT_EMPTY) {
                field[i - 1][j] = obj;
                field[i][j] = OBJECT_EMPTY;
                isFalled = true;
            }
        }
    }
    return isFalled;
}

static uint16_t checkFilledLines(void)
{
    uint16_t ret = 0;
    ballY = FIELD_H;
    for (int i = 0; i < FIELD_H; i++) {
        bool isFilled = true;
        for (int j = 0; j < FIELD_W; j++) {
            switch (field[i][j]) {
            case OBJECT_BOX:
                // do nothing
                break;
            case OBJECT_BALL_L:
            case OBJECT_BALL_R:
            case OBJECT_STAR:
                ballX = j;
                ballY = i;
            case OBJECT_EMPTY:
            case OBJECT_ENEMY:
            default:
                isFilled = false;
                break;
            }
        }
        if (isFilled) {
            bitSet(ret, i);
            for (int j = 0; j < FIELD_W; j++) {
                field[i][j] = OBJECT_EMPTY;
            }
        }
    }
    return ret;
}

/*---------------------------------------------------------------------------*/
/*                                Menu Handler                               */
/*---------------------------------------------------------------------------*/

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
    dprintln(F("Restart"));
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
    arduboy.drawRect2(16, 7, FIELD_H * IMG_OBJECT_W + 1, FIELD_W * IMG_OBJECT_H + 2, WHITE);
    for (int i = 0; i < FIELD_H; i++) {
        for (int j = 0; j < FIELD_W; j++) {
            drawObjectFast(j, i, 0, field[i][j]);
        }
    }
}

static void drawBunch(int8_t x, int8_t y, int8_t r, int8_t g, OBJECT_T* pBunch)
{
    /*static int8_t lastX, lastY, lastR, lastG;
    if (false) { // TODO
        drawBunch(lastX, lastY, lastR, lastG, NULL);
    }*/
    for (int i = 0; i < BUNCH_MAX; i++) {
        int8_t bx = x + bitRead(r + i, 1);
        int8_t by = y - bitRead(r + i + 1,  1);
        OBJECT_T obj = (pBunch) ? pBunch[i] : OBJECT_EMPTY;
        drawObjectFast(bx, by, g, obj);
        if (r == ROT_SINGLE) break;
    }
    /*lastX = x;
    lastY = y;
    lastR = r;
    lastG = g;*/
}

static void drawObjectFast(int8_t x, int8_t y, int8_t g, OBJECT_T obj)
{
    uint16_t offset = (FIELD_W - x) * WIDTH + getScreenX(x, y) - g / 8;
    if (obj == OBJECT_STAR && bitRead(record.playFrames, 0)) obj = OBJECT_STAR_BLINK; 
    memcpy_P(arduboy.getBuffer() + offset, imgObject[obj], IMG_OBJECT_W);
}

static void drawErasingEffect(void)
{
    for (int i = 0; i < FIELD_H; i++) {
        if (bitRead(filledLines, i)) {
            int16_t dx = getScreenX(0, i);
            for (int j = 0; j < IMG_OBJECT_W; j++) {
                uint8_t c = (random(FPS / 2) < gameCounter - FPS / 2);
                arduboy.drawFastVLine2(dx + j, 8, FIELD_W * IMG_OBJECT_H, c);
            }
        }
    }
}

static void drawBallEffect(void)
{
    int16_t dx = getScreenX(ballX, ballY);
    int16_t dy = getScreenY(ballX, ballY);
    if (ballE == 0) {
        dx -= ballG / 8;
    } else {
        dy += ballG / 8 * ballV;
    }
    arduboy.drawBitmap(dx, dy, imgObject[OBJECT_BALL], IMG_OBJECT_W, IMG_OBJECT_H, WHITE);
}

static void drawStarEffect(void)
{
    drawObjectFast(ballX, ballY, 0, OBJECT_STAR);
    int8_t y = ballY - ballE;
    int16_t dx = getScreenX(0, y) + (GAP_MAX - ballG) / 8;
    for (int i = 0; i < FIELD_W; i++) {
        if (field[y][i] == OBJECT_EMPTY) {
            int16_t dy = getScreenY(i, y);
            arduboy.drawFastVLine2(dx, dy, IMG_OBJECT_H, WHITE);
        }
    }
}

static void drawStatus(void)
{
    drawLabelLevel(0, 44);
    drawNumberV(6, 63, level, ALIGN_LEFT);
    drawLabelScore(122, 44);
    drawNumberV(122, -1, score, ALIGN_RIGHT);
    arduboy.drawRect2(0, 0, 8, 18, WHITE);
    if (starGuage < starGuageMax || bitRead(record.playFrames, 0)) {
        arduboy.fillRect2(1, 1, 6, starGuage * 16 / starGuageMax, WHITE);
    }
}
