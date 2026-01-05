/*
 * Actuator Control Task:
 * Includes PID control for all motors. Take information from encoders and state machine, perform jobs, report status.
 */

#include "task_manager.h" // Has FreeRTOS functions and globals defined

//specific includes
#include "task_actuatorControl.h"

void vActuatorTask(){
    for(;;){
        counterVar++;
        vTaskDelay(100);
    }
}
