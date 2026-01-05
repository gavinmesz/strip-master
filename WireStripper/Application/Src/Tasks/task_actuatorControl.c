/*
 * Actuator Control Task:
 * Includes PID control for all motors. Take information from encoders and state machine, perform jobs, report status.
 */

//main includes
#include "task_manager.h"
#include "task.h"

//specific includes
#include "task_actuatorControl.h"

void vActuatorTask(){
    for(;;){
        counterVar++;
        vTaskDelay(100);
    }
}
