#include "common.h"

/*  Typedefs  */

typedef struct {
    void(*initFunc)(void);
    MODE_T(*updateFunc)(void);
    void(*drawFunc)(void);
} MODULE_FUNCS;

/*  Local Functions (macros)  */

#define callInitFunc(idx)   ((void (*)(void)) pgm_read_word(&moduleTable[idx].initFunc))()
#define callUpdateFunc(idx) ((MODE_T (*)(void)) pgm_read_word(&moduleTable[idx].updateFunc))()
#define callDrawFunc(idx)   ((void (*)(void)) pgm_read_word(&moduleTable[idx].drawFunc))()

/*  Local Constants  */

PROGMEM static const MODULE_FUNCS moduleTable[] = {
    { initLogo,     updateLogo,     drawLogo },
    { initTitle,    updateTitle,    drawTitle },
    { initGame,     updateGame,     drawGame },
};

/*  Local Variables  */

static MODE_T mode;

/*---------------------------------------------------------------------------*/

void setup()
{
    ab.beginNoLogo();
    ab.setFrameRate(FPS);
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
