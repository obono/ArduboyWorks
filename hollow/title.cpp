#include "common.h"

/*  Defines  */

#define STATE_MENU      0
#define STATE_RECORD    1
#define STATE_CREDIT    2

#define RECORD_NOT_READ 0
#define RECORD_INITIAL  1
#define RECORD_STORED   2

#define EEPROM_ADDR_BASE    768
#define EEPROM_SIGNATURE    0x014E424FUL // "OBN\x01"

/*  Local Functions  */

static void readRecord(void);
static void setSound(bool enabled);
static void playSound1(void);
static void playSound2(void);

static void     eepSeek(int addr);
static uint8_t  eepRead8(void);
static uint16_t eepRead16(void);
static uint32_t eepRead32(void);
static void     eepReadBlock(void *p, size_t n);
static void     eepWrite8(uint8_t val);
static void     eepWrite16(uint16_t val);
static void     eepWrite32(uint32_t val);
static void     eepWriteBlock(const void *p, size_t n);

/*  Local Variables  */

PROGMEM static const uint8_t imgTitle1[] = { // "Hollow" 84x20
    0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xFE, 0xFE, 0xFE, 0xFE,
    0xFE, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0x80, 0x80, 0x00,
    0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
    0x00, 0x00, 0x80, 0x80, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0x80, 0x80, 0x00, 0x00, 0xC0, 0xC0,
    0xC0, 0xC0, 0xC0, 0xC0, 0x00, 0x00, 0x00, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0x80, 0x00, 0x00, 0xC0,
    0xC0, 0xC0, 0xC0, 0xC0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x7C, 0xFF, 0xFF, 0xFF, 0xFF, 0xC7, 0x83, 0x83, 0xC7,
    0xFF, 0xFF, 0xFF, 0xFF, 0x7C, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0x00, 0x00, 0x7C, 0xFF, 0xFF, 0xFF, 0xFF, 0xC7, 0x83, 0x83, 0xC7, 0xFF, 0xFF, 0xFF,
    0xFF, 0x7C, 0x00, 0x0F, 0x7F, 0xFF, 0xFF, 0xFF, 0xF0, 0xF0, 0xFF, 0xFF, 0x1F, 0x1F, 0xFF, 0xFF,
    0xFF, 0xF0, 0xF0, 0xFF, 0xFF, 0x7F, 0x07, 0x00, 0x07, 0x07, 0x07, 0x07, 0x07, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x07, 0x07, 0x07, 0x07, 0x07, 0x00, 0x00, 0x00, 0x01, 0x03, 0x03, 0x07,
    0x07, 0x07, 0x07, 0x07, 0x07, 0x03, 0x03, 0x01, 0x00, 0x00, 0x07, 0x07, 0x07, 0x07, 0x07, 0x00,
    0x00, 0x07, 0x07, 0x07, 0x07, 0x07, 0x00, 0x00, 0x00, 0x01, 0x03, 0x03, 0x07, 0x07, 0x07, 0x07,
    0x07, 0x07, 0x03, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x03,
    0x00, 0x00, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x03, 0x00, 0x00, 0x00,
};

PROGMEM static const uint8_t imgTitle2[] = { // "Seeker" 80x20
    0xF0, 0xF8, 0xFC, 0xFE, 0xFE, 0x9E, 0x1E, 0x1E, 0x1E, 0x1E, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x80, 0x80, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80,
    0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0x80, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
    0x00, 0x80, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0x00, 0x00, 0x80, 0x80, 0xC0, 0xC0, 0xC0, 0xC0,
    0xC0, 0xC0, 0x80, 0x00, 0x00, 0x00, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0x80, 0xC0, 0xC0, 0xC0, 0x00,
    0xE1, 0xC3, 0xC7, 0x87, 0x8F, 0x8F, 0x9F, 0xFF, 0xFE, 0xFE, 0xFC, 0xF8, 0x00, 0x00, 0x7C, 0xFF,
    0xFF, 0xFF, 0xFF, 0xBB, 0x39, 0x39, 0x3F, 0x3F, 0x3F, 0xBF, 0x3E, 0x00, 0x7C, 0xFF, 0xFF, 0xFF,
    0xFF, 0xBB, 0x39, 0x39, 0x3F, 0x3F, 0x3F, 0xBF, 0x3E, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x18,
    0x7E, 0xFF, 0xFF, 0xFF, 0xFF, 0xE7, 0x83, 0x00, 0x7C, 0xFF, 0xFF, 0xFF, 0xFF, 0xBB, 0x39, 0x39,
    0x3F, 0x3F, 0x3F, 0xBF, 0x3E, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, 0x03, 0x03, 0x03, 0x00,
    0x03, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x03, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01,
    0x03, 0x03, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x03, 0x00, 0x00, 0x00, 0x01, 0x03, 0x03,
    0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x03, 0x00, 0x00, 0x07, 0x07, 0x07, 0x07, 0x07, 0x00,
    0x00, 0x01, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x04, 0x01, 0x03, 0x03, 0x07, 0x07, 0x07, 0x07,
    0x07, 0x07, 0x07, 0x03, 0x00, 0x00, 0x07, 0x07, 0x07, 0x07, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
};

