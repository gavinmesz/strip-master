#ifndef TASK_DISPLAY_H
#define TASK_DISPLAY_H

int initDisplay();
void vDisplayTask();
extern int quantity;
extern int length;
extern int stripLength;
extern int stripCut; //Strip or strip and cut (1=Strip and cut)
extern uint32_t adcVals1[2];
extern uint32_t adcVals2[1];
extern uint32_t adcVals3[2];

#endif
