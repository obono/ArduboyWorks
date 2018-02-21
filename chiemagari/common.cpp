#include "common.h"

/*  Defines  */

enum {
    RECORD_NOT_READ = 0,
    RECORD_INITIAL,
    RECORD_STORED,
};

#define EEPROM_ADDR_BASE    864
#define EEPROM_SIGNATURE    0x044E424FUL // "OBN\x04"
#define EEPROM_INIT_CHECKSUM ((EEPROM_SIGNATURE & 0xFFFF) + (EEPROM_SIGNATURE >> 16) * 3)

#define EEPROM_ADDR_PIECES  (EEPROM_ADDR_BASE + 4)
#define EEPROM_SIZE_PIECES  20
#define EEPROM_ADDR_CONFIGS (EEPROM_ADDR_PIECES + EEPROM_SIZE_PIECES)

/*  Global Variables  */

MyArduboy   arduboy;
uint8_t     recordState = RECORD_NOT_READ;
bool        isSoundEnable;
bool        isHelpVisible;
uint8_t     clearCount;
uint32_t    playFrames;

/*  Local Variables  */
static int  eepAddr = EEPROM_STORAGE_SPACE_START;

/*---------------------------------------------------------------------------*/
/*                             Common Functions                              */
/*---------------------------------------------------------------------------*/

void readRecord(void)
{
    bool isVerified = false;
    eepSeek(EEPROM_ADDR_BASE);
    if (eepRead32() == EEPROM_SIGNATURE) {
        uint16_t checkSum = EEPROM_INIT_CHECKSUM;
        for (int i = 0; i < 13; i++) {
            checkSum += eepRead16() * (i * 2 + 5);
        }
        isVerified = (eepRead16() == checkSum);
    }

    if (isVerified) {
        eepSeek(EEPROM_ADDR_CONFIGS);
        isHelpVisible = eepRead8();
        clearCount = eepRead8();
        playFrames = eepRead32();
        recordState = RECORD_STORED;
    } else {
        isHelpVisible = true;
        clearCount = 0;
        playFrames = 0;
        recordState = RECORD_INITIAL;
    }
    setSound(arduboy.audio.enabled()); // Load Sound ON/OFF
}

bool readRecordPieces(uint16_t *pPiece)
{
    bool ret = false;
    if (recordState == RECORD_STORED) {
        eepSeek(EEPROM_ADDR_PIECES);
        eepReadBlock(pPiece, EEPROM_SIZE_PIECES);
        ret = true;
    }
    return ret;
}

void saveRecord(uint16_t *pPiece)
{
    if (recordState == RECORD_INITIAL) {
        eepSeek(EEPROM_ADDR_BASE);
        eepWrite32(EEPROM_SIGNATURE);
    } else {
        eepSeek(EEPROM_ADDR_PIECES);
    }
    eepWriteBlock(pPiece, EEPROM_SIZE_PIECES);
    eepWrite8(isHelpVisible);
    eepWrite8(clearCount);
    eepWrite32(playFrames);

    uint16_t checkSum = EEPROM_INIT_CHECKSUM;
    for (int i = 0; i < 10; i++) {
        checkSum += *pPiece++ * (i * 2 + 5);
    }
    checkSum += (isHelpVisible | clearCount << 8) * 25;
    checkSum += (playFrames & 0xFFFF) * 27 + (playFrames >> 16) * 29;
    eepWrite16(checkSum);

    arduboy.audio.saveOnOff(); // Save Sound ON/OFF
    recordState = RECORD_STORED;
}

void clearRecord(void)
{
    eepSeek(EEPROM_ADDR_BASE);
    for (int i = 0; i < 8; i++) {
        eepWrite32(0);
    }
    recordState = RECORD_INITIAL;
    dprintln("Clean EEPROM");
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
    isSoundEnable = on;
}

void playSoundTick(void)
{
    arduboy.tone2(440, 10);
}

void playSoundClick(void)
{
    arduboy.tone2(587, 20);
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
