#include "FakeKeyboard.h"
#include "HID.h"

/*  Defines  */

#define IS_ALPHA_WITH_SHIFT     true
#define IS_SPACE_WITH_SHIFT     false
#define IS_IME_SET_MODE_FIRST   false
#define IS_IME_ALPHA_NUM_FULL   true
#define IS_IME_CONTROL_BY_CTRL  false

#define HID_ID          2
#define MODIFIER_NONE   0x00
#define MODIFIER_CTRL   0x01
#define MODIFIER_SHIFT  0x02
#define ALPHA_MODIFIER  ((IS_ALPHA_WITH_SHIFT) ? MODIFIER_SHIFT : MODIFIER_NONE)

#define KEYINFO_SHIFT   0x40

enum : uint8_t {
    ROMAN_VWL_A = 0,
    ROMAN_VWL_I,
    ROMAN_VWL_U,
    ROMAN_VWL_E,
    ROMAN_VWL_O,
    ROMAN_VWL_N, // for 'N'
    ROMAN_VWL_P, // for '-'
    ROMAN_VWL_D, // for ','
};

enum : uint8_t {
    ROMAN_CNS_0 = 0,
    ROMAN_CNS_K,
    ROMAN_CNS_S,
    ROMAN_CNS_T,
    ROMAN_CNS_N,
    ROMAN_CNS_H,
    ROMAN_CNS_M,
    ROMAN_CNS_Y,
    ROMAN_CNS_R,
    ROMAN_CNS_W,
    ROMAN_CNS_G,
    ROMAN_CNS_Z,
    ROMAN_CNS_D,
    ROMAN_CNS_X, // for 'WY'
    ROMAN_CNS_B,
    ROMAN_CNS_P,
};

/*  Typedefs  */

typedef struct {
    char    letter;
    uint8_t info;
} KEYINFO_JP_T;

/*  Local Functions (macros)  */

#define keyInfo(hidCode, isShift)   ((hidCode) | KEYINFO_SHIFT * (isShift))
#define extractHIDCode(info)        ((info) & ~KEYINFO_SHIFT)
#define extractModifier(info)       (((info) & KEYINFO_SHIFT) ? MODIFIER_SHIFT : MODIFIER_NONE)

#define romanInfo(cns, vwl)         (ROMAN_VWL_ ## vwl | ROMAN_CNS_ ## cns << 3)
#define extractVowel(info)          ((info) & 0x07)
#define extractConsonant(info)      ((info) >> 3)

/*  Local Constants  */

PROGMEM static const uint8_t hidReportDescKeyboard[]  = {
    0x05, 0x01, 0x09, 0x06, 0xA1, 0x01, 0x85, 0x02, 0x05, 0x07, 0x19, 0xE0, 0x29, 0xE7, 0x15, 0x00,
    0x25, 0x01, 0x75, 0x01, 0x95, 0x08, 0x81, 0x02, 0x95, 0x01, 0x75, 0x08, 0x81, 0x03, 0x95, 0x06,
    0x75, 0x08, 0x15, 0x00, 0x25, 0xFF, 0x05, 0x07, 0x19, 0x00, 0x29, 0xFF, 0x81, 0x00, 0xC0,
};

