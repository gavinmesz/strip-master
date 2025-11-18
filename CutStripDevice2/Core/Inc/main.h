/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
extern int COUNTER_VAR;
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BMS_ALERT_Pin GPIO_PIN_1
#define BMS_ALERT_GPIO_Port GPIOC
#define LIGHT_IN_Pin GPIO_PIN_0
#define LIGHT_IN_GPIO_Port GPIOA
#define GAUGE_IN_Pin GPIO_PIN_1
#define GAUGE_IN_GPIO_Port GPIOA
#define USART_TX_Pin GPIO_PIN_2
#define USART_TX_GPIO_Port GPIOA
#define USART_RX_Pin GPIO_PIN_3
#define USART_RX_GPIO_Port GPIOA
#define BUCK5_PG_Pin GPIO_PIN_4
#define BUCK5_PG_GPIO_Port GPIOA
#define BUCK5_EN_Pin GPIO_PIN_5
#define BUCK5_EN_GPIO_Port GPIOA
#define ENC1_A_Pin GPIO_PIN_6
#define ENC1_A_GPIO_Port GPIOA
#define ENC1_B_Pin GPIO_PIN_7
#define ENC1_B_GPIO_Port GPIOA
#define BUCK12_PG_Pin GPIO_PIN_5
#define BUCK12_PG_GPIO_Port GPIOC
#define BUCK12_EN_Pin GPIO_PIN_0
#define BUCK12_EN_GPIO_Port GPIOB
#define OLED_DC_Pin GPIO_PIN_1
#define OLED_DC_GPIO_Port GPIOB
#define LDO_EN_Pin GPIO_PIN_2
#define LDO_EN_GPIO_Port GPIOB
#define OLED_CS_Pin GPIO_PIN_12
#define OLED_CS_GPIO_Port GPIOB
#define LA_MODE_Pin GPIO_PIN_13
#define LA_MODE_GPIO_Port GPIOB
#define LA_IN1_Pin GPIO_PIN_14
#define LA_IN1_GPIO_Port GPIOB
#define M2_SLP_Pin GPIO_PIN_15
#define M2_SLP_GPIO_Port GPIOB
#define M2_EN_Pin GPIO_PIN_6
#define M2_EN_GPIO_Port GPIOC
#define M2_MS2_Pin GPIO_PIN_7
#define M2_MS2_GPIO_Port GPIOC
#define M2_MS1_Pin GPIO_PIN_8
#define M2_MS1_GPIO_Port GPIOC
#define M2_FLT_Pin GPIO_PIN_9
#define M2_FLT_GPIO_Port GPIOC
#define M1_STEP_Pin GPIO_PIN_8
#define M1_STEP_GPIO_Port GPIOA
#define M2_STEP_Pin GPIO_PIN_9
#define M2_STEP_GPIO_Port GPIOA
#define LA_PWM_Pin GPIO_PIN_10
#define LA_PWM_GPIO_Port GPIOA
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define M2_DIR_Pin GPIO_PIN_15
#define M2_DIR_GPIO_Port GPIOA
#define M1_SLP_Pin GPIO_PIN_10
#define M1_SLP_GPIO_Port GPIOC
#define M1_EN_Pin GPIO_PIN_11
#define M1_EN_GPIO_Port GPIOC
#define M1_MS2_Pin GPIO_PIN_12
#define M1_MS2_GPIO_Port GPIOC
#define M1_MS1_Pin GPIO_PIN_2
#define M1_MS1_GPIO_Port GPIOD
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB
#define M1_FLT_Pin GPIO_PIN_4
#define M1_FLT_GPIO_Port GPIOB
#define M1_DIR_Pin GPIO_PIN_5
#define M1_DIR_GPIO_Port GPIOB
#define ENC2_A_Pin GPIO_PIN_6
#define ENC2_A_GPIO_Port GPIOB
#define ENC2_B_Pin GPIO_PIN_7
#define ENC2_B_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
// OLED display pins
#define OLED_CS_Pin GPIO_PIN_12
#define OLED_CS_GPIO_Port GPIOB // Double checked
#define OLED_DC_Pin GPIO_PIN_1
#define OLED_DC_GPIO_Port GPIOB
#define OLED_RST_Pin GPIO_PIN_0
#define OLED_RST_GPIO_Port GPIOB
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
