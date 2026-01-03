//Tasks included
#include "task_manager.h"
#include "task_display.h"
#include "task_actuatorControl.h"
#include "task_safety.h"
#include "task_stateMachine.h"

#include "usart.h"
#include "FreeRTOSConfig.h"

TaskHandle_t xDisplayTaskHandle = NULL;
TaskHandle_t xActuatorTaskHandle = NULL;
TaskHandle_t xSafetyTaskHandle = NULL;
TaskHandle_t xStateMachineTaskHandle = NULL;
int * x = 0;

void TaskManager_InitTasks(void){
    vTaskDelete(NULL);
}

void TaskManager_CreateAllTasks(void)
{

    BaseType_t xReturned;

    //Actuator Task: stack should be >1024 due to display buffer size
    xReturned = xTaskCreate(vActuatorTask, "Actuator", 512, NULL, configMAX_PRIORITIES-3, &xActuatorTaskHandle);

    configASSERT(xReturned == pdPASS);

    //Display Task: stack should be >1024 due to display buffer size
    xReturned = xTaskCreate(vDisplayTask, "Display",  2048, NULL, configMAX_PRIORITIES-4, &xDisplayTaskHandle);

    configASSERT(xReturned == pdPASS);

    // Safety Poll Task: Make sure all bucks present power good
    xReturned = xTaskCreate(vSafetyTask, "Safety",   512, NULL, configMAX_PRIORITIES-1, &xSafetyTaskHandle);

    configASSERT(xReturned == pdPASS);

    // State Machine Task: Monitor events and produce motor setpoints
    xReturned = xTaskCreate(vStateMachineTask, "StateMachine",   512, NULL, configMAX_PRIORITIES-2, &xStateMachineTaskHandle);

    configASSERT(xReturned == pdPASS);
}
