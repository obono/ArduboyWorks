#include <Arduboy.h>
#include "logo_module.h"
#include "title_module.h"
#include "game_module.h"

Arduboy    arduboy;
LogoModule  logoModule(arduboy);
TitleModule titleModule(arduboy);
GameModule  gameModule(arduboy);
IModule     *pCurModule;

void setup()
{
    arduboy.begin();
    arduboy.setFrameRate(60);
    logoModule.init();
    pCurModule = &logoModule;
}

void loop()
{
    if (!(arduboy.nextFrame())) return;
    bool isDone = pCurModule->update();
    pCurModule->draw();
    arduboy.display();
    if (isDone) {
        if (pCurModule == &titleModule) {
            pCurModule = &gameModule;
        } else {
            pCurModule = &titleModule;
        }
        pCurModule->init();
    }
}