PROGMEM static const uint8_t keyInfoTable[] = {
    /*  ASCII  */
    keyInfo(0x2C, false), keyInfo(0x1E, true),  keyInfo(0x34, true),  keyInfo(0x20, true),
    keyInfo(0x21, true),  keyInfo(0x22, true),  keyInfo(0x24, true),  keyInfo(0x34, false),
    keyInfo(0x26, true),  keyInfo(0x27, true),  keyInfo(0x25, true),  keyInfo(0x2E, true),
    keyInfo(0x36, false), keyInfo(0x2D, false), keyInfo(0x37, false), keyInfo(0x38, false),
    keyInfo(0x27, false), keyInfo(0x1E, false), keyInfo(0x1F, false), keyInfo(0x20, false),
    keyInfo(0x21, false), keyInfo(0x22, false), keyInfo(0x23, false), keyInfo(0x24, false),
    keyInfo(0x25, false), keyInfo(0x26, false), keyInfo(0x33, true),  keyInfo(0x33, false),
    keyInfo(0x36, true),  keyInfo(0x2E, false), keyInfo(0x37, true),  keyInfo(0x38, true),
    keyInfo(0x1F, true),  keyInfo(0x04, false), keyInfo(0x05, false), keyInfo(0x06, false),
    keyInfo(0x07, false), keyInfo(0x08, false), keyInfo(0x09, false), keyInfo(0x0A, false),
    keyInfo(0x0B, false), keyInfo(0x0C, false), keyInfo(0x0D, false), keyInfo(0x0E, false),
    keyInfo(0x0F, false), keyInfo(0x10, false), keyInfo(0x11, false), keyInfo(0x12, false),
    keyInfo(0x13, false), keyInfo(0x14, false), keyInfo(0x15, false), keyInfo(0x16, false),
    keyInfo(0x17, false), keyInfo(0x18, false), keyInfo(0x19, false), keyInfo(0x1A, false),
    keyInfo(0x1B, false), keyInfo(0x1C, false), keyInfo(0x1D, false), keyInfo(0x2F, false),
    keyInfo(0x31, false), keyInfo(0x30, false), keyInfo(0x23, true),  keyInfo(0x2D, true),
    /*  Japanese  */
    keyInfo(0x08, false), keyInfo(0x35, false), keyInfo(0x09, false), keyInfo(0x0C, false),
    keyInfo(0x2D, false), keyInfo(0x2E, false), keyInfo(0x16, false), keyInfo(0x04, false),
    keyInfo(0x0F, false), keyInfo(0x1E, false), keyInfo(0x37, false), keyInfo(0x27, true),
    keyInfo(0x27, false), keyInfo(0x17, false), keyInfo(0x26, false), keyInfo(0x14, false),
    keyInfo(0x33, false), keyInfo(0x06, false), keyInfo(0x1D, false), keyInfo(0x36, false),
    keyInfo(0x18, false), keyInfo(0x12, false), keyInfo(0x31, false), keyInfo(0x21, false),
    keyInfo(0x00, false), keyInfo(0x0E, false), keyInfo(0x23, false), keyInfo(0x0B, false),
    keyInfo(0x24, false), keyInfo(0x0D, false), keyInfo(0x34, false), keyInfo(0x1F, false),
    keyInfo(0x05, false), keyInfo(0x22, false), keyInfo(0x1A, false), keyInfo(0x20, false),
    keyInfo(0x1B, false), keyInfo(0x0A, false), keyInfo(0x25, false), keyInfo(0x38, false),
    keyInfo(0x11, false), keyInfo(0x07, false), keyInfo(0x00, false), keyInfo(0x19, false),
    keyInfo(0x10, false), keyInfo(0x13, false), keyInfo(0x15, false), keyInfo(0x1C, false),
    keyInfo(0x2F, false), keyInfo(0x30, false), keyInfo(0x2D, true),  keyInfo(0x36, true),
};

PROGMEM static const KEYINFO_JP_T keyInfoTableJP[] = {
    /*  ASCII  */
    { '"',  keyInfo(0x1F, true)  }, { '&',  keyInfo(0x23, true)  }, { '\'', keyInfo(0x24, true)  },
    { '(',  keyInfo(0x25, true)  }, { ')',  keyInfo(0x26, true)  }, { '*',  keyInfo(0x34, true)  },
    { '+',  keyInfo(0x33, true)  }, { ':',  keyInfo(0x34, false) }, { '=',  keyInfo(0x2D, true)  },
    { '@',  keyInfo(0x2F, false) }, { '^',  keyInfo(0x2E, false) }, { '_',  keyInfo(0x87, true)  },
    /*  Japanese  */
    { 0x61, keyInfo(0x87, false) }, { 0x76, keyInfo(0x32, false) }, { 0x92, keyInfo(0x89, false) }, 
};

