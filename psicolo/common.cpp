#include "common.h"

/*  Defines  */

#define EEPROM_ADDR_BASE    896
#define EEPROM_SIGNATURE    0x054E424FUL // "OBN\x05"
#define EEPROM_INIT_CHECKSUM ((EEPROM_SIGNATURE & 0xFFFF) + (EEPROM_SIGNATURE >> 16) * 3)

#define EEPROM_ADDR_DATA    (EEPROM_ADDR_BASE + 4)

#define PAD_REPEAT_DELAY    15
#define PAD_REPEAT_INTERVAL 5

#define calcCheckSum32(var, factor)  (((var) & 0xFFFF) * (factor) + ((var) >> 16) * ((factor) + 2))

enum RECORD_T {
    RECORD_NOT_READ = 0,
    RECORD_INITIAL,
    RECORD_STORED,
};

/*  Global Variables  */

MyArduboy   arduboy;
RECORD_T    recordState = RECORD_NOT_READ;
bool        isDirty = false;

uint32_t    playFrames;
uint32_t    shotCount;
uint32_t    bombCount;
uint32_t    killCount;
int8_t      padX, padY, padRepeatCount;

/*  Local Variables  */

PROGMEM static const byte soundTick[] = {
    0x90, 69, 0, 10, 0x80, 0xF0 // arduboy.tone2(440, 10);
};

PROGMEM static const byte soundClick[] = {
    0x90, 74, 0, 20, 0x80, 0xF0 // arduboy.tone2(587, 20);
};

static int16_t  eepAddr;
static uint16_t encPiecesCheckSum;

/*---------------------------------------------------------------------------*/
/*                             Common Functions                              */
/*---------------------------------------------------------------------------*/

void readRecord(void)
{
    bool isVerified = false;
    eepSeek(EEPROM_ADDR_BASE);
    if (eepRead32() == EEPROM_SIGNATURE) {
        uint16_t checkSum = EEPROM_INIT_CHECKSUM;
        for (int i = 0; i < 8; i++) {
            checkSum += eepRead16() * (i * 2 + 5);
        }
        isVerified = (eepRead16() == checkSum);
    }

    if (isVerified) {
        killCount = eepRead32();
        shotCount = eepRead32();
        bombCount = eepRead32();
        playFrames = eepRead32();
        recordState = RECORD_STORED;
        dprintln(F("Read record from EEPROM"));
        dprint(F("playFrames="));
        dprintln(playFrames);
    } else {
        killCount = 0;
        shotCount = 0;
        bombCount = 0;
        playFrames = 0;
        recordState = RECORD_INITIAL;
        isDirty = true;
    }
    setSound(arduboy.audio.enabled()); // Load Sound ON/OFF
}

void writeRecord(void)
{
    if (recordState == RECORD_INITIAL) {
        eepSeek(EEPROM_ADDR_BASE);
        eepWrite32(EEPROM_SIGNATURE);
    } else {
        eepSeek(EEPROM_ADDR_DATA);
    }
    eepWrite32(killCount);
    eepWrite32(shotCount);
    eepWrite32(bombCount);
    eepWrite32(playFrames);

    uint16_t checkSum = EEPROM_INIT_CHECKSUM;
    checkSum += calcCheckSum32(killCount, 5);
    checkSum += calcCheckSum32(shotCount, 9);
    checkSum += calcCheckSum32(bombCount, 13);
    checkSum += calcCheckSum32(playFrames, 17);
    eepWrite16(checkSum);

    arduboy.audio.saveOnOff(); // Save Sound ON/OFF
    recordState = RECORD_STORED;
    isDirty = false;
    dprintln(F("Write record to EEPROM"));
}

void clearRecord(void)
{
    eepSeek(EEPROM_ADDR_BASE);
    for (int i = 0; i < 8; i++) {
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

/*---------------------------------------------------------------------------*/
/*                              Sound Functions                              */
/*---------------------------------------------------------------------------*/

void setSound(bool on)
{
    if (on) {
        arduboy.audio.on();
    } else {
        arduboy.audio.off();
    }
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
