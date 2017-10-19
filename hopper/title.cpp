#include "common.h"

/*  Defines  */

#define STATE_MENU      0
#define STATE_RECORD    1
#define STATE_CREDIT    2

#define RECORD_NOT_READ 0
#define RECORD_INITIAL  1
#define RECORD_STORED   2

#define EEPROM_ADDR_BASE    800
#define EEPROM_SIGNATURE    0x024E424FUL // "OBN\x02"

/*  Local Functions  */

static void readRecord(void);
static void setSound(bool enabled);
static void playSound1(void);
static void playSound2(void);

/*  Local Variables  */

PROGMEM static const char menuGame[] = "START GAME";
PROGMEM static const char menuSound[] = "SOUND ";
PROGMEM static const char menuRecord[] = "RECORD";
PROGMEM static const char menuCredit[] = "CREDIT";

PROGMEM static const char * const menusTable[] = {
    menuGame, menuSound, menuRecord, menuCredit
};

PROGMEM static const char credit0[] = "- HOPPER -";
PROGMEM static const char credit1[] = "OCTOBER 2017";
PROGMEM static const char credit2[] = "PROGREMMED BY OBONO";
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
static uint16_t hiScore[10];
static uint16_t playCount;
static uint32_t playFrames;

void initTitle(void)
{
    state = STATE_MENU;
    toDraw = true;
    setSound(arduboy.audio.enabled());
    menuPos = 0;

    if (recordState == RECORD_NOT_READ) {
        readRecord();
    }
}

bool updateTitle(void)
{
    bool ret = false;
    if (state == STATE_MENU) {
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
            arduboy.printEx(0, 0, F("HOPPER TITLE SCRREN"));
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
    arduboy.tunes.tone(440, 10);
}

static void playSound2(void)
{
    arduboy.tunes.tone(587, 20);
}
