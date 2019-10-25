#include "common.h"
#include "data.h"

/*  Defines  */

#define FIELD_W     6
#define FIELD_H     14
#define BUNCH_MAX   3
#define ROT_SINGLE  5
#define GAP_MAX     64
#define FALLSPD_MAX (GAP_MAX / 2)
#define BALLSPD_MAX (GAP_MAX * 3 / 8)
#define BALLE_STUCK 7
#define STARSPD     (GAP_MAX / 8)
#define STARG_INIT  80
#define STARG_MAX   400
#define STARG_STEP1 40
#define STARG_STEP2 100
#define FIX_GRACE   (FPS / 2)
#define ERASE_WAIT  (FPS / 2)
#define FALL_WAIT   (FPS * 2 / 3)
#define NEXT_WAIT   (FPS / 3)
#define START_WAIT  (FPS * 2)
#define OVER_WAIT   (FPS * 10)
#define DYING_MAX   4
#define BONUS_THRES 10
#define LEVEL_MAX   99
#define LEVEL_NOVIC 10
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
    OBJECT_ENEMY_DYING1,
    OBJECT_ENEMY_DYING2,
    OBJECT_ENEMY_DYING3,
};

/*  Typedefs  */

typedef struct {
    int8_t x;
    int8_t y;
} DYING_T;

/*  Local Functions  */

static void     initParameters(void);
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
static bool     decayDyingEnemies(void);

static void     onContinue(void);
static void     onRestart(void);
static void     onQuit(void);

static void     drawObjectFast(int8_t x, int8_t y, int8_t g, OBJECT_T obj);
static void     drawField(void);
static void     clearLastResidues(void);
static void     drawStatus(bool isFullUpdate);
static void     drawBunch(int8_t x, int8_t y, int8_t r, int8_t g, OBJECT_T* pBunch);
static void     drawDyingEnemies(void);
static void     drawGameWait(void);
static void     drawGameControl(void);
static void     drawGameFall(void);
static void     drawGameErase(void);
static void     drawGameBall(void);
static void     drawGameStar(void);
static void     controlLed(void);

/*  Local Variables  */

PROGMEM static void (* const forwardGameFuncs[])(void) = {
    forwardGameWait,    forwardGameControl, forwardGameFall,
    forwardGameErase,   forwardGameBall,    forwardGameStar
};
#define forwardGame(mode) ((void (*)(void))pgm_read_ptr(forwardGameFuncs + (mode)))()

PROGMEM static void (* const drawGameSubFuncs[])(void) = {
    drawGameWait,   drawGameControl,    drawGameFall,
    drawGameErase,  drawGameBall,       drawGameStar
};
#define drawGameSub(mode) ((void (*)(void))pgm_read_ptr(drawGameSubFuncs + (mode)))()

static STATE_T      state = STATE_INIT;
static GAME_MODE_T  gameMode = GAME_MODE_WAIT;
static OBJECT_T     field[FIELD_H][FIELD_W];
static OBJECT_T     currentBunch[BUNCH_MAX], nextBunch[BUNCH_MAX];
static DYING_T      dyingEnemies[DYING_MAX];
static bool         isFastFall, isFieldFalled, isHiscore;
static int8_t       bunchX, bunchY, bunchR, bunchG;
static int8_t       ballX, ballY, ballV, ballG, ballE;
static int8_t       lastX, lastY, lastR, lastG, obtainedScoreY;
static uint8_t      level, bunchCount, fallSpeed, ballSpeed, ballFreq, scoreCoef;
static uint8_t      nextBall, boxesCount, enemiesCount, killedEnemies, maxHeight;
static int16_t      gameCounter, filledLines, starGuage, starGuageMax;
static uint32_t     score, obtainedScore;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initGame(void)
{
    initParameters();
    initField();
    tuneParameters();
    nextBall = ballFreq;
    setNextBunch();
    gameMode = GAME_MODE_WAIT;
    gameCounter = START_WAIT;

    state = STATE_START;
    isInvalid = true;
    arduboy.playScore2(soundStart, SND_PRIO_START);
    dprintln(F("Start Game!"));
}

