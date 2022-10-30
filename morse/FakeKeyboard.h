#pragma once

#include "common.h"

/*  Typedefs  */

typedef struct {
    uint8_t modifiers;
    uint8_t reserved;
    uint8_t keys[6];
} KEY_REPORT_T;

/*---------------------------------------------------------------------------*/

class FakeKeyboard
{
public:
    FakeKeyboard();
    void    reset(uint8_t mode, uint8_t keyboard, uint8_t imeMode);
    void    activated(void);
    void    sendLetter(char letter);

private:
    bool    sendKey(uint8_t letter, uint8_t alphaModifier = 0x00);
    bool    sendRomanKeys(uint8_t letter);
    void    sendSetModeKey(void);
    void    sendConfirmKey(void);
    void    sendAlphaNumKey(void);
    void    sendKeyStroke(uint8_t hidCode, uint8_t modifiers);
    bool    resolveHIDCode(uint8_t letter, uint8_t &hidCode, uint8_t &modifier);
    bool    resolveRomanInfo(uint8_t letter, uint8_t &vowel, uint8_t &consonant);

    KEY_REPORT_T keyReport;
    uint8_t lastLetter;
    bool    isJapanese, isKeyboardJP, isRoman, isFirstLetter;
};
