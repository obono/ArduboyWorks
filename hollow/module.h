#ifndef __MODULE_H__
#define __MODULE_H__

class IModule
{
public:
    virtual void init() = 0;
    virtual bool update() = 0;
    virtual void draw() = 0;
};

#endif // __MODULE_H__

