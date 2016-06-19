#include "game_module.h"
#include "images.h"

GameModule::GameModule(Arduboy &ab)
{
    this->ab = &ab;
}

void GameModule::init()
{
    px = 64;
    py = 32;
}

bool GameModule::update()
{
    if (ab->pressed(UP_BUTTON)) {
        py--;
    }
    if (ab->pressed(DOWN_BUTTON)) {
        py++;
    }
    if (ab->pressed(LEFT_BUTTON)) {
        px--;
    }
    if (ab->pressed(RIGHT_BUTTON)) {
        px++;
    }
    return false;
}

void GameModule::draw()
{
    ab->clear();
    ab->drawRect(px - 3, py - 3, 7, 7, WHITE);
}