PROGMEM static const uint8_t romanInfoTable[] = {
    romanInfo(0, I), romanInfo(R, O), romanInfo(H, A), romanInfo(N, I), romanInfo(H, O),
    romanInfo(H, E), romanInfo(T, O), romanInfo(T, I), romanInfo(R, I), romanInfo(N, U),
    romanInfo(R, U), romanInfo(W, O), romanInfo(W, A), romanInfo(K, A), romanInfo(Y, O),
    romanInfo(T, A), romanInfo(R, E), romanInfo(S, O), romanInfo(T, U), romanInfo(N, E),
    romanInfo(N, A), romanInfo(R, A), romanInfo(M, U), romanInfo(0, U), romanInfo(X, I),
    romanInfo(N, O), romanInfo(0, O), romanInfo(K, U), romanInfo(Y, A), romanInfo(M, A),
    romanInfo(K, E), romanInfo(H, U), romanInfo(K, O), romanInfo(0, E), romanInfo(T, E),
    romanInfo(0, A), romanInfo(S, A), romanInfo(K, I), romanInfo(Y, U), romanInfo(M, E),
    romanInfo(M, I), romanInfo(S, I), romanInfo(X, E), romanInfo(H, I), romanInfo(M, O),
    romanInfo(S, E), romanInfo(S, U), romanInfo(N, N), 0, 0, romanInfo(0, P), romanInfo(0, D),
};

PROGMEM static const char romanConsonantTable[] = "0KSTNHMYRWGZDXBP";
PROGMEM static const char romanVowelTable[] = "AIUEON-,";

/*---------------------------------------------------------------------------*/

FakeKeyboard::FakeKeyboard()
{
    static HIDSubDescriptor node(hidReportDescKeyboard, sizeof(hidReportDescKeyboard));
    HID().AppendDescriptor(&node);
    memset(&keyReport, 0x00, sizeof(keyReport));
}

void FakeKeyboard::reset(uint8_t mode, uint8_t keyboard, uint8_t imeMode)
{
    isJapanese = (mode == DECODE_MODE_JA);
    isKeyboardJP = (keyboard == KEYBOARD_JP);
    isRoman = (imeMode == IME_MODE_ROMAN);
    activated();
}

void FakeKeyboard::activated(void)
{
    isFirstLetter = true;
    lastLetter = '\0';
}

void FakeKeyboard::sendLetter(char letter)
{
    if (IS_IME_SET_MODE_FIRST && isFirstLetter) {
        sendSetModeKey();
        isFirstLetter = false;
    }
    if (isJapanese) {
        if (isKana(lastLetter) && !isKana(letter) ||
                isGraph(lastLetter) && isNonGraph(letter)) sendConfirmKey();
        bool ret = (isRoman && isKana(letter)) ?
                sendRomanKeys(letter) : sendKey(letter, ALPHA_MODIFIER);
        if (ret) {
            if (isGraph(letter)) sendAlphaNumKey();
            lastLetter = letter;
        }
    } else {
        if (sendKey(letter, ALPHA_MODIFIER)) lastLetter = letter;
    }
}

bool FakeKeyboard::sendKey(uint8_t letter, uint8_t alphaModifier)
{
    uint8_t hidCode, modifiers;
    if (resolveHIDCode(letter, hidCode, modifiers)) {
        if (isAlpha(letter)) modifiers |= alphaModifier;
        if (IS_SPACE_WITH_SHIFT && letter == ' ') modifiers |= MODIFIER_SHIFT;
        sendKeyStroke(hidCode, modifiers);
        return true;
    }
    return false;
}

