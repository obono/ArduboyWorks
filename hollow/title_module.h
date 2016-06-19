#ifndef __TITLE_MODULE_H__
#define __TITLE_MODULE_H__

#include <Arduboy.h>
#include "module.h"

class TitleModule : public IModule
{
public:
    TitleModule(Arduboy &ab);
    virtual void init();
    virtual bool update();
    virtual void draw();

private:
    Arduboy *ab;
};

#endif // __TITLE_MODULE_H__
