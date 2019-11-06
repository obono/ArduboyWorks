#include "common.h"
#include "data.h"

/*  Defines  */

#define PAD_REPEAT_MAX  (FPS * 16)
#define SIDE_DIGITS     7
#define DEC_POINT_SIZE  2

/*  Local Functions  */

static void processUsual(void);
static void processCredit(void);
static void sparkleLed(uint8_t num);

static void drawPiNumbers(void);
static void drawStatus(void);
static void drawCredit(void);

#define drawDigit(x, y, id) \
        arduboy.drawBitmap(x, y, imgDigit[id], IMG_DIGIT_W, IMG_DIGIT_H, WHITE)
#define drawBigDigit(x, y, id) \
        arduboy.drawBitmap(x, y, imgBigDigit[id], IMG_BIGDIGIT_W, IMG_BIGDIGIT_H, WHITE)
#define drawIcon(x, y, id) \
        arduboy.drawBitmap(x, y, imgIcon[id], IMG_ICON_W, IMG_ICON_H, WHITE)
#define drawChoice(x, y, id) \
        arduboy.drawBitmap(x, y, imgChoice[id], IMG_CHOICE_W, IMG_CHOICE_H, WHITE)
#define drawDecimalPoint(x, y) \
        arduboy.fillRect2(x, y, DEC_POINT_SIZE, DEC_POINT_SIZE, WHITE)

/*  Local Variables  */

PROGMEM static const char creditText[] = \
        APP_RELEASED "\0PROGREMMED BY OBONO\0\0" \
        "THIS PROGRAM IS\0RELEASED UNDER\0THE MIT LICENSE.\0\e";

int16_t keta, ketaMax, padRepeatCount;
uint8_t currentNum, playSpeed, waitInterval, waitCounter;
uint8_t ledRed, ledGreen, ledBlue, ledLevel;
bool    isLedOn, isSoundOn, isPlaying, isCredit;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initMain(void)
{
    keta = 0;
    ketaMax = getKetaMax();
    padRepeatCount = 0;
    currentNum = getPiNumber(keta);
    playSpeed = 2;
    waitInterval = getWaitInterval(playSpeed);
    ledLevel = 0;
    isLedOn = false;
    isSoundOn = false;
    isPlaying = false;
    isCredit = false;
    isInvalid = true;
    arduboy.setAudioEnabled(isSoundOn);
}

MODE_T updateMain(void)
{
    if (ledLevel > 0) ledLevel--;
    if (isCredit) {
        processCredit();
    } else {
        processUsual();
    }
    return MODE_MAIN;
}

