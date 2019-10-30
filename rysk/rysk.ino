#include "MyArduboy.h"

#if ARDUBOY_LIB_VER != ARDUBOY_LIB_VER_TGT
#error Unexpected version of Arduboy Library
#endif // It may work even if you use other version. So comment out the above line.

/*  Defines  */

//#define DEBUG
#define FPS             60
#define APP_TITLE       "RYSK"
#define APP_CODE        "OBN-Y00"
#define APP_VERSION     "0.02"
#define APP_RELEASED    "NOVEMBER 2019"

#define COOKIES     8
#define ANIM_MAX    16

#define PL_SIZE     4
#define CK_SIZE     2
#define SCALE       64
#define WIDTH_S     (WIDTH * SCALE)
#define HEIGHT_S    (HEIGHT * SCALE)
#define PL_SIZE_S   (PL_SIZE * SCALE)
#define CK_SIZE_S   (CK_SIZE * SCALE)
#define HIT_DIFF_S  PL_SIZE_S
#define NEW_DIFF_S  (PL_SIZE_S * 4)

/*  Functions  */

void initialize(void);
void update(void);
void movePlayer(void);
void newCookie(uint8_t idx);
void draw(void);

/*  Variables  */

PROGMEM static const uint8_t imgPlayer[9][8] = { // 8x8 x9
    { 0xFE, 0xF2, 0xDE, 0xDE, 0xF2, 0xFE, 0xFE, 0xFE },
    { 0xFE, 0xFE, 0xF2, 0xDE, 0xDE, 0xF2, 0xFE, 0xFE },
    { 0xFE, 0xFE, 0xFE, 0xF2, 0xDE, 0xDE, 0xF2, 0xFE },
    { 0xFE, 0xE2, 0xBE, 0xBE, 0xE2, 0xFE, 0xFE, 0xFE },
    { 0xFE, 0xFE, 0xE2, 0xBE, 0xBE, 0xE2, 0xFE, 0xFE },
    { 0xFE, 0xFE, 0xFE, 0xE2, 0xBE, 0xBE, 0xE2, 0xFE },
    { 0xFE, 0xC6, 0xBE, 0xBE, 0xC6, 0xFE, 0xFE, 0xFE },
    { 0xFE, 0xFE, 0xC6, 0xBE, 0xBE, 0xC6, 0xFE, 0xFE },
    { 0xFE, 0xFE, 0xFE, 0xC6, 0xBE, 0xBE, 0xC6, 0xFE },
};

PROGMEM static const byte soundStep[] = {
    0x90, 60, 0, 10, 0x80, 0xF0
};

PROGMEM static const byte soundCapture[] = {
    0x90, 80, 0, 20, 0x90, 84, 0, 40, 0x90, 88, 0, 40, 0x80, 0xF0
};

MyArduboy   arduboy;
int16_t     px, py;
int16_t     cx[COOKIES], cy[COOKIES];
int8_t      cvx[COOKIES], cvy[COOKIES];
uint16_t    score;
uint8_t     pc, pa;

/*  For Debugging  */

#ifdef DEBUG
bool    dbgPrintEnabled = true;
char    dbgRecvChar = '\0';

#define dprint(...)     (!dbgPrintEnabled || Serial.print(__VA_ARGS__))
#define dprintln(...)   (!dbgPrintEnabled || Serial.println(__VA_ARGS__))

static void dbgCheckSerialRecv(void) {
    int recv;
    while ((recv = Serial.read()) != -1) {
        switch (recv) {
        case 'd':
            dbgPrintEnabled = !dbgPrintEnabled;
            Serial.print("Debug output ");
            Serial.println(dbgPrintEnabled ? "ON" : "OFF");
            break;
        case 'r':
            //clearRecord();
            break;
        }
        if (recv >= ' ' && recv <= '~') {
            dbgRecvChar = recv;
        }
    }
}
#else
#define dprint(...)
#define dprintln(...)
#endif

/*---------------------------------------------------------------------------*/

