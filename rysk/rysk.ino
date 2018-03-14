#include "MyArduboy.h"

#if ARDUBOY_LIB_VER != ARDUBOY_LIB_VER_TGT
#error Unexpected version of Arduboy Library
#endif // It may work even if you use other version. So comment out the above line.

/*  Defines  */

//#define DEBUG
#define APP_VERSION     "0.01"

/*  Functions  */

void initialize(void);
void update(void);
void movePlayer(void);
void newCookie(void);
void controlLED(void);
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
int16_t     dx, dy;
uint16_t    score;
uint8_t     pc, pa;

/*  For Debugging  */

#ifdef DEBUG
bool    dbgPrintEnabled = true;
char    dbgRecvChar = '\0';
uint8_t dbgCaptureMode = 0;

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
        case 'c':
            dbgCaptureMode = 1;
            break;
        case 'C':
            dbgCaptureMode = 2;
            break;
        case 'r':
            clearRecord();
            break;
        }
        if (recv >= ' ' && recv <= '~') {
            dbgRecvChar = recv;
        }
    }
}

static void dbgScreenCapture() {
    if (dbgCaptureMode) {
        Serial.write((const uint8_t *)arduboy.getBuffer(), WIDTH * HEIGHT / 8);
        if (dbgCaptureMode == 1) {
            dbgCaptureMode = 0;
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
    arduboy.setFrameRate(60);
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
#ifdef DEBUG
    dbgScreenCapture();
    dbgRecvChar = '\0';
#endif
    arduboy.display();
}

/*---------------------------------------------------------------------------*/

void initialize(void)
{
    px = 128;
    py = 64;
    pc = 4;
    pa = 0;
    newCookie();
    score = 0;
    arduboy.setAudioEnabled(true);
}

void update(void)
{
    movePlayer();
    if (abs(px - dx) < 12 && abs(py - dy) < 12) {
        score++;
        arduboy.playScore2(soundCapture, 0);
        newCookie();
    }
    controlLED();
}

void movePlayer(void)
{
    /*  Move  */
    int vx = 0, vy = 0;
    if (arduboy.buttonPressed(LEFT_BUTTON))  vx--;
    if (arduboy.buttonPressed(RIGHT_BUTTON)) vx++;
    if (arduboy.buttonPressed(UP_BUTTON))    vy--;
    if (arduboy.buttonPressed(DOWN_BUTTON))  vy++;
    if (px + vx >= 8 && px + vx < 248) px += vx;
    if (py + vy >= 8 && py + vy < 120) py += vy;

    /*  Animation  */
    pc = (vy + 1) * 3 + (vx + 1);
    if (pc == 4) {
        pa = 0;
    } else {
        if (++pa >= 16) {
            arduboy.playScore2(soundStep, 1);
            pa = 0;
        }
    }
}

void newCookie(void)
{
    do {
        dx = random(4, 252);
        dy = random(4, 124);
    } while (abs(px - dx) < 32 && abs(py - dy) < 32);
}

void controlLED(void)
{
    uint8_t r, g, b;
    if (arduboy.buttonPressed(B_BUTTON)) {
        if (px < 128) {
            r = px / 2;
            g = 63;
        } else {
            r = 63;
            g = (255 - px) / 2;
        }
        b = py / 2;
    } else {
        r = b = g = 0;
    }
    arduboy.setRGBled(r, g, b);
}

void draw(void)
{
    arduboy.clear();
    arduboy.setCursor(0, 0);
    arduboy.print(score);
    arduboy.drawBitmap(px / 2 - 4, py / 2 - 4 - (pa >= 8), imgPlayer[pc], 8, 8, WHITE);
    arduboy.drawRect2(dx / 2 - 2, dy / 2 - 2, 4, 4, WHITE);
}
