#include "common.h"
#include "data.h"

/*  Defines  */

#define PAD_REPEAT_MAX  (FPS * 20)
#define DEC_POINT_SIZE  2

/*  Typedefs  */

/*  Local Functions  */

static void processUsual(void);
static void processCredit(void);
static void sparkleLed(uint8_t num);

static void drawPiNumbers(void);
static void drawDecimalPoint(int16_t x);
static void drawStatus(void);
static void drawCredit(void);

/*  Local Variables  */

PROGMEM static const char creditText[] = \
        "- " APP_TITLE " -\0\0\0" APP_RELEASED "\0PROGREMMED BY OBONO\0\0" \
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
    if (arduboy.buttonPressed(LEFT_BUTTON | RIGHT_BUTTON)) {
        if (padRepeatCount < PAD_REPEAT_MAX) padRepeatCount++;
    } else {
        padRepeatCount = 0;
    }
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
        if (padRepeatCount < FPS * 2) {
            if (padRepeatCount > 1 && padRepeatCount < FPS / 3 ||
                    padRepeatCount >= FPS / 3 && padRepeatCount < FPS && (padRepeatCount & 3) ||
                    padRepeatCount >= FPS && padRepeatCount < FPS * 2 && (padRepeatCount & 1)) {
                vx = 0;
            }
        } else {
            vx *= pow(2.0, (padRepeatCount - FPS * 2) / (double) FPS);
        }
    }

    if (keta > 0 && vx < 0 || keta < ketaMax && vx > 0) {
        keta += vx;
        if (keta < 0) keta = 0;
        if (keta > ketaMax) keta = ketaMax;
        currentNum = getPiNumber(keta);
        sparkleLed(currentNum);
        arduboy.playScore2(soundFigure[currentNum], 0);
        if (isPlaying && keta == ketaMax) {
            isPlaying = false;
        }
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
        arduboy.playScore2(soundFigure[currentNum], 0);
        isInvalid = true;
    }

    if (arduboy.buttonDown(A_BUTTON) && keta == 0) {
        playSoundClick();
        isPlaying = false;
        isCredit = true;
        isInvalid = true;
    }

    if (arduboy.buttonDown(B_BUTTON)) {
        isPlaying = !isPlaying;
        if (isPlaying) {
            sparkleLed(currentNum);
            arduboy.playScore2(soundFigure[currentNum], 0);
            waitCounter = waitInterval;
            padRepeatCount = 0;
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
    int16_t x = (keta <= 7) ? -DEC_POINT_SIZE : 0;
    for (int i = -7; i < 0; i++, x += IMG_FIGURE_W) {
        int16_t pos = keta + i;
        if (pos < 0) continue;
        int num = getPiNumber(pos);
        arduboy.drawBitmap(x, 24, imgFigure[num], IMG_FIGURE_W, IMG_FIGURE_H, WHITE);
        if (pos == 0) {
            drawDecimalPoint(x + IMG_FIGURE_W);
            x += DEC_POINT_SIZE;
        }
    }

    /*  Center  */
    arduboy.drawBitmap((WIDTH - IMG_BIGFIGURE_W) / 2, 16, imgBigFigure[currentNum],
            IMG_BIGFIGURE_W, IMG_BIGFIGURE_H, WHITE);

    /*  Right side  */
    x = WIDTH / 2 + IMG_FIGURE_W;
    if (keta == 0) {
        drawDecimalPoint(x);
        x += DEC_POINT_SIZE;
    }
    for (int i = 1; i <= 7; i++, x += IMG_FIGURE_W) {
        int16_t pos = keta + i;
        if (pos > ketaMax) break;
        int num = getPiNumber(pos);
        arduboy.drawBitmap(x, 24, imgFigure[num], IMG_FIGURE_W, IMG_FIGURE_H, WHITE);
    }
}

static void drawDecimalPoint(int16_t x)
{
    arduboy.fillRect2(x, 35, DEC_POINT_SIZE, DEC_POINT_SIZE, WHITE);
}

static void drawStatus(void)
{
    /*  Decimal place  */
    arduboy.printEx(0, 0, F("DECIMAL"));
    arduboy.printEx(12, 6, F("PLACE"));
    drawNumber(48, 3, keta);

    /*  Credit navigation  */
    if (keta == 0) {
        arduboy.drawBitmap(84, 0, imgIcon[IMG_ICON_ID_A], IMG_ICON_W, IMG_ICON_H, WHITE);
        arduboy.printEx(92, 1, F("CREDIT"));
    }

    /*  LED on / off  */
    arduboy.drawBitmap(0, 56, imgIcon[IMG_ICON_ID_UP], IMG_ICON_W, IMG_ICON_H, WHITE);
    arduboy.drawBitmap(8, 56, imgChoice[IMG_CHOICE_ID_LED_OFF + isLedOn],
            IMG_CHOICE_W, IMG_CHOICE_H, WHITE);

    /*  Sound on / off  */
    arduboy.drawBitmap(32, 56, imgIcon[IMG_ICON_ID_DOWN], IMG_ICON_W, IMG_ICON_H, WHITE);
    arduboy.drawBitmap(40, 56, imgChoice[IMG_CHOICE_ID_SOUND_OFF + isSoundOn],
            IMG_CHOICE_W, IMG_CHOICE_H, WHITE);

    /*  Speed  */
    arduboy.drawBitmap(64, 56, imgIcon[IMG_ICON_ID_LEFTRIGHT], IMG_ICON_W, IMG_ICON_H, WHITE);
    if (isPlaying) {
        drawNumber(75, 57, FPS / waitInterval);
        arduboy.print(F("/S"));
    } else {
        arduboy.printEx(75, 57, F("----"));
    }

    /*  Playing state  */
    arduboy.drawBitmap(104, 56, imgIcon[IMG_ICON_ID_B], IMG_ICON_W, IMG_ICON_H, WHITE);
    arduboy.drawBitmap(112, 56, imgChoice[IMG_CHOICE_ID_STOP + isPlaying],
            IMG_CHOICE_W, IMG_CHOICE_H, WHITE);
}

static void drawCredit(void)
{
    const char *p = creditText;
    int16_t y = 10;
    while (pgm_read_byte(p) != '\e') {
        uint8_t len = strnlen_P(p, 21);
        arduboy.printEx(64 - len * 3, y, (const __FlashStringHelper *) p);
        p += len + 1;
        y += (len == 0) ? 2 : 6;
    }
}
