#include "common.h"

/*  Defines  */

/*  Typedefs  */

/*  Local Functions  */

/*  Local Variables  */

static bool     toDraw;
static uint8_t  patternId;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initGallery(void)
{
    patternId = clearCount - 1;
    padRepeatCount = 0;
    toDraw = true;
}

MODE_T updateGallery(void)
{
    MODE_T ret = MODE_GALLERY;
    handleDPad();
    if (arduboy.buttonDown(A_BUTTON)) {
        playSoundClick;
        ret = MODE_MENU;
    } else if (padX != 0 && patternId + padX >= 0 && patternId + padX < clearCount) {
        playSoundTick();
        patternId += padX;
        toDraw = true;
    }
    return ret;
}

void drawGallery(void)
{
    if (toDraw) {
        arduboy.printEx(6, 6, F("GALLERY "));
        arduboy.print(patternId + 1);
        arduboy.print(F("/"));
        arduboy.print(clearCount);
        toDraw = false;
    }
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

