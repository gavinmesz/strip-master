//
// Motor test: CLI with button forwards and backwards.
//
#include "task_actuatorControl.h"
#include "task_motorTest.h"
#include "task_manager.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "usart.h" // CubeMX generated UART header

#define CLI_BUFFER_SIZE 64
extern UART_HandleTypeDef huart2;

void vMotorTestTask() {
    uint8_t rx_char;
    CLIMode currentMode = MODE_COMMAND;
    char cmd_buffer[32];
    uint8_t idx = 0;

    printf("\r\n--- System Initialized ---\r\nType 'jog' to enter Manual Control, 'status' for info.\r\n>");

    for (;;) {
        if (HAL_UART_Receive(&huart2, &rx_char, 1, 10) == HAL_OK) {

            if (currentMode == MODE_COMMAND) {
                // --- COMMAND MODE LOGIC ---
                if (rx_char == '\r' || rx_char == '\n') {
                    cmd_buffer[idx] = '\0';
                    printf("\r\n");

                    if (strcmp(cmd_buffer, "jog") == 0) {
                        currentMode = MODE_JOG;
                        printf("!!! ENTERING JOG MODE !!!\r\n");
                        printf("M1: A/D | M2: F/H | M3: J/L | SPACE: Stop | q: Exit\r\n>");
                    } else if (strcmp(cmd_buffer, "status") == 0) {
                        printf("Enc1: %d, Enc2: %d, State: %d\r\n>", encoder1, encoder2, motorStatus);
                    } else {
                        printf("Unknown Command.\r\n>");
                    }
                    idx = 0;
                } else {
                    HAL_UART_Transmit(&huart2, &rx_char, 1, 10); // Echo
                    if (idx < 31) cmd_buffer[idx++] = rx_char;
                }

            } else {
                // --- JOG MODE LOGIC ---
                switch (rx_char) {
                    case 'a': case 'A': speedMove(500, TO_BACK,  &Motor1); break;
                    case 'd': case 'D': speedMove(500, TO_FRONT, &Motor1); break;

                    case 'f': case 'F': speedMove(500, TO_BACK,  &Motor2); break;
                    case 'h': case 'H': speedMove(500, TO_FRONT, &Motor2); break;

                    case 'j': case 'J': speedMove(300, DOWN,     &Motor3); break;
                    case 'l': case 'L': speedMove(300, UP,       &Motor3); break;

                    case ' ': // Space to Emergency Stop
                        stopMotor(&Motor1); stopMotor(&Motor2); stopMotor(&Motor3);
                        printf("\r\n[HALT]");
                        break;

                    case 'q': // ESC key to exit Jog Mode
                        stopMotor(&Motor1); stopMotor(&Motor2); stopMotor(&Motor3);
                        currentMode = MODE_COMMAND;
                        printf("\r\nExited Jog Mode.\r\n>");
                        break;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}