#ifndef __GAME_MODULE_H__
#define __GAME_MODULE_H__

#include <Arduboy.h>
#include "module.h"
#include "column.h"

class GameModule : public IModule
{
public:
    GameModule(Arduboy &ab);
    virtual void init();
    virtual bool update();
    virtual void draw();

private:
    bool isHollow();
    Arduboy *ab;
    Column column[18];
    int columnOffset;
    int wavePhase;
    int caveMaxGap;
    int caveGap;
    int score;
    int px, pc, pa, pd, pj;
    int cm;
};

#endif // __GAME_MODULE_H__

