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

#define WHEEL_RADIUS 1
#define BASE_STEPS_PER_REV 200
#define FEED_SPEED 100
#define CUT_SPEED 100
#define STRIP_SPEED 50
#define PEEL_SPEED 100

#define M1_TO_CUT_DIST 10
#define M2_TO_CUT_DIST 10
#define TOLERANCE_STEP 5
#define SPIT_STEPS 200 + M2_TO_CUT_DIST
#define CUT_BACK_OFF 2

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

//Microstep values
static uint8_t microStep1;
static uint8_t microStep2;

//Finished wires
int finishedWires;

static int length_to_steps(int const length_mm, uint8_t const microStep) {
    return (length_mm/(2*3.14159*WHEEL_RADIUS))*BASE_STEPS_PER_REV*microStep;
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
    state = ~state;
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
    motor->TIMx->ARR = (uint32_t)(CLK_SPEED/speed);//pulses per second

    motor->TIMx->EGR = TIM_EGR_UG; //Trigger a reset due to register change
    motor->TIMx->SR &= ~(TIM_SR_UIF); //Reset the interrupt flag

    motor->ccr = (uint32_t)((float)(motor->TIMx->ARR)*0.5);
    HAL_GPIO_WritePin(motor->DIR_Port, motor->DIR_Pin, dir);
}

//Movement of x steps in one direction at speed.
//Return 0 if the step move has not finished yet. Return 1 if the DMA was started.
uint8_t stepMove(int const step, float const speed, Motor* motor) {
    int dir;
    int steptemp = step;
    if (step<0){
        dir = TO_FRONT;
        steptemp *= -1;
    } else {
        dir = TO_BACK;
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
    if (speed<0){
        dir = TO_FRONT;
        speedtemp *= -1;
    } else {
        dir = TO_BACK;
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

uint8_t cutWire() {
    //Todo: move M3 one full rotation
}

uint8_t stripWire() {
    //Todo: move M3, stop and back off when detected
}

uint8_t encoderMove(int const length, float const speed) {
    //Todo: move motor1 and 2 using encoders for reference
}

//Running the motor's job
void runJob() {
    switch (motorStatus) { //Internal states unimportant to the boss task
        case (IDLE): { //entry point
            finishedWires = 0;
            motorStatus = START;
            break;
        }
        case (START): { //Check if we met our goal
            if (finishedWires >= quantity){ //Return to IDLE state
                motorStatus = IDLE;
                systemState = ENGAGED;
            } else { //Feed new wire one strip length + distance from light to cutter (dead reckoning)
                if (stepMove(length_to_steps(stripLength+M1_TO_CUT_DIST, microStep1), FEED_SPEED, &Motor1)) {motorStatus = STRIP_ENGAGE1;}
            }
            break;
        }
        case (STRIP_ENGAGE1): { //Engage the cutters once the previous move is finished.
            if (Motor1.motorDone == 1) {
                if (stripWire()) { //Engages M3 then backs off a lil
                    motorStatus = M1_PEEL;
                }
            }
            break;
        }
        case (M1_PEEL): { //use motor 1 to peel the insulation off
            if (stepMove(-(length_to_steps(stripLength, microStep1)+TOLERANCE_STEP), PEEL_SPEED, &Motor1)) {motorStatus = M1_FULL_LENGTH_FEED;}
            break;
        }
        case (M1_FULL_LENGTH_FEED): { // Move the full length
            if (Motor1.motorDone == 1) { //Motor finished, start full length move
                if (encoderMove(length+TOLERANCE_STEP, FEED_SPEED)) {motorStatus = CUT;}
            }
            break;
        }
        case (CUT): { //Cut the wire
            if (stepMove(BASE_STEPS_PER_REV, CUT_SPEED, &Motor3)) {motorStatus = CALIBRATE_AND_M2_STRIP;}
            break;
        }
        case (CALIBRATE_AND_M2_STRIP): { //Feed M1 to light to recalibrate, feed M2 one strip length
            if (Motor3.motorDone == 1) {
                if (stepMove(-length_to_steps(stripLength, microStep2), FEED_SPEED, &Motor2)) {
                    if (speedMove(-FEED_SPEED, &Motor1)) {
                        while (wirePresent(*WIRE_END_DETECT)) { //Poll while wire is present
                            vTaskDelay(10);
                        }
                        stopMotor(&Motor1); //Stop when the wire is not detected anymore
                        stepMove(TOLERANCE_STEP, FEED_SPEED, &Motor1);// Move forward a lil
                        motorStatus = STRIP_ENGAGE2;
                    }
                }
            }
            break;
        }
        case (STRIP_ENGAGE2): {
            if (Motor2.motorDone == 1 && Motor1.motorDone == 1) {
                if (stripWire()) { //engage teeth again
                    motorStatus = M2_PEEL;
                }
            }
            break;
        }
        case (M2_PEEL): { //Spit out wire with M2
            if (stepMove((length_to_steps(stripLength, microStep1)+TOLERANCE_STEP), PEEL_SPEED, &Motor2)) {
                motorStatus = SPIT;
            }
            break;
        }
        case (SPIT): {  //Spit out the wire
            if (stepMove(SPIT_STEPS, PEEL_SPEED, &Motor2)) {
                motorStatus = RESTART;
            }
            break;
        }
        case (RESTART): { //When wire is spit out, increment finished wires and restart the process
            if (Motor2.motorDone == 1) {
                finishedWires++;
                motorStatus = START;
            }
        }
        default: {systemState = SAFETY_ERROR;}
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
    microSet(0, Motor1);
    microSet(0, Motor2);
    microSet(0, Motor3);

    //Startup routines (when ready)
    //M1 and M2
    //Wakeup, enable, set microstep, no faults
    //M3
    //nEnable, reset if you need to home, no faults

    for(;;){
        // //Acknowledge that job was received.
        switch (systemState) {
            case (NONE): {
                motorStatus = IDLE;
                stopAllMotors();
                break;
            }
            case (SAFETY_ERROR):{ //Todo: Have to make sure that the motors stop
                motorStatus = IDLE;
                stopAllMotors();
                break;
            }

            //Engage action
            case (ENGAGE): {
                //Move wire feed until LIGHT_IN1 hit
                speedMove(100, &Motor1);
                systemState = ENGAGING;
                break;
            }
            case (ENGAGING): {
                if (wirePresent(*WIRE_END_DETECT)) {
                    systemState = ENGAGED;
                }
                break;
            }
            case (ENGAGED): {
                stopMotor(&Motor1);
                break;
            }

            //Disengage Action
            case (DISENGAGE): {
                speedMove(-100, &Motor1);
                systemState = DISENGAGING;
                break;
            }
            case (DISENGAGING): {
                if (!wirePresent(*WIRE_IN_DETECT)) {
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
