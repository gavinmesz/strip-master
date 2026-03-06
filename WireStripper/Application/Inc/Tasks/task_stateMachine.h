//
// Created by Admin on 11/22/2025.
//

#ifndef WIRESTRIPPER_TASK_STATEMACHINE_H
#define WIRESTRIPPER_TASK_STATEMACHINE_H

#define WIRE_IN_DETECT &adcVals3[0]
#define WIRE_END_DETECT &adcVals3[1]
#define WIRE_DETECT_LOW_THRES 0.5
#define WIRE_DETECT_HIGH_THRES 1

void vStateMachineTask();

typedef enum {
    CHECKS,
    NONE,
    ENGAGE,
    ENGAGING,
    DISENGAGE,
    DISENGAGING,
    ENGAGED,
    JOB_RUNNING,
    SAFETY_ERROR
} SystemStatus;

extern volatile SystemStatus systemState;
extern volatile uint8_t stop_button;
extern volatile uint8_t go_button;
extern volatile uint8_t safetyOK;
extern volatile uint8_t job_finish;
extern uint32_t ulNotifiedValue;

uint8_t wirePresent(float adc);

#endif //WIRESTRIPPER_TASK_STATEMACHINE_H