void setup()
{
#ifdef DEBUG
    Serial.begin(115200);
#endif
    arduboy.beginNoLogo();
    arduboy.setFrameRate(FPS);
    initialize();
}

void loop()
{
#ifdef DEBUG
    dbgCheckSerialRecv();
#endif
    if (!(arduboy.nextFrame())) {
        return;
    }
    update();
    draw();
    arduboy.display();
}

/*---------------------------------------------------------------------------*/

void initialize(void)
{
    px = WIDTH_S / 2;
    py = HEIGHT_S / 2;
    pc = 4;
    pa = 0;
    for (uint8_t idx = 0; idx < COOKIES; idx++) {
        newCookie(idx);
    }
    score = 0;
    arduboy.setAudioEnabled(true);
}

void update(void)
{
    movePlayer();
    for (uint8_t idx = 0; idx < COOKIES; idx++) {
        if (cx[idx] + cvx[idx] < CK_SIZE_S || cx[idx] + cvx[idx] >= WIDTH_S - CK_SIZE_S) cvx[idx] = -cvx[idx];
        if (cy[idx] + cvy[idx] < CK_SIZE_S || cy[idx] + cvy[idx] >= HEIGHT_S - CK_SIZE_S) cvy[idx] = -cvy[idx];
        cx[idx] += cvx[idx];
        cy[idx] += cvy[idx];
        if (abs(px - cx[idx]) < HIT_DIFF_S && abs(py - cy[idx]) < HIT_DIFF_S) {
            score++;
            arduboy.playScore2(soundCapture, 0);
            newCookie(idx);
        }
    }
}

void movePlayer(void)
{
    /*  Move  */
    int8_t vx = 0, vy = 0;
    if (arduboy.buttonPressed(LEFT_BUTTON))  vx--;
    if (arduboy.buttonPressed(RIGHT_BUTTON)) vx++;
    if (arduboy.buttonPressed(UP_BUTTON))    vy--;
    if (arduboy.buttonPressed(DOWN_BUTTON))  vy++;
    pc = (vy + 1) * 3 + (vx + 1);
    int8_t vs = (vx != 0 && vy != 0) ? (SCALE * 3 / 8) : (SCALE / 2);
    vx *= vs;
    vy *= vs;
    if (px + vx >= PL_SIZE_S && px + vx < WIDTH_S - PL_SIZE_S) px += vx;
    if (py + vy >= PL_SIZE_S && py + vy < HEIGHT_S - PL_SIZE_S) py += vy;

    /*  Animation  */
    if (pc == 4) {
        pa = 0;
    } else {
        if (++pa >= ANIM_MAX) {
            arduboy.playScore2(soundStep, 1);
            pa = 0;
        }
    }
}

void newCookie(uint8_t idx)
{
    do {
        cx[idx] = random(CK_SIZE_S, WIDTH_S - CK_SIZE_S);
        cy[idx] = random(CK_SIZE_S, HEIGHT_S - CK_SIZE_S);
    } while (abs(px - cx[idx]) < NEW_DIFF_S && abs(py - cy[idx]) < NEW_DIFF_S);
    float d = random(2048) * PI / 1024.0;
    cvx[idx] = (int8_t) (cos(d) * (float) SCALE);
    cvy[idx] = (int8_t) (sin(d) * (float) SCALE);
}

void draw(void)
{
    arduboy.clear();
    arduboy.setCursor(0, 0);
    arduboy.print(score);
    int16_t dx = px / SCALE - PL_SIZE;
    int16_t dy = py / SCALE - PL_SIZE - (pa >= ANIM_MAX / 2);
    arduboy.drawBitmap(dx, dy, imgPlayer[pc], PL_SIZE * 2, PL_SIZE * 2, WHITE);
    for (uint8_t idx = 0; idx < COOKIES; idx++) {
        dx = cx[idx] / SCALE - CK_SIZE;
        dy = cy[idx] / SCALE - CK_SIZE;
        arduboy.drawRect2(dx, dy, CK_SIZE * 2, CK_SIZE * 2, WHITE);
    }
}
