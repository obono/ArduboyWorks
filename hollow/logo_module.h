#ifndef __LOGO_MODULE_H__
#define __LOGO_MODULE_H__

#include <Arduboy.h>
#include "module.h"

class LogoModule : public IModule
{
public:
    LogoModule(Arduboy &ab);
    virtual void init();
    virtual bool update();
    virtual void draw();

private:
    Arduboy *ab;
    int count;
};

#endif // __LOGO_MODULE_H__
