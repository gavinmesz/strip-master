#ifndef TASK_DISPLAY_H
#define TASK_DISPLAY_H

#include "../User/Config/DEV_Config.h"

extern int COUNTER_VAR;
extern UWORD Imagesize;
extern UBYTE *BlackImage;

int initDisplay();
void vDisplayTask();

#endif
