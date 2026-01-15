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

#define FEED_SPEED 100; //pulse/sec
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

void stepMove(int step) {
    //Configure PWM for one shot step move with fixed frequency.
    //Initiate one shot pulse train.
}

void speedMove() {
    //configure PWM for speed frequency
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
        (&htim2)->Instance->RCR = 100;
        __HAL_TIM_ENABLE(&htim2);
        // while (motorStatus!=IDLE) {
        //     if (goalReached) {
        //         motorStatus = IDLE;
        //     }
        //     vTaskDelay(1000);
        // }
        vTaskDelay(1000);
    }
}
