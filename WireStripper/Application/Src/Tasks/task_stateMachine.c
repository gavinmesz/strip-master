//
// Created by Admin on 11/22/2025.
//
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

//task specific includes
#include "../../Inc/Tasks/task_stateMachine.h"
#include <stdio.h>
#include "usart.h"
#include "task_manager.h"

void vStateMachineTask() {
    uint8_t smMsg[] = {4};
    for (;;) {
        *x = 3;
        // HAL_UART_Transmit(&huart3, smMsg, 1, 1000);
        // printf("State Machine\r\n");
        vTaskDelay(100);
    }
}
