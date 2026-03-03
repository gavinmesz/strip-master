/*
 * State Machine Task:
 * Determines current device state and sends motor jobs.
 */

#include "task_manager.h" // Has FreeRTOS functions and globals defined

//task specific includes
#include "task_stateMachine.h"

#include <stdlib.h>

#include "BQ7692006PWR.h"
#include "main.h"
#include "stm32f4xx_hal_gpio.h"
#include "task_display.h"
#include "task_safety.h"

SystemStatus systemState;

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

volatile uint8_t stop_button;
volatile uint8_t go_button;
volatile uint8_t safetyOK;
volatile uint8_t job_finish;

//Is the wire detected? 0 for no, 1 for yes
uint8_t wirePresent(float adc){
    if (adc < WIRE_DETECT_LOW_THRES) { //voltage low means light blocked
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

    for (;;) {
        switch (systemState) {
            case CHECKS: {
            /*
            *  a. Power OK?
            *  b. wire not detected at #2? Make sure wire not jammed in already
            *  If these aren't true, something is wrong, SAFETY ERROR
            */
                turnOnBAT();
                vTaskDelay(250); // Wait for safety checks to run and for system to turn on.
                if (safetyOK && wirePresent(*WIRE_IN_DETECT)) {
                    systemState = NONE;
                } else {
                    systemState = SAFETY_ERROR;
                }
            }
            case NONE: {
            /* 1. Stay NONE while waiting for interrupt flags
             * 2. Deal with interrupt flags
             *  a. Wire detected -> Feed wire in until wire detect 2. ENGAGE..., WAIT_FOR_USER.
             *  b. Safety flag -> Disable HV power. Require restart to move on. Display?. POWER_ERROR.
             */
                if (!safetyOK) {
                    systemState = SAFETY_ERROR;
                }
                else if (wirePresent(*WIRE_IN_DETECT)) {
                    systemState = ENGAGE;
                }
            }
            case DISENGAGE: {
                if (job_finish) {
                    systemState = NONE;
                }
            }
            case ENGAGED: {
                /*
                *  a. STOP -> Disengage wire job to actuator control, stepMove. DISENGAGING... Return to NONE.
                *  b. GO -> Send "start job" to actuator control. JOB_RUNNING
                *  c. Power safety Flag -> Disable HV Power. Require restart to move on. Display? POWER_ERROR.
                */
                if (!safetyOK) {
                    systemState = SAFETY_ERROR;
                } else if (go_button) {
                    systemState = JOB_RUNNING;
                } else if (stop_button) {
                    systemState = DISENGAGE;
                }
            }
            case JOB_RUNNING: {
                /*
                * 4. Job is running, interrupt flags
                *  a. STOP -> E-stop. Disable HV Power. Require restart to move on. Display? E-STOP.
                *  b. Power safety Flag -> Disable HV Power. Require restart to move on. Display? POWER_ERROR.
                *  c. Job finished -> Actuator task will set JOB_RUNNING to Done
                */
                if (!safetyOK || stop_button) {
                    systemState = SAFETY_ERROR;
                }
            }
            case SAFETY_ERROR: {
                //Here forever, shut down power
                turnOffBAT();
                HAL_GPIO_WritePin(LDO_EN_GPIO_Port,LDO_EN_Pin, GPIO_PIN_RESET);
            }
            default: {
                systemState = SAFETY_ERROR; //Should never be in an unknown state
            }
        }
        vTaskDelay(20);
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(GPIO_Pin);
    if (GPIO_Pin == BMS_INT_Pin || GPIO_Pin == M2_nFLT_Pin || GPIO_Pin == M1_nFLT_Pin) {
        safetyOK = 0;
    }
}
