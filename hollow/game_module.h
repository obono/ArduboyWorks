#ifndef __GAME_MODULE_H__
#define __GAME_MODULE_H__

#include <Arduboy.h>
#include "module.h"

class GameModule : public IModule
{
public:
    GameModule(Arduboy &ab);
    virtual void init();
    virtual bool update();
    virtual void draw();

private:
    Arduboy *ab;
    int px, py;
};

#endif // __GAME_MODULE_H__
