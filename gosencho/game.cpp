#include "common.h"
#include "data.h"

/*  Defines  */

#define MONEY_MAX       48

#define SCROLL_MAX      250
#define SCROLL_SHIFT    8
#define PROGRESS_USUAL  96
#define HOLD_COUNT_MAX  ((SCROLL_MAX << SCROLL_SHIFT) - PROGRESS_USUAL)

#define OBJECT_TYPE_NONE    0
#define OBJECT_TYPE_YEN_MIN 1
#define OBJECT_TYPE_YEN_MAX 9
#define OBJECT_TYPE_PLAYER  10

enum STATE_T : uint8_t {
    STATE_PLAYING = 0,
    STATE_MENU,
    STATE_LEAVE,
};

/*  Typedefs  */

typedef struct {
    int16_t x;
    uint8_t y;
    uint8_t type;
} OBJECT_T;

typedef struct {
    uint8_t         w, h;
    uint8_t const   *bitmap;
    uint16_t        value;
    uint8_t const   *sound;
    uint8_t         color;
} OBJECT_INFO_T;

/*  Local Functions  */

static void     handlePlaying(void);
static void     updateMoney(int16_t v);
static void     appendMoney(int16_t v, uint8_t typeMin, uint8_t typeMax, bool isStrict);
static bool     removeMoneyIfNeeded(OBJECT_T &m, int16_t v);
static bool     isColliding(OBJECT_T &obj1, OBJECT_T &obj2, int16_t v = 0);
static void     addScore(uint16_t p);
static void     updateLED(uint8_t type);
static void     kakushiwaza(void);

static void     onContinue(void);
static void     onConfirmReset(void);
static void     onReset(void);
static void     onQuit(void);

static void     drawPlaying(void);
static void     drawObject(OBJECT_T &obj);
static void     drawScore(void);

/*  Local Functions (macros)  */

#define objectWidth(idx)    ((uint8_t) pgm_read_byte(&objectInfo[idx].w))
#define objectHeight(idx)   ((uint8_t) pgm_read_word(&objectInfo[idx].h))
#define objectBitmap(idx)   ((uint8_t *) pgm_read_word(&objectInfo[idx].bitmap))
#define objectValue(idx)    ((uint16_t) pgm_read_word(&objectInfo[idx].value))
#define objectSound(idx)    ((uint8_t *) pgm_read_word(&objectInfo[idx].sound))
#define objectColor(idx)    ((uint8_t) pgm_read_word(&objectInfo[idx].color))
#define colorInfo(r, g, b)  ((r) * 36 + (g) * 6 + (b))

/*  Local Constants  */

PROGMEM static const OBJECT_INFO_T objectInfo[] = {
    { 0, 0,   NULL,        0,     NULL,          0                  }, // none
    { 7, 7,   img1Yen,     1,     sound1Yen,     colorInfo(2, 3, 3) },
    { 7, 7,   img5Yen,     5,     sound5Yen,     colorInfo(4, 4, 0) },
    { 9, 9,   img10Yen,    10,    sound10Yen,    colorInfo(6, 1, 0) },
    { 9, 9,   img50Yen,    50,    sound50Yen,    colorInfo(3, 3, 4) },
    { 9, 9,   img100Yen,   100,   sound100Yen,   colorInfo(3, 4, 3) },
    { 11, 11, img500Yen,   500,   sound500Yen,   colorInfo(4, 3, 3) },
    { 19, 11, img1000Yen,  1000,  sound1000Yen,  colorInfo(2, 2, 5) },
    { 19, 11, img5000Yen,  5000,  sound5000Yen,  colorInfo(3, 1, 4) },
    { 21, 11, img10000Yen, 10000, sound10000Yen, colorInfo(5, 2, 2) },
    { 15, 16, NULL,        0,     NULL,          0                  }, // player
};

/*  Local Variables  */

