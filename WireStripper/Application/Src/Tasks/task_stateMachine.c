/*
 * State Machine Task:
 * Determines current device state and sends motor jobs.
 */

#include "task_manager.h" // Has FreeRTOS functions and globals defined

//task specific includes
#include "task_stateMachine.h"

void vStateMachineTask() {
    for (;;) {
        counterVar++;
        vTaskDelay(100);
    }
}
