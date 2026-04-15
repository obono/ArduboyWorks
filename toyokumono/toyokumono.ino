#include "common.h"

/*  Typedefs  */

typedef struct {
    void(*initFunc)(void);
    MODE_T(*updateFunc)(void);
    void(*drawFunc)(void);
} MODULE_FUNCS;

/*  Local Constants  */

PROGMEM static const MODULE_FUNCS moduleTable[] = {
    { initLogo,     updateLogo,     drawLogo },
    { initTitle,    updateTitle,    drawTitle },
    { initGame,     updateGame,     drawGame },
};

/*  Local Functions (macros)  */

#define callInitFunc(idx)   ((void (*)(void)) pgm_read_word((uint16_t) &moduleTable[idx].initFunc))()
#define callUpdateFunc(idx) ((MODE_T (*)(void)) pgm_read_word((uint16_t) &moduleTable[idx].updateFunc))()
#define callDrawFunc(idx)   ((void (*)(void)) pgm_read_word((uint16_t) &moduleTable[idx].drawFunc))()

/*  Local Variables  */

static MODE_T mode;

/*---------------------------------------------------------------------------*/

void setup()
{
    ab.beginNoLogo();
    ab.setFrameRate(FPS);
    //ab.setTextColors(WHITE, WHITE);
    ab.initAudio(1);
    readRecord();
    callInitFunc(mode);
}

void loop()
{
    if (!(ab.nextFrame())) return;
    handleDPad();
    MODE_T nextMode = callUpdateFunc(mode);
    callDrawFunc(mode);
    ab.display();
    if (mode != nextMode) {
        mode = nextMode;
        callInitFunc(mode);
    }
}
