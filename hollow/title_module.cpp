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
    return (ab->pressed(A_BUTTON) || ab->pressed(B_BUTTON));
}

void TitleModule::draw()
{
    ab->clear();
    ab->setCursor(0, 0);
    ab->print(F("Hollow Seeker\nTitle Screen"));
}

