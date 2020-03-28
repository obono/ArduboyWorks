#include "common.h"

/*  Defines  */

enum STATE_T : uint8_t {
    STATE_INIT = 0,
    STATE_TOP,
    STATE_SETTINGS,
    STATE_CREDIT,
    STATE_CODE_ENTRY,
    STATE_STARTED,
};

#define IMG_TITLE_W     128
#define IMG_TITLE_H     21
#define IMG_SUBTITLE_W  34
#define IMG_SUBTITLE_H  32
#define IMG_JETL_W      7
#define IMG_JETL_H      8
#define IMG_JETR_W      9
#define IMG_JETR_H      5
#define IMG_SPINNERS_W  6
#define IMG_SPINNERS_H  3

/*  Local Functions  */

static void handleSettings(void);
static void handleCodeEntry(void);
static uint32_t randomGameSeed(void);
static void handleAnyButton(void);

static void onTop(void);
static void onStart(void);
static void onNewBattle(void);
static void onSettings(void);
static void onCredit(void);
static void onMenuDifficulty(void);
static void onMenuToggleItem(void);

static void drawTop(void);
static void drawSettings(void);
static void drawCredit(void);
static void drawCodeEntry(void);
static void drawTitleLogo(void);
static void drawText(const char *p, int lines);

/*  Local Functions (macros)  */

#define circulate(n, v, m)  (((n) + (v) + (m)) % (m))
#define callHandlerFunc(n)  ((void (*)(void)) pgm_read_ptr(handlerFuncTable + n))()
#define callDrawerFunc(n)   ((void (*)(void)) pgm_read_ptr(drawerFuncTable + n))()

/*  Local Variables  */

PROGMEM static const uint8_t imgTitle[384] = { // 128x21
    0x00, 0x00, 0xF8, 0xFF, 0xFF, 0xFF, 0x0F, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFE, 0xFC, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x0F, 0x0F, 0xFF, 0xFF, 0xFE, 0xFE, 0xFC, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0xFF, 0xFF, 0xFE, 0xFE,
    0xF8, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0F, 0x00, 0x0F, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0x0F, 0x0F, 0x0F, 0xE0, 0xF0, 0xF8, 0xF8, 0xF8, 0x38, 0x38, 0xF8, 0xF0, 0xF0, 0xE0,
    0x80, 0xFE, 0xFF, 0xFF, 0xFF, 0xE3, 0xE0, 0xE3, 0xFF, 0xFF, 0xFF, 0xFC, 0x80, 0x00, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFB, 0xF1, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0xFF, 0xFF, 0xFD, 0xFD,
    0xF8, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0x00, 0x00, 0x00, 0x87, 0x8F, 0x9F, 0x9F, 0x3F, 0x7E, 0xFC, 0xFD, 0xF9, 0xF1, 0xE1,
    0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x01, 0x01, 0x01, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00, 0x1F, 0x1F,
    0x1F, 0x1F, 0x1F, 0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
    0x1E, 0x1E, 0x1F, 0x1F, 0x1F, 0x0F, 0x07, 0x00, 0x03, 0x07, 0x0F, 0x1F, 0x1F, 0x1E, 0x1E, 0x1F,
    0x1F, 0x0F, 0x0F, 0x03, 0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1E, 0x1E, 0x1F, 0x1F, 0x1F, 0x0F,
    0x07, 0x00, 0x03, 0x07, 0x0F, 0x1F, 0x1F, 0x1E, 0x1E, 0x1F, 0x1F, 0x0F, 0x0F, 0x03, 0x00, 0x1F,
    0x1F, 0x1F, 0x1F, 0x1F, 0x1E, 0x1E, 0x1E, 0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1E, 0x1E, 0x1E,
    0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1E, 0x1E, 0x1E, 0x1E, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F,
    0x1F, 0x1F, 0x00, 0x00, 0x00, 0x07, 0x0F, 0x0F, 0x1F, 0x1C, 0x1C, 0x1F, 0x1F, 0x0F, 0x0F, 0x07
};

