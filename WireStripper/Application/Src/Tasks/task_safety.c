/*
 * Safety Task:
 * Absolutely must run periodically. Make sure that all PG pins are OK. Send status.
 */

//main includes
#include "task_manager.h"
#include "task.h"

//task specific includes
#include "task_safety.h"

void vSafetyTask() {
    //Poll for power good across bucks

    for (;;) {
        counterVar++;
        vTaskDelay(100);
    }
}
