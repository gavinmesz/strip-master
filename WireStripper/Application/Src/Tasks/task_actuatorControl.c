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
#define CLK_SPEED 1000000;
#define FEED_SPEED 100; //max is 2000 pulses per second,
#define MICROSTEP 1;

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
int goalReached;

//Present Encoder Values
int encoder1;
int encoder2;

//Duty cycle of step movement
uint32_t dcDMA;

void changeSpeed(int speed, float const dc, TIM_TypeDef *TIMx, uint32_t channel) {
    //Require arr value for specific frequency of pulses. 100pps = 1Mhz/10000. 1000 = 1Mhz/100. 1000 = arr.
    TIMx->ARR = 1000000/speed;//pulses per second
    TIMx->CCR1 = 50;
}

void stepMove(int const step, float const speed, TIM_TypeDef *TIMx, uint32_t channel) {
    //Configure PWM for one shot step move with fixed frequency.
    //Setup DMA duty cycle, this should be sent "steps" number of times.
    TIMx->ARR = (uint32_t)(1000000.0/speed);//pulses per second
    dcDMA = (uint32_t)((float)(TIMx->ARR)*0.5);

    TIMx->EGR = TIM_EGR_UG; //Trigger a reset to clock in the RCR change
    TIMx->SR &= ~(TIM_SR_UIF); //Reset the interrupt flag

    HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, &dcDMA, step);
}

void speedMove(int const speed, TIM_TypeDef *TIMx, uint32_t channel) {
    //configure PWM for speed frequency
    changeSpeed(speed, 50, TIMx, channel);
    HAL_TIM_PWM_Start(&htim1, channel);
}

void stopMotor(uint32_t channel) {
    HAL_TIM_PWM_Stop(&htim1, channel);
}

void cutWire() {

}

void stripWire() {

}


void vActuatorTask(){
    //Initialize motor variables
    motorStatus = IDLE;
    encoder1 = 0;
    encoder2 = 0;
    //Set the necessary pins
    // HAL_GPIO_WritePin(M1_MS1_GPIO_Port, M1_MS1_Pin, GPIO_PIN_SET);

    for(;;){
        //Motor status set by stateMachine
        stepMove(500, 100, TIM1, TIM_CHANNEL_1);
        // while (motorStatus!=IDLE) {
        //     if (goalReached) {
        //         motorStatus = IDLE;
        //     }
        //     vTaskDelay(1000);
        // }
        vTaskDelay(7000);
    }
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    /* Prevent unused argument(s) compilation warning */
    HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);

    /* NOTE : This function should not be modified, when the callback is needed,
              the HAL_TIM_PWM_PulseFinishedCallback could be implemented in the user file
     */
}
