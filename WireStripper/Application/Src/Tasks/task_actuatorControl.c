/*
 * Actuator Control Task:
 * Includes PID control for all motors. Take information from encoders and state machine, perform jobs, report status.
 */

#include "task_manager.h" // Has FreeRTOS functions and globals defined

//specific includes
#include "task_actuatorControl.h"
#include <assert.h>
#include "main.h"
#include "stm32f4xx_hal_gpio.h"
#include "tim.h"
#include "stm32f4xx_ll_tim.h"
#include "task_display.h"
#include "task_stateMachine.h"
#include <limits.h>

#define WHEEL_RADIUS 4.7625
#define BASE_STEPS_PER_REV 200.0
#define FEED_SPEED 400
#define CUT_SPEED 50
#define STRIP_SPEED 50
#define PEEL_SPEED 400

#define M1_TO_CUT_DIST 10
#define M2_TO_CUT_DIST 38
//~65mm from in light to cut
//~39mm from M1 to cut
#define TOLERANCE_STEP 5
#define SPIT_STEPS (500 + M2_TO_CUT_DIST)
#define CUT_BACK_OFF 5
#define CUT_HOME 300

#define GAUGE_IN (1<<2)

#define M1_MICRO 0
#define M2_MICRO 0
#define M3_MICRO 0

#define STRIP_PULL 0
#define ENCODER_PRESENT 0

MotorStatus motorStatus;

//Motors
Motor Motor1;
Motor Motor2;
Motor Motor3;

//Present Encoder Values
int encoder1;
int encoder2;

//Duty cycle of step movement
static uint32_t dcDMA1;
static uint32_t dcDMA2;
static uint32_t dcDMA3;

//Finished wires
int finishedWires;

static int length_to_steps(float const length_mm, int const microStep) {
    return (length_mm/(2.0*3.14159*WHEEL_RADIUS))*BASE_STEPS_PER_REV*(1 << microStep);
}


uint8_t homeSetM3() {
    //0 if the homing was unsuccessful (M3 is currently running a distance job)
    uint8_t temp = 0;
    if (Motor3.motorDone){
        HAL_GPIO_WritePin(M3_nHOME_GPIO_Port, M3_nHOME_Pin, 0);
        HAL_Delay(pdMS_TO_TICKS(1));
        temp=1;
    }
    HAL_GPIO_WritePin(M3_nHOME_GPIO_Port, M3_nHOME_Pin, 1);
    return temp;
}

void enableMotor(uint8_t state, Motor const motor) {
    //Reminder M3 is nEN
    if (motor.motor_num == M3) {state = ~state;}
    HAL_GPIO_WritePin(motor.EN_Port, motor.EN_Pin, state);
}

void wakeMotor(uint8_t state, Motor const motor) {
    //M3 doesn't have a sleep pin
    switch (motor.motor_num) {
        case M1: {
            HAL_GPIO_WritePin(M1_nSLP_GPIO_Port, M1_nSLP_Pin, state);
            break;
        }
        case M2: {
            HAL_GPIO_WritePin(M2_nSLP_GPIO_Port, M2_nSLP_Pin, state);
            break;
        }
        default: {
            break;
        }
    }
}

void microSet(uint8_t const microStep, Motor const motor) {
    /*
     * Microstep values: Wire Feed
     * 0b00 -> MS1 LOW and MS2 LOW: no microstepping
     * 0b01 -> MS1 HIGH and MS2 LOW: half step
     * 0b10 -> MS1 LOW and MS2 HIGH: quarter step
     * 0b11 -> MS1 HIGH and MS2 HIGH: eigth step
     *
     * Microstep values: Cutter
     * 0b00 -> MS1 LOW and MS2 LOW: no microstepping
     * 0b01 -> MS1 HIGH and MS2 LOW: half step
     * 0b10 -> MS1 LOW and MS2 HIGH: Wave drive
     * 0b11 -> MS1 HIGH and MS2 HIGH: reserved
     *
     * I assume that the user will input the correct microstep values
     */
    HAL_GPIO_WritePin(motor.MS1_Port, motor.MS1_Pin, microStep & 1);
    HAL_GPIO_WritePin(motor.MS2_Port, motor.MS2_Pin, microStep>>1 & 1);
}

//change speed of motor. M1 and M2 are coupled in speed.
void changeSpeed(float const speed, uint8_t const dir, Motor *motor) {
    //Require arr value for specific frequency of pulses. 100pps = 1Mhz/10000. 1000 = 1Mhz/100. 1000 = arr.
    uint32_t arr = (uint32_t)(CLK_SPEED/speed);
    if (arr>65535) {
        arr = 65535;
    }

    motor->TIMx->ARR = arr;//pulses per second

    motor->TIMx->EGR = TIM_EGR_UG; //Trigger a reset due to register change
    motor->TIMx->SR &= ~(TIM_SR_UIF); //Reset the interrupt flag

    motor->ccr = (uint32_t)((float)(motor->TIMx->ARR)*0.5);
    __HAL_TIM_SET_COMPARE(motor->htim, motor->channel, motor->ccr);

    HAL_GPIO_WritePin(motor->DIR_Port, motor->DIR_Pin, dir);
}