MODE_T updateGame(void)
{
    switch (state) {
    case STATE_START:
        if (--gameCounter <= NEXT_WAIT) {
            record.playCount++;
            state = STATE_PLAYING;
            isInvalid = true;
        }
        break;
    case STATE_PLAYING:
        record.playFrames++;
        isRecordDirty = true;
        handleDPadV();
        forwardGame(gameMode);
        if (state == STATE_PLAYING && arduboy.buttonDown(A_BUTTON)) {
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
        if (--gameCounter <= OVER_WAIT - FPS) {
            if (arduboy.buttonDown(A_BUTTON | B_BUTTON) || gameCounter <= 0) state = STATE_LEAVE;
        }
        break;
    default:
        break;
    }
#ifdef DEBUG
    if (state == STATE_PLAYING) {
        if (dbgRecvChar == 'l' && level < LEVEL_MAX) {
            level++;
            bunchCount = 0;
            tuneParameters();
        }
        if (dbgRecvChar == 's') {
            starGuage = starGuageMax;
        }
    }
#endif
    return (state == STATE_LEAVE) ? MODE_TITLE : MODE_GAME;
}

void drawGame(void)
{
    if (state == STATE_LEAVE) return;
    if (state == STATE_MENU) {
        if (isInvalid) {
            arduboy.fillRect2(0, 24, IMG_OBJECT_W * 2, IMG_OBJECT_H * 2, BLACK);
            arduboy.fillRect2(16, 8, (FIELD_H - 1) * IMG_OBJECT_W, FIELD_W * IMG_OBJECT_H, BLACK);
            arduboy.setRGBled(0, 0, 0);
        }
        drawMenuItems(isInvalid);
        isInvalid = false;
        return;
    }

    if (isInvalid) {
        arduboy.clear();
        drawField();
    } else {
        clearLastResidues();
    }
    drawStatus(isInvalid);
    if (gameMode != GAME_MODE_CONTROL || bunchY < FIELD_H - 2) {
        drawBunch(2, FIELD_H, (nextBunch[1] == OBJECT_EMPTY) ? ROT_SINGLE : 0, 0, nextBunch);
    }
    if (state == STATE_PLAYING) drawGameSub(gameMode);
    controlLed();
    isInvalid = false;
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void initParameters(void)
{
    level = startLevel;
    score = 0;
    bunchCount = 0;
    starGuage = 0;
    starGuageMax = STARG_INIT;
    isFieldFalled = false;
    obtainedScore = 0;
    if (startLevel == HILEVEL3) {
        score = HISCORE3;
    } else if (startLevel == HILEVEL4) {
        score = HISCORE4;
        starGuageMax = STARG_INIT * 2;
    }
}

static void initField(void)
{
    for (int i = 0; i < FIELD_H; i++) {
        for (int j = 0; j < FIELD_W; j++) {
            field[i][j] = OBJECT_EMPTY;
        }
    }
    for (int i = 0; i < DYING_MAX; i++) {
        dyingEnemies[i].y = -1;
    }
    boxesCount = 0;
    enemiesCount = 0;
    maxHeight = 0;
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
    bunchY = FIELD_H;
    bunchR = (currentBunch[1] == OBJECT_EMPTY) ? ROT_SINGLE : 0;
    bunchG = 0;
    lastY = 0;
    isFastFall = false;
    killedEnemies = 0;
    gameMode = GAME_MODE_CONTROL;
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
            if (isFastFall) score++;
            bunchG += GAP_MAX;
            gameCounter = FIX_GRACE;
        } else {
            bunchG = 0;
            if (gameCounter == FIX_GRACE) arduboy.playScore2(soundFall, SND_PRIO_CONTROL);
            gameCounter--;
            if (isFastFall) gameCounter = 0;
        }
    }
    if (gameCounter > 0) {
        if (padX != 0 && isMoveable(bunchX + padX, bunchY, bunchR)) bunchX += padX;
        if (arduboy.buttonDown(UP_BUTTON_V | B_BUTTON) && bunchR != ROT_SINGLE &&
                isMoveable(bunchX, bunchY, (bunchR + 1) & 3)) {
            bunchR = (bunchR + 1) & 3;
            arduboy.playScore2(soundRotate, SND_PRIO_CONTROL);
        }
    } else {
        placeCurrentBunch();
        gameMode = GAME_MODE_FALL;
        isInvalid = true;
    }
}