PROGMEM static const uint8_t imgMan[] = { // 40x40
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xC0, 0xE0, 0xF0, 0x70, 0x78, 0x78, 0x00, 0xF8,
    0xFC, 0x06, 0xF3, 0xF9, 0xFD, 0xFD, 0xFD, 0xF9, 0xF2, 0x04, 0xF8, 0x00, 0x60, 0x40, 0x80, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0xD8, 0xD6, 0xCB, 0xD4,
    0xEA, 0xE4, 0xEA, 0xF5, 0xF2, 0xF5, 0xF2, 0xF9, 0xF7, 0xF6, 0xEC, 0xED, 0xEB, 0xEB, 0xEB, 0xE9,
    0xF4, 0xFA, 0xF9, 0xF8, 0xFA, 0x79, 0x7A, 0x7B, 0x7C, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x07, 0x07, 0x07, 0x3F, 0xC7, 0x07, 0x03, 0xAB, 0x53, 0xFB, 0xFB, 0xFB, 0xFD,
    0xFD, 0x81, 0x81, 0xFD, 0xFD, 0xFD, 0xFE, 0xC0, 0xC0, 0xFE, 0xFE, 0xFE, 0xFE, 0xFF, 0x1F, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x20, 0x10, 0x28, 0x48, 0x84, 0x04,
    0xA5, 0x56, 0xAC, 0x59, 0xAA, 0x5D, 0xB7, 0x57, 0xB7, 0x6F, 0xAF, 0x69, 0xA8, 0x28, 0xE8, 0x6E,
    0xAF, 0x57, 0xB7, 0x57, 0xAB, 0x54, 0xAA, 0x56, 0xEA, 0x42, 0xE2, 0x16, 0x0C, 0x04, 0x08, 0xF0,
    0x00, 0x03, 0x04, 0x04, 0x04, 0x05, 0x02, 0x03, 0x01, 0x0F, 0xF9, 0x20, 0x20, 0x25, 0x22, 0x25,
    0x2A, 0x35, 0x2A, 0x35, 0xEA, 0x20, 0x3F, 0xF5, 0x2A, 0x35, 0x2A, 0x25, 0x22, 0x25, 0x20, 0xF0,
    0x3F, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00,
};

PROGMEM static const uint8_t imgEyes[] = { // 10x8x3
    0xFD, 0x81, 0x81, 0xFD, 0xFD, 0xFD, 0xFE, 0xC0, 0xC0, 0xFE, 0xFD, 0x8D, 0x8D, 0xFD, 0xFD, 0xFD,
    0xFE, 0xC6, 0xC6, 0xFE, 0xDD, 0xDD, 0xDD, 0xDD, 0xFD, 0xFD, 0xEE, 0xEE, 0xEE, 0xEE,
};

PROGMEM static const uint8_t imgHeadLight[] = { // 7x8
    0x3C, 0x7E, 0xFF, 0xFF, 0xFF, 0x7E, 0x3C,
};

PROGMEM static const byte sound1[] = {
    0x90, 69, 0, 10, 0x80, 0xF0
};

PROGMEM static const byte sound2[] = {
    0x90, 74, 0, 20, 0x80, 0xF0
};

PROGMEM static const char menuGame[] = "START GAME";
PROGMEM static const char menuSound[] = "SOUND ";
PROGMEM static const char menuRecord[] = "RECORD";
PROGMEM static const char menuCredit[] = "CREDIT";

PROGMEM static const char * const menusTable[] = {
    menuGame, menuSound, menuRecord, menuCredit
};

PROGMEM static const char credit0[] = "- " APP_TITLE " -";
PROGMEM static const char credit1[] = APP_RELEASED;
PROGMEM static const char credit2[] = "PROGRAMMED BY OBONO";
PROGMEM static const char credit3[] = "THIS PROGRAM IS";
PROGMEM static const char credit4[] = "RELEASED UNDER";
PROGMEM static const char credit5[] = "THE MIT LICENSE.";