//Movement of x steps in one direction at speed.
//Return 0 if the step move has not finished yet. Return 1 if the DMA was started.
uint8_t stepMove(int const step, float const speed, Motor* motor) {
    int dir;
    int steptemp = step;
    if (motor == &Motor3) { //-ve speed is the correct way
        if (step<0){
            dir = DOWN;
            steptemp *= -1;
        } else {
            dir = UP;
        }
    } else {
        if (step<0){
            dir = TO_FRONT;
            steptemp *= -1;
        } else {
            dir = TO_BACK;
        }
    }
    if (motor->motorDone) {
        changeSpeed(speed, dir, motor);
        HAL_TIM_PWM_Start_DMA(motor->htim, motor->channel, &motor->ccr, steptemp);
        motor->motorDone=0;
        return 1;
    }
    return 0;
}

//continuous movement in one direction
uint8_t speedMove(int speed, Motor* motor) {
    int dir;
    int speedtemp = speed;
    if (motor == &Motor3) { //-ve speed is the correct way
        if (speed<0){
            dir = DOWN;
            speedtemp *= -1;
        } else {
            dir = UP;
        }
    } else {
        if (speed<0){
            dir = TO_FRONT;
            speedtemp *= -1;
        } else {
            dir = TO_BACK;
        }
    }
    if (motor->motorDone) {
        changeSpeed(speedtemp, dir, motor);
        HAL_TIM_PWM_Start(motor->htim, motor->channel);
        motor->motorDone=0;
        return 1;
    }
    return 0;
}

void stopMotor(Motor *motor) {
    HAL_TIM_PWM_Stop(motor->htim, motor->channel);
    motor->motorDone = 1;
}

void stopAllMotors() {
    stopMotor(&Motor1);
    stopMotor(&Motor2);
    stopMotor(&Motor3);
}

uint8_t stripWire() {
    //Wait for core detect event
    speedMove(STRIP_SPEED, &Motor3);

    uint32_t localNotifyVal = 0;
    BaseType_t result = xTaskNotifyWait(0x00, ULONG_MAX, &localNotifyVal, 0);

    if (result == pdTRUE) {
        if (localNotifyVal & GAUGE_IN) {
            stopAllMotors();
            return 1;
        }
    }

    return 0;
}

uint8_t encoderMove(int length, int const speed, uint32_t OG_Reading) {
    uint32_t enc = __HAL_TIM_GET_COUNTER(&htim3);

    if (OG_Reading-enc<length_to_steps(length, M1_MICRO)) {
        speedMove(speed, &Motor1); //Move motor at speed
        speedMove(speed, &Motor2); //Move motor at speed
        return 0;
    }
    stopAllMotors();
    return 1;
}