void drawMain(void)
{
    if (isInvalid) {
        arduboy.clear();
        if (isCredit) {
            drawCredit();
        } else {
            drawPiNumbers();
            drawStatus();
        }
        isInvalid = false;
    }
    arduboy.setRGBled(ledRed * ledLevel, ledGreen * ledLevel, ledBlue * ledLevel);
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void processUsual(void)
{
    int16_t vx = 0;
    if (isPlaying) {
        int8_t vs = 0;
        if (arduboy.buttonDown(LEFT_BUTTON) && playSpeed > 0) vs--;
        if (arduboy.buttonDown(RIGHT_BUTTON) && playSpeed < PLAY_SPEED_MAX - 1) vs++;
        if (vs != 0) {
            playSpeed += vs;
            waitInterval = getWaitInterval(playSpeed);
            if (waitCounter > waitInterval) waitCounter = waitInterval;
            isInvalid = true;
        }
        if (--waitCounter <= 0) {
            vx = 1;
            waitCounter = waitInterval;
        }
    } else {
        if (arduboy.buttonPressed(LEFT_BUTTON))  vx--;
        if (arduboy.buttonPressed(RIGHT_BUTTON)) vx++;
        if (vx != 0) {
            if (padRepeatCount < PAD_REPEAT_MAX) padRepeatCount++;
            if (padRepeatCount < FPS * 2) {
                if (padRepeatCount > 1 && padRepeatCount < FPS / 3 ||
                        padRepeatCount >= FPS / 3 && padRepeatCount < FPS && (padRepeatCount & 3) ||
                        padRepeatCount >= FPS && padRepeatCount < FPS * 2 && (padRepeatCount & 1)) {
                    vx = 0;
                }
            } else {
                vx *= pow(2.0, (padRepeatCount - FPS * 2) / (double) FPS);
            }
        } else {
            if (padRepeatCount >= FPS * 2) isInvalid = true;
            padRepeatCount = 0;
        }
    }

    if (keta > 0 && vx < 0 || keta < ketaMax && vx > 0) {
        keta += vx;
        if (keta < 0) keta = 0;
        if (keta > ketaMax) keta = ketaMax;
        currentNum = getPiNumber(keta);
        sparkleLed(currentNum);
        arduboy.playScore2(soundDigit[currentNum], 0);
        if (keta == ketaMax) isPlaying = false;
        isInvalid = true;
    }

    if (arduboy.buttonDown(UP_BUTTON)) {
        isLedOn = !isLedOn;
        sparkleLed(currentNum);
        isInvalid = true;
    }

    if (arduboy.buttonDown(DOWN_BUTTON)) {
        isSoundOn = !isSoundOn;
        arduboy.setAudioEnabled(isSoundOn);
        arduboy.playScore2(soundDigit[currentNum], 0);
        isInvalid = true;
    }

    if (arduboy.buttonDown(A_BUTTON) && !isPlaying && keta == 0) {
        playSoundClick();
        isCredit = true;
        padRepeatCount = 0;
        isInvalid = true;
    }

    if (arduboy.buttonDown(B_BUTTON)) {
        isPlaying = !isPlaying;
        padRepeatCount = 0;
        if (isPlaying) {
            sparkleLed(currentNum);
            arduboy.playScore2(soundDigit[currentNum], 0);
            waitCounter = waitInterval;
            if (keta == ketaMax) isPlaying = false;
        }
        isInvalid = true;
    }

}

static void processCredit(void)
{
    if (arduboy.buttonDown(A_BUTTON | B_BUTTON)) {
        playSoundClick();
        isCredit = false;
        isInvalid = true;
    }
}

static void sparkleLed(uint8_t num)
{
    if (isLedOn) {
        uint8_t ledValue = pgm_read_byte(ledValues + currentNum);
        ledRed = ledValue / 25;
        ledGreen = ledValue / 5 % 5;
        ledBlue = ledValue % 5;
        ledLevel = LED_LEVEL_START;
    } else {
        ledLevel = 0;
    }
}

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void drawPiNumbers(void)
{
    /*  Left side  */
    int16_t x = (WIDTH - IMG_BIGDIGIT_W) / 2 - IMG_DIGIT_W * SIDE_DIGITS;
    if (keta <= SIDE_DIGITS) x -= DEC_POINT_SIZE;
    for (int i = -SIDE_DIGITS; i < 0; i++, x += IMG_DIGIT_W) {
        int16_t pos = keta + i;
        if (pos < 0) continue;
        drawDigit(x, 24, getPiNumber(pos));
        if (pos == 0) {
            drawDecimalPoint(x + IMG_DIGIT_W, 35);
            x += DEC_POINT_SIZE;
        }
    }

    /*  Center  */
    drawBigDigit((WIDTH - IMG_BIGDIGIT_W) / 2, 16, currentNum);

    /*  Right side  */
    x = (WIDTH + IMG_BIGDIGIT_W) / 2;
    if (keta == 0) {
        drawDecimalPoint(x, 35);
        x += DEC_POINT_SIZE;
    }
    for (int i = 1; i <= SIDE_DIGITS; i++, x += IMG_DIGIT_W) {
        int16_t pos = keta + i;
        if (pos > ketaMax) {
            arduboy.drawBitmap(x, 24, imgDots, IMG_DOTS_W, IMG_DOTS_H, WHITE);
            break;
        }
        drawDigit(x, 24, getPiNumber(pos));
    }
}

static void drawStatus(void)
{
    /*  Decimal place  */
    arduboy.printEx(0, 0, F("DECIMAL"));
    arduboy.printEx(12, 6, F("PLACE"));
    drawNumber(48, 3, keta);

    /*  Credit navigation  */
    if (!isPlaying && keta == 0) {
        drawIcon(84, 0, IMG_ICON_ID_A);
        arduboy.printEx(92, 1, F("CREDIT"));
    }

    /*  LED on / off  */
    drawIcon(0, 56, IMG_ICON_ID_UP);
    drawChoice(8, 56, IMG_CHOICE_ID_LED_OFF + isLedOn);

    /*  Sound on / off  */
    drawIcon(32, 56, IMG_ICON_ID_DOWN);
    drawChoice(40, 56, IMG_CHOICE_ID_SOUND_OFF + isSoundOn);

    /*  Speed  */
    drawIcon(64, 56, IMG_ICON_ID_LEFTRIGHT);
    if (isPlaying) {
        uint8_t speed = FPS / waitInterval;
        drawNumber(75, 57, speed);
        drawChoice((speed < 10) ? 81 : 87, 56, IMG_CHOICE_ID_PERSEC);
    } else {
        arduboy.printEx(75, 57, F("----"));
    }

    /*  Playing state  */
    drawIcon(104, 56, IMG_ICON_ID_B);
    drawChoice(112, 56, IMG_CHOICE_ID_STOP + isPlaying);

    /*  Position bar  */
    if (padRepeatCount >= FPS * 2) {
        int16_t x = keta / ((ketaMax + 1) / WIDTH);
        arduboy.drawFastVLine2(x, 42, 5, WHITE);
        arduboy.drawFastHLine2(0, 44, WIDTH, WHITE);
    }
}

static void drawCredit(void)
{
    arduboy.drawBitmap(24, 2, imgPi, IMG_PI_W, IMG_PI_H, WHITE);
    for (int i = 0; i < 3; i++) {
        drawBigDigit(56 + i * IMG_BIGDIGIT_W, 2, (i == 2) ? 10 : i * 2 + 2);
    }
    const char *p = creditText;
    int16_t y = 30;
    while (pgm_read_byte(p) != '\e') {
        uint8_t len = strnlen_P(p, 21);
        arduboy.printEx(64 - len * 3, y, (const __FlashStringHelper *) p);
        p += len + 1;
        y += (len == 0) ? 2 : 6;
    }
}
