//
// Created by Gavin on 11/22/2025.
//

//main includes
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

//task specific includes
#include "../../Inc/Tasks/task_safety.h"
#include "usart.h"

void vSafetyTask() {
    //Poll for power good across bucks
    uint8_t safetyMsg[] = {3};

    for (;;) {
        HAL_UART_Transmit(&huart3, safetyMsg, 1, 1000);
        // printf("Safety\r\n");
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