//Running the motor's job
void runJob() {
    switch (motorStatus) {
        //Internal states unimportant to the boss task
        case (IDLE): { //entry point
            finishedWires = 0;
            motorStatus = START;
            break;
        }
        case (START): { //Check if we met our goal
            if (finishedWires >= 1){ //Return to IDLE state
                stopAllMotors();
                motorStatus = IDLE;
                systemState = ENGAGED;
            } else { //Feed new wire one strip length + distance from light to cutter (dead reckoning)
                if (HAL_GPIO_ReadPin(UX_SW_GPIO_Port,UX_SW_Pin)) { //If we are doing cut or strip/cut. HIGH = CUT
                    if (stepMove(length_to_steps(M1_TO_CUT_DIST, M1_MICRO), FEED_SPEED, &Motor1)) {
                        motorStatus = M1_FULL_LENGTH_FEED;
                    }
                } else { // LOW = strip/cut
                    if (stepMove(length_to_steps(stripLength+M1_TO_CUT_DIST, M1_MICRO), FEED_SPEED, &Motor1)) {
                        motorStatus = STRIP_ENGAGE1;
                    }
                }
            }
            break;
        }
        case (STRIP_ENGAGE1): { //Engage the cutters once the previous move is finished.
            if (Motor1.motorDone == 1) {
                if (stripWire()) { //Engages M3 until core hit
                    if (stepMove(-CUT_BACK_OFF, CUT_SPEED, &Motor3)) {
                        if (STRIP_PULL) {
                            motorStatus = M1_PEEL;
                        }else {
                            motorStatus = M1_FULL_LENGTH_FEED;
                            encoder1 = __HAL_TIM_GET_COUNTER(&htim3);
                            encoder2 = __HAL_TIM_GET_COUNTER(&htim4);
                        }
                    }
                    else {
                        systemState = SAFETY_ERROR;
                    }
                }
            }
            break;
        }
        case (M1_PEEL): { //use motor 1 to peel the insulation off
            if (stepMove(-(length_to_steps(stripLength, M1_MICRO)), PEEL_SPEED, &Motor1)) {
                motorStatus = M1_FULL_LENGTH_FEED;
                encoder1 = __HAL_TIM_GET_COUNTER(&htim3);
                encoder2 = __HAL_TIM_GET_COUNTER(&htim4);
            }

            break;
        }
        case (M1_FULL_LENGTH_FEED): { // Move the full length
            if (Motor1.motorDone == 1) { //Motor finished, start full length move
                if (ENCODER_PRESENT) {
                    if (encoderMove(length, FEED_SPEED, encoder1)) {motorStatus = CUT;}
                } else {
                    stepMove(length_to_steps(length, M1_MICRO), FEED_SPEED, &Motor1); //Assume this works
                    stepMove(length_to_steps(length, M2_MICRO), FEED_SPEED, &Motor2); //Assume this works
                    motorStatus = CUT;
                }
            }
            break;
        }
        case (CUT): { //Cut the wire
            if (Motor2.motorDone == 1 && Motor1.motorDone == 1) {
                if (stepMove(BASE_STEPS_PER_REV, CUT_SPEED, &Motor3)) {
                    motorStatus = CALIBRATE_AND_M2_STRIP;
                }
            }
            break;
        }
        case (CALIBRATE_AND_M2_STRIP): {
            //Feed M1 to light to recalibrate, feed M2 one strip length
            if (Motor3.motorDone == 1) {
                if (HAL_GPIO_ReadPin(UX_SW_GPIO_Port, UX_SW_Pin)) { // If we are in CUT mode, skip to spit
                    motorStatus = SPIT;
                } else {
                    if (stepMove(-length_to_steps(stripLength, M2_MICRO), FEED_SPEED, &Motor2)) {
                        speedMove(-FEED_SPEED, &Motor1);
                        motorStatus = WAITING_FOR_WIRE_RESET;
                    }
                }
            }
            break;
        }
        case (WAITING_FOR_WIRE_RESET): {
            if (!wirePresent(adcVals3[0])) {
                //Poll while wire is present
                stopMotor(&Motor1); //Stop when the wire is not detected anymore
                stepMove(TOLERANCE_STEP, FEED_SPEED, &Motor1);// Move forward a lil
                motorStatus = STRIP_ENGAGE2;
            }
            break;
        }
        case (STRIP_ENGAGE2): {
            if (Motor2.motorDone == 1 && Motor1.motorDone == 1) {
                if (stripWire()) { //engage teeth again
                    if (stepMove(-CUT_BACK_OFF, CUT_SPEED, &Motor3)) {
                        if (STRIP_PULL) {
                            motorStatus = M2_PEEL;
                        } else {
                            motorStatus = SPIT;
                        }
                    }
                    else {
                        systemState = SAFETY_ERROR;
                    }
                }
            }
            break;
        }
        case (M2_PEEL): { //Spit out wire with M2
            if (stepMove((length_to_steps(stripLength, M1_MICRO)), PEEL_SPEED, &Motor2)) {
                motorStatus = BACK_UP_CUTTER;
            }
            break;
        }
        case (BACK_UP_CUTTER): {
            if (Motor2.motorDone) {
                if (stepMove(-CUT_HOME, FEED_SPEED, &Motor3)) {
                    motorStatus = SPIT;
                }; //Back up to normal position
            }
            break;
        }
        case (SPIT): {  //Spit out the wire
            if (Motor3.motorDone) {
                if (stepMove(SPIT_STEPS, PEEL_SPEED, &Motor2)) {
                    motorStatus = RESTART;
                }
            }
            break;
        }
        case (RESTART): { //When wire is spit out, increment finished wires and restart the process
            if (Motor2.motorDone == 1) {
                finishedWires++;
                motorStatus = START;
            }
            break;
        }
        default: {
            systemState = SAFETY_ERROR;
            break;
        }
    }
}

