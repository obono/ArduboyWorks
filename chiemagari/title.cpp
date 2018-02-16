#include "common.h"

/*  Defines  */

enum {
    STATE_MENU = 0,
    STATE_RECORD,
    STATE_CREDIT,
};

enum {
    MENU_START = 0,
    MENU_SOUND,
    MENU_RECORD,
    MENU_CREDIT,
};

enum {
    RECORD_NOT_READ = 0,
    RECORD_INITIAL,
    RECORD_STORED,
};

#define EEPROM_ADDR_BASE    864
#define EEPROM_SIGNATURE    0x044E424FUL // "OBN\x04"

/*  Local Functions  */

static bool handleButtons(void);
static void setSound(bool enabled);
static void playSound1(void);
static void playSound2(void);
static void readRecord(void);

static void drawTitleMenu(void);
static void drawTitleRecord(void);
static void drawTitleCredit(void);

/*  Local Variables  */

PROGMEM static const uint8_t imgTitle[512] = { // 128x32
    0x00, 0x00, 0x80, 0xE0, 0xFE, 0x7E, 0x3E, 0xFE, 0xEC, 0xE0, 0xE0, 0x20, 0x30, 0x38, 0xB8, 0xB0,
    0x20, 0xFC, 0xFC, 0xF8, 0xF0, 0xF0, 0x10, 0x10, 0x10, 0xF8, 0xF8, 0xF8, 0xF0, 0xF0, 0x00, 0x00,
    0x10, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0xFE, 0xFE, 0xFE, 0xFE, 0xB2, 0xFC, 0xDC, 0xD8, 0x90,
    0x90, 0x90, 0x90, 0xFE, 0xFE, 0xFE, 0xFE, 0x96, 0x90, 0xD0, 0xFC, 0xFC, 0xD8, 0x90, 0x00, 0x00,
    0x00, 0x00, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xFE, 0xFE, 0xFC, 0x04, 0x00,
    0x00, 0xFE, 0xFE, 0xFE, 0xFC, 0x04, 0x00, 0x00, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xFE, 0xFE, 0xFE, 0x84, 0xC0, 0xC0, 0x80, 0xF8, 0xF8, 0xF8,
    0xF0, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x18, 0x1E, 0x1E, 0x1C, 0x18, 0x10, 0x00,
    0x02, 0x02, 0x03, 0x03, 0x82, 0xC2, 0xFA, 0xFF, 0xFF, 0x3F, 0x1F, 0x32, 0xF2, 0xF3, 0xE3, 0xE3,
    0x83, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x20, 0x20, 0x20, 0xFF, 0xFF, 0xFF, 0x7F, 0x7F, 0x00, 0x00,
    0x04, 0x1C, 0x1C, 0x5C, 0x4C, 0x4C, 0x4C, 0x7F, 0x7F, 0x5F, 0x5F, 0x46, 0x42, 0x42, 0x46, 0x46,
    0x44, 0x44, 0x44, 0x5F, 0x5F, 0x5F, 0xFF, 0xF4, 0xF4, 0xE4, 0xC6, 0x87, 0x07, 0x06, 0x04, 0x00,
    0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0x02, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0x02,
    0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0x02, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0x04, 0x00, 0x00,
    0x02, 0x02, 0x02, 0x82, 0xFA, 0xFF, 0xFF, 0xFF, 0xFF, 0xC3, 0xC3, 0x83, 0x03, 0xFF, 0xFF, 0xFF,
    0xFF, 0x18, 0xF8, 0xE8, 0x08, 0x08, 0x08, 0xE8, 0xFC, 0xFC, 0xFC, 0x78, 0x30, 0x00, 0x00, 0x00,
    0x00, 0x04, 0x06, 0x07, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0x42, 0x42, 0x42, 0x43, 0x43, 0x43,
    0x42, 0x42, 0x42, 0x42, 0x42, 0xFF, 0xFF, 0xFF, 0xFE, 0xFE, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x10, 0xD2, 0xD2, 0x12, 0xD2, 0xD2, 0xD2, 0xD2, 0xD2, 0x52, 0xD2, 0xD2, 0xD2,
    0xD2, 0x92, 0x12, 0x12, 0x12, 0x92, 0xFF, 0xFF, 0xFF, 0xBF, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0x02, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0x02,
    0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0x02, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,
    0x40, 0x70, 0x3C, 0x1F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, 0x07, 0x07, 0xE7, 0xFF, 0xFF, 0x7F,
    0x1F, 0x00, 0x03, 0x9F, 0xFF, 0xFC, 0xFE, 0xFF, 0xFF, 0x8F, 0x83, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x1F, 0x3F, 0x3F, 0x1F, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x08, 0x1C, 0x1E, 0x1F, 0x1F, 0x0F, 0x00, 0x1F, 0x3F, 0x3F, 0x3F, 0x3F, 0x38, 0x38, 0x39, 0x3B,
    0x3B, 0x3B, 0x38, 0x3C, 0x3F, 0x3F, 0x3F, 0x18, 0x01, 0x0F, 0x1F, 0x1F, 0x1E, 0x1E, 0x00, 0x00,
    0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x08, 0x08, 0x08, 0x0F, 0x0F, 0x0F, 0x0F, 0x08, 0x08,
    0x08, 0x0F, 0x0F, 0x0F, 0x0F, 0x08, 0x08, 0x08, 0x3F, 0x3F, 0x3F, 0x1F, 0x1F, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x20, 0x38, 0x1E, 0x2F, 0x27, 0x31, 0x18,
    0x1C, 0x0E, 0x0F, 0x07, 0x03, 0x01, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x1F, 0x1E, 0x06, 0x06, 0x00,
};

