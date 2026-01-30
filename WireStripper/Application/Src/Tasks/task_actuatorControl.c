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
#include "task_stateMachine.h"

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
    START,
    CALIB, //M1 feed to wire detecta
    FEED1, //M1 feed one strip length past cutters
    STRIP1, //cutter engage to strip, back off very slightly (loop would start here if wire #2)
    PEEL1, //M1 motor reverse distance
    FEED2, //M1,M2 feed forward one full length
    CUT, //cutter engage full and disengge
    FEED3, //M1 motor reverse to wire detect, M2 reverse one strip length
    STRIP2, //cutter engage to strip, back off very slightly
    PEEL2, //M2 push out fully, M1 feed forward 1 strip length past.
}MotorStatus;

static MotorStatus motorStatus;

typedef enum {
    PLACEHOLDER,
    M1,
    M2,
    M3
}MotorNum;

typedef struct {
    MotorNum motor_num;

    //Timer Peripheral
    TIM_HandleTypeDef* htim;
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
static Motor Motor1;
static Motor Motor2;
static Motor Motor3;

//Present Encoder Values
int encoder1;
int encoder2;

//Duty cycle of step movement
uint32_t dcDMA1;
uint32_t dcDMA2;
uint32_t dcDMA3;

static uint8_t homeSetM3() {
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

static void enableMotor(uint8_t state, Motor const motor) {
    //Reminder M3 is nEN
    if (motor.motor_num == M3) {state = ~state;}
    HAL_GPIO_WritePin(motor.EN_Port, motor.EN_Pin, state);
}

static void wakeMotor(uint8_t state, Motor const motor) {
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

static void microSet(uint8_t const microStep, Motor const motor) {
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
static void changeSpeed(float const speed, uint8_t const dir, Motor *motor) {
    //Require arr value for specific frequency of pulses. 100pps = 1Mhz/10000. 1000 = 1Mhz/100. 1000 = arr.
    motor->TIMx->ARR = (uint32_t)(CLK_SPEED/speed);//pulses per second

    motor->TIMx->EGR = TIM_EGR_UG; //Trigger a reset due to register change
    motor->TIMx->SR &= ~(TIM_SR_UIF); //Reset the interrupt flag

    motor->ccr = (uint32_t)((float)(motor->TIMx->ARR)*0.5);
    HAL_GPIO_WritePin(motor->DIR_Port, motor->DIR_Pin, dir);
}

//Movement of x steps in one direction at speed.
//Return 0 if the step move has not finished yet. Return 1 if the DMA was started.
static uint8_t stepMove(int const step, float const speed, uint8_t const dir, Motor* motor) {
    if (motor->motorDone) {
        changeSpeed(speed, dir, motor);
        HAL_TIM_PWM_Start_DMA(motor->htim, motor->channel, &motor->ccr, step);
        motor->motorDone=0;
        return 1;
    }
    return 0;
}

//continuous movement in one direction
static uint8_t speedMove(int const speed, uint8_t const dir, Motor* motor) {
    if (motor->motorDone) {
        changeSpeed(speed, dir, motor);
        HAL_TIM_PWM_Start(motor->htim, motor->channel);
        motor->motorDone=0;
        return 1;
    }
    return 0;
}

static void stopMotor(Motor const motor) {
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
        //Acknowledge that job was received.
        if (systemState == ENGAGE) {
            job_finish = 0;
        } else if (systemState == DISENGAGE) {
            job_finish = 0;
        } else if (systemState == JOB_RUNNING) {
            job_finish = 0;
        }

        //Motor status set by stateMachine
        stepMove(100, 100, TO_FRONT,&Motor1);
        stepMove(200, 100, TO_FRONT,&Motor2);
        stepMove(300, 100, TO_FRONT,&Motor3);
        vTaskDelay(5000);
    }
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
    /* Prevent unused argument(s) compilation warning */
    //Check if TIM state is not busy (state 0x02U)
    if (htim == Motor1.htim) {
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
