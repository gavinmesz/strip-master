/*
 * Safety Task:
 * Absolutely must run periodically. Make sure that all PG pins are OK. Send status.
 */

#define CELL_SHUT_DOWN 3.5

/*
         * Notes on the BMS
         * When entering into normal from ship mode: 250ms
         * Full updates given every 250ms
         *
         * V(cell) = GAIN * ADC(cell) + OFFSET. GAIN and OFFSET are found in the registers.
         * Coulomb counter: 16 bit integrating ADC
         * CC reading = 12-bit 2's complement * 8.44uV/LSB. CC_READY
         * - Gain error of around -0.55%FSR across all temps. counter offset of around -0.8uV.
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
         * - Must avoid undesirable enable combinations.
         * - Charge pump on driver will automatically enable but should def enable it upon startup.
         *
         * Cell balancing:
         * - Up to the host controller to set which algorithm is used. Adjacent cells cannot be balanced simultaneously.
         *
         * Alert:
         * - Host controller can manually pull up this pin to disable the pack. It can also drive it high if it needs to
         * - No internal debounce on the pin.
         * - DEVICE_XREADY -> clear this bit after a fault
         * - OVRD_ALERT -> when STM32 is pulling up ALERT.
         *
         * I2C
         * - 100kHz, slave, 7-bits address factory programmed.
         * - Block writes allowed by sending additional data bytes before the stop. auto increments register address.
         * - CRC (optional): x8 + x2 + x + 1, initial value is 0. single - calculated over slave address, reg addy, and data.
         * block write - first data byte calc same as single. subsequent calculated over data byte only.
         * - Bad CRC -> I2C slave will NACK the CRC and will enter idle. Repeated start is available.
         * - Timing requirements in the datasheet.
         *
         * Modes
         * - SHIP mode entered after every POR event. Super low power mode.
         *
         * Registers
         * SYS_CTRL1: read load present, RW adc enable, RW temp sel, shutdown command
         * SYS_CTRL2: delays, CC_EN, CC_ONESHOT, discharge and charge ON.
         * PROTECT1/PROTECT2: OCD and SCD thresholds and delay
         * PROTECT3: UV, OV delay
         * - If OV delay is 2s and UV delay is 4s, PROTECT 3 should be programmed with 0x50.
         * OV_TRIP/UV_TRIP: thresholds
         * - Ex. if the OV threshold is 4.3V, offset 0 and gain 382, the desired threshold is 11257 (0x2BF9). OV_TRIP
         * should be programmed with 0xBF.
         * CC_CFG_REGISTER: Set bits to 0x19 upon device startup
         * Lots of read only registers: cell voltage, vbat calculation, temperature, CC reading, ADC gain and offset
         *
         */

#include "task_manager.h" // Has FreeRTOS functions and globals defined

//task specific includes
#include "task_safety.h"
#include "BQ7692006PWR.h"
#include "task_actuatorControl.h"
#include "task_stateMachine.h"
#include "task_display.h"

BQ76920_t BMS;
float packCurrent;
float SOH;

static uint8_t checkSafety()  {
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET); //Heartbeat
    BMS.Vcell[0] = getCellVoltage(&BMS, VC1_HI); //~1ms
    BMS.Vcell[1] = getCellVoltage(&BMS, VC2_HI); //~1ms
    BMS.Vcell[2] = getCellVoltage(&BMS, VC3_HI); //~1ms
    BMS.Vcell[3] = getCellVoltage(&BMS, VC5_HI); //~1ms

    BMS.Vpack = getPackVoltage(&BMS); // Get V pack requires 2 bytes register read. ~1ms
    packCurrent = getCurrent(&BMS); // in mA requires 2 bytes register read. ~1ms
    if (colour%10==0) {
        BMS.SOC = SOCPack(&BMS, packCurrent, BMS.Vpack);
        SOH = SOHPack(&BMS);
    }
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET); //Heartbeat

    uint8_t result = 1;
    if (!HAL_GPIO_ReadPin(BUCK12_PG_GPIO_Port, BUCK12_PG_Pin) && HAL_GPIO_ReadPin(BUCK12_EN_GPIO_Port, BUCK12_EN_Pin)) {
        result = 0;
    }

    if (BMS.Vcell[0]<CELL_SHUT_DOWN || BMS.Vcell[1]<CELL_SHUT_DOWN || BMS.Vcell[2]<CELL_SHUT_DOWN || BMS.Vcell[3]<CELL_SHUT_DOWN) {
        result = 0;
    }

    // if (!HAL_GPIO_ReadPin(M1_nFLT_GPIO_Port,M1_nFLT_Pin) || !HAL_GPIO_ReadPin(M2_nFLT_GPIO_Port, M2_nFLT_Pin) || !HAL_GPIO_ReadPin(M3_nFLT_GPIO_Port, M3_nFLT_Pin)) {
    //     result = 0;
    // }
    return result;
}

void vSafetyTask() {
    safetyOK = 1;
    for (;;) {
        switch (systemState) {
            case SAFETY_ERROR: {
                checkSafety();
                readAlert(&BMS);
                break;
            }
            default: {
                if (!checkSafety()) {
                    safetyOK = 0;
                }
                break;
            }
        }

        counterVar++;
        vTaskDelay(200); //5Hz
    }
}