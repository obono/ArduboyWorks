#include "column.h"
#include <Arduboy.h>

void Column::set(int top, int bottom)
{
    this->top = top;
    this->bottom = bottom;
}

void Column::grow(bool isSpace, Column &lastColumn)
{
    int lastDiff = lastColumn.bottom - lastColumn.top;
    int curDiff = rand() % 3;
    if (isSpace) curDiff = 8 - curDiff;
    int adjust = (lastColumn.bottom - 34) / 5;
    int change = rand() % (17 - abs(curDiff - lastDiff) - abs(adjust)) - 8;
    if (curDiff > lastDiff) change += curDiff - lastDiff;
    if (adjust < 0) change -= adjust;
    int bottom = lastColumn.bottom + change;
    if (bottom < 16) bottom = 16;
    if (bottom > 56) bottom = 56;
    set(bottom - curDiff, bottom); 
}

