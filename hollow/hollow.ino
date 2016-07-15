#include "common.h"

typedef struct {
    void (*initFunc)();
    bool (*updateFunc)();
    void (*drawFunc)();
} MODULE_FUNCS;

enum MODE {
    LOGO_MODE = 0,
    TITLE_MODE,
    GAME_MODE
};

static const MODULE_FUNCS moduleTable[] = {
    { initLogo,  updateLogo,  drawLogo  }, 
    { initTitle, updateTitle, drawTitle }, 
    { initGame,  updateGame,  drawGame  }, 
};

Arduboy arduboy;

static MODE mode = LOGO_MODE;

void setup()
{
    arduboy.begin();
    arduboy.setFrameRate(60);
    moduleTable[LOGO_MODE].initFunc();
}

void loop()
{
    if (!(arduboy.nextFrame())) return;
    bool isDone = moduleTable[mode].updateFunc();
    moduleTable[mode].drawFunc();
    arduboy.display();
    if (isDone) {
        mode = (mode == TITLE_MODE) ? GAME_MODE : TITLE_MODE;
        moduleTable[mode].initFunc();
    }
}

