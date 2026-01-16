/*
 * Actuator Control Task:
 * Includes PID control for all motors. Take information from encoders and state machine, perform jobs, report status.
 */

#include "task_manager.h" // Has FreeRTOS functions and globals defined

//specific includes
#include "task_actuatorControl.h"
#include "main.h"
#include "stm32f4xx_hal_gpio.h"
#include "tim.h"
#include "stm32f4xx_ll_tim.h"

//PULSES AT 100Hz right now
#define CLK_SPEED 1000000.0
#define MICROSTEP 1

//Motor constants
#define M1_TIMER &htim1
#define M1_TIMDEF TIM1
#define M1_CHANNEL TIM_CHANNEL_1

#define M2_TIMER &htim1
#define M2_TIMDEF TIM1
#define M2_CHANNEL TIM_CHANNEL_2

#define M3_TIMER &htim1
#define M3_TIMDEF TIM1
#define M3_CHANNEL TIM_CHANNEL_3

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

//Present Encoder Values
int encoder1;
int encoder2;

//DMA transfer flags
uint8_t M1Done;
uint8_t M2Done;
uint8_t M3Done;

//Duty cycle of step movement
uint32_t dcDMA;

void changeSpeed(int const speed, float const dc, TIM_TypeDef *TIMx, uint32_t const channel) {
    //Require arr value for specific frequency of pulses. 100pps = 1Mhz/10000. 1000 = 1Mhz/100. 1000 = arr.
    TIMx->ARR = (uint32_t)(CLK_SPEED/speed);//pulses per second
    if (channel == M1_CHANNEL) {
        TIMx->CCR1 = (uint32_t)((float)(TIMx->ARR)*dc);
    } else if (channel == M2_CHANNEL) {
        TIMx->CCR2 = (uint32_t)((float)(TIMx->ARR)*dc);
    } else if (channel == M3_CHANNEL) {
        TIMx->CCR3 = (uint32_t)((float)(TIMx->ARR)*dc);
    }
}

//Return 0 if the step move has not finished yet. Return 1 if the DMA was started.
uint8_t stepMove(int const step, float const speed, TIM_TypeDef *TIMx, uint32_t const channel) {
    TIMx->ARR = (uint32_t)(CLK_SPEED/speed);//pulses per second
    dcDMA = (uint32_t)((float)(TIMx->ARR)*0.5);

    TIMx->EGR = TIM_EGR_UG; //Trigger a reset due to register change
    TIMx->SR &= ~(TIM_SR_UIF); //Reset the interrupt flag

    if (channel == M1_CHANNEL && M1Done) {
        HAL_TIM_PWM_Start_DMA(&htim1, M1_CHANNEL, &dcDMA, step);
        M1Done = 0;
    } else if (channel == M2_CHANNEL && M2Done) {
        HAL_TIM_PWM_Start_DMA(&htim1, M2_CHANNEL, &dcDMA, step);
        M2Done = 0;
    } else if (channel == M3_CHANNEL  && M3Done) {
        HAL_TIM_PWM_Start_DMA(&htim1, M3_CHANNEL, &dcDMA, step);
        M3Done = 0;
    } else {
        return 0;
    }
    return 1;
}

void speedMove(int const speed, TIM_TypeDef *TIMx, uint32_t const channel) {
    changeSpeed(speed, 0.5, TIMx, channel);
    HAL_TIM_PWM_Start(&htim1, channel); //continuous PWM, does not stop unless you tell it to
}

void stopMotor(TIM_HandleTypeDef *htim, uint32_t const channel) {
    if (channel == M1_CHANNEL) {
        HAL_TIM_PWM_Stop(htim, M1_CHANNEL);
    } else if (channel == M2_CHANNEL) {
        HAL_TIM_PWM_Stop(htim, M2_CHANNEL);
    } else if (channel == M3_CHANNEL) {
        HAL_TIM_PWM_Stop(htim, M3_CHANNEL);
    }
}

void cutWire() {
    //Sequence of events on motor 3
}

void stripWire() {
    //Sequence of movements on motor 3
}


void vActuatorTask(){
    //Initialize motor variables
    motorStatus = IDLE;
    encoder1 = 0;
    encoder2 = 0;
    //Set the necessary pins

    for(;;){
        //Motor status set by stateMachine
        // while (motorStatus!=IDLE) {
        //     if (goalReached) {
        //         motorStatus = IDLE;
        //     }
        //     vTaskDelay(1000);
        // }
        vTaskDelay(5000);
    }
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    uint8_t dmaStream;
    /* Prevent unused argument(s) compilation warning */
    if () {
        // HAL_TIM_PWM_Stop_DMA(htim, TIM_CHANNEL_1);
        M1Done = 1;
    }


    /* NOTE : This function should not be modified, when the callback is needed,
              the HAL_TIM_PWM_PulseFinishedCallback could be implemented in the user file
     */
}
