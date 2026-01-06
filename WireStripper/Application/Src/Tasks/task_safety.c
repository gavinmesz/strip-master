/*
 * Safety Task:
 * Absolutely must run periodically. Make sure that all PG pins are OK. Send status.
 */

#include "task_manager.h" // Has FreeRTOS functions and globals defined

//task specific includes
#include "task_safety.h"

void vSafetyTask() {
    for (;;) {
        counterVar++;
        vTaskDelay(100);
    }
}
