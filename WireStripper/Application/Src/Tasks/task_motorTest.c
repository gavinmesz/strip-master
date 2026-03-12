/*
 * Motor Test Task
 * Quick test program for the motors. Jog and run a certain distance.
 */

#include "task_actuatorControl.h"
#include "task_motorTest.h"
#include "task_manager.h"
#include "task_safety.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "task_display.h"

#ifndef GAUGE_IN
#define GAUGE_IN (1<<2)
#endif

extern UART_HandleTypeDef huart2;

// Jog state variables
static int jog_speed = 100;
static uint8_t hold_to_run_active = 0; // 0 = Continuous, 1 = Hold-to-Run
uint8_t gauge_detect;

// Independent timeouts for each motor
static uint32_t last_m1_tick = 0;
static uint32_t last_m2_tick = 0;
static uint32_t last_m3_tick = 0;
static const uint32_t JOG_TIMEOUT_MS = 150;

static uint8_t step_requested[4] = {0, 0, 0, 0};

#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif

PUTCHAR_PROTOTYPE {
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

void print_logo() {
    printf("\r\n  _____ _____ __          __ _      _______ ______  _____ _______ ");
    printf("\r\n / ____|  __ \\\\ \\        / /| |    |__   __|  ____|/ ____|__   __|");
    printf("\r\n| (___ | |__) |\\ \\  /\\  / / | |       | |  | |__  | (___    | |   ");
    printf("\r\n \\___ \\|  ___/  \\ \\/  \\/ /  | |       | |  |  __|  \\___ \\   | |   ");
    printf("\r\n ____) | |       \\  /\\  /   | |____   | |  | |____ ____) |  | |   ");
    printf("\r\n|_____/|_|        \\/  \\/    |______|  |_|  |______|_____/   |_|   \r\n");
}

void print_help() {
    printf("\r\n--- SPWL TEST COMMANDS ---");
    printf("\r\nstatus                : Show encoders and BMS status");
    printf("\r\npins                  : Show status of all motor GPIO pins");
    printf("\r\nsetpin m[1-3] [pin] [val] : Set GPIO pin. Pins: en, dir, ms1, ms2, nslp, nrst");
    printf("\r\nsetpin buck12 [val]   : Set BUCK12_EN pin (1=ON, 0=OFF)");
    printf("\r\njog                   : Enter Real-time control mode");
    printf("\r\nstep m[1-3] [s] [pps] : Move motor (use - for reverse, e.g., step m1 -500 200)");
    printf("\r\nstrip                 : Run M2 until gauge triggers, then back off");
    printf("\r\nhelp                  : Show this menu\r\n");
}

void print_motor_pins() {
    printf("\r\n------ MOTOR PIN STATUS ------\r\n");
    printf("M1: EN=%d DIR=%d MS1=%d MS2=%d nSLP=%d nFLT=%d\r\n",
           HAL_GPIO_ReadPin(M1_EN_GPIO_Port, M1_EN_Pin),
           HAL_GPIO_ReadPin(M1_DIR_GPIO_Port, M1_DIR_Pin),
           HAL_GPIO_ReadPin(M1_MS1_GPIO_Port, M1_MS1_Pin),
           HAL_GPIO_ReadPin(M1_MS2_GPIO_Port, M1_MS2_Pin),
           HAL_GPIO_ReadPin(M1_nSLP_GPIO_Port, M1_nSLP_Pin),
           HAL_GPIO_ReadPin(M1_nFLT_GPIO_Port, M1_nFLT_Pin));

    printf("M2: EN=%d DIR=%d MS1=%d MS2=%d nSLP=%d nFLT=%d\r\n",
           HAL_GPIO_ReadPin(M2_EN_GPIO_Port, M2_EN_Pin),
           HAL_GPIO_ReadPin(M2_DIR_GPIO_Port, M2_DIR_Pin),
           HAL_GPIO_ReadPin(M2_MS1_GPIO_Port, M2_MS1_Pin),
           HAL_GPIO_ReadPin(M2_MS2_GPIO_Port, M2_MS2_Pin),
           HAL_GPIO_ReadPin(M2_nSLP_GPIO_Port, M2_nSLP_Pin),
           HAL_GPIO_ReadPin(M2_nFLT_GPIO_Port, M2_nFLT_Pin));

    printf("M3: nEN=%d DIR=%d SM0=%d SM1=%d nRST=%d nFLT=%d\r\n",
           HAL_GPIO_ReadPin(M3_nEN_GPIO_Port, M3_nEN_Pin),
           HAL_GPIO_ReadPin(M3_DIR_GPIO_Port, M3_DIR_Pin),
           HAL_GPIO_ReadPin(M3_SM0_GPIO_Port, M3_SM0_Pin),
           HAL_GPIO_ReadPin(M3_SM1_GPIO_Port, M3_SM1_Pin),
           HAL_GPIO_ReadPin(M3_RST_GPIO_Port, M3_RST_Pin),
           HAL_GPIO_ReadPin(M3_nFLT_GPIO_Port, M3_nFLT_Pin));

    printf("BUCK12: EN=%d\r\n>",
           HAL_GPIO_ReadPin(BUCK12_EN_GPIO_Port, BUCK12_EN_Pin));
}

void vMotorTestTask(void *argument) {
    uint8_t rx_char;
    CLIMode currentMode = MODE_COMMAND;
    char cmd_buffer[64];
    uint8_t idx = 0;

    Motor1 = (Motor) { M1, M1_TIMER, TIM8, TIM_CHANNEL_1, TIM8->CCR1,
                       M1_EN_GPIO_Port, M1_EN_Pin, M1_DIR_GPIO_Port, M1_DIR_Pin,
                       M1_MS1_GPIO_Port, M1_MS1_Pin, M1_MS2_GPIO_Port, M1_MS2_Pin,
                       M1_nFLT_GPIO_Port, M1_nFLT_Pin, 1 };

    Motor2 = (Motor) { M2, M2_TIMER, TIM8, TIM_CHANNEL_4, TIM8->CCR4,
                       M2_EN_GPIO_Port, M2_EN_Pin, M2_DIR_GPIO_Port, M2_DIR_Pin,
                       M2_MS1_GPIO_Port, M2_MS1_Pin, M2_MS2_GPIO_Port, M2_MS2_Pin,
                       M2_nFLT_GPIO_Port, M2_nFLT_Pin, 1 };

    Motor3 = (Motor) { M3, M3_TIMER, TIM1, TIM_CHANNEL_1, TIM1->CCR1,
                       M3_nEN_GPIO_Port, M3_nEN_Pin, M3_DIR_GPIO_Port, M3_DIR_Pin,
                       M3_SM0_GPIO_Port, M3_SM0_Pin, M3_SM1_GPIO_Port, M3_SM1_Pin,
                       M3_nFLT_GPIO_Port, M3_nFLT_Pin, 1 };

    microSet(2, Motor1);
    microSet(2, Motor2);
    microSet(0, Motor3);

    wakeMotor(0, Motor1);
    wakeMotor(0, Motor2);

    print_logo();
    print_help();
    printf(">");

    for (;;) {
        for (int i = 1; i <= 3; i++) {
            Motor* m = (i == 1) ? &Motor1 : (i == 2) ? &Motor2 : &Motor3;
            if (m->motorDone && step_requested[i]) {
                step_requested[i] = 0;
                printf("\r\nM%d movement finished\r\n>", i);
                if(idx > 0) HAL_UART_Transmit(&huart2, (uint8_t*)cmd_buffer, idx, 10);
            }
        }

        if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_ORE)) {
            __HAL_UART_CLEAR_OREFLAG(&huart2);
        }

        if (HAL_UART_Receive(&huart2, &rx_char, 1, 5) == HAL_OK) {
            if (currentMode == MODE_COMMAND) {
                if (rx_char == '\r' || rx_char == '\n') {
                    cmd_buffer[idx] = '\0';
                    printf("\r\n");

                    if (idx == 0) {
                        printf(">");
                    }
                    else if (strcmp(cmd_buffer, "help") == 0) {
                        print_help();
                        printf(">");
                    }
                    else if (strcmp(cmd_buffer, "jog") == 0) {
                        currentMode = MODE_JOG;
                        printf("!!! JOG MODE ACTIVE !!!\r\n");
                        printf("M1:A/D | M2:F/H | M1+M2:C/B | M3:J/L | Spd:+/- | Mode: M | [SPACE]:Stop | [Q]:Exit\r\n");
                        printf("Speed: %d pps | Mode: %s\r\n>", jog_speed, hold_to_run_active ? "Hold-to-Run" : "Continuous");
                    }
                    else if (strcmp(cmd_buffer, "status") == 0) {
                        printf("\r\n------ STATUS ------\r\n");
                        printf("Enc1: %d | Enc2: %d\r\n", adcVals3[0], adcVals3[1]);
                        printf("Pack Current: %.2f mA\r\n", packCurrent);
                        printf("Pack Voltage: %.2f V\r\n", BMS.Vpack);
                        printf("Cell Voltages: %.2fV, %.2fV, %.2fV, %.2fV \r\n>", BMS.Vcell[0], BMS.Vcell[1], BMS.Vcell[2], BMS.Vcell[3]);
                    }
                    else if (strcmp(cmd_buffer, "pins") == 0) {
                        print_motor_pins();
                    }
                    else if (strncmp(cmd_buffer, "setpin", 6) == 0) {
                        int m_num = 0, val = 0;
                        char pin_name[10];

                        if (sscanf(cmd_buffer, "setpin m%d %9s %d", &m_num, pin_name, &val) == 3) {
                            GPIO_TypeDef* port = NULL;
                            uint16_t pin = 0;
                            int valid = 1;

                            // Map the pin strings to the hardware macros
                            if (m_num == 1) {
                                if (strcmp(pin_name, "en") == 0) { port = M1_EN_GPIO_Port; pin = M1_EN_Pin; }
                                else if (strcmp(pin_name, "dir") == 0) { port = M1_DIR_GPIO_Port; pin = M1_DIR_Pin; }
                                else if (strcmp(pin_name, "ms1") == 0) { port = M1_MS1_GPIO_Port; pin = M1_MS1_Pin; }
                                else if (strcmp(pin_name, "ms2") == 0) { port = M1_MS2_GPIO_Port; pin = M1_MS2_Pin; }
                                else if (strcmp(pin_name, "nslp") == 0 || strcmp(pin_name, "slp") == 0) { port = M1_nSLP_GPIO_Port; pin = M1_nSLP_Pin; }
                                else valid = 0;
                            } else if (m_num == 2) {
                                if (strcmp(pin_name, "en") == 0) { port = M2_EN_GPIO_Port; pin = M2_EN_Pin; }
                                else if (strcmp(pin_name, "dir") == 0) { port = M2_DIR_GPIO_Port; pin = M2_DIR_Pin; }
                                else if (strcmp(pin_name, "ms1") == 0) { port = M2_MS1_GPIO_Port; pin = M2_MS1_Pin; }
                                else if (strcmp(pin_name, "ms2") == 0) { port = M2_MS2_GPIO_Port; pin = M2_MS2_Pin; }
                                else if (strcmp(pin_name, "nslp") == 0 || strcmp(pin_name, "slp") == 0) { port = M2_nSLP_GPIO_Port; pin = M2_nSLP_Pin; }
                                else valid = 0;
                            } else if (m_num == 3) {
                                // M3 has slightly different pin nomenclature (nEN, SM0, SM1, nRST). Handled gracefully:
                                if (strcmp(pin_name, "en") == 0 || strcmp(pin_name, "nen") == 0) { port = M3_nEN_GPIO_Port; pin = M3_nEN_Pin; }
                                else if (strcmp(pin_name, "dir") == 0) { port = M3_DIR_GPIO_Port; pin = M3_DIR_Pin; }
                                else if (strcmp(pin_name, "ms1") == 0 || strcmp(pin_name, "sm0") == 0) { port = M3_SM0_GPIO_Port; pin = M3_SM0_Pin; }
                                else if (strcmp(pin_name, "ms2") == 0 || strcmp(pin_name, "sm1") == 0) { port = M3_SM1_GPIO_Port; pin = M3_SM1_Pin; }
                                else if (strcmp(pin_name, "nrst") == 0 || strcmp(pin_name, "rst") == 0) { port = M3_RST_GPIO_Port; pin = M3_RST_Pin; }
                                else valid = 0;
                            } else {
                                valid = 0;
                            }

                            if (valid && port != NULL) {
                                HAL_GPIO_WritePin(port, pin, val ? GPIO_PIN_SET : GPIO_PIN_RESET);
                                printf("M%d %s set to %d\r\n>", m_num, pin_name, val ? 1 : 0);
                            } else {
                                printf("Invalid motor or pin name. Try: en, dir, ms1, ms2, nslp, nrst\r\n>");
                            }
                        }
                        // --- NEW BLOCK FOR BUCK12_EN ---
                        else if (sscanf(cmd_buffer, "setpin buck12 %d", &val) == 1) {
                            HAL_GPIO_WritePin(BUCK12_EN_GPIO_Port, BUCK12_EN_Pin, val ? GPIO_PIN_SET : GPIO_PIN_RESET);
                            printf("BUCK12_EN set to %d\r\n>", val ? 1 : 0);
                        }
                        // -------------------------------
                        else {
                            printf("Usage: setpin m[1-3] [pin] [0|1] OR setpin buck12 [0|1]\r\n>");
                        }
                    }
                    else if (strncmp(cmd_buffer, "step", 4) == 0) {
                        int m_num = 0, move_steps = 0, pps = 0;
                        if (sscanf(cmd_buffer, "step m%d %d %d", &m_num, &move_steps, &pps) == 3) {
                            if (m_num >= 1 && m_num <= 3 && pps > 0) {
                                Motor* target = (m_num == 1) ? &Motor1 : (m_num == 2) ? &Motor2 : &Motor3;

                                if (target->motorDone) {
                                    printf("M%d: Moving %d steps at %d pps...\r\n", m_num, abs(move_steps), pps);
                                    if (stepMove(move_steps, (float)pps, target)) {
                                        step_requested[m_num] = 1;
                                    }
                                } else {
                                    printf("M%d is BUSY\r\n>", m_num);
                                }
                            } else {
                                printf("Invalid input. Speed must be > 0.\r\n>");
                            }
                        } else {
                            printf("Usage: step m[1-3] [steps] [pps]\r\n>");
                        }
                    }
                    else if (strcmp(cmd_buffer, "strip") == 0) {
                        printf("M2 stripping (waiting for GAUGE_IN). Press 'q' to cancel...\r\n");

                        // 'h' direction maps to negative speed
                        if (speedMove(30, &Motor3)) {
                            uint8_t gauge_hit = 0;
                            uint32_t notifiedValue = 0;

                            while (!gauge_hit) {
                                gauge_detect = 1;
                                // Bailout condition to prevent endless spinning
                                if (HAL_UART_Receive(&huart2, &rx_char, 1, 0) == HAL_OK && (rx_char == 'q' || rx_char == 'Q')) {
                                    printf("\r\nStrip test cancelled.\r\n>");
                                    break;
                                }

                                // Poll for the FreeRTOS task notification (10ms timeout per loop)
                                if (xTaskNotifyWait(0x00, ULONG_MAX, &notifiedValue, pdMS_TO_TICKS(10)) == pdTRUE) {
                                    if (notifiedValue & GAUGE_IN) {
                                        gauge_hit = 1;
                                    }
                                }
                            }
                            gauge_detect = 0;

                            stopMotor(&Motor3);

                            if (gauge_hit) {
                                printf("\r\nGAUGE_IN detected! Backing off 100 steps...\r\n>");

                                // Back off in the opposite direction (positive steps)
                                if (stepMove(-200, (float)100, &Motor3)) {
                                    step_requested[2] = 1; // Flag so the main loop prints when the backoff finishes
                                }
                            }
                        } else {
                            printf("M2 is currently BUSY.\r\n>");
                        }
                    }
                    else {
                        printf("Unknown: %s\r\n>", cmd_buffer);
                    }
                    idx = 0;
                }
                else if (rx_char == 0x08 || rx_char == 0x7F) {
                    if (idx > 0) {
                        idx--;
                        HAL_UART_Transmit(&huart2, (uint8_t*)"\b \b", 3, 10);
                    }
                }
                else {
                    HAL_UART_Transmit(&huart2, &rx_char, 1, 10);
                    if (idx < 63) cmd_buffer[idx++] = rx_char;
                }
            }
            else {
                // --- JOG MODE LOGIC ---
                if (rx_char == 'q' || rx_char == 'Q') {
                    stopAllMotors();
                    currentMode = MODE_COMMAND;
                    printf("\r\nExited Jog Mode.\r\n>");
                } else if (rx_char == 'm' || rx_char == 'M') {
                    stopAllMotors();
                    hold_to_run_active = !hold_to_run_active;
                    printf("\r\nMode: %s", hold_to_run_active ? "Hold-to-Run" : "Continuous");
                } else if (rx_char == ' ') {
                    stopAllMotors();
                    step_requested[1] = step_requested[2] = step_requested[3] = 0;
                    printf("\r\n[HALT]");
                } else if (rx_char == '+' || rx_char == '=') {
                    jog_speed += 10;
                    printf("\r\nSpeed: %d pps", jog_speed);
                } else if (rx_char == '-' || rx_char == '_') {
                    if (jog_speed > 10) jog_speed -= 10;
                    printf("\r\nSpeed: %d pps", jog_speed);
                } else {
                    // Route the keypress to the correct motor and update its specific timeout tick
                    if (rx_char == 'a' || rx_char == 'A') { last_m1_tick = HAL_GetTick(); speedMove(-jog_speed,  &Motor1); }
                    if (rx_char == 'd' || rx_char == 'D') { last_m1_tick = HAL_GetTick(); speedMove(jog_speed, &Motor1); }
                    if (rx_char == 'f' || rx_char == 'F') { last_m2_tick = HAL_GetTick(); speedMove(-jog_speed,  &Motor2); }
                    if (rx_char == 'h' || rx_char == 'H') { last_m2_tick = HAL_GetTick(); speedMove(jog_speed, &Motor2); }
                    if (rx_char == 'j' || rx_char == 'J') { last_m3_tick = HAL_GetTick(); speedMove(-jog_speed,  &Motor3); }
                    if (rx_char == 'l' || rx_char == 'L') { last_m3_tick = HAL_GetTick(); speedMove(jog_speed, &Motor3); }

                    // Composite Controls: M1 & M2 Simultaneous
                    if (rx_char == 'c' || rx_char == 'C') {
                        last_m1_tick = HAL_GetTick();
                        last_m2_tick = HAL_GetTick();
                        speedMove(-jog_speed, &Motor1);
                        speedMove(-jog_speed, &Motor2);
                    }
                    if (rx_char == 'b' || rx_char == 'B') {
                        last_m1_tick = HAL_GetTick();
                        last_m2_tick = HAL_GetTick();
                        speedMove(jog_speed, &Motor1);
                        speedMove(jog_speed, &Motor2);
                    }
                }
            }
        }

        // --- Independent Timeout Checks for Hold-to-Run Mode ---
        if (currentMode == MODE_JOG && hold_to_run_active) {
            uint32_t current_tick = HAL_GetTick();

            if (!Motor1.motorDone && (current_tick - last_m1_tick) > JOG_TIMEOUT_MS) {
                stopMotor(&Motor1);
            }
            if (!Motor2.motorDone && (current_tick - last_m2_tick) > JOG_TIMEOUT_MS) {
                stopMotor(&Motor2);
            }
            if (!Motor3.motorDone && (current_tick - last_m3_tick) > JOG_TIMEOUT_MS) {
                stopMotor(&Motor3);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}