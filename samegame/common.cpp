#include "common.h"

/*  Defines  */

#define EEPROM_ADDR_BASE    688
#define EEPROM_SIGNATURE    0x114E424FUL // "OBN\x11"

#define PAD_REPEAT_DELAY    (FPS / 4)
#define PAD_REPEAT_INTERVAL (FPS / 12)

enum RECORD_STATE_T {
    RECORD_NOT_READ = 0,
    RECORD_INITIAL,
    RECORD_STORED,
};

/*  Global Variables  */

MyArduboy   arduboy;
RECORD_T    record;
int8_t      padX, padY, padRepeatCount;
uint16_t    lastScore;
bool        isInvalid, isRecordDirty;

/*  Local Functions  */

static uint16_t calcCheckSum();

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

PROGMEM static const byte imgObjectDefault[] = {
    0x2E, 0xD6, 0xE8, 0xA2, 0x22, 0x2A, 0xFE, 0xA8, 0x14, 0x41, 0xCF, 0x79, 0xA1, 0xAA, 0xAA, 0x0A
};

PROGMEM static const byte soundTick[] = {
    0x90, 69, 0, 10, 0x80, 0xF0 // arduboy.tone2(440, 10);
};

PROGMEM static const byte soundClick[] = {
    0x90, 74, 0, 20, 0x80, 0xF0 // arduboy.tone2(587, 20);
};

static RECORD_STATE_T   recordState = RECORD_NOT_READ;
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
        memset(&record, 0, sizeof(record));
        restoreObjectImage();
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

bool enterScore(uint32_t score)
{
    int r;
    for (r = 10; r > 0; r--) {
        uint32_t hiscore = record.hiscore[r - 1];
        if (hiscore >= score) break;
        if (r <= 9) record.hiscore[r] = hiscore;
    }
    if (r < 10) {
        record.hiscore[r] = score;
    }
    lastScore = score;
    return (r == 0);
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

void drawNumber(int16_t x, int16_t y, int32_t value)
{
    arduboy.setCursor(x, y);
    arduboy.print(value);
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

void restoreObjectImage(void)
{
    memcpy_P(record.imgObject, imgObjectDefault, sizeof(record.imgObject));
}

void drawObject(int16_t x, int16_t y, uint8_t imgId)
{
    if (x < 0 || x > WIDTH - IMG_OBJECT_W ||
            y < 0 || y > HEIGHT - IMG_OBJECT_H || imgId >= OBJECT_TYPES) return;
    uint8_t *pBuffer = arduboy.getBuffer() + (y >> 3) * WIDTH + x;
    uint8_t yOffs = y &= 7;
    uint8_t bit = imgId * IMG_OBJECT_W * IMG_OBJECT_H;
    uint8_t *pImg = record.imgObject + (bit >> 3);
    for (uint8_t i = 0; i < IMG_OBJECT_W; i++, pBuffer++) {
        bit &= 7;
        uint8_t pattern = *pImg >> bit;
        if (bit > 8 - IMG_OBJECT_H) pattern |= *(pImg + 1) << (8 - bit);
        pattern &= (1 << IMG_OBJECT_H) - 1;
        *pBuffer |= pattern << yOffs;
        if (yOffs >= 8 - IMG_OBJECT_H) *(pBuffer + WIDTH) |= pattern >> (8 - yOffs);
        if ((bit += IMG_OBJECT_H) >= 8) pImg++;
    }
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
    arduboy.playScore2(soundTick, 255);
}

void playSoundClick(void)
{
    arduboy.playScore2(soundClick, 255);
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
