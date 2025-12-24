//main includes
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

//specific includes
#include "task_actuatorControl.h"
#include "task_display.h"
#include "usart.h"


void vActuatorTask(){
    uint8_t actMsg[] = {2};

    for(;;){
        // printf("Actuator Control\r\n");
        HAL_UART_Transmit(&huart3, actMsg, 1, 1000);
        COUNTER_VAR++;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