bool FakeKeyboard::sendRomanKeys(uint8_t letter)
{
    bool ret = false;
    uint8_t vowel, consonant;
    if (letter == 0x90 || letter == 0x91) {
        if (resolveRomanInfo(lastLetter, vowel, consonant) && (consonant == ROMAN_CNS_H ||
                letter == 0x90 && consonant >= ROMAN_CNS_K && consonant <= ROMAN_CNS_T)) {
            consonant += ROMAN_CNS_G - ROMAN_CNS_K + (letter == 0x91);
            sendKey('\b');
            ret = true;
        }
    } else {
        ret = resolveRomanInfo(letter, vowel, consonant);
    }

    if (ret) {
        if (consonant == ROMAN_CNS_X) {
            sendKey('W');
            sendKey('Y');
        } else if (consonant >= ROMAN_CNS_K) {
            sendKey(pgm_read_byte(&romanConsonantTable[consonant]));
        }
        sendKey(pgm_read_byte(&romanVowelTable[vowel]));
    }
    return ret;
}

void FakeKeyboard::sendSetModeKey(void)
{
    if (isJapanese) {
        if (isKeyboardJP) {
            sendKeyStroke(0x88, MODIFIER_SHIFT); // KATAKANA key
        } else {
            // no default key assignment for KATAKANA mode
        }
    } else {
        // no default key assignment for ALPHA-NUM mode
    }
}

void FakeKeyboard::sendConfirmKey(void)
{
    if (IS_IME_CONTROL_BY_CTRL) {
        sendKey('M', MODIFIER_CTRL);
    } else {
        sendKey('\n');
    }
}

void FakeKeyboard::sendAlphaNumKey(void)
{
    if (IS_IME_CONTROL_BY_CTRL) {
        sendKey((IS_IME_ALPHA_NUM_FULL) ? 'P' : 'T', MODIFIER_CTRL);
    } else {
        sendKeyStroke((IS_IME_ALPHA_NUM_FULL) ? 0x42 : 0x43, MODIFIER_NONE); // F9 or F10
    }
}

void FakeKeyboard::sendKeyStroke(uint8_t hidCode, uint8_t modifiers)
{
    keyReport.keys[0] = hidCode;
    keyReport.modifiers = modifiers;
    HID().SendReport(HID_ID, &keyReport, sizeof(keyReport)); // press
    keyReport.keys[0] = 0x00;
    keyReport.modifiers = MODIFIER_NONE;
    HID().SendReport(HID_ID, &keyReport, sizeof(keyReport)); // release
}

bool FakeKeyboard::resolveHIDCode(uint8_t letter, uint8_t &hidCode, uint8_t &modifiers)
{
    uint8_t info = keyInfo(0x00, false);
    if (letter == '\b') {
        info = keyInfo(0x2A, false);
    } else if (letter == '\n') {
        info = keyInfo(0x28, false);
    } else if (isPrint(letter) || isKana(letter)) {
        if (isKeyboardJP) {
            const KEYINFO_JP_T *p = keyInfoTableJP;
            uint8_t loopCount = 0;
            if (letter <= '@') {
                loopCount = 10;
            } else if (letter >= '^') {
                p = &keyInfoTableJP[10];
                loopCount = 5;
            }
            while (loopCount--) {
                if (letter == pgm_read_byte(&p->letter)) {
                    info = pgm_read_byte(&p->info);
                    break;
                }
                p++;
            }
        }
        if (!info) info = pgm_read_byte(&keyInfoTable[letter - ' ']);
    }

    if (info) {
        hidCode = extractHIDCode(info);
        modifiers = extractModifier(info);
        return true;
    }
    return false;
}

bool FakeKeyboard::resolveRomanInfo(uint8_t letter, uint8_t &vowel, uint8_t &consonant)
{
    if (isKana(letter)) {
        uint8_t info = pgm_read_byte(&romanInfoTable[letter - 0x60]);
        vowel = extractVowel(info);
        consonant = extractConsonant(info);
        return true;
    }
    return false;
}
