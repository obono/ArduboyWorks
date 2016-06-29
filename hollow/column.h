#ifndef __COLUMN_H__
#define __COLUMN_H__

class Column
{
public:
    void set(int top, int bottom);
    void grow(bool isSpace, Column &lastColumn);
    int top;
    int bottom;
private:
};

#endif // __GAME_MODULE_H__