PROGMEM static const uint8_t imgSubTitle[136] = { // 34x32
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x7E, 0x1F, 0xBC, 0x78, 0xE0, 0xC0,
    0xC0, 0x80, 0x80, 0x80, 0x80, 0x40, 0x40, 0x20, 0xB0, 0x58, 0xAC, 0x5C, 0xB8, 0x78, 0xF0, 0xE0,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x80, 0x40, 0x20, 0xA0, 0x10, 0x88, 0x47, 0xA0, 0x50, 0xAA, 0x55,
    0xAA, 0xF5, 0x0B, 0xE7, 0x06, 0x0C, 0x0A, 0x1C, 0x7A, 0xF5, 0xFA, 0xFD, 0x7E, 0x03, 0x01, 0x01,
    0x02, 0x05, 0x0B, 0x07, 0x0E, 0x31, 0x49, 0x94, 0xBA, 0x7D, 0xFE, 0xED, 0xC6, 0x07, 0x07, 0x09,
    0x12, 0x25, 0x4A, 0x55, 0x8A, 0x95, 0x2B, 0x56, 0x30, 0x50, 0xB8, 0x7F, 0xFF, 0xFF, 0xFC, 0xE0,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x05, 0x07, 0x03,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x04, 0x08, 0x15, 0x2A, 0x55,
    0x6F, 0xDF, 0xFF, 0x78, 0x00, 0x00, 0x00, 0x00
};

PROGMEM static const uint8_t imgJetL[][7] = { // 7x8 x3
    { 0x70, 0xFC, 0x0E, 0x06, 0x03, 0x03, 0x03 },
    { 0x70, 0xFC, 0x7E, 0x3E, 0x0F, 0x0F, 0x0F },
    { 0x70, 0xFC, 0x7E, 0x3E, 0x3F, 0x1F, 0x0F }
};

PROGMEM static const uint8_t imgJetR[][9] = { // 9x5 x3
    { 0x0C, 0x0E, 0x06, 0x03, 0x03, 0x03, 0x06, 0x0E, 0x04 },
    { 0x0C, 0x0E, 0x1E, 0x0F, 0x0F, 0x0F, 0x0E, 0x0E, 0x04 },
    { 0x0C, 0x0E, 0x1E, 0x1F, 0x1F, 0x1F, 0x0E, 0x0E, 0x04 }
};

PROGMEM static const uint8_t imgSpinners[][6] = { // 3x6 x2
    { 0x08, 0x0C, 0x0E, 0x0E, 0x0C, 0x08 }, { 0x01, 0x03, 0x07, 0x07, 0x03, 0x01 }
};

PROGMEM static const char creditText[] = \
        "- " APP_TITLE " -\0\0\0" APP_RELEASED "\0PROGREMMED BY OBONO\0\0" \
        "THIS PROGRAM IS\0RELEASED UNDER\0THE MIT LICENSE.\0\e";

PROGMEM static void(*const handlerFuncTable[])(void) = {
    NULL, handleMenu, handleSettings, handleAnyButton, handleCodeEntry
};

PROGMEM static void(*const drawerFuncTable[])(void) = {
    NULL, drawTop, drawSettings, drawCredit, drawCodeEntry
};

static STATE_T  state = STATE_INIT;
static int32_t  gameSeedTmp;
static uint8_t  gameSeedX;
static bool     isSettingChenged;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initTitle(void)
{
    if (state == STATE_INIT) {
        arduboy.initAudio(1);
        readRecord();
    }
    onTop();
}

MODE_T updateTitle(void)
{
    handleDPad();
    counter++;
    callHandlerFunc(state);
    randomSeed(rand() ^ micros()); // Shuffle random
    return (state == STATE_STARTED) ? MODE_GAME : MODE_TITLE;
}