PROGMEM static const char * const creditsTable[] = {
    credit0, NULL, credit1, credit2, NULL, credit3, credit4, credit5
};

static uint8_t  state;
static bool     toDraw;
static bool     sound;
static int8_t   menuPos;
static uint16_t lastScore = 0;
static uint8_t  recordState = RECORD_NOT_READ;
static int16_t  eepAddr;
static uint16_t hiScore[10];
static uint16_t playCount;
static uint32_t playFrames;
static uint8_t  eyesWait;
static int8_t   lightColor;
static bool     lightBlink;
static uint8_t  lightWait;

void initTitle(void)
{
    state = STATE_MENU;
    toDraw = true;
    setSound(arduboy.audio.enabled());
    menuPos = 0;

    if (recordState == RECORD_NOT_READ) {
        readRecord();
    }

    eyesWait = rnd(100) + 150;
    lightColor = WHITE;
    lightBlink = false;
    lightWait = rnd(180) + 60;
}

bool updateTitle(void)
{
    bool ret = false;
    if (state == STATE_MENU) {
        /*  Animation  */
        if (eyesWait-- == 0) eyesWait = rnd(100) + 150;
        if (lightBlink && (lightWait & 1)) lightColor = WHITE - lightColor;
        if (lightWait-- == 0) {
            lightBlink = !lightBlink;
            lightWait = rnd(180) + 60;
        }
        /*  Button handling  */
        if (arduboy.buttonDown(UP_BUTTON)) {
            if (menuPos-- == 0) menuPos = 3;
            playSound1();
            toDraw = true;
        }
        if (arduboy.buttonDown(DOWN_BUTTON)) {
            if (menuPos++ == 3) menuPos = 0;
            playSound1();
            toDraw = true;
        }
        if (arduboy.buttonDown(A_BUTTON | B_BUTTON)) {
            switch (menuPos) {
            case 0:
                ret = true;
                arduboy.audio.saveOnOff();
                break;
            case 1:
                setSound(!sound);
                playSound2();
                break;
            case 2:
                state = STATE_RECORD;
                playSound2();
                break;
            case 3:
                state = STATE_CREDIT;
                playSound2();
                break;
            }
            toDraw = true;
        }
    } else {
        if (arduboy.buttonDown(A_BUTTON | B_BUTTON)) {
            state = STATE_MENU;
            playSound2();
            toDraw = true;
        }
    }

    randomSeed(rand() ^ micros()); // Shuffle random
    return ret;
}

void drawTitle(void)
{
    if (toDraw) {
        char buf[22];
        const char *p;
        arduboy.clear();
        switch (state) {
        case STATE_MENU:
            arduboy.drawBitmap(3, 0, imgTitle1, 84, 20, WHITE);
            arduboy.drawBitmap(45, 19, imgTitle2, 80, 20, WHITE);
            arduboy.drawBitmap(4, 24, imgMan, 40, 40, WHITE);
            if (lastScore > 0) {
                arduboy.printEx(98, 0, F("LAST"));
                arduboy.printEx(98, 6, F("SCORE"));
                sprintf(buf, "%5d", lastScore);
                arduboy.printEx(98, 14, buf);
            }
            for (int i = 0; i < 4; i++) {
                p = pgm_read_word(menusTable + i);
                strcpy_P(buf, p);
                arduboy.printEx(64 - (i == menuPos) * 4, i * 6 + 40, buf);
                if (p == menuSound) {
                    arduboy.print((sound) ? F("ON") : F("OFF"));
                }
            }
            arduboy.fillRect2(52, menuPos * 6 + 40, 5, 5, WHITE);
            break;
        case STATE_RECORD:
            arduboy.printEx(22, 4, F("BEST 10 SCORES"));
            arduboy.drawFastHLine2(0, 12, 127, 12);
            for (int i = 0; i < 2; i++) {
                for (int j = 0; j < 5; j++) {
                    int r = i * 5 + j;
                    arduboy.printEx(i * 60 + 4 - (r == 9) * 6, j * 6 + 14, F("["));
                    arduboy.print(r + 1);
                    arduboy.print(F("] "));
                    arduboy.print(hiScore[r]);
                }
            }
            arduboy.drawFastHLine2(0, 44, 127, 44);
            arduboy.printEx(16, 48, F("PLAY COUNT "));
            arduboy.print(playCount);
            arduboy.printEx(16, 54, F("PLAY TIME  "));
            arduboy.print(playFrames / 3600); // minutes
            sprintf(buf, "'%02d\"", playFrames / 60 % 60); // seconds
            arduboy.print(buf);
            break;
        case STATE_CREDIT:
            for (int i = 0; i < 8; i++) {
                p = pgm_read_word(creditsTable + i);
                if (p != NULL) {
                    strcpy_P(buf, p);
                    uint8_t len = strnlen(buf, sizeof(buf));
                    arduboy.printEx(64 - len * 3, i * 6 + 8, buf);
                }
            }
            break;
        }
        toDraw = false;
    }

    /*  Animation  */
    if (state == STATE_MENU) {
        uint8_t ani = max(3 - eyesWait / 4, 0);
        if (ani == 3) ani = 1;
        memcpy_P(arduboy.getBuffer() + 660, imgEyes + ani * 10, 10);
        //arduboy.fillRect2(20, 40, 10, 8, BLACK);
        //arduboy.drawBitmap(20, 40, imgEyes + ani * 10, 10, 8, WHITE);
        arduboy.drawBitmap(22, 26, imgHeadLight, 7, 8, lightColor);
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
        eepSeek(EEPROM_ADDR_BASE);
        eepWrite32(EEPROM_SIGNATURE);
    }
    eepSeek(EEPROM_ADDR_BASE + 4 + r * 2);
    for (int i = r; i < 10; i++) {
        eepWrite16(hiScore[i]);
    }
    eepWrite16(playCount);
    eepWrite32(playFrames);

    uint16_t checkSum = (EEPROM_SIGNATURE & 0xFFFF) + (EEPROM_SIGNATURE >> 16) * 2;
    for (int i = 0; i < 10; i++) {
        checkSum += hiScore[i] * (i + 3);
    }
    checkSum += playCount * 13;
    checkSum += (playFrames & 0xFFFF) * 14 + (playFrames >> 16) * 15;
    eepWrite16(checkSum);

    recordState = RECORD_STORED;
    return r;
}

