#include "common.h"

/*  Defines  */

enum {
    STATE_START = 0,
    STATE_GAME,
    STATE_OVER,
    STATE_PAUSE,
};

#define BOXES           128
#ifdef DEBUG
#define SNOWS           16
#else
#define SNOWS           64
#endif


enum {
    ALIGN_LEFT = 0,
    ALIGN_CENTER,
    ALIGN_RIGHT,
};

#define coord(x)        ((x) * 64)
#define coordInv(x)     ((x) / 64)
#define secs(s)         ((s) * 60)
#define mid(a, b, c)    max(min((b), (c)), (a))
#define sign(n)         (((n) > 0) - ((n) < 0))

/*  Typedefs  */

typedef struct {
    int16_t x, y;
    int8_t  vx, vy;
} BOX;

typedef struct {
    int8_t x, y;
} SNOW;

/*  Local Functions  */

static void startGame(boolean isFirst);
static void moveCat(void);
static void initBoxes(void);
static void moveBoxes(void);
static void boundBox(BOX *pBox, int gap);
static int  getNewBoxIndex(void);
static void initSnows(void);
static void moveSnows(void);

static void drawCat(void);
static void drawBoxes(void);
static void drawSnows(void);
static void drawStrings(void);

/*  Local Variables  */

PROGMEM static const byte soundStart[] = {
    0x90, 72, 0, 100, 0x80, 0, 25,
    0x90, 74, 0, 100, 0x80, 0, 25,
    0x90, 76, 0, 100, 0x80, 0, 25,
    0x90, 77, 0, 100, 0x80, 0, 25,
    0x90, 79, 0, 200, 0x80, 0xF0
};

PROGMEM static const byte soundOver[] = {
    0x90, 55, 0, 120, 0x80, 0, 10,
    0x90, 54, 0, 140, 0x80, 0, 20,
    0x90, 53, 0, 160, 0x80, 0, 30,
    0x90, 52, 0, 180, 0x80, 0, 40,
    0x90, 51, 0, 200, 0x80, 0, 50,
    0x90, 50, 0, 220, 0x80, 0, 60,
    0x90, 49, 0, 240, 0x80, 0, 70,
    0x90, 48, 0, 260, 0x80, 0xF0
};

static uint8_t  state;
static bool     toDraw;
static uint32_t gameFrames;
static int      counter;
static int      timer;
static int8_t   ignoreCnt;
static uint     score;
static bool     isHiscore;
static uint8_t  catX;
static uint8_t  catAnim;
static uint8_t  boxCnt, boxIdx;
static BOX      boxAry[BOXES];
static SNOW     snowAry[SNOWS];

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initGame(void)
{
    startGame(true);
}

bool updateGame(void)
{
#ifdef DEBUG
    if (dbgRecvChar == 'z') {
        startGame(false);
    }
#endif

    /*  In case of pausing  */
    if (state == STATE_PAUSE) {
        if (arduboy.buttonDown(A_BUTTON | B_BUTTON)) {
            state = STATE_GAME;
            ignoreCnt = 30; // a half second
            toDraw = true;
            dprintln("Resume");
        }
        return false;
    }

    /*  Usual case  */
    moveCat();
    moveSnows();
    if (state == STATE_START) {
        if (--counter == 0) {
            state = STATE_GAME;
        }
    } else if (state == STATE_GAME) {
        if (timer > 0) timer--;
        moveBoxes();
        gameFrames++;
        if (boxCnt == 0) {
            state = STATE_OVER;
            isHiscore = (setLastScore(score, gameFrames) == 0);
            counter = secs(8);
            arduboy.playScore2(soundOver, 1);
            dprint("Game Over: score=");
            dprintln(score);
        } else if (ignoreCnt == 0 && arduboy.buttonDown(A_BUTTON | B_BUTTON)) {
            state = STATE_PAUSE;
            dprintln("Pause");
        }
    } else if (state == STATE_OVER) {
        counter--;
        if (ignoreCnt == 0 && arduboy.buttonDown(A_BUTTON | B_BUTTON)) {
            startGame(false);
        }
    }
    if (ignoreCnt > 0) ignoreCnt--;
    toDraw = true;

    return (state == STATE_OVER && counter == 0);
}

