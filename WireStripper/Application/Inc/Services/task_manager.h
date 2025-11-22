#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include "FreeRTOS.h"
#include "task.h"

extern TaskHandle_t xDisplayTaskHandle;
extern TaskHandle_t xActuatorTaskHandle;
extern TaskHandle_t xSafetyTaskHandle;

void TaskManager_InitTasks(void);
void TaskManager_CreateAllTasks(void);

#endif
