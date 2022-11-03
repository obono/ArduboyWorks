#include "common.h"

/*  Defines  */

#define EEPROM_ADDR_BASE    624
#define EEPROM_SIGNATURE    0x154E424FUL // "OBN\x15"

#define PAD_REPEAT_DELAY    (FPS / 4)
#define PAD_REPEAT_INTERVAL (FPS / 12)

#define IMG_BUTTONS_W       7
#define IMG_BUTTONS_H       7

#define TONE_DURATION       1000
#define TONE_PRIORITY       1

enum RECORD_STATE_T : uint8_t {
    RECORD_NOT_READ = 0,
    RECORD_INITIAL,
    RECORD_STORED,
};

enum USB_STATE_T : uint8_t {
    USB_DISCONNECTED = 0,
    USB_POWERED,
    USB_ADDRESSING,
    USB_CONFIGURED,
};

/*  Global Variables  */

MyArduboy2  arduboy;
RECORD_T    record;
uint8_t     counter;
int8_t      padX, padY, padRepeatCount;
bool        isInvalid, isRecordDirty;

/*  Local Functions  */

static uint16_t calcCheckSum();
static uint8_t  calcLEDPower(int8_t a);

static void     eepSeek(int addr);
static uint8_t  eepRead8(void);
static uint16_t eepRead16(void);
static uint32_t eepRead32(void);
static void     eepReadBlock(void *p, size_t n);
static void     eepWrite8(uint8_t val);
static void     eepWrite16(uint16_t val);
static void     eepWrite32(uint32_t val);
static void     eepWriteBlock(const void *p, size_t n);

/*  Local Constants  */

PROGMEM static const uint8_t imgButtons[][7] = { // 7x7 x2
    { 0x3E, 0x47, 0x6B, 0x6D, 0x6D, 0x41, 0x3E },
    { 0x3E, 0x41, 0x55, 0x55, 0x51, 0x65, 0x3E }
};

PROGMEM static const RECORD_T recordInitial = {
    0,  // Play frames
    0,  // Made letters
    3,  // 14 WPM
    DECODE_MODE_EN,
    KEYBOARD_NONE,
    IME_MODE_ROMAN,
    LED_LOW,
    8,  // Blue
    80, // 800Hz
};

/*  Local Variables  */

static RECORD_STATE_T   recordState = RECORD_NOT_READ;
static USB_STATE_T      usbState = USB_DISCONNECTED;
static int16_t          eepAddr;

/*---------------------------------------------------------------------------*/
/*                             Common Functions                              */
/*---------------------------------------------------------------------------*/

void readRecord(void)
{
    bool isVerified = false;
    eepSeek(EEPROM_ADDR_BASE);
    if (eepRead32() == EEPROM_SIGNATURE) {
        eepReadBlock(&record, sizeof(record));
        isVerified = (eepRead16() == calcCheckSum());
    }

    if (isVerified) {
        recordState = RECORD_STORED;
        isRecordDirty = false;
        dprintln(F("Read record from EEPROM"));
    } else {
        memcpy_P(&record, &recordInitial, sizeof(record));
        recordState = RECORD_INITIAL;
        isRecordDirty = true;
    }
    setSound(arduboy.isAudioEnabled()); // Load Sound ON/OFF
}

void writeRecord(void)
{
    if (!isRecordDirty) return;
    if (recordState == RECORD_INITIAL) {
        eepSeek(EEPROM_ADDR_BASE);
        eepWrite32(EEPROM_SIGNATURE);
    } else {
        eepSeek(EEPROM_ADDR_BASE + 4);
    }
    eepWriteBlock(&record, sizeof(record));
    eepWrite16(calcCheckSum());
    arduboy.audio.saveOnOff(); // Save Sound ON/OFF
    recordState = RECORD_STORED;
    isRecordDirty = false;
    dprintln(F("Write record to EEPROM"));
}

static uint16_t calcCheckSum()
{
    uint16_t checkSum = (EEPROM_SIGNATURE & 0xFFFF) + (EEPROM_SIGNATURE >> 16) * 3;
    uint16_t *p = (uint16_t *) &record;
    for (int i = 0; i < sizeof(record) / 2; i++) {
        checkSum += *p++ * (i * 2 + 5);
    }
    return checkSum;
}

void clearRecord(void)
{
    eepSeek(EEPROM_ADDR_BASE);
    for (int i = 0; i < (sizeof(record) + 6) / 4; i++) {
        eepWrite32(0);
    }
    recordState = RECORD_INITIAL;
    dprintln(F("Clear EEPROM"));
}

void handleDPad(void)
{
    padX = padY = 0;
    if (arduboy.buttonPressed(LEFT_BUTTON | RIGHT_BUTTON | UP_BUTTON | DOWN_BUTTON)) {
        if (++padRepeatCount >= (PAD_REPEAT_DELAY + PAD_REPEAT_INTERVAL)) {
            padRepeatCount = PAD_REPEAT_DELAY;
        }
        if (padRepeatCount == 1 || padRepeatCount == PAD_REPEAT_DELAY) {
            if (arduboy.buttonPressed(LEFT_BUTTON))  padX--;
            if (arduboy.buttonPressed(RIGHT_BUTTON)) padX++;
            if (arduboy.buttonPressed(UP_BUTTON))    padY--;
            if (arduboy.buttonPressed(DOWN_BUTTON))  padY++;
        }
    } else {
        padRepeatCount = 0;
    }
}