PROGMEM static const char menuText[] = "START GAME\0SOUND \0RECORD\0CREDIT";
PROGMEM static const char creditText[] = "- " APP_TITLE " -\0\0" APP_RELEASED \
        "\0PROGREMMED BY OBONO\0\0THIS PROGRAM IS\0" "RELEASED UNDER\0" "THE MIT LICENSE.";

static uint8_t  state;
static bool     toDraw;
static bool     sound;
static int8_t   menuPos;
static uint16_t lastScore = 0;
static uint8_t  recordState = RECORD_NOT_READ;
static uint16_t hiScore[10];
static uint16_t playCount;
static uint32_t playFrames;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initTitle(void)
{
    state = STATE_MENU;
    menuPos = MENU_START;
    toDraw = true;
    setSound(arduboy.audio.enabled());
    if (recordState == RECORD_NOT_READ) {
        readRecord();
    }
}

bool updateTitle(void)
{
    bool ret = false;
    if (state == STATE_MENU) {
        ret = handleButtons();
    } else {
        if (arduboy.buttonDown(A_BUTTON | B_BUTTON)) {
            state = STATE_MENU;
            playSound2();
            toDraw = true;
        }
    }

#ifdef DEBUG
    if (dbgRecvChar == 'r') {
        arduboy.eepSeek(EEPROM_ADDR_BASE);
        for (int i = 0; i < 8; i++) {
            arduboy.eepWrite32(0);
        }
        recordState = RECORD_INITIAL;
        dprintln("Clean EEPROM");
    }
#endif

    randomSeed(rand() ^ micros()); // Shuffle random
    return ret;
}

void drawTitle(void)
{
    if (toDraw) {
        arduboy.clear();
        switch (state) {
        case STATE_MENU:
            drawTitleMenu();
            break;
        case STATE_RECORD:
            drawTitleRecord();
            break;
        case STATE_CREDIT:
            drawTitleCredit();
            break;
        }
        toDraw = false;
    }
}

