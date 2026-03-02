/*
 * Motor Test Task
 * Quick test program for the motors. Jog and run a certain distance.
 */

#include "task_actuatorControl.h"
#include "task_motorTest.h"
#include "task_manager.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern UART_HandleTypeDef huart2;

// Jog state variables
static int jog_speed = 100;
static uint8_t hold_to_run_active = 0; // 0 = Continuous, 1 = Hold-to-Run

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
    printf("\r\nstatus                : Show encoders");
    printf("\r\njog                   : Enter Real-time control mode");
    printf("\r\nstep m[1-3] [s] [pps] : Move motor (use - for reverse, e.g., step m1 -500 200)");
    printf("\r\nhelp                  : Show this menu\r\n");
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

    microSet(0, Motor1);
    microSet(0, Motor2);
    microSet(0, Motor3);

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
                        printf("Enc1: %d | Enc2: %d\r\n>", encoder1, encoder2);
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
                    jog_speed += 50;
                    printf("\r\nSpeed: %d pps", jog_speed);
                } else if (rx_char == '-' || rx_char == '_') {
                    if (jog_speed > 50) jog_speed -= 50;
                    printf("\r\nSpeed: %d pps", jog_speed);
                } else {
                    // Route the keypress to the correct motor and update its specific timeout tick
                    if (rx_char == 'a' || rx_char == 'A') { last_m1_tick = HAL_GetTick(); speedMove(jog_speed,  &Motor1); }
                    if (rx_char == 'd' || rx_char == 'D') { last_m1_tick = HAL_GetTick(); speedMove(-jog_speed, &Motor1); }
                    if (rx_char == 'f' || rx_char == 'F') { last_m2_tick = HAL_GetTick(); speedMove(jog_speed,  &Motor2); }
                    if (rx_char == 'h' || rx_char == 'H') { last_m2_tick = HAL_GetTick(); speedMove(-jog_speed, &Motor2); }
                    if (rx_char == 'j' || rx_char == 'J') { last_m3_tick = HAL_GetTick(); speedMove(jog_speed,  &Motor3); }
                    if (rx_char == 'l' || rx_char == 'L') { last_m3_tick = HAL_GetTick(); speedMove(-jog_speed, &Motor3); }

                    // Composite Controls: M1 & M2 Simultaneous
                    if (rx_char == 'c' || rx_char == 'C') {
                        last_m1_tick = HAL_GetTick();
                        last_m2_tick = HAL_GetTick();
                        speedMove(jog_speed, &Motor1);
                        speedMove(jog_speed, &Motor2);
                    }
                    if (rx_char == 'b' || rx_char == 'B') {
                        last_m1_tick = HAL_GetTick();
                        last_m2_tick = HAL_GetTick();
                        speedMove(-jog_speed, &Motor1);
                        speedMove(-jog_speed, &Motor2);
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