#include "title_module.h"

TitleModule::TitleModule(Arduboy &ab)
{
    this->ab = &ab;
}

void TitleModule::init()
{
}

bool TitleModule::update()
{
    return (ab->pressed(B_BUTTON));
}

void TitleModule::draw()
{
    ab->clear();
    ab->setCursor(4, 9);
    ab->print(F("Title Screen"));
}