uint8_t setLastScore(int score, uint32_t frames)
{
    lastScore = score;

    /*  Updarte best 10  */
    int r = 10;
    uint16_t h;
    while (r > 0 && (h = hiScore[r - 1]) < score) {
        if (--r < 9) hiScore[r + 1] = h;
    }
    if (r < 10) hiScore[r] = score;
    playCount++;
    playFrames += frames;

    /*  Store record to EEPROM  */
    if (recordState == RECORD_INITIAL) {
        arduboy.eepSeek(EEPROM_ADDR_BASE);
        arduboy.eepWrite32(EEPROM_SIGNATURE);
    }
    arduboy.eepSeek(EEPROM_ADDR_BASE + 4 + r * 2);
    for (int i = r; i < 10; i++) {
        arduboy.eepWrite16(hiScore[i]);
    }
    arduboy.eepWrite16(playCount);
    arduboy.eepWrite32(playFrames);

    uint16_t checkSum = (EEPROM_SIGNATURE & 0xFFFF) + (EEPROM_SIGNATURE >> 16) * 2;
    for (int i = 0; i < 10; i++) {
        checkSum += hiScore[i] * (i + 3);
    }
    checkSum += playCount * 13;
    checkSum += (playFrames & 0xFFFF) * 14 + (playFrames >> 16) * 15;
    arduboy.eepWrite16(checkSum);

    recordState = RECORD_STORED;
    return r;
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static bool handleButtons()
{
    bool ret = false;
    if (arduboy.buttonDown(UP_BUTTON)) {
        if (menuPos-- == MENU_START) menuPos = MENU_CREDIT;
        playSound1();
        toDraw = true;
    }
    if (arduboy.buttonDown(DOWN_BUTTON)) {
        if (menuPos++ == MENU_CREDIT) menuPos = MENU_START;
        playSound1();
        toDraw = true;
    }
    if (arduboy.buttonDown(A_BUTTON | B_BUTTON)) {
        switch (menuPos) {
        case MENU_START:
            ret = true;
            arduboy.audio.saveOnOff();
            break;
        case MENU_SOUND:
            setSound(!sound);
            playSound2();
            break;
        case MENU_RECORD:
            state = STATE_RECORD;
            playSound2();
            break;
        case MENU_CREDIT:
            state = STATE_CREDIT;
            playSound2();
            break;
        }
        toDraw = true;
    }
    return ret;
}

static void setSound(bool enabled)
{
    if (enabled) {
        arduboy.audio.on();
    } else {
        arduboy.audio.off();
    }
    sound = enabled;
}

static void playSound1(void)
{
    arduboy.tone2(440, 10);
}

static void playSound2(void)
{
    arduboy.tone2(587, 20);
}

static void readRecord(void)
{
    bool isRegular = false;
    arduboy.eepSeek(EEPROM_ADDR_BASE);
    if (arduboy.eepRead32() == EEPROM_SIGNATURE) {
        uint16_t checkSum = (EEPROM_SIGNATURE & 0xFFFF) + (EEPROM_SIGNATURE >> 16) * 2;
        for (int i = 0; i < 13; i++) {
            checkSum += arduboy.eepRead16() * (i + 3);
        }
        isRegular = (arduboy.eepRead16() == checkSum);
    }

    if (isRegular) {
        /*  Read record from EEPROM  */
        arduboy.eepSeek(EEPROM_ADDR_BASE + 4);
        for (int i = 0; i < 10; i++) {
            hiScore[i] = arduboy.eepRead16();
        }
        playCount = arduboy.eepRead16();
        playFrames = arduboy.eepRead32();
        recordState = RECORD_STORED;
    } else {
        /*  Initialize record  */
        for (int i = 0; i < 10; i++) {
            hiScore[i] = 0;
        }
        playCount = 0;
        playFrames = 0;
        recordState = RECORD_INITIAL;
    }
}

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void drawTitleMenu(void)
{
    /*  Title logo  */
    arduboy.drawBitmap(0, 0, imgTitle, 128, 32, WHITE);

    /*  Menu items  */
    char buf[12];
    const char *p = menuText;
    for (int i = 0; i < 4; i++) {
        strcpy_P(buf, p);
        p += arduboy.printEx(68 - (i == menuPos) * 4, i * 6 + 40, buf) + 1;
        if (i == MENU_SOUND) {
            arduboy.print((sound) ? F("ON") : F("OFF"));
        }
    }
    arduboy.fillRect2(56, menuPos * 6 + 40, 5, 5, WHITE);
}

static void drawTitleRecord(void)
{
    arduboy.printEx(22, 4, F("BEST 10 SCORES"));
    arduboy.drawFastHLine2(0, 12, 128, WHITE);
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 5; j++) {
            int r = i * 5 + j;
            arduboy.printEx(i * 60 + 4 - (r == 9) * 6, j * 6 + 14, F("["));
            arduboy.print(r + 1);
            arduboy.print(F("] "));
            arduboy.print(hiScore[r]);
        }
    }
    arduboy.drawFastHLine2(0, 44, 128, WHITE);
    arduboy.printEx(16, 48, F("PLAY COUNT "));
    arduboy.print(playCount);
    arduboy.printEx(16, 54, F("PLAY TIME  "));
    arduboy.print(playFrames / 3600); // minutes
    char buf[6];
    sprintf(buf, "'%02d\"", playFrames / 60 % 60); // seconds
    arduboy.print(buf);
}

static void drawTitleCredit(void)
{
    char buf[20];
    const char *p = creditText;
    for (int i = 0; i < 8; i++) {
        strcpy_P(buf, p);
        uint8_t len = strnlen(buf, sizeof(buf));
        p += arduboy.printEx(64 - len * 3, i * 6 + 8, buf) + 1;
    }
}
