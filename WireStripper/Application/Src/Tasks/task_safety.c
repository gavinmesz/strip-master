//
// Created by Gavin on 11/22/2025.
//

//main includes
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

//task specific includes
#include "../../Inc/Tasks/task_safety.h"
#include <stdio.h>

#include "task.h"

void vSafetyTask() {
    //Poll for power good across bucks
    for (;;) {
        printf("Safety");
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
