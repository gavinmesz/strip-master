#include "task_actuatorControl.h"
#include "task_motorTest.h"
#include "task_manager.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern UART_HandleTypeDef huart2;

/* Per-motor settings */
static int speeds_int[4] = {0, 100, 100, 100}; // Initialized to 100 pps
static uint8_t step_requested[4] = {0, 0, 0, 0};

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
    printf("\r\nstatus            : Show encoders and current speeds");
    printf("\r\njog               : Enter Real-time control mode");
    printf("\r\nspeed m[1-3] [v]  : Set motor speed (Note: M1 & M2 are coupled)");
    printf("\r\nstep m[1-3] [s]   : Move motor (use - for reverse, e.g., step m1 -500)");
    printf("\r\nhelp              : Show this menu\r\n");
}

void vMotorTestTask(void *argument) {
    uint8_t rx_char;
    CLIMode currentMode = MODE_COMMAND;
    char cmd_buffer[64];
    uint8_t idx = 0;

    print_logo();
    print_help();
    printf(">"); // Linux-style prompt (no newline)

    for (;;) {
        // --- Movement Finished Observer ---
        for (int i = 1; i <= 3; i++) {
            Motor* m = (i == 1) ? &Motor1 : (i == 2) ? &Motor2 : &Motor3;
            // Only notify if motor is ready AND we were actually waiting for it
            if (m->motorDone && step_requested[i]) {
                step_requested[i] = 0;
                printf("\r\nM%d movement finished\r\n>", i);
                // Reprint whatever the user was typing so the prompt stays clean
                if(idx > 0) HAL_UART_Transmit(&huart2, (uint8_t*)cmd_buffer, idx, 10);
            }
        }

        if (HAL_UART_Receive(&huart2, &rx_char, 1, 5) == HAL_OK) {
            if (currentMode == MODE_COMMAND) {
                // --- Handle Enter Key ---
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
                        printf("!!! JOG MODE ACTIVE !!!\r\nM1:A/D | M2:F/H | M3:J/L | [SPACE]:Stop | [Q]:Exit\r\n>");
                    }
                    else if (strcmp(cmd_buffer, "status") == 0) {
                        printf("\r\n------ STATUS ------\r\n");
                        printf("Enc1: %d | Enc2: %d\r\n", encoder1, encoder2);
                        printf("Speeds -> M1/M2: %d | M3: %d\r\n>", speeds_int[1], speeds_int[3]);
                    }
                    // --- Speed Command: "speed m1 500" ---
                    else if (strncmp(cmd_buffer, "speed", 5) == 0) {
                        int m_num = 0;
                        int val_int = 0;
                        if (sscanf(cmd_buffer, "speed m%d %d", &m_num, &val_int) == 2) {
                            if (m_num >= 1 && m_num <= 3) {
                                float val_f = (float)val_int;
                                if (m_num == 1 || m_num == 2) {
                                    // Hardware Coupling: Update both M1 and M2 (TIM8)
                                    changeSpeed(val_f, TO_FRONT, &Motor1);
                                    changeSpeed(val_f, TO_FRONT, &Motor2);
                                    speeds_int[1] = val_int;
                                    speeds_int[2] = val_int;
                                    printf("M1 & M2 Speed Updated: %d pps\r\n>", val_int);
                                } else {
                                    changeSpeed(val_f, DOWN, &Motor3);
                                    speeds_int[3] = val_int;
                                    printf("M3 Speed Updated: %d pps\r\n>", val_int);
                                }
                            }
                        } else {
                            printf("Usage: speed m[1-3] [pps]\r\n>");
                        }
                    }
                    // --- Step Command: "step m1 -1000" ---
                    else if (strncmp(cmd_buffer, "step", 4) == 0) {
                        int m_num = 0, move_steps = 0;
                        if (sscanf(cmd_buffer, "step m%d %d", &m_num, &move_steps) == 2) {
                            if (m_num >= 1 && m_num <= 3) {
                                Motor* target = (m_num == 1) ? &Motor1 : (m_num == 2) ? &Motor2 : &Motor3;
                                uint8_t dir = (move_steps >= 0) ? TO_FRONT : TO_BACK;
                                // For M3, use DOWN/UP mapping
                                if (m_num == 3) dir = (move_steps >= 0) ? DOWN : UP;

                                if (target->motorDone) {
                                    printf("M%d: Moving %d steps at %d pps...\r\n",
                                           m_num, abs(move_steps), speeds_int[m_num]);

                                    if (stepMove(abs(move_steps), (float)speeds_int[m_num], dir, target)) {
                                        step_requested[m_num] = 1;
                                    }
                                } else {
                                    printf("M%d is BUSY\r\n>", m_num);
                                }
                            }
                        } else {
                            printf("Usage: step m[1-3] [steps]\r\n>");
                        }
                    }
                    else {
                        printf("Unknown: %s\r\n>", cmd_buffer);
                    }
                    idx = 0;
                }
                // --- Handle Backspace ---
                else if (rx_char == 0x08 || rx_char == 0x7F) {
                    if (idx > 0) {
                        idx--;
                        HAL_UART_Transmit(&huart2, (uint8_t*)"\b \b", 3, 10);
                    }
                }
                // --- Echo Character ---
                else {
                    HAL_UART_Transmit(&huart2, &rx_char, 1, 10);
                    if (idx < 63) cmd_buffer[idx++] = rx_char;
                }
            }
            else {
                // --- JOG MODE LOGIC ---
                if (rx_char == 'q' || rx_char == 'Q') {
                    stopMotor(&Motor1); stopMotor(&Motor2); stopMotor(&Motor3);
                    currentMode = MODE_COMMAND;
                    printf("\r\nExited Jog Mode.\r\n>");
                } else if (rx_char == ' ') {
                    stopMotor(&Motor1); stopMotor(&Motor2); stopMotor(&Motor3);
                    step_requested[1] = step_requested[2] = step_requested[3] = 0;
                    printf("\r\n[HALT]");
                } else {
                    // Pull from updated speeds_int array
                    if (rx_char == 'a' || rx_char == 'A') speedMove((float)speeds_int[1], TO_BACK,  &Motor1);
                    if (rx_char == 'd' || rx_char == 'D') speedMove((float)speeds_int[1], TO_FRONT, &Motor1);
                    if (rx_char == 'f' || rx_char == 'F') speedMove((float)speeds_int[2], TO_BACK,  &Motor2);
                    if (rx_char == 'h' || rx_char == 'H') speedMove((float)speeds_int[2], TO_FRONT, &Motor2);
                    if (rx_char == 'j' || rx_char == 'J') speedMove((float)speeds_int[3], DOWN,     &Motor3);
                    if (rx_char == 'l' || rx_char == 'L') speedMove((float)speeds_int[3], UP,       &Motor3);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}