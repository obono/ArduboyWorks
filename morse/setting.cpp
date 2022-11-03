#include "common.h"

/*  Defines  */

#define SETTING_ITEMS   9

/*  Local Functions  */

static void changeSetting(int8_t pos, int8_t v, bool isDown);
static void changeSettingCommon(void);
static bool isIMEModeAvailable(void);
static void onExit(void);
static void onDecodeMode(void);
static void onTestSignal(void);
static void onKeyboard(void);
static void onIMEMode(void);

static void drawSettingParams(int8_t pos);

/*  Local Constants  */

PROGMEM static const char decodeModeLabels[][9] = { "ALPHABET", "JAPANESE" };
PROGMEM static const char ledLabels[][5] = { "OFF", "LOW", "HIGH" };
PROGMEM static const char ledColorLabels[][8] = {
    "RED",      "ORANGE",   "YELLOW",   "Y-GREEN",  "GREEN",    "EMERALD",
    "CYAN",     "AZURE",    "BLUE",     "VIOLET",   "MAGENTA",  "CRIMSON",  "WHITE"
};
PROGMEM static const char keyboardLabels[][6] = { "NONE", "US101", "JP106" };
PROGMEM static const char imeModeLabels[][6] = { "ROMAN", "KANA" };
PROGMEM static const char notAvailableLabel[] = "(N/A)";

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
    addMenuItem(F("KEYBOARD"), onKeyboard);
    addMenuItem(F("IME MODE"), onIMEMode);
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
        int8_t pos = getMenuItemPos();
        if (pos == 4 && !arduboy.isAudioEnabled() || pos == 6 && record.led == LED_OFF ||
                pos == 8 && !isIMEModeAvailable()) {
            pos = circulate(pos, padY, SETTING_ITEMS);
            setMenuItemPos(pos);
        }
        changeSetting(pos, padX, arduboy.buttonDown(LEFT_BUTTON | RIGHT_BUTTON));
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

static void changeSetting(int8_t pos, int8_t v, bool isDown)
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
            if (arduboy.isAudioEnabled() && v != 0) {
                if (!isDown) {
                    uint8_t freq = record.toneFreq;
                    v *= 3 + ((freq >= 27) + (freq >= 80) + (freq >= 133)) * 2;
                }
                record.toneFreq = circulate(record.toneFreq, v, TONE_FREQ_MAX);
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
            if (record.led != LED_OFF && v != 0) {
                record.ledColor = circulate(record.ledColor, v, LED_COLOR_MAX);
                changeSettingCommon();
            }
            break;
        case 7: // Keyboard
            if (isDown) {
                record.keyboard = circulate(record.keyboard, v, KEYBOARD_MAX);
                changeSettingCommon();
            }
            break;
        case 8: // IME Mode
            if (isDown) onIMEMode();
            break;
    }
}

static void changeSettingCommon(void)
{
    if (counter == 0) playSoundTick();
    isSettingChanged = true;
    isRecordDirty = true;
}

static bool isIMEModeAvailable(void)
{
    return (record.decodeMode == DECODE_MODE_JA && record.keyboard != KEYBOARD_NONE);
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

static void onKeyboard(void)
{
    record.keyboard = circulate(record.keyboard, 1, KEYBOARD_MAX);
    changeSettingCommon();
}

static void onIMEMode(void)
{
    if (isIMEModeAvailable()) {
        record.imeMode++;
        changeSettingCommon();
    }
}

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void drawSettingParams(int8_t pos)
{
    for (int8_t i = 0; i < SETTING_ITEMS; i++) {
        arduboy.setCursor(79 - (i == pos) * 4, 8 + i * 6);
        const char *p = NULL;
        switch (i) {
            default:
            case 0: // Exit
                break;
            case 1: // Decode Mode
                p = decodeModeLabels[record.decodeMode];
                break;
            case 2: // Speed
                arduboy.print(72 / (record.unitFrames + 2));
                p = (const char*)F(" WPM");
                break;
            case 3: // Sound
                p = (const char*)(arduboy.isAudioEnabled() ? F("ON") : F("OFF"));
                break;
            case 4: // Tone Freq
                if (arduboy.isAudioEnabled()) {
                    arduboy.print(record.toneFreq * 5 + 400);
                    p = (const char*)F(" HZ");
                } else {
                    p = notAvailableLabel;
                }
                break;
            case 5: // LED
                p = ledLabels[record.led];
                break;
            case 6: // LED color
                p = (record.led == LED_OFF) ? notAvailableLabel : ledColorLabels[record.ledColor];
                break;
            case 7: // Keyboard
                p = keyboardLabels[record.keyboard];
                break;
            case 8: // IME Mode
                p = (isIMEModeAvailable()) ? imeModeLabels[record.imeMode] : notAvailableLabel;
                break;
        }
        if (p != NULL) arduboy.print((const __FlashStringHelper *)p);
    }
}
