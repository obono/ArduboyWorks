#include "common.h"
#include "fortune.h"
#include "Iambic.h"
#include "Decoder.h"
#include "Encoder.h"
#include "FakeKeyboard.h"

/*  Defines  */

#define CONSOLE_W       21
#define CONSOLE_H       10

#define IMG_COMPUTER_W  40
#define IMG_COMPUTER_H  24

#define RECENT_FRAMES_MAX   (FPS * 60 * 10)
#define RECENT_FRAMES_SOME  (FPS * 60 * 3)
#define RECENT_LETTERS_MAX  200

/*  Local Functions  */

static void applySettings(void);
static void clearConsole(void);

static void handleSignal(void);
static bool handleSignalByButtons(void);
static bool handleSignalByFortune(void);
static void handleExtraButtons(void);

static void dealDecodedLetter(char letter);
static void flushFortuneLetters(void);
static void appendLetter(char letter);
static void backSpace(void);
static void lineFeed(void);

static void setupMenu(void);
static void onContinue(void);
static void onFortune(void);
static void onConfirmClear(void);
static void onClear(void);
static void onList(void);
static void onSettings(void);
static void onCredit(void);

static void drawConsoleLetters(void);
static void drawDottedHLine(uint8_t x, uint8_t y, uint8_t w);
static void drawDottedVLine(uint8_t x, uint8_t y, uint8_t h);

/*  Local Constants  */

PROGMEM static const uint8_t imgComputer[120] = { // 40x24
    0xFC, 0x74, 0x74, 0x74, 0x74, 0x74, 0x74, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFF, 0x00, 0x3F, 0x24, 0x2E, 0xE4, 0xE8, 0x2C, 0x24, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xFF, 0x00,
    0x00, 0x00, 0x00, 0x03, 0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x72, 0x5A,
    0x56, 0x52, 0x51, 0x54, 0x56, 0x52, 0x54, 0x56, 0x52, 0x54, 0x56, 0x52, 0x54, 0x56, 0x52, 0x54,
    0x56, 0x72, 0x28, 0x14, 0x0A, 0x05, 0x03, 0x00
};

PROGMEM static const char welcomeEN[] = APP_TITLE "\0(A) MENU\0(B) SIGNAL ON\0";
PROGMEM static const char welcomeJP[] =
        "\x8C\x92\x6A\x8E \x7F\x80\x90\x77 \x70\x8F\x89\x86\x77\x85\0"
        "(A) \x87\x63\x86\x92\0(B) \x89\x8F\x80\x90\x77 \x7A\x8F\0";

/*  Local Variables  */

static Iambic       iambic;
static Decoder      decoder;
static Encoder      encoder;
static FakeKeyboard fakeKeyboard;

static MODE_T   nextMode;
static uint16_t recentFrames;
static uint8_t  consoleX, consoleY, recentLetters;
static char     letters[CONSOLE_H][CONSOLE_W + 1], lastLetter;
static const char *pFortune;
static bool     isFirst = true, isKeyboardActive, isIgnoreButton, isLastSignalOn, isMenuActive;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initConsole(void)
{
    if (isFirst) {
        clearConsole();
        const char *p = (record.decodeMode == DECODE_MODE_JA) ? welcomeJP : welcomeEN;
        size_t len;
        do {
            len = strnlen_P(p, CONSOLE_W);
            memcpy_P(letters[consoleY], p, len);
            p += len + 1;
            consoleY++;
        } while (len > 0);
        recentLetters = 0;
        recentFrames = 0;
        isFirst = false;
    }
    applySettings();
    pFortune = NULL;
    isKeyboardActive = false;
    isIgnoreButton = true;
    isLastSignalOn = false;
    isMenuActive = false;
    isInvalid = true;
}

MODE_T updateConsole(void)
{
    bool isKeyboardActiveNow = (record.keyboard != KEYBOARD_NONE && isUSBConfigured());
    if (isKeyboardActiveNow != isKeyboardActive) {
        isKeyboardActive = isKeyboardActiveNow;
        if (isKeyboardActive) fakeKeyboard.activated();
        lastLetter = ' ';
        isInvalid = true;
    }

    nextMode = MODE_CONSOLE;
    handleDPad();
    if (isMenuActive) {
        if (arduboy.buttonDown(A_BUTTON)) {
            onContinue();
        } else {
            handleMenu();
        }
    } else {
        if (isIgnoreButton) {
            isIgnoreButton = (arduboy.buttonsState() != 0);
        } else {
            handleSignal();
            handleExtraButtons();
        }
        if (recentLetters > 0) recentFrames++;
        record.playFrames++;
        isRecordDirty = true;
        counter++;
    }
    randomSeed(rand() ^ micros()); // Shuffle random
    return nextMode;
}

