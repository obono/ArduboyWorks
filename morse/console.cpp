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
static void onSettings(void);
static void onCredit(void);

static void drawConsoleLetters(void);

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
        strcpy_P(letters[0], (const char *)F(APP_TITLE));
        strcpy_P(letters[1], (const char *)F("(A):MENU"));
        strcpy_P(letters[2], (const char *)F("(B):SIGNAL ON"));
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
    if (isIgnoreButton) {
        isIgnoreButton = (arduboy.buttonsState() > 0);
    } else if (isMenuActive) {
        if (arduboy.buttonDown(A_BUTTON)) {
            onContinue();
        } else {
            handleDPad();
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
        if (arduboy.buttonDown(UP_BUTTON)) {
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
        isInvalid = true;
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
    } else {
        consoleY++;
    }
}

static void setupMenu(void)
{
    clearMenuItems();
    addMenuItem(F("CONTINUE"), onContinue);
    addMenuItem(F("CLEAR"), onClear);
    addMenuItem(F("SETTINGS"), onSettings);
    addMenuItem(F("CREDIT"), onCredit);
    setMenuCoords(19, 20, 89, 23, true);
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
    if (isInvalid) {
        arduboy.clear();
        for (int i = 0; i < CONSOLE_H; i++) {
            arduboy.printEx(0, i * 6, letters[i]);
        }
        isInvalid = false;
    } else {
        arduboy.fillRect(0, 61, 62, 2, BLACK);
    }

    char c = (counter & 1) ? decoder.getCandidate() : '_';
    if (c >= '\0' && c < ' ') c = ' ';
    arduboy.printEx(consoleX * 6, consoleY * 6, c);
    drawMorseCode(0, 61, decoder.getCurrentCode());
}
