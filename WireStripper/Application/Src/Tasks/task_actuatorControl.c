//main includes
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

//specific includes
#include "task_actuatorControl.h"
#include <stdio.h>
#include "task_display.h"

void vActuatorTask(){


    for(;;){
        printf("Actuator Control\r\n");
        COUNTER_VAR++;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
