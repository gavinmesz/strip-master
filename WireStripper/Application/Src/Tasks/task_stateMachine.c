//
// Created by Admin on 11/22/2025.
//
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

//task specific includes
#include "../../Inc/Tasks/task_stateMachine.h"
#include <stdio.h>

void vStateMachineTask() {
    for (;;) {
        printf("State Machine");
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
