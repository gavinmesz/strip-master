#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include "FreeRTOS.h"
#include "task.h"

extern TaskHandle_t xDisplayTaskHandle;
extern TaskHandle_t xActuatorTaskHandle;
extern TaskHandle_t xSafetyTaskHandle;
extern TaskHandle_t xStateMachineTaskHandle;
extern TaskHandle_t xMotorTestHandle;

void TaskManager_InitTasks(void);
void TaskManager_CreateAllTasks(void);

extern int counterVar;

#endif
