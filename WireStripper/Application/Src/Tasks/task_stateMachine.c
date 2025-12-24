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

void vStateMachineTask() {
    uint8_t smMsg[] = {4};
    for (;;) {
        HAL_UART_Transmit(&huart3, smMsg, 1, 1000);
        // printf("State Machine\r\n");
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