static STATE_T  state;
static OBJECT_T player, money[MONEY_MAX];
static uint16_t progress, holdCount;
static uint8_t  nextMoneyIdx, capturedMoneyType, ledR, ledG, ledB, ledV;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initGame(void)
{
    ab.playScore(soundStart, SND_PRIO_START);
    record.playCount++;
    isRecordDirty = true;

    for (uint8_t i = 0; i < MONEY_MAX; i++) {
        money[i].type = OBJECT_TYPE_NONE;
    }
    nextMoneyIdx = 0;
    player.x = 8;
    player.y = 27;
    player.type = OBJECT_TYPE_PLAYER;
    counter = 0;
    progress = 0;
    holdCount = 0;
    ledV = 0;

    state = STATE_PLAYING;
    isInvalid = true;
}

MODE_T updateGame(void)
{
    if (state == STATE_LEAVE) return MODE_TITLE;
    (state == STATE_PLAYING) ? handlePlaying() : handleMenu();
    return MODE_GAME;
}

void drawGame(void)
{
    if (state == STATE_LEAVE) return;
    (state == STATE_PLAYING) ? drawPlaying() : drawMenuItems(isInvalid);
    isInvalid = false;
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void handlePlaying(void)
{
    /*  Move the player  */
    player.x += ab.buttonPressed(RIGHT_BUTTON) - ab.buttonPressed(LEFT_BUTTON);
    player.x = constrain(player.x, 0, 113);
    player.y += ab.buttonPressed(DOWN_BUTTON) - ab.buttonPressed(UP_BUTTON);
    player.y = constrain(player.y, 6, 48);
    counter++;

    /*  Scroll and control money  */
    if (ab.buttonPressed(B_BUTTON)) {
        if (holdCount < HOLD_COUNT_MAX) holdCount++;
    } else {
        holdCount = 0;
    }
    progress += holdCount + PROGRESS_USUAL;
    int16_t scroll = progress >> SCROLL_SHIFT;
    capturedMoneyType = OBJECT_TYPE_NONE;
    updateMoney(scroll);
    if (scroll > 0) {
        progress -= scroll << SCROLL_SHIFT;
        uint8_t baseType = (holdCount >> 9) + OBJECT_TYPE_YEN_MIN;
        uint8_t typeMin = constrain(baseType - 10, OBJECT_TYPE_YEN_MIN, OBJECT_TYPE_YEN_MAX);
        uint8_t typeMax = min(baseType, OBJECT_TYPE_YEN_MAX);
        bool isStrict = (scroll < 32);
        while (scroll > 0) {
            appendMoney(scroll, typeMin, typeMax, isStrict);
            scroll -= (isStrict) ? 1 : 4;
        }
    }
    if (capturedMoneyType != OBJECT_TYPE_NONE) {
        ab.playScore(objectSound(capturedMoneyType), SND_PRIO_MONEY);
    }
    updateLED(capturedMoneyType);

    /*  Save and display the menu  */
    if (ab.buttonDown(A_BUTTON)) {
        playSoundClick();
        clearMenuItems();
        addMenuItem(F("CONTINUE"), onContinue);
        addMenuItem(F("BECOME PENNILESS"), onConfirmReset);
        addMenuItem(F("BACK TO TITLE"), onQuit);
        setMenuCoords(10, 23, 107, 17, true, true);
        setMenuItemPos(0);
        writeRecord();
        ab.setRGBled(0, 0, 0);
        state = STATE_MENU;
    }

    kakushiwaza();
    isInvalid = true;
}

static void updateMoney(int16_t v)
{
    for (int i = 0; i < MONEY_MAX; i++) {
        OBJECT_T &m = money[i];
        if (m.type == OBJECT_TYPE_NONE) continue;
        m.x -= v;
        removeMoneyIfNeeded(m, v);
    }
}

static void appendMoney(int16_t v, uint8_t typeMin, uint8_t typeMax, bool isStrict)
{
    OBJECT_T &next = money[nextMoneyIdx];
    if (next.type != OBJECT_TYPE_NONE) return;

    next.type = random(typeMin, typeMax + 1);
    next.x = WIDTH - v + random(8);
    next.y = random(6, HEIGHT - objectHeight(next.type) + 1);
    if (removeMoneyIfNeeded(next, v)) return;

    bool c = false;
    if (isStrict) {
        for (int i = 0; i < MONEY_MAX && !c; i++) {
            OBJECT_T &m = money[i];
            if (i != nextMoneyIdx && m.type != OBJECT_TYPE_NONE) c = isColliding(m, next);
        }
    }
    if (c) {
        next.type = OBJECT_TYPE_NONE;
    } else {
        nextMoneyIdx = circulate(nextMoneyIdx, 1, MONEY_MAX);
    }
}

static bool removeMoneyIfNeeded(OBJECT_T &m, int16_t v)
{
    bool isOut = (-m.x >= objectWidth(m.type));
    if (isOut || isColliding(m, player, v)) {
        addScore(objectValue(m.type));
        if (!isOut) capturedMoneyType = m.type;
        m.type = OBJECT_TYPE_NONE;
        return true;
    }
    return false;
}

static bool isColliding(OBJECT_T &obj1, OBJECT_T &obj2, int16_t v)
{
    return obj2.x - obj1.x < objectWidth(obj1.type) + 1 + v &&
           obj1.x - obj2.x < objectWidth(obj2.type) + 1 &&
           obj2.y - obj1.y < objectHeight(obj1.type) + 1 &&
           obj1.y - obj2.y < objectHeight(obj2.type) + 1;
}

static void addScore(uint16_t p)
{
    record.score[0] += p;
    while (record.score[0] >= 100000000UL) {
        record.score[1]++;
        record.score[0] -= 100000000UL;
    }
    if (record.score[1] >= 100000000UL) {
        record.score[0] = record.score[1] = 99999999UL;
    }
    isRecordDirty = true;
}

static void updateLED(uint8_t type)
{
    if (type != OBJECT_TYPE_NONE) {
        uint8_t c = objectColor(type);
        ledR = c / 36;
        ledG = c / 6 % 6;
        ledB = c % 6;
        ledV = 8;
    } else {
        if (ledV > 0) ledV--;
    }
    ab.setRGBled(ledR * ledV, ledG * ledV, ledB * ledV);
}

static void kakushiwaza(void)
{
    static uint32_t z = 0;
    for (uint8_t i = 2; i <= 7; i++) {
        if (ab.buttonDown(bit(i))) z = z * 6UL % 0346522000UL + i - 2;
    }
    if (z == 0342666371UL) *((long *)&record + 1) = 0x2FAF07F;
}

/*---------------------------------------------------------------------------*/
/*                               Menu Handlers                               */
/*---------------------------------------------------------------------------*/

static void onContinue(void)
{
    playSoundClick();
    state = STATE_PLAYING;
    isInvalid = true;
}

static void onConfirmReset(void)
{
    int16_t y = getMenuItemPos() * 6 + 25;
    setConfirmMenu(y, onReset, onContinue);
}

static void onReset(void)
{
    ab.playScore(soundOver, SND_PRIO_OVER);
    record.score[0] = record.score[1] = 0UL;
    isRecordDirty = true;
    writeRecord();
    state = STATE_PLAYING;
    isInvalid = true;
}

static void onQuit(void)
{
    playSoundClick();
    state = STATE_LEAVE;
}

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void drawPlaying(void)
{
    ab.clear();
    for (int i = 0; i < MONEY_MAX; i++) {
        OBJECT_T &m = money[i];
        if (m.type != OBJECT_TYPE_NONE) drawObject(m);
    }
    ab.drawBitmap(player.x, player.y, imgPlayer[counter / 4 & 3], IMG_PLAYER_W, IMG_PLAYER_H);
    drawScore();
    if (record.score[1] >= 50000000UL) {
        ab.fillRect(0, 24, WIDTH, 15, WHITE);
        if (counter & 16) {
            ab.setTextColor(BLACK);
            ab.printEx(34, 26, F("YOU'VE GOT"));
            ab.printEx(19, 32, F("$5 QUADRILLION!"));
            ab.setTextColor(WHITE);
        }
    }
}

static void drawObject(OBJECT_T &obj)
{
    uint8_t t = obj.type;
    ab.drawBitmap(obj.x, obj.y, objectBitmap(t), objectWidth(t), objectHeight(t));
}

static void drawScore(void)
{
    uint8_t x = 122;
    uint32_t v = record.score[0];
    do {
        ab.printEx(x, 0, (char)('0' + v % 10));
        v /= 10UL;
        x -= 6;
        if (x == 74) v = record.score[1];
    } while (v > 0 || record.score[1] > 0 && x > 74);
    ab.printEx(x, 0, '$');
}
