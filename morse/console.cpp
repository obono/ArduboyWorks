#include "common.h"
#include "Iambic.h"
#include "Decoder.h"
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
static void appendLetter(char c);
static void backSpace(void);
static void lineFeed(void);
static void setupMenu(void);
static void onContinue(void);
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

PROGMEM static const char welcomeEN[] = APP_TITLE "\0(A) MENU\0(B) SIGNAL ON";
PROGMEM static const char welcomeJP[] =
        "\x8C\x92\x6A\x8E \x7F\x80\x90\x77 \x70\x8F\x89\x86\x77\x85\0"
        "(A) \x87\x63\x86\x92\0(B) \x89\x8F\x80\x90\x77 \x7A\x8F";

/*  Local Variables  */

static Iambic       iambic;
static Decoder      decoder;
static FakeKeyboard fakeKeyboard;

static MODE_T   nextMode;
static uint16_t recentFrames;
static uint8_t  consoleX, consoleY, recentLetters;
static char     letters[CONSOLE_H][CONSOLE_W + 1];
static bool     isFirst = true, isKeyboardActive, isIgnoreButton, isLastSignalOn, isMenuActive;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initConsole(void)
{
    if (isFirst) {
        clearConsole();
        const char *p = (record.decodeMode == DECODE_MODE_JA) ? welcomeJP : welcomeEN;
        for (uint8_t i = 0; i < 3; i++) {
            size_t len = strnlen_P(p, CONSOLE_W);
            memcpy_P(letters[i], p, len);
            p += len + 1;
        }
        consoleY = 4;
        recentLetters = 0;
        recentFrames = 0;
        isFirst = false;
    }
    applySettings();
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
        if (isKeyboardActive) {
            fakeKeyboard.activated();
        }
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
        }
        record.playFrames++;
        isRecordDirty = true;
        counter++;
    }
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
    fakeKeyboard.reset(record.decodeMode, record.keyboard, record.imeMode);
}

static void clearConsole(void)
{
    memset(letters, 0, sizeof(letters));
    consoleX = 0;
    consoleY = 0;
}

static void handleSignal(void)
{
    bool isIambicShortOn = arduboy.buttonPressed(LEFT_BUTTON);
    bool isIambicLongOn = arduboy.buttonPressed(RIGHT_BUTTON);
    bool isSignalOn = iambic.isSignalOn(isIambicShortOn, isIambicLongOn) ||
            arduboy.buttonPressed(B_BUTTON);
    char decodedChar = decoder.appendSignal(isSignalOn);

    if (!isSignalOn && decoder.getCurrentCode() == CODE_INITIAL) {
        if (padY < 0) { // arduboy.buttonDown(UP_BUTTON)
            decoder.forceStable();
            decodedChar = '\b';
            playSoundTick();
        }
        if (arduboy.buttonDown(DOWN_BUTTON)) {
            decoder.forceStable();
            decodedChar = '\n';
            playSoundTick();
        }
        if (arduboy.buttonDown(A_BUTTON)) {
            decoder.forceStable();
            if (recentFrames >= RECENT_FRAMES_SOME) {
                writeRecord();
                recentLetters = 0;
                recentFrames = 0;
            }
            setupMenu();
        }
    }

    if (isSignalOn != isLastSignalOn) {
        (isSignalOn) ? indicateSignalOn() : indicateSignalOff();
        isLastSignalOn = isSignalOn;
    }

    if (decodedChar != '\0') {
        if (isKeyboardActive) {
            fakeKeyboard.sendLetter(decodedChar);
        } else {
            if (decodedChar == '\b') {
                backSpace();
            } else if (decodedChar == '\n') {
                lineFeed();
            } else {
                appendLetter(decodedChar);
            }
        }
        if (!isNonGraph(decodedChar)) {
            record.madeLetters++;
            recentLetters++;
            if (recentLetters >= RECENT_LETTERS_MAX || recentFrames > RECENT_FRAMES_MAX) {
                writeRecord();
                recentLetters = 0;
                recentFrames = 0;
            }
        }
    }
    if (recentLetters > 0) recentFrames++;
}

static void appendLetter(char c)
{
    letters[consoleY][consoleX] = c;
    if (++consoleX == CONSOLE_W) lineFeed();
}

static void backSpace(void)
{
    if (consoleX > 0) {
        consoleX--;
        letters[consoleY][consoleX] = '\0';
    } else if (consoleY > 0) {
        consoleY--;
        for (consoleX = CONSOLE_W - 1; consoleX > 0; consoleX--) {
            if (letters[consoleY][consoleX] != '\0') break;
        }
        letters[consoleY][consoleX] = '\0';
    }
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

static void setupMenu(void)
{
    clearMenuItems();
    addMenuItem(F("CONTINUE"), onContinue);
    addMenuItem(F("CLEAR"), onClear);
    addMenuItem(F("CODE LIST"), onList);
    addMenuItem(F("SETTINGS"), onSettings);
    addMenuItem(F("CREDIT"), onCredit);
    setMenuCoords(19, 17, 89, 29, true);
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

static void onClear(void)
{
    clearConsole();
    playSoundClick();
    isIgnoreButton = true;
    isMenuActive = false;
    isInvalid = true;
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
    static char lastLetter = ' ';
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
            lastLetter = c;
            if (counter & 1) c = ' ';
        }
        arduboy.setTextSize(2);
        arduboy.printEx(68, 23, c);
        arduboy.setTextSize(1);
    } else {
        lastLetter = ' ';
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
