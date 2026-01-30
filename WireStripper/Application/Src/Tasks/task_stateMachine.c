/*
 * State Machine Task:
 * Determines current device state and sends motor jobs.
 */

#include "task_manager.h" // Has FreeRTOS functions and globals defined

//task specific includes
#include "task_stateMachine.h"

#include <stdlib.h>

#include "main.h"
#include "stm32f4xx_hal_gpio.h"
#include "task_actuatorControl.h"
#include "task_display.h"

#define WIRE_IN_DETECT &adcVals[1]
#define WIRE_END_DETECT &adcVals[2]
#define WIRE_DETECT_LOW_THRES 0.5
#define WIRE_DETECT_HIGH_THRES 1

SystemStatus systemState;

/*
 * 0. Startup checks. All should be true before moving on.
 *  a. Power OK?
 *  b. wire not detected at #2?
 *  c. check if peripherals successfully initiated.
 * 1. Stay IDLE while waiting for interrupt flags
 * 2. Deal with interrupt flags
 *  a. Wire detected -> Feed wire in until wire detect 2. FEEDING..., WAIT_FOR_USER.
 *  b. Safety flag -> Disable HV power. Require restart to move on. Display?. POWER_ERROR.
 * 3. Wait for button Press
 *  a. STOP -> Disengage wire job to actuator control, stepMove. DISENGAGING... Return to IDLE.
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
static uint8_t wirePresent(float adc){
    if (adc < WIRE_DETECT_LOW_THRES) { //voltage low means light blocked
        return 1;
    } else if (adc > WIRE_DETECT_HIGH_THRES) { //voltage high means no blockage
        return 0;
    }
}

void vStateMachineTask() {
    static uint8_t shutdown = 0;
    systemState = CHECKS;

    for (;;) {
        switch (systemState) {
            case CHECKS: {
            /*
            *  a. Power OK?
            *  b. wire not detected at #2? Make sure wire not jammed in already
            *  If these aren't true, something is wrong, SAFETY ERROR
            */
                if (safetyOK && wirePresent(*WIRE_IN_DETECT)) {
                    systemState = IDLE;
                } else {
                    systemState = SAFETY_ERROR;
                }
            }
            case IDLE: {
            /* 1. Stay IDLE while waiting for interrupt flags
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
            case ENGAGE: {
                if (job_finish) {
                    systemState = ENGAGED;
                }
            }
            case DISENGAGE: {
                if (job_finish) {
                    systemState = IDLE;
                }
            }
            case ENGAGED: {
                /*
                *  a. STOP -> Disengage wire job to actuator control, stepMove. DISENGAGING... Return to IDLE.
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
                *  c. Job finished -> Disable HV Power. Move to ENGAGED.
                */
                if (!safetyOK || stop_button) {
                    systemState = SAFETY_ERROR;
                } else if (job_finish) {
                    systemState = ENGAGED;
                }
            }
            case SAFETY_ERROR: {
                //Here forever, shut down power
                if (HAL_GPIO_ReadPin(BUCK12_EN_GPIO_Port, BUCK12_EN_Pin)) {
                    HAL_GPIO_WritePin(BUCK12_EN_GPIO_Port, BUCK12_EN_Pin, GPIO_PIN_RESET);
                }
            }
            default: {
                systemState = SAFETY_ERROR; //Should never be in an unknown state
            }
        }
        vTaskDelay(100);
    }
}
