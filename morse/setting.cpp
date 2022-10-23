#include "common.h"

/*  Local Functions  */

static void changeSetting(uint8_t pos, int8_t v, bool isDown);
static void changeSettingCommon(void);
static void onExit(void);
static void onDecodeMode(void);
static void onTestSignal(void);

static void drawSettingParams(uint8_t pos);

/*  Local Constants  */

PROGMEM static const char decodeModeLabels[][9] = { "ALPHABET", "JAPANESE" };
PROGMEM static const char ledLabels[][5] = { "OFF", "LOW", "HIGH" };
PROGMEM static const char ledColorLabels[][8] = {
    "RED",      "ORANGE",   "YELLOW",   "Y-GREEN",  "GREEN",    "EMERALD",
    "CYAN",     "AZURE",    "BLUE",     "VIOLET",   "MAGENTA",  "CRIMSON",  "WHITE"
};

/*  Local Variables  */

static MODE_T nextMode;
static bool   isSettingChanged;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initSetting(void)
{
    clearMenuItems();
    addMenuItem(F("EXIT"), onExit);
    addMenuItem(F("MODE"), onDecodeMode);
    addMenuItem(F("SPEED"), onTestSignal);
    addMenuItem(F("SOUND"), onTestSignal);
    addMenuItem(F("TONE FREQ"), onTestSignal);
    addMenuItem(F("LED"), onTestSignal);
    addMenuItem(F("LED COLOR"), onTestSignal);
    setMenuCoords(7, 8, 64, 53, false);
    setMenuItemPos(0);
    counter = 0;
    isInvalid = true;
    isSettingChanged = true;
}

MODE_T updateSetting(void)
{
    nextMode = MODE_SETTING;
    handleDPad();
    if (counter > 0) padY = 0;
    if (arduboy.buttonDown(A_BUTTON)) {
        onExit();
    } else {
        handleMenu();
        changeSetting(getMenuItemPos(), padX, arduboy.buttonDown(LEFT_BUTTON | RIGHT_BUTTON));
        if (padY != 0) isSettingChanged = true;
    }
    if (counter > 0) {
        if (--counter == 0 || nextMode != MODE_SETTING) indicateSignalOff();
    }
    return nextMode;
}

void drawSetting(void)
{
    if (isInvalid) {
        arduboy.clear();
        arduboy.printEx(34, 0, F("[SETTINGS]"));
    } else if (isSettingChanged) {
        arduboy.fillRect(75, 8, 52, 56, BLACK);
    }
    drawMenuItems(isInvalid);
    if (isSettingChanged) {
        drawSettingParams(getMenuItemPos());
        isSettingChanged = false;
    }
    isInvalid = false;
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void changeSetting(uint8_t pos, int8_t v, bool isDown)
{
    switch (pos) {
        default:
        case 0: // Exit
            break;
        case 1: // Decode Mode
            if (isDown) onDecodeMode();
            break;
        case 2: // Speed
            if (v != 0) {
                record.unitFrames = circulate(record.unitFrames, -v, UNIT_FRAMES_MAX);
                if (record.unitFrames == 9 || record.unitFrames == 11) record.unitFrames -= v;
                changeSettingCommon();
            }
            break;
        case 3: // Sound
            if (isDown) {
                arduboy.toggleAudioEnabled();
                changeSettingCommon();
            }
            break;
        case 4: // Tone Freq
            if (v != 0) {
                record.toneFreq = circulate(record.toneFreq, v, TONE_FREQ_MAX);
                if (record.toneFreq > 40 && (record.toneFreq & 1)) record.toneFreq += v;
                changeSettingCommon();
            }
            break;
        case 5: // LED
            if (isDown) {
                record.led = circulate(record.led, v, LED_MAX);
                changeSettingCommon();
            }
            break;
        case 6: // LED Color
            if (v != 0) {
                record.ledColor = circulate(record.ledColor, v, LED_COLOR_MAX);
                changeSettingCommon();
            }
            break;
    }
}

static void changeSettingCommon(void)
{
    if (counter == 0) playSoundTick();
    isSettingChanged = true;
    isRecordDirty = true;
}

static void onExit(void)
{
    writeRecord();
    playSoundClick();
    nextMode = MODE_CONSOLE;
}

static void onDecodeMode(void)
{
    record.decodeMode++;
    changeSettingCommon();
}

static void onTestSignal(void)
{
    indicateSignalOn();
    counter = record.unitFrames + 3;
}

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void drawSettingParams(uint8_t pos)
{
    for (int i = 0; i < 7; i++) {
        arduboy.setCursor(79 - (i == pos) * 4, 8 + i * 6);
        switch (i) {
            default:
            case 0: // Exit
                break;
            case 1: // Decode Mode
                arduboy.print((const __FlashStringHelper *)decodeModeLabels[record.decodeMode]);
                break;
            case 2: // Speed
                arduboy.print(72 / (record.unitFrames + 2));
                arduboy.print(F(" WPM"));
                break;
            case 3: // Sound
                arduboy.print(arduboy.isAudioEnabled() ? F("ON") : F("OFF"));
                break;
            case 4: // Tone Freq
                arduboy.print(record.toneFreq * 10 + 400);
                arduboy.print(F(" HZ"));
                break;
            case 5: // LED
                arduboy.print((const __FlashStringHelper *)ledLabels[record.led]);
                break;
            case 6: // LED color
                arduboy.print((const __FlashStringHelper *)ledColorLabels[record.ledColor]);
                break;
        }
    }
}
