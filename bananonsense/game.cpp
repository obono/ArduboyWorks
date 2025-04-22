#include "common.h"
#include "data.h"

/*  Defines  */

#define FIELD_W         16
#define FIELD_H         7
#define DRAW_FIELD_W    WIDTH
#define DRAW_FIELD_H    (HEIGHT - IMG_LETTERS_H)
#define TARGET_SIZE     6
#define CURSOR_W        (IMG_LETTERS_W * TARGET_SIZE)
#define CURSOR_H        IMG_LETTERS_H
#define DRAW_CURSOR_X   ((DRAW_FIELD_W - CURSOR_W) / 2)
#define DRAW_CURSOR_Y   ((DRAW_FIELD_H - CURSOR_H) / 2)

enum GAME_STATE_T : uint8_t {
    STATE_START = 0,
    STATE_PLAYING,
    STATE_FOUND,
    STATE_TIME_UP,
    STATE_GAME_OVER,
    STATE_MENU,
    STATE_LEAVE,
};

enum LETTER_T : uint8_t {
    LETTER_B = 0,
    LETTER_N,
    LETTER_A,
    LETTER_BLANK,
};

#define BITS_PER_LETTER 2
#define TARGET_PATTERN  0x998 // "BANANA" = 0b100110011000
#define TIMER_MAX       (FPS * 30) // 30 seconds
#define BITS_PER_SAYING 5

/*  Typedefs  */

typedef struct {
    uint8_t     letter : 2;
    uint8_t     time : 6;
} UNIT_T;

/*  Local Functions  */

static void handleStart(void);
static void handlePlaying(void);
static void handleInterlude(void);
static void handleGameOver(void);

static void setupWave(void);
static void setupMessage(uint8_t idx);
static void setupField(void);
static void updateField(void);
static bool isTarget(int8_t x, int8_t y);

static void onContinue(void);
static void onQuit(void);

static void drawStart(void);
static void drawPlaying(void);
static void drawInterlude(void);
static void drawGameOver(void);
static void drawMessage(int16_t y, char *p);
static void drawField(bool isInterlude);
static int8_t calcDrawLetterGap(int8_t offset, int8_t size);
static bool isInside(int8_t x, int8_t y, int8_t focusX, int8_t focusY);
static void drawCursor(int16_t x, int16_t y, int8_t e);
static void drawTimer(void);

/*  Local Functions (macros)  */

#define callHandlerFunc(n)  ((void (*)(void)) pgm_read_ptr(handlerFuncTable + n))()
#define callDrawerFunc(n)   ((void (*)(void)) pgm_read_ptr(drawerFuncTable + n))()

/*  Local Constants  */

PROGMEM static void(*const handlerFuncTable[])(void) = {
    handleStart, handlePlaying, handleInterlude, handleInterlude, handleGameOver, handleMenu
};

PROGMEM static void(*const drawerFuncTable[])(void) = {
    drawStart, drawPlaying, drawInterlude, drawInterlude, drawGameOver
};

/*  Local Variables  */

static GAME_STATE_T state;
static UNIT_T field[FIELD_H][FIELD_W];
static int16_t timer;
static uint8_t score;
static int8_t targetX, targetY, cursorX, cursorY, offsetX, offsetY;
static char message[80];

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initGame(void)
{
    score = 0;
    setupWave();
}

MODE_T updateGame(void)
{
    callHandlerFunc(state);
    return (state == STATE_LEAVE) ? MODE_TITLE : MODE_GAME;
}

