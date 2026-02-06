//
// Created by Admin on 11/22/2025.
//

#ifndef WIRESTRIPPER_TASK_STATEMACHINE_H
#define WIRESTRIPPER_TASK_STATEMACHINE_H

void vStateMachineTask();

typedef enum {
    CHECKS,
    NONE,
    ENGAGE,
    DISENGAGE,
    ENGAGED,
    JOB_RUNNING,
    SAFETY_ERROR
} SystemStatus;

extern SystemStatus systemState;
extern volatile uint8_t stop_button;
extern volatile uint8_t go_button;
extern volatile uint8_t safetyOK;
extern volatile uint8_t job_finish;

#endif //WIRESTRIPPER_TASK_STATEMACHINE_H