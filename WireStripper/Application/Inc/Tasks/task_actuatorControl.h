#ifndef TASK_WATCHDOG_H
#define TASK_WATCHDOG_H
#include "tim.h"
#include "timers.h"

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
#define TO_FRONT GPIO_PIN_RESET
#define TO_BACK GPIO_PIN_SET
#define UP GPIO_PIN_RESET
#define DOWN GPIO_PIN_SET

#define ENABLE 1
#define DISABLE 0

void vActuatorTask();

typedef enum {
    PLACEHOLDER,
    M1,
    M2,
    M3
} MotorNum;

typedef struct {
    MotorNum motor_num;
    TIM_HandleTypeDef* htim;
    TIM_TypeDef *TIMx;
    uint32_t channel;
    uint32_t ccr;
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
} Motor;

//Status
typedef enum {
    START,
    STRIP_ENGAGE1,
    M1_FULL_LENGTH_FEED,
    CUT,
    FEED_INLET_BACK,
    WAITING_FOR_WIRE_RESET,
    PRE_REDATUM_JOG,
    WAITING_FOR_REDATUM,
    STRIP_ENGAGE2,
    STRIP_LENGTH_2,
    SPIT,
    RESTART,
    IDLE // Added to match your vActuatorTask initialization
} MotorStatus;

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim);
uint8_t homeSetM3(void);
void enableMotor(uint8_t state, Motor const motor);
void wakeMotor(uint8_t state, Motor const motor);
void microSet(uint8_t const microStep, Motor const motor);
void changeSpeed(float const speed, uint8_t const dir, Motor *motor);
uint8_t stepMove(int const step, float const speed,  Motor* motor);
uint8_t speedMove(int const speed, Motor* motor);
void stopMotor(Motor *motor);
uint8_t cutWire(void);
uint8_t stripWire(void);
void stopAllMotors();

extern Motor Motor1;
extern Motor Motor2;
extern Motor Motor3;

extern int encoder1;
extern int encoder2;

extern MotorStatus motorStatus;

extern TimerHandle_t xStopGlitch;
extern TimerHandle_t xCoreDetectDelay;

#endif
