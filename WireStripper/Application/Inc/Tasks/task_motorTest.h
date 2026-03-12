//
// Created by Admin on 2/6/2026.
//

#ifndef WIRESTRIPPER_TASK_MOTORTEST_H
#define WIRESTRIPPER_TASK_MOTORTEST_H


void vMotorTestTask();

typedef enum {
    MODE_COMMAND,
    MODE_JOG
} CLIMode;

extern uint8_t gauge_detect;

#endif //WIRESTRIPPER_TASK_MOTORTEST_H