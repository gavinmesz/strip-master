/*
 * Safety Task:
 * Absolutely must run periodically. Make sure that all PG pins are OK. Send status.
 */

#include "task_manager.h" // Has FreeRTOS functions and globals defined

//task specific includes
#include "task_safety.h"

void vSafetyTask() {
    for (;;) {

        /*
         * Notes on the BMS
         * When entering into normal from ship mode: 250ms
         * Full updates given every 250ms
         *
         * V(cell) = GAIN * ADC(cell) + OFFSET. GAIN and OFFSET are found in the registers.
         * Coulomb counter: 16 bit integrating ADC
         * CC reading = 12-bit 2's complement * 8.44uV/LSB
         *
         * Pack voltage: Vbat = 4*GAIN*ADC(cell) + (#cells * OFFSET)
         *
         * Protection: Make sure to load new values during a startup sequence every time.
         * - Protection thresholds and delays should be set by MCU to warn of any possible delays this system might have.
         * - OCD SCD: Upon fault, the delay counter will count up then set ALERT to high. SYS_STAT hold fault condition.
         * PROTECT1, PROTECT2 holds configuration.
         * - OV, UV: OV_TRIP (3.15V ri 4.7V) 10-XXXX-XXXX-1000, UV_TRIP (1.58V to 3.1V) 01-XXXX-XXXX-0000. Calc in datasheet.
         * - Have to manually clear the ALERT pin.
         *
         * FETs:
         * Have to manually control the discharge FET.
         *
         * Cell balancing:
         * -
         *
         *
         *
         */

        counterVar++;
        vTaskDelay(100);
    }
}
