//main includes
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

//specific includes
#include "task_actuatorControl.h"
#include "task_display.h"
#include "usart.h"
#include "task_manager.h"
#include "cmsis_gcc.h"

void vActuatorTask(){
    uint8_t actMsg[] = {2};

    volatile uint32_t primask = __get_PRIMASK();
    volatile uint32_t basepri = __get_BASEPRI();
    volatile uint32_t tick    = xTaskGetTickCount();

    for(;;){
        // printf("Actuator Control\r\n");
        // HAL_UART_Transmit(&huart3, actMsg, 1, 1000);
        // COUNTER_VAR++;
        primask = __get_PRIMASK();
        basepri = __get_BASEPRI();
        tick    = xTaskGetTickCount();
        *x = 4;
        vTaskDelay(100);
    }
}
