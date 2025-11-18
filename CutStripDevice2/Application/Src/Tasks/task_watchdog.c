//main includes
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "task_watchdog.h"

//specific includes
#include <stdio.h>
#include "task_display.h"

void vWatchdogTask(){
	for(;;){
		printf("Watchdog\r\n");
		COUNTER_VAR++;
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}
