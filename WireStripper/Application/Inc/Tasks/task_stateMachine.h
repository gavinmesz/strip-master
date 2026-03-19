//
// Created by Admin on 11/22/2025.
//

#ifndef WIRESTRIPPER_TASK_STATEMACHINE_H
#define WIRESTRIPPER_TASK_STATEMACHINE_H

#define WIRE_IN_DETECT &adcVals3[0]
#define WIRE_END_DETECT &adcVals3[1]
#define WIRE_DETECT_THRES 3900 //Todo: Requires confirmation

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
    SAFETY_ERROR,
    HALT
} SystemStatus;

typedef enum {
    SYSTEM_OK,
    BATTERY_DEAD,
    ESTOP,
    BUCK_FAIL,
    OTHER_ERROR
} ErrStatus;

//Photodiodes
#define INLET 1
#define OUTLET 0

extern volatile SystemStatus systemState;
extern volatile uint8_t stop_button;
extern volatile uint8_t go_button;
extern volatile uint8_t safetyOK;
extern volatile uint8_t job_finish;

uint8_t wirePresent(uint32_t adc);

#endif //WIRESTRIPPER_TASK_STATEMACHINE_H