void drawGame(void)
{
    if (state < STATE_MENU) {
        if (isInvalid) {
            ab.clear();
            callDrawerFunc(state);
        }
    } else if (state == STATE_MENU) {
        if (isInvalid) {
            ab.clear();
            drawTimer();
        }
        drawMenuItems(isInvalid);
    }
    isInvalid = false;
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void handleStart(void)
{
    if (++counter == FPS * 2) {
        state = STATE_PLAYING;
        isInvalid = true;
    }
}

static void handlePlaying(void)
{
    /*  Update Field  */
    timer--;
    if ((timer & 3) == 0) {
        updateField();
    }

    /*  Move Cursor  */
    offsetX -= (offsetX > 0 && offsetX < 8) ? 1 : offsetX >> 3;
    offsetY -= (offsetY > 0 && offsetY < 8) ? 1 : offsetY >> 3;
    if (padX != 0 || padY != 0) {
        playSoundTick();
        cursorX = circulate(cursorX, padX, FIELD_W);
        cursorY = circulate(cursorY, padY, FIELD_H);
        offsetX += padX * IMG_LETTERS_W;
        offsetY += padY * IMG_LETTERS_H;
    }

    /*  State Transition  */
    if (ab.buttonDown(B_BUTTON)) {
        if (isTarget(cursorX, cursorY)) {
            /*  Found the target  */
            ab.playScore(soundFind, SND_PRIO_FIND);
            score++;
            record.banana++;
            isRecordDirty = true;
            state = STATE_FOUND;
            counter = 0;
        } else {
            /*  Missed the target  */
            ab.playScore(soundError, SND_PRIO_ERROR);
            timer -= FPS * 3; // 3 seconds;
        }
    }
    if (state == STATE_PLAYING) {
        if (timer <= 0) {
            /*  Game Over  */
            ab.playScore(soundError, SND_PRIO_ERROR);
            writeRecord();
            state = STATE_TIME_UP;
            counter = 0;
        } else if (ab.buttonDown(A_BUTTON)) {
            /*  Save and display the menu  */
            playSoundClick();
            writeRecord();
            clearMenuItems();
            addMenuItem(F("CONTINUE"), onContinue);
            addMenuItem(F("BACK TO TITLE"), onQuit);
            setMenuCoords(10, 26, 107, 11, true, true);
            setMenuItemPos(0);
            state = STATE_MENU;
        }
    }
    isInvalid = true;
}

static void handleInterlude(void)
{
    if (++counter == FPS) {
        if (state == STATE_FOUND) {
            setupWave();
        } else {
            ab.playScore(soundOver, SND_PRIO_OVER);
            state = STATE_GAME_OVER;
            timer = FPS * 10;
        }
    }
    isInvalid = true;
}

static void handleGameOver(void)
{
    if ((++counter & 3) == 0) {
        updateField();
    }
    if (--offsetY <= -IMG_LETTERS_H) {
        cursorY = (cursorY + 1) % FIELD_H;
        offsetY += IMG_LETTERS_H;
    }
    if (ab.buttonDown(B_BUTTON)) {
        initGame();
    } else if (ab.buttonDown(A_BUTTON)) {
        playSoundClick();
        state = STATE_LEAVE;
    } else if (--timer == 0) {
        state = STATE_LEAVE;
    } 
    isInvalid = true;
}

static void setupWave(void)
{
    if (score == 0) {
        ab.playScore(soundStart, SND_PRIO_START);
        strcpy_P(message, PSTR("MOVE THE CURSOR AND FIND \"BANANA\"."));
    } else {
        ab.playWave(WAVE_BANANA_FREQ, waveBanana, WAVE_BANANA_LENGTH, SND_PRIO_BANANA);
        setupMessage(random(SAYINGS_MAX));
    }
    setupField();
    state = STATE_START;
    counter = 0;
    isInvalid = true;
} 

static void setupMessage(uint8_t idx)
{
    uint16_t bitOffset = pgm_read_word(&sayingsOffset[idx]) * BITS_PER_SAYING;
    const uint8_t *pSrc = &sayingsData[bitOffset >> 3]; // bitOffset / 8
    uint8_t bits = bitOffset & 7; // bitOffset % 8
    uint16_t data = pgm_read_byte(pSrc++) >> bits;
    bits = 8 - bits;
    char *pDest = message;
    while (true) {
        if (bits < BITS_PER_SAYING) {
            data |= pgm_read_byte(pSrc++) << bits;
            bits += 8;
        }
        uint8_t a = data & maskbits(BITS_PER_SAYING);
        *pDest++ = (a < 26) ? a + 'A' : pgm_read_byte(&sayingGraphs[a - 26]);
        data >>= BITS_PER_SAYING;
        bits -= BITS_PER_SAYING;
        if (a == maskbits(BITS_PER_SAYING)) break;
    }
    *pDest = '\0';
}

static void setupField(void)
{
    for (uint8_t i = 0; i < FIELD_H; i++) {
        for (uint8_t j = 0; j < FIELD_W; j++) {
            UNIT_T *p = &field[i][j];
            bool loop = true;
            while (loop) {
                p->letter = random(4);
                p->time = (p->letter == LETTER_BLANK) ? random(8) + 1 : 0;
                if (j < 5) {
                    loop = false;
                } else if (j < FIELD_W - 1) {
                    loop = p->letter == LETTER_A && isTarget(j - 5, i);
                } else {
                    switch (p->letter) {
                        case LETTER_B:
                            loop = isTarget(j, i);
                            break;
                        case LETTER_N:
                            loop = isTarget(j - 2, i) || isTarget(j - 4, i);
                            break;
                        case LETTER_A:
                            loop = isTarget(j - 5, i) || isTarget(j - 3, i) || isTarget(j - 1, i);
                            break;
                        default:
                            loop = false;
                            break;
                    }
                }
            }
        }
    }
    targetX = random(FIELD_W);
    targetY = random(FIELD_H);
    for (uint8_t i = 0; i < TARGET_SIZE; i++) {
        uint8_t letter = TARGET_PATTERN >> i * BITS_PER_LETTER & maskbits(BITS_PER_LETTER);
        field[targetY][(targetX + i) % FIELD_W] = { letter, 0 };
    }
    cursorX = cursorY = offsetX = offsetY = 0;
    timer = TIMER_MAX;
}

static void updateField(void)
{
    UNIT_T *p;
    for (uint8_t i = 0; i < FIELD_H; i++) {
        for (uint8_t j = 0; j < FIELD_W; j++) {
            p = &field[i][j];
            if (p->time > 0) {
                if (--p->time == 0 && p->letter == LETTER_BLANK) p->time = 8;
            }
        }
    }
    if (score > 0) {
        p = &field[random(FIELD_H)][random(FIELD_W)];
        if (p->time == 0) p->time = (score < 15) ? score * 4 : 60;
    }
}

static bool isTarget(int8_t x, int8_t y)
{
    while (x < 0) x += FIELD_W;
    uint16_t pattern = 0;
    for (uint8_t i = 0; i < TARGET_SIZE; i++) {
        pattern |= field[y][(x + i) % FIELD_W].letter << i * BITS_PER_LETTER;
    }
    return (pattern == TARGET_PATTERN);
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
    ab.printEx(43, 24, F("WAVE"));
    ab.printEx(73 + (score < 9) * IMG_LETTERS_W, 24, score + 1);
    drawMessage(36, message);
}

static void drawPlaying(void)
{
    drawField(false);
    drawTimer();
}

static void drawInterlude(void)
{
    drawField(true);
    drawTimer();
}

static void drawGameOver(void)
{
    drawField(true);
    for (int8_t i = -3; i <= 3; i += 2) {
        int16_t x = 16 + i % 3, y = 24 + i / 3;
        ab.drawBitmap(x, y, imgGameOver, IMG_GAME_OVER_W, IMG_GAME_OVER_H, BLACK);
    }
    ab.drawBitmap(16, 24, imgGameOver, IMG_GAME_OVER_W, IMG_GAME_OVER_H);
    ab.printEx(1 + (score < 10) * 3, 58, F("YOU FOUND "));
    ab.print(score);
    ab.print(F(" BANANAS!"));
}

static void drawMessage(int16_t y, char *p)
{
    uint8_t n = 0;
    char *pStart = p, *pMark, c;
    do {
        c = *++p;
        if (c == ' ' || c == '\0') pMark = p;
        if (++n >= 21 || c == '\0') {
            uint8_t len = pMark - pStart;
            *pMark = '\0';
            ab.printEx(64 - len * 3, y, pStart);
            y += 6;
            n -= len + 1;
            pStart = pMark + 1;
        }
    } while (c != '\0');
}

static void drawField(bool isInterlude)
{
    int16_t cursorDx = DRAW_CURSOR_X + offsetX, cursorDy = DRAW_CURSOR_Y + offsetY;

    int8_t y = circulate(cursorY, -roundup(cursorDy, IMG_LETTERS_H), FIELD_H);
    int16_t dy = calcDrawLetterGap(offsetY, IMG_LETTERS_H);
    while (dy < DRAW_FIELD_H) {
        int8_t x = circulate(cursorX, -roundup(cursorDx, IMG_LETTERS_W), FIELD_W);
        int16_t dx = calcDrawLetterGap(offsetX, IMG_LETTERS_W);
        while (dx < DRAW_FIELD_W) {
            bool isInsideTarget = isInside(x, y, targetX, targetY);
            if (!isInterlude || (counter & 4) || !isInsideTarget) {
                UNIT_T *p = &field[y][x];
                uint8_t idx = p->letter;
                bool isForced = (isInside(x, y, cursorX, cursorY) || isInsideTarget && isInterlude);
                if (p->time > 0 && (p->letter == LETTER_BLANK || !isForced)) {
                    idx = p->time % 8 + 3;
                    if (idx > 7) idx = 14 - idx;
                }
                ab.drawBitmap(dx, dy, imgLetters[idx], IMG_LETTERS_W, IMG_LETTERS_H);
            }
            x = (x + 1) % FIELD_W;
            dx += IMG_LETTERS_W;
        }
        y = (y + 1) % FIELD_H;
        dy += IMG_LETTERS_H;
    }

    ab.fillRect(0, DRAW_FIELD_H, DRAW_FIELD_W, HEIGHT - DRAW_FIELD_H, BLACK);
    if (!isInterlude) {
        drawCursor(cursorDx, cursorDy, 2);
        if (timer & 1) drawCursor(cursorDx, cursorDy, 3);
    }
}

static int8_t calcDrawLetterGap(int8_t offset, int8_t size)
{
    int8_t gap = offset % size;
    if (offset > 0 && gap) gap -= size;
    return gap;
}

static bool isInside(int8_t x, int8_t y, int8_t focusX, int8_t focusY)
{
    if (y != focusY) return false;
    int8_t over = focusX + TARGET_SIZE - FIELD_W;
    if (over > 0) {
        return (x >= focusX && x < FIELD_W || x >= 0 && x < over);
    } else {
        return (x >= focusX && x < focusX + TARGET_SIZE);
    }
}

static void drawCursor(int16_t x, int16_t y, int8_t e)
{
    ab.drawRect(x - e, y - e, CURSOR_W + e * 2 - 1, CURSOR_H + e * 2 - 1, WHITE);
}

static void drawTimer(void)
{
    ab.printEx(0, 58, F("TIME"));
    ab.fillRect(24, 58, (timer << 4) / 276, 5, WHITE);
}
