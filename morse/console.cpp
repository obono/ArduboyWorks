#include "common.h"
#include "Iambic.h"
#include "Decoder.h"

/*  Defines  */

#define CONSOLE_W       21
#define CONSOLE_H       10

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
static void drawDottedLine(uint8_t x, uint8_t y, uint8_t w);

/*  Local Constants  */

PROGMEM static const char welcomeEN[] = APP_TITLE "\0(A) MENU\0(B) SIGNAL ON";
PROGMEM static const char welcomeJP[] =
        "\x8C\x92\x6A\x8E \x7F\x80\x90\x77 \x70\x8F\x89\x86\x77\x85\0"
        "(A) \x87\x63\x86\x92\0(B) \x89\x8F\x80\x90\x77 \x7A\x8F";

/*  Local Variables  */

static Iambic   iambic;
static Decoder  decoder;
static MODE_T   nextMode;
static uint8_t  consoleX, consoleY;
static char     letters[CONSOLE_H][CONSOLE_W + 1];
static bool     isFirst = true, isIgnoreButton, isMenuActive, isLastSignalOn;

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
        isFirst = false;
    }
    applySettings();
    isIgnoreButton = true;
    isLastSignalOn = false;
    isMenuActive = false;
    isInvalid = true;
}

MODE_T updateConsole(void)
{
    nextMode = MODE_CONSOLE;
    handleDPad();
    if (isIgnoreButton) {
        isIgnoreButton = (arduboy.buttonsState() > 0);
    } else if (isMenuActive) {
        if (arduboy.buttonDown(A_BUTTON)) {
            onContinue();
        } else {
            handleMenu();
        }
    } else {
        handleSignal();
    }
    counter++;
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
            setupMenu();
        }
    }

    if (isSignalOn != isLastSignalOn) {
        (isSignalOn) ? indicateSignalOn() : indicateSignalOff();
        isLastSignalOn = isSignalOn;
    }

    if (decodedChar != '\0') {
        if (decodedChar == '\b') {
            backSpace();
        } else if (decodedChar == '\n') {
            lineFeed();
        } else {
            appendLetter(decodedChar);
        }
    }
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
    static uint8_t lastConsoleX, lastConsoleY;
    if (isInvalid) {
        arduboy.clear();
        drawDottedLine(0, 0, WIDTH);
        drawDottedLine(64, 62, WIDTH - 64);
        for (int i = 0; i < CONSOLE_H; i++) {
            arduboy.printEx(0, i * 6 + 2, letters[i]);
        }
    } else {
        char c = letters[lastConsoleY][lastConsoleX];
        if (c >= '\0' && c < ' ') c = ' ';
        arduboy.printEx(lastConsoleX * 6, lastConsoleY * 6 + 2, c);
        arduboy.fillRect(0, 62, 64, 2, BLACK);
    }

    char c = (counter & 1) ? decoder.getCandidate() : '_';
    if (c >= '\0' && c < ' ') c = ' ';
    arduboy.printEx(consoleX * 6, consoleY * 6 + 2, c);
    lastConsoleX = consoleX;
    lastConsoleY = consoleY;

    uint8_t w = drawMorseCode(0, 62, decoder.getCurrentCode());
    drawDottedLine(w, 62, 64 - w);
}

static void drawDottedLine(uint8_t x, uint8_t y, uint8_t w)
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