void vActuatorTask(){
    //Initialize motor variables.
    motorStatus = IDLE;
    finishedWires = 0;

    Motor1 = (Motor) {
        M1,
        //Timer Peripheral
        M1_TIMER,
        TIM8,
        TIM_CHANNEL_1,
        TIM8->CCR1,
        //Common Pins
        M1_EN_GPIO_Port,
        M1_EN_Pin,
        M1_DIR_GPIO_Port,
        M1_DIR_Pin,
        M1_MS1_GPIO_Port,
        M1_MS1_Pin,
        M1_MS2_GPIO_Port,
        M1_MS2_Pin,
        M1_nFLT_GPIO_Port,
        M1_nFLT_Pin,
        1
    };

    Motor2 = (Motor) {
        M2,
        //Timer Peripheral
        M2_TIMER,
        TIM8,
        TIM_CHANNEL_4,
        TIM8->CCR4,
        //Common Pins
        M2_EN_GPIO_Port,
        M2_EN_Pin,
        M2_DIR_GPIO_Port,
        M2_DIR_Pin,
        M2_MS1_GPIO_Port,
        M2_MS1_Pin,
        M2_MS2_GPIO_Port,
        M2_MS2_Pin,
        M2_nFLT_GPIO_Port,
        M2_nFLT_Pin,
        1
    };
    Motor3 = (Motor) {
        M3,
        //Timer Peripheral
        M3_TIMER,
        TIM1,
        TIM_CHANNEL_1,
        TIM1->CCR1,
        //Common Pins
        M3_nEN_GPIO_Port,
        M3_nEN_Pin,
        M3_DIR_GPIO_Port,
        M3_DIR_Pin,
        M3_SM0_GPIO_Port,
        M3_SM0_Pin,
        M3_SM1_GPIO_Port,
        M3_SM1_Pin,
        M3_nFLT_GPIO_Port,
        M3_nFLT_Pin,
        1
    };

    //Initialize microstepping configurations
    microSet(M1_MICRO, Motor1); // quarter step
    microSet(M2_MICRO, Motor2); // quarter step
    microSet(M3_MICRO, Motor3); // full step

    //Startup routines (when ready)
    //M1 and M2
    HAL_GPIO_WritePin(Motor1.EN_Port, Motor1.EN_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(Motor2.EN_Port, Motor2.EN_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(M1_nSLP_GPIO_Port, M1_nSLP_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(M2_nSLP_GPIO_Port, M2_nSLP_Pin, GPIO_PIN_SET);

    //Wakeup, enable, set microstep, no faults
    //M3
    HAL_GPIO_WritePin(M3_nEN_GPIO_Port, M3_nEN_Pin, GPIO_PIN_RESET);
    //nEnable, reset if you need to home, no faults

    for(;;){
        // //Acknowledge that job was received.
        switch (systemState) {
            case (NONE): {
                motorStatus = IDLE;
                break;
            }
            case (CHECKS): {
                motorStatus = IDLE;
                break;
            }
            case (SAFETY_ERROR):{ //Todo: Have to make sure that the motors stop IRL
                motorStatus = IDLE;

                stopAllMotors();
                enableMotor(0, Motor1);
                enableMotor(0, Motor2);
                enableMotor(0, Motor3);
                wakeMotor(0, Motor1);
                wakeMotor(0, Motor2);
                break;
            }

            //Engage action
            case (ENGAGE): {
                //Move wire feed until LIGHT_IN1 hit
                speedMove(300, &Motor1);
                systemState = ENGAGING;
                break;
            }
            case (ENGAGING): {
                if (adcVals3[0]<3980) {
                    stopMotor(&Motor1);
                    systemState = ENGAGED;
                }
                break;
            }
            case (ENGAGED): {
                break;
            }

            //Disengage Action
            case (DISENGAGE): {
                stepMove(-500, FEED_SPEED, &Motor1);
                systemState = DISENGAGING;
                break;
            }
            case (DISENGAGING): {
                if (Motor1.motorDone && !wirePresent(adcVals3[1])) {
                    systemState = NONE;
                }
                break;
            }

            //Set by boss task, run job
            case (JOB_RUNNING): {
                runJob();
                break;
            }

            default:{systemState = SAFETY_ERROR;}
        }

        //Refresh every 10ms
        vTaskDelay(10);
    }
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
    /* Prevent unused argument(s) compilation warning */
    //Check if TIM state is not busy (state 0x02U)
    if (htim->Instance == TIM8) {
        Motor1.motorDone = TIM_CHANNEL_STATE_GET(Motor1.htim, Motor1.channel)!=0x02U;
        if (Motor1.motorDone){HAL_TIM_PWM_Stop_DMA(Motor1.htim, Motor1.channel);}
        Motor2.motorDone = TIM_CHANNEL_STATE_GET(Motor2.htim, Motor2.channel)!=0x02U;
        if (Motor2.motorDone){HAL_TIM_PWM_Stop_DMA(Motor2.htim, Motor2.channel);}
    } else if (htim == Motor3.htim) {
        Motor3.motorDone = TIM_CHANNEL_STATE_GET(Motor3.htim, Motor3.channel)!=0x02U;
        if (Motor3.motorDone){HAL_TIM_PWM_Stop_DMA(Motor3.htim, Motor3.channel);}
    }
    /* NOTE : This function should not be modified, when the callback is needed,
              the HAL_TIM_PWM_PulseFinishedCallback could be implemented in the user file
     */
}