void drawTime(int16_t x, int16_t y, uint32_t frames)
{
    uint16_t h = frames / (FPS * 3600UL);
    uint8_t m = frames / (FPS * 60) % 60;
    uint8_t s = frames / FPS % 60;
    arduboy.setCursor(x, y);
    if (h == 0 && m == 0) {
        if (s < 10) arduboy.print('0');
        arduboy.print(s);
        arduboy.print('.');
        arduboy.print(frames / (FPS / 10) % 10);
    } else {
        if (h > 0) {
            arduboy.print(h);
            arduboy.print(':');
            if (m < 10) arduboy.print('0');
        }
        arduboy.print(m);
        arduboy.print(':');
        if (s < 10) arduboy.print('0');
        arduboy.print(s);
    }
}

void drawButtonIcon(int16_t x, int16_t y, bool isB)
{
    arduboy.drawBitmap(x, y, imgButtons[isB],
            IMG_BUTTONS_W, IMG_BUTTONS_H, arduboy.getTextColor());
}

uint8_t drawMorseCode(int16_t x, int16_t y, uint16_t code)
{
    uint16_t left = x;
    bool isAvailable = false;
    for (uint16_t mask = 0x100; mask > 0; mask >>= 1) {
        if (isAvailable) {
            uint8_t w = (code & mask) ? 6 : 2;
            arduboy.fillRect(x, y, w, 2, WHITE);
            x += w + 2;
        } else if (code & mask) {
            isAvailable = true;
        }
    }
    return x - left;
}

static uint8_t calcLEDPower(int8_t a)
{
    a = 4 - abs(6 - a);
    a = constrain(a, 0, 2);
    return a;
}

void indicateSignalOn(void)
{
    uint8_t r, g, b;
    if (record.ledColor == 12) {
        r = g = b = 2;
    } else {
        r = calcLEDPower((record.ledColor + 6) % 12);
        g = calcLEDPower((record.ledColor + 2) % 12);
        b = calcLEDPower((record.ledColor - 2) % 12);
    }
    arduboy.setRGBled(r * record.led * 32, g * record.led * 16, b * record.led * 48);
    arduboy.playTone(record.toneFreq * 5 + 400, TONE_DURATION, TONE_PRIORITY);
}

void indicateSignalOff(void)
{
    arduboy.setRGBled(0, 0, 0);
    arduboy.stopTone();
}

void checkUSBStatus(void)
{
    if (USBSTA & _BV(VBUS)) { // whether VBUS is supplied
        bool isConfigured = USBDevice.configured();
        if (usbState == USB_DISCONNECTED) usbState = USB_POWERED;
        if (usbState == USB_POWERED && !isConfigured) usbState = USB_ADDRESSING;
        if (usbState == USB_ADDRESSING && isConfigured) usbState = USB_CONFIGURED;
    } else {
        usbState = USB_DISCONNECTED;
    }
}

bool isUSBConfigured(void)
{
    return (usbState == USB_CONFIGURED);
}

/*---------------------------------------------------------------------------*/
/*                              Sound Functions                              */
/*---------------------------------------------------------------------------*/

void setSound(bool on)
{
    arduboy.setAudioEnabled(on);
    dprint(F("audioEnabled="));
    dprintln(on);
}

void playSoundTick(void)
{
    arduboy.playTone(440, 10);
}

void playSoundClick(void)
{
    arduboy.playTone(587, 20);
}

/*---------------------------------------------------------------------------*/
/*                             EEPROM Functions                              */
/*---------------------------------------------------------------------------*/

static void eepSeek(int addr)
{
    eepAddr = max(addr, EEPROM_STORAGE_SPACE_START);
}

static uint8_t eepRead8(void)
{
    eeprom_busy_wait();
    return eeprom_read_byte((const uint8_t *) eepAddr++);
}

static uint16_t eepRead16(void)
{
    eeprom_busy_wait();
    uint16_t ret = eeprom_read_word((const uint16_t *)eepAddr);
    eepAddr += 2;
    return ret;
}

static uint32_t eepRead32(void)
{
    eeprom_busy_wait();
    uint32_t ret = eeprom_read_dword((const uint32_t *) eepAddr);
    eepAddr += 4;
    return ret;
}

static void eepReadBlock(void *p, size_t n)
{
    eeprom_busy_wait();
    eeprom_read_block(p, (const void *) eepAddr, n);
    eepAddr += n;
}

static void eepWrite8(uint8_t val)
{
    eeprom_busy_wait();
    eeprom_write_byte((uint8_t *) eepAddr, val);
    eepAddr++;
}

static void eepWrite16(uint16_t val)
{
    eeprom_busy_wait();
    eeprom_write_word((uint16_t *)eepAddr, val);
    eepAddr += 2;
}

static void eepWrite32(uint32_t val)
{
    eeprom_busy_wait();
    eeprom_write_dword((uint32_t *)eepAddr, val);
    eepAddr += 4;
}

static void eepWriteBlock(const void *p, size_t n)
{
    eeprom_busy_wait();
    eeprom_write_block(p, (void *) eepAddr, n);
    eepAddr += n;
}
