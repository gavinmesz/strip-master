/*
 * State Machine Task:
 * Determines current device state and sends motor jobs.
 */

#define TEST 0

#include "task_manager.h" // Has FreeRTOS functions and globals defined

//task specific includes
#include "task_stateMachine.h"

#include <limits.h>
#include <stdlib.h>
#include "BQ7692006PWR.h"
#include "main.h"
#include "stm32f4xx_hal_gpio.h"
#include "FreeRTOS.h"
#include "task_actuatorControl.h"
#include "task_display.h"
#include "task_motorTest.h"
#include "task_safety.h"

volatile SystemStatus systemState;

/*
 * 0. Startup checks. All should be true before moving on.
 *  a. Power OK?
 *  b. wire not detected at #2?
 *  c. check if peripherals successfully initiated.
 * 1. Stay NONE while waiting for interrupt flags
 * 2. Deal with interrupt flags
 *  a. Wire detected -> Feed wire in until wire detect 2. FEEDING..., WAIT_FOR_USER.
 *  b. Safety flag -> Disable HV power. Require restart to move on. Display?. POWER_ERROR.
 * 3. Wait for button Press
 *  a. STOP -> Disengage wire job to actuator control, stepMove. DISENGAGING... Return to NONE.
 *  b. GO -> Send "start job" to actuator control. JOB_RUNNING
 *  c. Power safety Flag -> Disable HV Power. Require restart to move on. Display? POWER_ERROR.
 * 4. Job is running, interrupt flags
 *  a. STOP -> E-stop. Disable HV Power. Require restart to move on. Display? E-STOP.
 *  b. Power safety Flag -> Disable HV Power. Require restart to move on. Display? POWER_ERROR.
 *  c. Job finished -> Disable HV Power. Move to ENGAGED.
 */

#define GO_BUTTON (1<<0)
#define STOP_BUTTON (1<<1)
#define GAUGE_IN (1<<2)

volatile uint8_t safetyOK;
volatile uint8_t job_finish;
uint32_t ulNotifiedValue;

//Is the wire detected? 0 for no, 1 for yes
uint8_t wirePresent(uint32_t adc){
    if (adc < WIRE_DETECT_THRES) { //voltage low means light blocked
        return 1;
    }
    return 0;
}

void turnOffBAT() {
    turnDSGOff(&BMS);
    turnCHGOff(&BMS);
}

void turnOnBAT() {
    turnDSGOn(&BMS);
    turnCHGOn(&BMS);
}

void vStateMachineTask() {
    systemState = CHECKS;
    //
    for (;;) {
    if (!safetyOK) { //Must poll before every cycle
        systemState = SAFETY_ERROR;
    }

    switch (systemState) {
    case CHECKS: {
    /*
    *  a. Power OK?
    *  b. wire not detected at #2? Make sure wire not jammed in already
    *  If these aren't true, something is wrong, SAFETY ERROR
    */
        turnOnBAT();
        vTaskDelay(250); // Wait for safety checks to run and for system to turn on.
        if (safetyOK) {
            HAL_GPIO_WritePin(BUCK12_EN_GPIO_Port, BUCK12_EN_Pin, GPIO_PIN_SET);
            systemState = NONE;
        }
        break;
    }
    case NONE: {
    /* 1. Stay NONE while waiting for interrupt flags
     * 2. Deal with interrupt flags
     *  a. Wire detected -> Feed wire in until wire detect 2. ENGAGE..., WAIT_FOR_USER.
     *  b. Safety flag -> Disable HV power. Require restart to move on. Display?. POWER_ERROR.
     */
        if (wirePresent(adcVals3[1])) {
            systemState = ENGAGE;
        }
        break;
    }
    case DISENGAGE: {
        if (job_finish) {
            systemState = NONE;
        }
        break;
    }
    case ENGAGED: {
        /*
        *  a. STOP -> Disengage wire job to actuator control, stepMove. DISENGAGING... Return to NONE.
        *  b. GO -> Send "start job" to actuator control. JOB_RUNNING
        *  c. Power safety Flag -> Disable HV Power. Require restart to move on. Display? POWER_ERROR.
        */
        uint32_t localNotifyVal = 0;

        BaseType_t result = xTaskNotifyWait(0x00, ULONG_MAX, &localNotifyVal, 0);

        if (result == pdTRUE) {
            if (localNotifyVal & STOP_BUTTON) {
                systemState = DISENGAGE;
            }
            if (localNotifyVal & GO_BUTTON) {
                systemState = JOB_RUNNING;
            }
        }
        break;
    }
    case JOB_RUNNING: {
        /*
        * 4. Job is running, interrupt flags
        *  a. STOP -> E-stop. Disable HV Power. Require restart to move on. Display? E-STOP.
        *  b. Power safety Flag -> Disable HV Power. Require restart to move on. Display? POWER_ERROR.
        *  c. Job finished -> Actuator task will set JOB_RUNNING to Done
        */

        uint32_t localNotifyVal = 0;
        BaseType_t result = xTaskNotifyWait(0x00, ULONG_MAX, &localNotifyVal, 0);

        if (result == pdTRUE) {
            if (localNotifyVal & STOP_BUTTON) {
                systemState = SAFETY_ERROR;
            }
        }
        break;
    }
    case SAFETY_ERROR: {
        //Here forever, shut down power
        turnOffBAT();
        HAL_GPIO_WritePin(BUCK12_EN_GPIO_Port, BUCK12_EN_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LDO_EN_GPIO_Port,LDO_EN_Pin, GPIO_PIN_RESET);
        break;
    }
        default: {
            //When in "ENGAGING" or "DISENGAGING" State
            break;
        }
    }
            vTaskDelay(20);
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    /* Prevent unused argument(s) compilation warning */
    UNUSED(GPIO_Pin);
    // if (GPIO_Pin == BMS_INT_Pin || GPIO_Pin == M2_nFLT_Pin || GPIO_Pin == M1_nFLT_Pin) {
    //     safetyOK = 0;
    // }

    if (GPIO_Pin == STOP_BUT_Pin && (systemState == ENGAGED || systemState == JOB_RUNNING)) { // Detect falling edge
        xTaskNotifyFromISR(xStateMachineTaskHandle, STOP_BUTTON, eSetBits, &xHigherPriorityTaskWoken);
    }
    if (GPIO_Pin == GO_BUT_Pin && systemState == ENGAGED) { //Detect falling edge
        xTaskNotifyFromISR(xStateMachineTaskHandle, GO_BUTTON, eSetBits, &xHigherPriorityTaskWoken);
    }
    if (GPIO_Pin == GAUGE_IN_Pin && !TEST && (motorStatus == STRIP_ENGAGE1 || motorStatus == STRIP_ENGAGE2)) { //Detect 3V3 rising edge
        xTaskNotifyFromISR(xActuatorTaskHandle, GAUGE_IN, eSetBits, &xHigherPriorityTaskWoken);
    }
    if (GPIO_Pin == GAUGE_IN_Pin && TEST && gauge_detect) {
        xTaskNotifyFromISR(xMotorTestHandle, GAUGE_IN, eSetBits, &xHigherPriorityTaskWoken);
    }
}