void drawTitle(void)
{
    if (state == STATE_STARTED) return;
    if (isInvalid) arduboy.clear();
    callDrawerFunc(state);
    isInvalid = false;
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void handleSettings(void)
{
    switch (getMenuItemPos()) {
    case 0:
        if (padX != 0) {
            playSoundTick();
            record.gameRank = circulate(record.gameRank - 1, padX, GAME_RANK_MAX) + 1;
            isSettingChenged = true;
        }
        break;
    case 1:
    case 2:
    case 3:
        if (arduboy.buttonDown(LEFT_BUTTON | RIGHT_BUTTON)) onMenuToggleItem();
        break;
    default:
        break;
    }
    handleMenu();
    if (arduboy.buttonDown(UP_BUTTON | DOWN_BUTTON)) isSettingChenged = true;
}

static void handleCodeEntry(void)
{
    int8_t vx = arduboy.buttonDown(RIGHT_BUTTON) - arduboy.buttonDown(LEFT_BUTTON);
    if (vx != 0 && gameSeedX + vx >= 0 && gameSeedX + vx < GAME_SEED_TOKEN_MAX) {
        playSoundTick();
        gameSeedX += vx;
        isSettingChenged = true;
    }
    if (padY != 0) {
        playSoundTick();
        int32_t k = 1;
        for (int i = 0; i < gameSeedX; i++, k *= GAME_SEED_TOKEN_VAL) { ; }
        int8_t token = (gameSeedTmp / k) % GAME_SEED_TOKEN_VAL;
        if (token + padY < 0 || token + padY >= GAME_SEED_TOKEN_VAL) {
            k = (1 - GAME_SEED_TOKEN_VAL) * k;
        }
        gameSeedTmp += padY * k;
        isSettingChenged = true;
    }
    if (arduboy.buttonDown(A_BUTTON)) {
        playSoundClick();
        state = STATE_TOP;
        isInvalid = true;
    } else if (arduboy.buttonDown(B_BUTTON)) {
        if (record.gameSeed != gameSeedTmp) record.isCleared = false;
        record.gameSeed = gameSeedTmp;
        onStart();
    }
}

static uint32_t randomGameSeed(void)
{
    uint32_t ret = 0;
    for (int i = 0; i < GAME_SEED_TOKEN_MAX; i++) {
        ret = ret * GAME_SEED_TOKEN_VAL + random(GAME_SEED_TOKEN_ALP);
    }
    return ret;
}

static void handleAnyButton(void)
{
    if (arduboy.buttonDown(A_BUTTON | B_BUTTON)) {
        playSoundClick();
        state = STATE_TOP;
        isInvalid = true;
    }
}

/*---------------------------------------------------------------------------*/
/*                               Menu Handlers                               */
/*---------------------------------------------------------------------------*/

static void onTop(void)
{
    if (state == STATE_SETTINGS) playSoundClick();
    clearMenuItems();
    if (record.playCount > 0) {
        addMenuItem(F("RETRY"), onStart);
        addMenuItem(NULL, NULL);
    }
    addMenuItem(F("NEW BATTLE"), onNewBattle);
    addMenuItem(F("SETTINGS"), onSettings);
    addMenuItem(F("CREDIT"), onCredit);
    setMenuItemPos((state == STATE_SETTINGS) ? getMenuItemCount() - 2 : record.isCleared * 2);
    setMenuCoords(44, 38, 84, 26, false, true);
    state = STATE_TOP;
    isInvalid = true;
    dprintln(F("Title screen"));
}

static void onStart(void)
{
    state = STATE_STARTED;
    dprintln(F("Game start"));
}

static void onNewBattle(void)
{
    if (record.isCodeManual) {
        playSoundClick();
        gameSeedX = 0;
        gameSeedTmp = (record.gameSeed == GAME_SEED_MAX) ? randomGameSeed() : record.gameSeed;
        state = STATE_CODE_ENTRY;
        isInvalid = true;
        isSettingChenged = true;
        dprintln(F("Code entry"));
    } else {
        record.isCleared = false;
        record.gameSeed = randomGameSeed();
        onStart();
    }
}

static void onSettings(void)
{
    playSoundClick();
    clearMenuItems();
    addMenuItem(F("DIFFICULTY"), onMenuDifficulty);
    addMenuItem(F("PATTERN"), onMenuToggleItem);
    addMenuItem(F("LED BLINK"), onMenuToggleItem);
    addMenuItem(F("DETECT EDGE"), onMenuToggleItem);
    addMenuItem(F("EXIT"), onTop);
    setMenuItemPos(0);
    setMenuCoords(2, 12, 124, 30, false, false);
    state = STATE_SETTINGS;
    isInvalid = true;
    isSettingChenged = true;
    dprintln(F("Settings"));
}

static void onCredit(void)
{
    playSoundClick();
    state = STATE_CREDIT;
    isInvalid = true;
    dprintln(F("Show credit"));
}

static void onMenuDifficulty(void)
{
    playSoundClick();
    record.gameRank = record.gameRank % GAME_RANK_MAX + 1;
    isSettingChenged = true;
}

static void onMenuToggleItem(void)
{
    playSoundClick();
    bitToggle(*((uint8_t *)&record + 3), getMenuItemPos() + 3); // Trick!!
    isSettingChenged = true;
}

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void drawTop(void)
{
    if (isInvalid) {
        drawTitleLogo();
        arduboy.drawBitmap(6, 32, imgSubTitle, IMG_SUBTITLE_W, IMG_SUBTITLE_H);
    }
    int a = counter & 3;
    uint8_t color = (a == 0);
    a = (a == 0) ? 2 : a - 1;
    arduboy.drawBitmap(8, 39, imgJetL[a], IMG_JETL_W, IMG_JETL_H, color);
    arduboy.drawBitmap(21, 34, imgJetR[a], IMG_JETR_W, IMG_JETR_H, color);
    drawMenuItems(isInvalid);
    if (record.playCount > 0 && getMenuItemPos() == 0) {
        arduboy.setTextColor(bitRead(counter, 0));
        printGameSeed(88, 39, record.gameSeed);
        arduboy.setTextColor(WHITE);
    }
}

static void drawSettings(void)
{
    if (isInvalid) {
        arduboy.printEx(34, 4, F("[SETTINGS]"));
        arduboy.drawFastHLine(0, 44, 128, WHITE);
        arduboy.printEx(10, 48, F("PLAY COUNT "));
        arduboy.print(record.playCount);
        arduboy.printEx(10, 54, F("PLAY TIME"));
        drawTime(76, 54, record.playFrames);
    }
    drawMenuItems(isInvalid);
    if (isSettingChenged) {
        for (int i = 0; i < 4; i++) {
            int16_t dx = 86 - (getMenuItemPos() == i) * 4, dy = i * 6 + 12;
            bool isConfigSet = bitRead(*((uint8_t *)&record + 3), i + 3);
            switch (i) {
            case 0:
                arduboy.printEx(dx, dy, record.gameRank);
                arduboy.print(' ');
                break;
            case 1:
                arduboy.printEx(dx, dy, (isConfigSet) ? F("SPECIFY") : F("RANDOM "));
                break;
            case 2:
            case 3:
                arduboy.printEx(dx, dy, (isConfigSet) ? F("ON ") : F("OFF"));
                break;
            default:
                break;
            }
        }
        isSettingChenged = false;
    }
}

static void drawCredit(void)
{
    if (isInvalid) drawText(creditText, 11);
}

static void drawCodeEntry(void)
{
    if (isInvalid) {
        drawTitleLogo();
        arduboy.printEx(28, 57, F("CANCEL   START"));
        drawButtonIcon(16, 56, false);
        drawButtonIcon(70, 56, true);
    }
    if (isSettingChenged) {
        if (!isInvalid) arduboy.fillRect(37, 33, 54, 18, BLACK);
        arduboy.setTextSize(2);
        printGameSeed(35, 37, gameSeedTmp);
        arduboy.setTextSize(1);
    }
    if (isSettingChenged || (counter & 3) == 0) {
        uint8_t color = bitRead(counter, 2);
        int16_t dx = 37 + gameSeedX * 12;
        arduboy.drawBitmap(dx, 32, imgSpinners[0], IMG_SPINNERS_W, IMG_SPINNERS_H, color);
        arduboy.drawBitmap(dx, 48, imgSpinners[1], IMG_SPINNERS_W, IMG_SPINNERS_H, color);
    }
    isSettingChenged = false;
}

static void drawTitleLogo(void)
{
    arduboy.drawBitmap((WIDTH - IMG_TITLE_W) / 2, 8, imgTitle, IMG_TITLE_W, IMG_TITLE_H);
#ifdef DEBUG
    arduboy.printEx(98, 30, F("DEBUG"));
#endif
}

static void drawText(const char *p, int16_t y)
{
    while (pgm_read_byte(p) != '\e') {
        uint8_t len = strnlen_P(p, 21);
        arduboy.printEx(64 - len * 3, y, (const __FlashStringHelper *) p);
        p += len + 1;
        y += (len == 0) ? 2 : 6;
    }
}
