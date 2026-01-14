/*
 * Actuator Control Task:
 * Includes PID control for all motors. Take information from encoders and state machine, perform jobs, report status.
 */

#include "task_manager.h" // Has FreeRTOS functions and globals defined

//specific includes
#include "task_actuatorControl.h"

//PID Terms
#define P1 = (float)1;
#define I1 = (float)0;
#define D1 = (float)0;
#define P2 = (float)1;
#define I2 = (float)0;
#define D2 = (float)0;

//Trajectories
typedef enum {
    IDLE,
    STEP1,
    STEP2,
    STEP3,
    STEP4,
    STEP5
}MotorStatus;
MotorStatus motorStatus;

//Motor Values
int encoder1Step;
int encoder2Step;

void vActuatorTask(){
    //Initialize variables
    motorStatus = IDLE;
    encoder1Step = 0;
    encoder2Step = 0;
    for(;;){
        counterVar++;
        vTaskDelay(100);
    }
}