static void forwardGameFall(void)
{
    if (gameCounter > 0) {
        gameCounter--;
        if (ballE == BALLE_STUCK) {
            ballG -= ballSpeed;
            while (ballG < 0) {
                ballG += GAP_MAX;
                if (!decayDyingEnemies()) ballE == 0;
            }
        }
        if (gameCounter == 0 && obtainedScore > 0) {
            obtainedScore = 0;
            isInvalid = true;
        }
        return;
    }
    if (fallObject()) {
        isFieldFalled = true;
        isInvalid = true;
        return;
    }
    if (isFieldFalled) {
        arduboy.playScore2(soundFall, SND_PRIO_CONTROL);
        isFieldFalled = false;
    }

    filledLines = checkFilledLines();
    if (filledLines) {
        arduboy.playScore2(soundErase, SND_PRIO_EFFECT);
        gameMode = GAME_MODE_ERASE;
        gameCounter = ERASE_WAIT;
    } else if (ballY < FIELD_H) {
        OBJECT_T obj = field[ballY][ballX];
        if (obj == OBJECT_STAR) {
            gameMode = GAME_MODE_STAR;
            ballV = 0;
            ballG = GAP_MAX - STARSPD;
        } else {
            gameMode = GAME_MODE_BALL;
            ballV = (obj == OBJECT_BALL_L) ? -1 : 1;
            ballG = 0;
            ballE = 0;
            lastX = 0;
        }
    } else if (field[FIELD_H - 2][2] == OBJECT_EMPTY &&
            (nextBunch[1] == OBJECT_EMPTY || field[FIELD_H - 2][3] == OBJECT_EMPTY)) {
        if (++bunchCount >= LEVEL_FREQ && level < LEVEL_MAX) {
            level++;
            bunchCount = 0;
            tuneParameters();
            arduboy.playScore2(((level % 5 == 0)) ? soundLevel5Up : soundLevel1Up, SND_PRIO_EFFECT);
        }
        gameMode = GAME_MODE_WAIT;
        gameCounter = NEXT_WAIT;
     } else {
        isHiscore = enterScore(score, level);
        writeRecord();
        gameCounter = OVER_WAIT;
        state = STATE_OVER;
        isInvalid = true;
        arduboy.playScore2(soundOver, SND_PRIO_OVER);
        dprintln(F("Game over..."));
    }
}

static void forwardGameErase(void)
{
    if (--gameCounter > 0) return;
    uint8_t lines = 0;
    obtainedScoreY = -1;
    for (int i = 0; i < FIELD_H; i++) {
        if (bitRead(filledLines, i)) {
            lines++;
            if (obtainedScoreY == -1) obtainedScoreY = i;
        }
    }
    boxesCount -= lines * FIELD_W;
    obtainedScore = (lines * (lines - 1) + 1) * scoreCoef * 20UL;
    if (killedEnemies > 0) {
        obtainedScore *= (boxesCount == 0 && enemiesCount == 0) ? 3UL : 2UL;
    }
    score += obtainedScore;
    gameMode = GAME_MODE_FALL;
    gameCounter = FALL_WAIT;
    //isInvalid = true;
    dprint(F("Erase lines:"));
    dprintln(lines);
}

