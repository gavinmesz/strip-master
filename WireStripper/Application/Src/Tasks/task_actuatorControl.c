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

//PULSES AT 100Hz right now
#define CLK_SPEED 1000000.0
#define MICROSTEP 1

//Motor constants
#define M1_TIMER &htim8
#define M1_TIM TIM8
#define M1_CHANNEL TIM_CHANNEL_1
#define M2_TIMER &htim8
#define M2_TIM TIM8
#define M2_CHANNEL TIM_CHANNEL_4
#define M3_TIMER &htim1
#define M3_TIM TIM1
#define M3_CHANNEL TIM_CHANNEL_1

//Direction pins for motor control
#define TO_FRONT GPIO_PIN_SET
#define TO_BACK GPIO_PIN_RESET
#define UP GPIO_PIN_SET
#define DOWN GPIO_PIN_RESET

#define ENABLE 1
#define DISABLE 0

//Status
typedef enum {
    IDLE,
    CALIB, //M1 feed to wire detect
    FEED1, //M1 feed one strip length past cutters
    STRIP1, //cutter engage to strip, back off very slightly (loop would start here if wire #2)
    PEEL1, //M1 motor reverse distance
    FEED2, //M1,M2 feed forward one full length
    CUT, //cutter engage full and disengage
    FEED3, //M1 motor reverse to wire detect, M2 reverse one strip length
    STRIP2, //cutter engage to strip, back off very slightly
    PEEL2, //M2 push out fully, M1 feed forward 1 strip length past.
}MotorStatus;
MotorStatus motorStatus;
uint8_t goalReached;

typedef enum {
    M1,
    M2,
    M3
}MotorNum;

typedef struct {
    MotorNum motor_num;

    //Timer Peripheral
    TIM_HandleTypeDef *htim;
    TIM_TypeDef *TIMx;
    uint32_t channel;
    uint32_t ccr;

    //Common Pins
    GPIO_TypeDef* EN_Port;
    uint16_t EN_Pin;
    GPIO_TypeDef* DIR_Port;
    uint16_t DIR_Pin;
    GPIO_TypeDef* MS1_Port;
    uint16_t MS1_Pin;
    GPIO_TypeDef* MS2_Port;
    uint16_t MS2_Pin;
    GPIO_TypeDef* nFAULT_Port;
    uint16_t nFAULT_Pin;

    uint8_t motorDone;
}Motor;

//Motors
static Motor motors[3];

//Present Encoder Values
int encoder1;
int encoder2;

//Duty cycle of step movement
static uint32_t dcDMA1;
static uint32_t dcDMA2;
static uint32_t dcDMA3;

uint8_t homeSetM3() {
    //0 if the homing was unsuccessful (M3 is currently running a distance job)
    uint8_t temp = 0;
    if (motors[2].motorDone){
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

void nWakeMotor(uint8_t const state, Motor const motor) {
    //Reminder M3 is nEN
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
void changeSpeed(float const speed, uint8_t const dir, Motor motor) {
    //Require arr value for specific frequency of pulses. 100pps = 1Mhz/10000. 1000 = 1Mhz/100. 1000 = arr.
    motor.TIMx->ARR = (uint32_t)(CLK_SPEED/speed);//pulses per second

    motor.TIMx->EGR = TIM_EGR_UG; //Trigger a reset due to register change
    motor.TIMx->SR &= ~(TIM_SR_UIF); //Reset the interrupt flag

    motor.ccr = (uint32_t)((float)(motor.TIMx->ARR)*0.5);
    HAL_GPIO_WritePin(motor.DIR_Port, motor.DIR_Pin, dir);
}

//Movement of x steps in one direction at speed.
//Return 0 if the step move has not finished yet. Return 1 if the DMA was started.
uint8_t stepMove(int const step, float const speed, uint8_t const dir, Motor motor) {
    if (motor.motorDone) {
        changeSpeed(speed, dir, motor);
        HAL_TIM_PWM_Start_DMA(motor.htim, motor.channel, &motor.ccr, step);
        motor.motorDone=0;
        return 1;
    }
    return 0;
}

//continuous movement in one direction
uint8_t speedMove(int const speed, uint8_t const dir, Motor motor) {
    if (motor.motorDone) {
        changeSpeed(speed, dir, motor);
        HAL_TIM_PWM_Start(motor.htim, motor.channel);
        motor.motorDone=0;
        return 1;
    }
    return 0;
}

void stopMotor(Motor const motor) {
    HAL_TIM_PWM_Stop(motor.htim, motor.channel);
}

void cutWire() {
    //Sequence of events on motor 3
}

void stripWire() {
    //Sequence of movements on motor 3

    //Set speed
    //stop once wire detected
}


void vActuatorTask(){
    //Initialize motor variables.
    motorStatus = IDLE;
    encoder1 = 0;
    encoder2 = 0;

    //Initialize DMA transfer flags.
    motors[0].motorDone=1;
    motors[1].motorDone=1;
    motors[2].motorDone=1;

    //Initialize microstepping configurations.
    microSet(0, motors[0]);
    microSet(0, motors[1]);
    microSet(0, motors[2]);

    //Startup routines
    //M1 and M2
    //Wakeup, enable, set microstep, no fault
    //M3
    //nEnable, reset if you need to home, no faults

    for(;;){
        //Motor status set by stateMachine
        stepMove(100, 100, TO_FRONT,M1_CHANNEL);
        stepMove(200, 100, TO_FRONT,M2_CHANNEL);
        stepMove(300, 100, TO_FRONT,M3_CHANNEL);
        vTaskDelay(5000);
    }
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
    /* Prevent unused argument(s) compilation warning */
    //enabled but empty streams
    //Check if TIM state is not busy
    if (htim == M1_TIMER) {
        M1Done = TIM_CHANNEL_STATE_GET(M1_TIMER, M1_CHANNEL)!=0x02U;
        if (M1Done){HAL_TIM_PWM_Stop_DMA(M1_TIMER, TIM_CHANNEL_1);}
        M2Done = TIM_CHANNEL_STATE_GET(M2_TIMER, M2_CHANNEL)!=0x02U;
        if (M2Done){HAL_TIM_PWM_Stop_DMA(M2_TIMER, TIM_CHANNEL_2);}
    } else if (htim == M3_TIMER) {
        M3Done = TIM_CHANNEL_STATE_GET(M3_TIMER, M3_CHANNEL)!=0x02U;
        if (M3Done){HAL_TIM_PWM_Stop_DMA(M3_TIMER, TIM_CHANNEL_3);}
    }
    /* NOTE : This function should not be modified, when the callback is needed,
              the HAL_TIM_PWM_PulseFinishedCallback could be implemented in the user file
     */
}
