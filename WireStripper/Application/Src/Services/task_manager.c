#define TEST 1

//Tasks included
#include "task_manager.h"
#include "task_display.h"
#include "task_actuatorControl.h"
#include "task_safety.h"
#include "task_stateMachine.h"
#include "task_motorTest.h"
#include "C:\Users\Admin\Desktop\4A\MTE481\strip-master\WireStripper\Drivers\BMS\Inc\BQ7692006PWR.h"

#include "FreeRTOSConfig.h"

TaskHandle_t xDisplayTaskHandle = NULL;
TaskHandle_t xActuatorTaskHandle = NULL;
TaskHandle_t xSafetyTaskHandle = NULL;
TaskHandle_t xStateMachineTaskHandle = NULL;
TaskHandle_t xMotorTestHandle = NULL;
int counterVar;

void TaskManager_InitTasks(void){
    //Display
    if (initDisplay() != 1) {
        configASSERT(0);
    }

    //Motor actuators

    //Safety/Power
    BMS.i2cHandle = &hi2c2;
    BQ76920_Initialize(&BMS, &hi2c2); // Init BMS
    HAL_GPIO_WritePin(LDO_EN_GPIO_Port, LDO_EN_Pin, GPIO_PIN_SET);

    //Display
    //Assign nonsense values to user config variables to begin
    quantity = -1;
    length = -1;
    stripLength = -1;
    stripCut = -1;
}

void TaskManager_CreateAllTasks(void)
{
    BaseType_t xReturned;
    if ( TEST ) {
        //Testing Task
        xReturned = xTaskCreate(vMotorTestTask, "MotorTest", 512, NULL, configMAX_PRIORITIES-3, &xMotorTestHandle);
        xReturned = xTaskCreate(vSafetyTask, "Safety",   512, NULL, configMAX_PRIORITIES-1, &xSafetyTaskHandle);
        xReturned = xTaskCreate(vDisplayTask, "Display",  2048, NULL, configMAX_PRIORITIES-4, &xDisplayTaskHandle);


        configASSERT(xReturned == pdPASS);

        // xReturned = xTaskCreate(vStateMachineTask, "StateMachine",   512, NULL, configMAX_PRIORITIES-2, &xStateMachineTaskHandle);
        //
        // configASSERT(xReturned == pdPASS);
    } else {

        //Actuator Task
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
}