void drawConsole(void)
{
    (isMenuActive) ? drawMenuItems(isInvalid) : drawConsoleLetters();
    isInvalid = false;
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void applySettings(void)
{
    uint8_t unitFrames = record.unitFrames + 2;
    iambic.reset(unitFrames);
    decoder.reset(unitFrames, record.decodeMode);
    encoder.reset(unitFrames, record.decodeMode);
    fakeKeyboard.reset(record.decodeMode, record.keyboard, record.imeMode);
}

static void clearConsole(void)
{
    memset(letters, 0, sizeof(letters));
    consoleX = 0;
    consoleY = 0;
    decoder.forceStable(true);
}

/*---------------------------------------------------------------------------*/

static void handleSignal(void)
{
    bool isSignalOn = (pFortune == NULL) ? handleSignalByButtons() : handleSignalByFortune();
    if (isSignalOn != isLastSignalOn) {
        (isSignalOn) ? indicateSignalOn() : indicateSignalOff();
        isLastSignalOn = isSignalOn;
    }

    char letter = decoder.appendSignal(isSignalOn);
    if (letter != '\0') {
        dealDecodedLetter(letter);
        if (letter != ' ') lastLetter = (isNonGraph(letter)) ? ' ' : letter;
    }

}

static bool handleSignalByButtons(void)
{
    bool isIambicShortOn = arduboy.buttonPressed(LEFT_BUTTON);
    bool isIambicLongOn = arduboy.buttonPressed(RIGHT_BUTTON);
    return iambic.isSignalOn(isIambicShortOn, isIambicLongOn) || arduboy.buttonPressed(B_BUTTON);
}

static bool handleSignalByFortune(void)
{
    bool ret = false;
    if (encoder.isEncoding()) {
        ret = encoder.forwardFrame();
    } else {
        char letter = pgm_read_byte(++pFortune);
        if (letter != '\0') {
            encoder.setLetter(letter);
        } else {
            decoder.forceStable();
            dealDecodedLetter('\n');
            pFortune = NULL;
        }
    }
    return ret;
}

static void handleExtraButtons(void)
{
    if (pFortune != NULL) {
        if (arduboy.buttonsState() != 0) {
            if (isLastSignalOn) {
                indicateSignalOff();
                isLastSignalOn = false;
            }
            if (arduboy.buttonDown(B_BUTTON)) {
                if (!isKeyboardActive) flushFortuneLetters();
                playSoundClick();
                isIgnoreButton = true;
            } else {
                playSoundTick();
            }
            decoder.forceStable();
            pFortune = NULL;
            isIgnoreButton = true;
        }
    } else if (decoder.getCurrentCode() == CODE_INITIAL) {
        if (padY < 0) { // arduboy.buttonDown(UP_BUTTON)
            decoder.forceStable();
            dealDecodedLetter('\b');
            playSoundTick();
        }
        if (arduboy.buttonDown(DOWN_BUTTON)) {
            decoder.forceStable();
            dealDecodedLetter('\n');
            playSoundTick();
        }
        if (arduboy.buttonDown(A_BUTTON)) setupMenu();
    }
}

/*---------------------------------------------------------------------------*/

static void dealDecodedLetter(char letter)
{
    if (isKeyboardActive) {
        fakeKeyboard.sendLetter(letter);
    } else {
        if (letter == '\b') {
            backSpace();
        } else if (letter == '\n') {
            lineFeed();
        } else {
            appendLetter(letter);
        }
    }

    if (!isNonGraph(letter) && pFortune == NULL) {
        record.madeLetters++;
        recentLetters++;
        if (recentLetters >= RECENT_LETTERS_MAX || recentFrames > RECENT_FRAMES_MAX) {
            writeRecord();
            recentLetters = 0;
            recentFrames = 0;
        }
    }
}

static void flushFortuneLetters(void)
{
    if (decoder.getCurrentCode() == CODE_INITIAL) {
        pFortune++;
    }
    decoder.forceStable();
    char letter;
    while ((letter = pgm_read_byte(pFortune++)) != '\0') {
        appendLetter(letter);
    }
    lineFeed();
    pFortune = NULL;
    isInvalid = true;
}

static void appendLetter(char letter)
{
    letters[consoleY][consoleX] = letter;
    if (++consoleX == CONSOLE_W) lineFeed();
}

static void backSpace(void)
{
    if (consoleX > 0) {
        consoleX--;
    } else if (consoleY > 0) {
        consoleY--;
        for (consoleX = CONSOLE_W - 1; consoleX > 0; consoleX--) {
            if (letters[consoleY][consoleX] != '\0') break;
        }
    }
    char *p = &letters[consoleY][consoleX];
    if (*p == '(') decoder.forceStable(true);
    *p = '\0';
}

static void lineFeed(void)
{
    consoleX = 0;
    if (consoleY == CONSOLE_H - 1) {
        memcpy(letters[0], letters[1], sizeof(letters[0]) * (CONSOLE_H - 1));
        memset(letters[CONSOLE_H - 1], 0, sizeof(letters[0]));
        isInvalid = true;
    } else {
        consoleY++;
    }
}

/*---------------------------------------------------------------------------*/

static void setupMenu(void)
{
    decoder.forceStable();
    if (recentFrames >= RECENT_FRAMES_SOME) {
        writeRecord();
        recentLetters = 0;
        recentFrames = 0;
    }
    clearMenuItems();
    addMenuItem(F("CONTINUE"), onContinue);
    addMenuItem(F("FORTUNE WORDS"), onFortune);
    addMenuItem(F("CLEAR CONSOLE"), onConfirmClear);
    addMenuItem(F("CODE LIST"), onList);
    addMenuItem(F("SETTINGS"), onSettings);
    addMenuItem(F("CREDIT"), onCredit);
    setMenuCoords(18, 14, 91, 35, true);
    setMenuItemPos(0);
    playSoundClick();
    isMenuActive = true;
    isInvalid = true;
}

static void onContinue(void)
{
    playSoundClick();
    isIgnoreButton = true;
    isMenuActive = false;
    isInvalid = true;
}

static void onFortune(void)
{
    uint8_t idx = random(FORTUNE_IDX_MAX);
    pFortune = (const char *)pgm_read_ptr(
            (record.decodeMode == DECODE_MODE_JA) ? &fortuneTableJP[idx] : &fortuneTableEN[idx]);
#ifdef TEST_FORTUNE
    if (arduboy.buttonPressed(LEFT_BUTTON|RIGHT_BUTTON)) {
        pFortune = (record.decodeMode == DECODE_MODE_JA) ? testFortuneJP : testFortuneEN;
    } 
#endif
    encoder.setLetter(pgm_read_byte(pFortune));
    decoder.forceStable(true);
    onContinue();
}

static void onConfirmClear(void)
{
    setConfirmMenu(34, onClear, onContinue);
}

static void onClear(void)
{
    clearConsole();
    onContinue();
}

static void onList(void)
{
    playSoundClick();
    nextMode = MODE_LIST;
}

static void onSettings(void)
{
    playSoundClick();
    nextMode = MODE_SETTING;
}

static void onCredit(void)
{
    playSoundClick();
    nextMode = MODE_CREDIT;
}

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void drawConsoleLetters(void)
{
    static uint8_t lastConsoleX, lastConsoleY;
    if (isInvalid) {
        arduboy.clear();
        drawDottedHLine(0, 0, WIDTH);
        drawDottedHLine(64, 62, WIDTH - 64);
        for (int i = 0; i < CONSOLE_H; i++) {
            arduboy.printEx(0, i * 6 + 2, letters[i]);
        }
        if (isKeyboardActive) {
            arduboy.fillRect(39, 17, 49, 29, BLACK);
            drawDottedHLine(40, 18, 46);
            drawDottedHLine(40, 44, 46);
            drawDottedVLine(40, 19, 25);
            drawDottedVLine(86, 19, 25);
            arduboy.drawBitmap(44, 20, imgComputer, IMG_COMPUTER_W, IMG_COMPUTER_H);
        }
    } else {
        if (!isKeyboardActive) {
            char c = letters[lastConsoleY][lastConsoleX];
            if (isNonGraph(c)) c = ' ';
            arduboy.printEx(lastConsoleX * 6, lastConsoleY * 6 + 2, c);
        }
        arduboy.fillRect(0, 62, 64, 2, BLACK);
    }

    char c = decoder.getCandidate();
    if (isNonGraph(c)) c = ' ';
    if (isKeyboardActive) {
        if (decoder.getCurrentCode() == CODE_INITIAL) {
            c = lastLetter;
        } else {
            if (counter & 1) c = ' ';
        }
        arduboy.setTextSize(2);
        arduboy.printEx(68, 23, c);
        arduboy.setTextSize(1);
    } else {
        if (counter & 1) c = '_';
        arduboy.printEx(consoleX * 6, consoleY * 6 + 2, c);
        lastConsoleX = consoleX;
        lastConsoleY = consoleY;
    }

    uint8_t w = drawMorseCode(0, 62, decoder.getCurrentCode());
    drawDottedHLine(w, 62, 64 - w);
}

static void drawDottedHLine(uint8_t x, uint8_t y, uint8_t w)
{
    if (w == 0) return;
    x++; // assumed that x is even number
    w--;
    uint8_t yOdd = y & 7;
    uint8_t d = 1 << yOdd;
    uint8_t *p = arduboy.getBuffer() + x + (y / 8) * WIDTH;
    for (uint8_t i = 0; i < w; i += 2, p += 2) {
        *p |= d;
    }
}

static void drawDottedVLine(uint8_t x, uint8_t y, uint8_t h)
{
    for (uint8_t i = 0; i < h; i += 2, y += 2) arduboy.drawPixel(x, y);
}