static void readRecord(void)
{
    bool isRegular = false;
    eepSeek(EEPROM_ADDR_BASE);
    if (eepRead32() == EEPROM_SIGNATURE) {
        uint16_t checkSum = (EEPROM_SIGNATURE & 0xFFFF) + (EEPROM_SIGNATURE >> 16) * 2;
        for (int i = 0; i < 13; i++) {
            checkSum += eepRead16() * (i + 3);
        }
        isRegular = (eepRead16() == checkSum);
    }

    if (isRegular) {
        /*  Read record from EEPROM  */
        eepSeek(EEPROM_ADDR_BASE + 4);
        for (int i = 0; i < 10; i++) {
            hiScore[i] = eepRead16();
        }
        playCount = eepRead16();
        playFrames = eepRead32();
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
    arduboy.playScore2(sound1, 255);
}

static void playSound2(void)
{
    arduboy.playScore2(sound2, 255);
}

/*---------------------------------------------------------------------------*/
/*                             EEPROM Functions                              */
/*---------------------------------------------------------------------------*/

void eepSeek(int addr)
{
    eepAddr = max(addr, EEPROM_STORAGE_SPACE_START);
}

uint8_t eepRead8(void)
{
    eeprom_busy_wait();
    return eeprom_read_byte((const uint8_t *) eepAddr++);
}

uint16_t eepRead16(void)
{
    eeprom_busy_wait();
    uint16_t ret = eeprom_read_word((const uint16_t *)eepAddr);
    eepAddr += 2;
    return ret;
}

uint32_t eepRead32(void)
{
    eeprom_busy_wait();
    uint32_t ret = eeprom_read_dword((const uint32_t *) eepAddr);
    eepAddr += 4;
    return ret;
}

void eepReadBlock(void *p, size_t n)
{
    eeprom_busy_wait();
    eeprom_read_block(p, (const void *) eepAddr, n);
    eepAddr += n;
}

void eepWrite8(uint8_t val)
{
    eeprom_busy_wait();
    eeprom_write_byte((uint8_t *) eepAddr, val);
    eepAddr++;
}

void eepWrite16(uint16_t val)
{
    eeprom_busy_wait();
    eeprom_write_word((uint16_t *)eepAddr, val);
    eepAddr += 2;
}

void eepWrite32(uint32_t val)
{
    eeprom_busy_wait();
    eeprom_write_dword((uint32_t *)eepAddr, val);
    eepAddr += 4;
}

void eepWriteBlock(const void *p, size_t n)
{
    eeprom_busy_wait();
    eeprom_write_block(p, (void *) eepAddr, n);
    eepAddr += n;
}
