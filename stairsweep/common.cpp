#include "common.h"

/*  Defines  */

#define EEPROM_ADDR_BASE    992
#define EEPROM_SIGNATURE    0x084E424FUL // "OBN\x08"

enum RECORD_STATE_T {
    RECORD_NOT_READ = 0,
    RECORD_INITIAL,
    RECORD_STORED,
};

/*  Global Variables  */

MyArduboyV  arduboy;
RECORD_T    record;
bool        isRecordDirty;
uint16_t    lastScore;
bool        isInvalid;

/*  Local Functions  */

static uint16_t calcCheckSum(void);
static void     printColonAnd2Digits(uint8_t value);

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

static uint16_t calcCheckSum(void)
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

bool enterScore(uint16_t score)
{
    int r = 10;
    uint16_t h;
    while (r > 0 && (h = record.hiscore[r - 1]) < score) {
        if (--r < 9) record.hiscore[r + 1] = h;
    }
    if (r < 10) record.hiscore[r] = score;
    lastScore = score;
    return (r == 0);
}

void drawNumber(int16_t x, int16_t y, int16_t digits, int32_t value)
{
    if (digits > 0) digits--;
    for (int32_t tmp = value; digits > 0 && tmp >= 10; tmp /= 10, digits--) { ; }
    arduboy.setCursor(x, y - digits * 6);
    arduboy.print(value);
}

void drawTime(int16_t x, int16_t y, uint32_t frames)
{
    arduboy.setCursor(x, y);
    arduboy.print(frames / (FPS * 3600UL));
    printColonAnd2Digits(frames / (FPS * 60) % 60);
    printColonAnd2Digits(frames / FPS % 60);
}

static void printColonAnd2Digits(uint8_t value)
{
    arduboy.print(':');
    if (value < 10) arduboy.print('0');
    arduboy.print(value);
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