static void forwardGameBall(void)
{
    ballG -= ballSpeed;
    while (ballG < 0 && gameMode == GAME_MODE_BALL)  {
        ballG += GAP_MAX;
        decayDyingEnemies();
        if (field[ballY][ballX] == OBJECT_ENEMY) {
            enemiesCount--;
            killedEnemies++;
            starGuage++;
            uint16_t enemyPoint = (killedEnemies + 1) * scoreCoef;
            score += enemyPoint;
            obtainedScore += enemyPoint;
            dyingEnemies[DYING_MAX - 1].x = ballX;
            dyingEnemies[DYING_MAX - 1].y = ballY;
            arduboy.playScore2(soundKill, SND_PRIO_EFFECT);
        }
        field[ballY][ballX] = OBJECT_EMPTY;
        if (ballY > 0 && field[ballY - 1][ballX] != OBJECT_BOX) {
            ballY--;
            ballE = 0;
        } else {
            ballE |= (ballX == 0 || field[ballY][ballX - 1] == OBJECT_BOX) | 2;
            ballE |= (ballX == FIELD_W - 1 || field[ballY][ballX + 1] == OBJECT_BOX) * 4;
            if (ballE == BALLE_STUCK) {
                obtainedScoreY = ballY;
                if (enemiesCount == 0 && killedEnemies >= BONUS_THRES) {
                    score += obtainedScore;
                    obtainedScore *= 2UL;
                }
                gameMode = GAME_MODE_FALL;
                gameCounter = FALL_WAIT;
                isInvalid = true;
                dprint(F("Killed(Ball):"));
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
    ballG += STARSPD;
    while (ballG >= GAP_MAX && gameMode == GAME_MODE_STAR) {
        ballG -= GAP_MAX;
        int8_t y = ballY - ballV;
        bool isCancelled = false;
        if (y == ballY) {
            if (arduboy.buttonPressed(UP_BUTTON_V | B_BUTTON)) {
                isCancelled = true;
                arduboy.playScore2(soundStarCancel, SND_PRIO_EFFECT);
            } else {
                arduboy.playScore2(soundStar, SND_PRIO_EFFECT);
            }
        } else {
            for (int i = 0; i < FIELD_W; i++) {
                if (field[y][i] == OBJECT_ENEMY) {
                    field[y][i] = OBJECT_EMPTY;
                    enemiesCount--;
                    killedEnemies++;
                    starGuage++;
                    uint16_t enemyPoint = (killedEnemies + 1) * scoreCoef;
                    score += enemyPoint;
                    obtainedScore += enemyPoint;
                }
            }
        }
        if (y == 0 || ++ballV > 7 || isCancelled) {
            field[ballY][ballX] = OBJECT_EMPTY;
            if (killedEnemies == 0) {
                obtainedScore = scoreCoef * 2000UL;
                score += obtainedScore;
                starGuage += starGuageMax / 2;
            }
            obtainedScoreY = y;
            gameMode = GAME_MODE_FALL;
            gameCounter = FALL_WAIT;
            isInvalid = true;
            dprint(F("Killed(Star):"));
            dprintln(killedEnemies);
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
            if (starGuageMax < STARG_MAX) {
                starGuageMax += (starGuageMax < STARG_MAX / 2) ? STARG_STEP1 : STARG_STEP2;
            }
            dprint(F("starGuageMax="));
            dprintln(starGuageMax);
        } else {
            isBall = true;
            isSingle = (level < LEVEL_NOVIC);
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
    maxHeight = 0;
    for (int i = 1; i < FIELD_H; i++) {
        for (int j = 0; j < FIELD_W; j++) {
            OBJECT_T obj = field[i][j];
            if (obj != OBJECT_EMPTY) {
                maxHeight = i;
                if (field[i - 1][j] == OBJECT_EMPTY) {
                    field[i - 1][j] = obj;
                    field[i][j] = OBJECT_EMPTY;
                    isFalled = true;
                }
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

static bool decayDyingEnemies(void)
{
    bool isDying = false;
    for (int i = 0; i < DYING_MAX - 1; i++) {
        DYING_T d = dyingEnemies[i + 1];
        isDying = isDying || (d.y >= 0);
        dyingEnemies[i] = d;
    }
    dyingEnemies[DYING_MAX - 1].y = -1;
    return isDying;
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

static void drawObjectFast(int8_t x, int8_t y, int8_t g, OBJECT_T obj)
{
    uint16_t offset = (FIELD_W - x) * WIDTH + getScreenX(x, y) - g / 8;
    if (obj == OBJECT_STAR && bitRead(record.playFrames, 0)) obj = OBJECT_STAR_BLINK; 
    memcpy_P(arduboy.getBuffer() + offset, imgObject[obj], IMG_OBJECT_W);
}

static void drawField(void)
{
    arduboy.drawFastHLine2(16, 7, (FIELD_H - 1) * IMG_OBJECT_W + 1, WHITE);
    arduboy.drawFastHLine2(16, 56, (FIELD_H - 1) * IMG_OBJECT_W + 1, WHITE);
    arduboy.drawFastVLine2(120, 8, FIELD_W * IMG_OBJECT_H, WHITE);
    for (int i = 0; i < FIELD_H; i++) {
        for (int j = 0; j < FIELD_W; j++) {
            OBJECT_T obj = field[i][j];
            if (obj != OBJECT_EMPTY) drawObjectFast(j, i, 0, obj);
        }
    }
}

static void clearLastResidues(void)
{
    arduboy.fillRect2(6, 52, 6, 12, BLACK);
    arduboy.fillRect2(122, 0, 6, 42, BLACK);
    if (gameMode == GAME_MODE_CONTROL && lastY >= bunchY) {
        drawBunch(lastX, lastY, lastR, lastG, NULL);
    }
    if (gameMode == GAME_MODE_BALL && lastX != 0) {
        arduboy.fillRect2(lastX, lastY, IMG_OBJECT_W, IMG_OBJECT_H, BLACK);
    }
}

static void drawStatus(bool isFullUpdate)
{
    drawLabelLevel(0, 44);
    drawNumberV(6, 63, level, ALIGN_LEFT);
    drawNumberV(122, -1, score, ALIGN_RIGHT);
    arduboy.drawRect2(0, 0, 8, 18, WHITE);
    int8_t h = starGuage * 16 / starGuageMax;
    if (starGuage >= starGuageMax && bitRead(record.playFrames, 0)) h = 0;
    arduboy.fillRect2(1, 1, 6, h, WHITE);
    arduboy.fillRect2(1, 1 + h, 6, 16 - h, BLACK);
    if (isFullUpdate) {
        drawLabelScore(122, 44);
        if (state == STATE_START) {
            arduboy.drawBitmap(57, 12, imgReady, IMG_READY_W, IMG_READY_H, WHITE);
        }
        if (state == STATE_OVER) {
            arduboy.drawRect2(49, -1, 30, HEIGHT + 2, WHITE);
            arduboy.fillRect2(50, 0, 28, HEIGHT, BLACK);
            arduboy.drawBitmap(60, 0, imgGameOver1, IMG_GAMEOVER1_W, IMG_GAMEOVER1_H, WHITE);
            arduboy.drawBitmap(60, 56, imgGameOver2, IMG_GAMEOVER2_W, IMG_GAMEOVER2_H, WHITE);
        }
    }
    if (state == STATE_OVER && isHiscore) {
        uint8_t c = bitRead(gameCounter, 3);
        arduboy.setTextColor(c, c);
        arduboy.printEx(72, 61, F("NEW RECORD"));
        arduboy.setTextColor(WHITE, WHITE);
    }
}

static void drawBunch(int8_t x, int8_t y, int8_t r, int8_t g, OBJECT_T* pBunch)
{
    for (int i = 0; i < BUNCH_MAX; i++) {
        int8_t bx = x + bitRead(r + i, 1);
        int8_t by = y - bitRead(r + i + 1,  1);
        OBJECT_T obj = (pBunch) ? pBunch[i] : OBJECT_EMPTY;
        drawObjectFast(bx, by, g, obj);
        if (r == ROT_SINGLE) break;
    }
}

static void drawDyingEnemies(void)
{
    for (int i = 0; i < DYING_MAX; i++) {
        int8_t bx = dyingEnemies[i].x;
        int8_t by = dyingEnemies[i].y;
        if (by >= 0) {
            OBJECT_T obj = (OBJECT_T) (OBJECT_ENEMY_DYING1 + (DYING_MAX - 1 - i));
            if (i == 0) obj = OBJECT_EMPTY;
            drawObjectFast(bx, by, 0, obj);
        }
    }
}

static void drawGameWait(void)
{
    if (ballE == BALLE_STUCK) drawDyingEnemies();
}

static void drawGameControl(void)
{
    drawBunch(bunchX, bunchY, bunchR, bunchG, currentBunch);
    lastX = bunchX;
    lastY = bunchY;
    lastR = bunchR;
    lastG = bunchG;
}

static void  drawGameFall(void)
{
    if (ballE == BALLE_STUCK) drawDyingEnemies();
    if (obtainedScore > 0) {
        uint16_t dx = getScreenX(0, obtainedScoreY) + 2;
        uint16_t dy = HEIGHT / 2 - 1;
        arduboy.setTextColor(BLACK, BLACK);
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                drawNumberV(dx + j, dy + i, obtainedScore, ALIGN_CENTER);
            }
        }
        arduboy.setTextColor(WHITE, WHITE);
        if (bitRead(gameCounter, 1)) drawNumberV(dx, dy, obtainedScore, ALIGN_CENTER);
    }
}

static void drawGameErase(void)
{
    for (int i = 0; i < FIELD_H; i++) {
        if (bitRead(filledLines, i)) {
            int16_t dx = getScreenX(0, i);
            for (int j = 0; j < IMG_OBJECT_W; j++) {
                uint8_t c = (random(ERASE_WAIT) < gameCounter - 1);
                arduboy.drawFastVLine2(dx + j, 8, FIELD_W * IMG_OBJECT_H, c);
            }
        }
    }
}

static void drawGameBall(void)
{
    drawDyingEnemies();
    int16_t dx = getScreenX(ballX, ballY);
    int16_t dy = getScreenY(ballX, ballY);
    if (ballE == 0) {
        dx -= ballG / 8;
    } else {
        dy += ballG / 8 * ballV;
    }
    arduboy.drawBitmap(dx, dy, imgObject[OBJECT_BALL], IMG_OBJECT_W, IMG_OBJECT_H, WHITE);
    lastX = dx;
    lastY = dy;
}

static void drawGameStar(void)
{
    drawObjectFast(ballX, ballY, 0, OBJECT_STAR);
    if (ballV == 0) return;
    int8_t y = ballY - ballV;
    for (int i = y + (ballG < STARSPD); i >= y; i--) {
        for (int j = 0; j < FIELD_W; j++) {
            OBJECT_T obj = field[i][j];
            if (obj == OBJECT_ENEMY) {
                obj = (OBJECT_T)(OBJECT_ENEMY + ballG / 16);
            }
            drawObjectFast(j, i, 0, obj);
        }
    }
    int16_t dx = getScreenX(0, y) + ballG / 8;
    arduboy.drawFastVLine2(dx, 8, FIELD_W * IMG_OBJECT_H, WHITE);
}

static void controlLed(void)
{
    uint8_t r = 0, g = 0;
    if (state == STATE_PLAYING) {
        if (maxHeight >= FIELD_H - 4) {
            uint8_t grade = FIELD_H - maxHeight - 1;
            uint8_t cycle = 1 << (grade + 3);
            int8_t  tmp = cycle / 2 - (record.playFrames & (cycle - 1));
            tmp = (abs(tmp) << (3 - grade)) - grade * 8;
            if (tmp > 0) r = tmp;
        }
    }
    if (state == STATE_OVER) {
        int16_t tmp = (gameCounter - (OVER_WAIT - 32)) * 2;
        if (tmp > 0) {
            r = tmp;
            g = tmp;
        }
    }
    arduboy.setRGBled(r, g, 0);
}
