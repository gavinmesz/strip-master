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
#include "../User/Config/DEV_Config.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define SERVO_PWM_Pin GPIO_PIN_7
#define SERVO_PWM_GPIO_Port GPIOF
#define MCO_Pin GPIO_PIN_0
#define MCO_GPIO_Port GPIOH
#define LIGHT_ON_1_Pin GPIO_PIN_2
#define LIGHT_ON_1_GPIO_Port GPIOC
#define LIGHT_ON_2_Pin GPIO_PIN_3
#define LIGHT_ON_2_GPIO_Port GPIOC
#define UX_KNOB2_A_Pin GPIO_PIN_0
#define UX_KNOB2_A_GPIO_Port GPIOA
#define UX_KNOB2_B_Pin GPIO_PIN_1
#define UX_KNOB2_B_GPIO_Port GPIOA
#define LIGHT_IN_2_Pin GPIO_PIN_3
#define LIGHT_IN_2_GPIO_Port GPIOA
#define GAUGE_IN_Pin GPIO_PIN_4
#define GAUGE_IN_GPIO_Port GPIOA
#define UX_POT_Pin GPIO_PIN_5
#define UX_POT_GPIO_Port GPIOA
#define VBAT_ADC_Pin GPIO_PIN_6
#define VBAT_ADC_GPIO_Port GPIOA
#define ENC1_B_Pin GPIO_PIN_7
#define ENC1_B_GPIO_Port GPIOA
#define LDO_EN_Pin GPIO_PIN_5
#define LDO_EN_GPIO_Port GPIOC
#define LD1_Pin GPIO_PIN_0
#define LD1_GPIO_Port GPIOB
#define BUCK12_PG_Pin GPIO_PIN_1
#define BUCK12_PG_GPIO_Port GPIOB
#define BUCK12_EN_Pin GPIO_PIN_2
#define BUCK12_EN_GPIO_Port GPIOB
#define ST_5V_Pin GPIO_PIN_11
#define ST_5V_GPIO_Port GPIOF
#define M2_SLP_Pin GPIO_PIN_13
#define M2_SLP_GPIO_Port GPIOF
#define M1_SLP_Pin GPIO_PIN_14
#define M1_SLP_GPIO_Port GPIOF
#define M1_EN_Pin GPIO_PIN_15
#define M1_EN_GPIO_Port GPIOF
#define M1_MS2_Pin GPIO_PIN_0
#define M1_MS2_GPIO_Port GPIOG
#define M1_MS1_Pin GPIO_PIN_1
#define M1_MS1_GPIO_Port GPIOG
#define M1_nFLT_Pin GPIO_PIN_7
#define M1_nFLT_GPIO_Port GPIOE
#define M1_DIR_Pin GPIO_PIN_8
#define M1_DIR_GPIO_Port GPIOE
#define M1_STEP_Pin GPIO_PIN_9
#define M1_STEP_GPIO_Port GPIOE
#define M2_EN_Pin GPIO_PIN_10
#define M2_EN_GPIO_Port GPIOE
#define M2_STEP_Pin GPIO_PIN_11
#define M2_STEP_GPIO_Port GPIOE
#define M2_MS2_Pin GPIO_PIN_12
#define M2_MS2_GPIO_Port GPIOE
#define M3_STP_Pin GPIO_PIN_13
#define M3_STP_GPIO_Port GPIOE
#define M2_MS1_Pin GPIO_PIN_14
#define M2_MS1_GPIO_Port GPIOE
#define M2_nFLT_Pin GPIO_PIN_15
#define M2_nFLT_GPIO_Port GPIOE
#define BMS_INT_Pin GPIO_PIN_12
#define BMS_INT_GPIO_Port GPIOB
#define M2_DIR_Pin GPIO_PIN_13
#define M2_DIR_GPIO_Port GPIOB
#define LD3_Pin GPIO_PIN_14
#define LD3_GPIO_Port GPIOB
#define M3_RST_Pin GPIO_PIN_15
#define M3_RST_GPIO_Port GPIOB
#define STLK_RX_Pin GPIO_PIN_8
#define STLK_RX_GPIO_Port GPIOD
#define STLK_TX_Pin GPIO_PIN_9
#define STLK_TX_GPIO_Port GPIOD
#define M3_nEN_Pin GPIO_PIN_10
#define M3_nEN_GPIO_Port GPIOD
#define M3_SM1_Pin GPIO_PIN_11
#define M3_SM1_GPIO_Port GPIOD
#define ENC2_A_Pin GPIO_PIN_12
#define ENC2_A_GPIO_Port GPIOD
#define ENC2_B_Pin GPIO_PIN_13
#define ENC2_B_GPIO_Port GPIOD
#define M3_SM0_Pin GPIO_PIN_14
#define M3_SM0_GPIO_Port GPIOD
#define M3_nFLT_Pin GPIO_PIN_15
#define M3_nFLT_GPIO_Port GPIOD
#define M3_DIR_Pin GPIO_PIN_2
#define M3_DIR_GPIO_Port GPIOG
#define M3_nHOME_Pin GPIO_PIN_3
#define M3_nHOME_GPIO_Port GPIOG
#define ENC1_A_Pin GPIO_PIN_6
#define ENC1_A_GPIO_Port GPIOC
#define OLED_CS_Pin GPIO_PIN_8
#define OLED_CS_GPIO_Port GPIOC
#define OLED_RST_Pin GPIO_PIN_9
#define OLED_RST_GPIO_Port GPIOC
#define OLED_DC_Pin GPIO_PIN_8
#define OLED_DC_GPIO_Port GPIOA
#define USB_DM_Pin GPIO_PIN_11
#define USB_DM_GPIO_Port GPIOA
#define USB_DP_Pin GPIO_PIN_12
#define USB_DP_GPIO_Port GPIOA
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define UX_KNOB1_A_Pin GPIO_PIN_15
#define UX_KNOB1_A_GPIO_Port GPIOA
#define UX_KNOB2_BUT_Pin GPIO_PIN_10
#define UX_KNOB2_BUT_GPIO_Port GPIOC
#define UX_KNOB1_BUT_Pin GPIO_PIN_11
#define UX_KNOB1_BUT_GPIO_Port GPIOC
#define LED3_Pin GPIO_PIN_6
#define LED3_GPIO_Port GPIOD
#define LED2_Pin GPIO_PIN_7
#define LED2_GPIO_Port GPIOD
#define LED1_Pin GPIO_PIN_9
#define LED1_GPIO_Port GPIOG
#define LD2_Pin GPIO_PIN_7
#define LD2_GPIO_Port GPIOB
#define UX_KNOB1_B_Pin GPIO_PIN_9
#define UX_KNOB1_B_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