void drawGame(void)
{
    if (toDraw) {
        arduboy.clear();
        arduboy.drawFastVLine2(0, 0, 64, WHITE);
        arduboy.drawFastVLine2(127, 0, 64, WHITE);
        drawSnows();
        drawBoxes();
        drawCat();
        drawStrings();
        toDraw = false;
    }
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void startGame(boolean isFirst)
{

    state = STATE_START;
    gameFrames = 0;
    counter = secs(2);
    timer = secs(120); // 2 minutes
    score = 0;
    catX = 64;
    catAnim = 0;

    initBoxes();
    if (isFirst) {
        initSnows();
    }
    arduboy.playScore2(soundStart, 0);
    dprintln("Start Game");

}

static void moveCat(void)
{
    int vx = 0;
    if (arduboy.buttonPressed(LEFT_BUTTON)) vx--;
    if (arduboy.buttonPressed(RIGHT_BUTTON)) vx++;
    catX = mid(0, catX + vx, 128);
}

static void initBoxes(void)
{
    BOX *pBox = &boxAry[0];
    pBox->x = coord(64);
    pBox->y = coord(4);
    pBox->vx = 0;
    pBox->vy = 0;
    for (int i = 1; i < BOXES; i++) {
        boxAry[i].x = -1;
    }
    boxCnt = 1;
    boxIdx = 1;
}

static void moveBoxes(void)
{
    for (int i = 0; i < BOXES; i++) {
        BOX *pBox = &boxAry[i];
        if (pBox->x < 0) continue;
        uint16_t lastY = pBox->y;
        pBox->x += pBox->vx;
        pBox->y += pBox->vy;
        pBox->vy++;

        if (pBox->x < coord(4)) {
            pBox->x = coord(4);
            pBox->vx = -pBox->vx * 3 / 4;
        } else if (pBox->x > coord(124)) {
            pBox->x = coord(124);
            pBox->vx = -pBox->vx * 3 / 4;
        }

        int gap = pBox->x - coord(catX);
        if (timer > 0 && pBox->y >= coord(60) && lastY < coord(60) && abs(gap) <= coord(12)) {
            boundBox(pBox, coordInv(gap));
            score++;
        } else if (pBox->y > coord(70)) {
            pBox->x = -1;
            boxCnt--;
        }
    }
}

static void boundBox(BOX *pBox, int gap)
{
    int16_t x = pBox->x;
    int8_t  vx = pBox->vx;
    int8_t  vy = -pBox->vy / 2;

    int mul = 2;
    if (timer > secs(30)) {
        mul = 2;
        gap -= 4;
    } else {
        mul = 3;
        gap -= 8;
    }
    for (int i = 0; i < mul; i++, gap += 8) {
        if (i > 0 && boxCnt < BOXES) {
            pBox = &boxAry[getNewBoxIndex()];
            pBox->x = x;
            boxCnt++;
        }
        pBox->y = coord(60);
        pBox->vx = vx + gap;
        pBox->vy = vy - (40 - abs(gap));
    }
}

static int getNewBoxIndex(void)
{
    do {
        boxIdx = (boxIdx + 1) % BOXES;
    } while (boxAry[boxIdx].x >= 0);
    return boxIdx;
}

static void initSnows(void)
{
    for (int i = 0; i < SNOWS; i++) {
        SNOW *pSnow = &snowAry[i];
        pSnow->x = random(128);
        pSnow->y = random(64);
    }
}

static void moveSnows(void)
{
    for (int i = 0; i < SNOWS; i++) {
        SNOW *pSnow = &snowAry[i];
        pSnow->x += random(-1, 2);
        pSnow->y += random(2);
        if (pSnow->y >= 64) {
            pSnow->x = random(128);
            pSnow->y = 0;
        }
    }
}

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void drawCat(void)
{
    arduboy.fillRect2(catX - 12, 60, 24, 4, WHITE);
}

static void drawBoxes(void)
{
    for (int i = 0; i < BOXES; i++) {
        BOX *pBox = &boxAry[i];
        if (pBox->x < 0) continue;
        int16_t x = coordInv(pBox->x);
        int16_t y = coordInv(pBox->y);
        arduboy.drawRect2(x - 4, y - 4, 8, 8, WHITE);
    }
}

static void drawSnows(void)
{
    for (int i = 0; i < SNOWS; i++) {
        SNOW *pSnow = &snowAry[i];
        arduboy.drawPixel(pSnow->x, pSnow->y, WHITE);
    }
}

static void drawStrings(void)
{
    /*  Score  */
    arduboy.setCursor(2, 0);
    arduboy.print(score);

    /*  Timer  */
    char buf[6];
    arduboy.setCursor(102, 0);
    if (timer >= secs(60)) {
        arduboy.print(timer / 3600);
        sprintf(buf, "'%02d", timer % 3600 / 60);
        arduboy.print(buf);
    } else {
        sprintf(buf, "%02d\"", timer / 60);
        arduboy.print(buf);
        arduboy.print(timer % 60 / 6);
    }

    /*  Etc.  */
    if (state == STATE_START) {
        arduboy.printEx(49, 29, F("READY"));
    } else if (state == STATE_GAME) {
        if (timer <= secs(30) && timer > secs(27) && timer % 8 < 4) {
            arduboy.printEx(46, 12, F("FEVER!"));
        }
    } else if (state == STATE_OVER) {
        arduboy.printEx(37, 29, F("GAME OVER"));
        if (isHiscore && counter % 8 < 4) {
            arduboy.printEx(31, 46, F("NEW RECORD!"));
        }
    } else if (state == STATE_PAUSE) {
        arduboy.printEx(49, 46, F("PAUSE"));
    }

}
