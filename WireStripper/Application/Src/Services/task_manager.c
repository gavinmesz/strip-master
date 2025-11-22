//Tasks included
#include "task_manager.h"
#include "task_display.h"
#include "task_actuatorControl.h"
#include "task_safety.h"
#include "task_stateMachine.h"

#include <stdio.h>

TaskHandle_t xDisplayTaskHandle = NULL;
TaskHandle_t xActuatorTaskHandle = NULL;
TaskHandle_t xSafetyTaskHandle = NULL;
TaskHandle_t xStateMachineTaskHandle = NULL;

void TaskManager_InitTasks(void){
    if(initDisplay()==-1){
        printf("Failed to Initialize Display\r\n");
    }
}

void TaskManager_CreateAllTasks(void)
{
    BaseType_t xReturned;

    //Actuator Task: stack should be >1024 due to display buffer size
    xReturned = xTaskCreate(vActuatorTask, "Actuator", 256, NULL, 32, &xActuatorTaskHandle);

    configASSERT(xReturned == pdPASS);

    //Display Task: stack should be >1024 due to display buffer size
    xReturned = xTaskCreate(vDisplayTask, "Display",  2*1024, NULL, 3, &xDisplayTaskHandle);

    configASSERT(xReturned == pdPASS);

    // Safety Poll Task: Make sure all bucks present power good
    xReturned = xTaskCreate(vSafetyTask, "Safety",   256, NULL, 2, &xSafetyTaskHandle);

    configASSERT(xReturned == pdPASS);

    // State Machine Task: Monitor events and produce motor setpoints
    xReturned = xTaskCreate(vStateMachineTask, "StateMachine",   256, NULL, 31, &xStateMachineTaskHandle);

    configASSERT(xReturned == pdPASS);
}
