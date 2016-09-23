#include "common.h"

void initTitle()
{
}

bool updateTitle()
{
    return (arduboy.pressed(B_BUTTON));
}

void drawTitle()
{
    arduboy.clear();
    arduboy.setCursor(0, 0);
    arduboy.print(F("Hollow Seeker\nTitle Screen"));
}
