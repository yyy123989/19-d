/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define AD_CONVST_Pin GPIO_PIN_0
#define AD_CONVST_GPIO_Port GPIOA
#define AD_BUSY_Pin GPIO_PIN_1
#define AD_BUSY_GPIO_Port GPIOA
#define AD_CS_Pin GPIO_PIN_4
#define AD_CS_GPIO_Port GPIOA
#define AD_SCLK_Pin GPIO_PIN_5
#define AD_SCLK_GPIO_Port GPIOA
#define AD_DOUTA_Pin GPIO_PIN_6
#define AD_DOUTA_GPIO_Port GPIOA
#define AD_RESET_Pin GPIO_PIN_4
#define AD_RESET_GPIO_Port GPIOC
#define AD_RANGE_Pin GPIO_PIN_5
#define AD_RANGE_GPIO_Port GPIOC
#define AD_OS0_Pin GPIO_PIN_6
#define AD_OS0_GPIO_Port GPIOC
#define AD_OS1_Pin GPIO_PIN_7
#define AD_OS1_GPIO_Port GPIOC
#define AD_OS2_Pin GPIO_PIN_8
#define AD_OS2_GPIO_Port GPIOC
#define AD_PAR_SER_Pin GPIO_PIN_9
#define AD_PAR_SER_GPIO_Port GPIOC
#define AD_STBY_Pin GPIO_PIN_10
#define AD_STBY_GPIO_Port GPIOC
#define AD_FRSTDATA_Pin GPIO_PIN_11
#define AD_FRSTDATA_GPIO_Port GPIOC
#define AD_WR_Pin GPIO_PIN_12
#define AD_WR_GPIO_Port GPIOC
#define DDS_RESET_Pin GPIO_PIN_8
#define DDS_RESET_GPIO_Port GPIOA
#define USART1_TX_Pin GPIO_PIN_9
#define USART1_TX_GPIO_Port GPIOA
#define USART1_RX_Pin GPIO_PIN_10
#define USART1_RX_GPIO_Port GPIOA
#define USART2_SCREEN_TX_Pin GPIO_PIN_2
#define USART2_SCREEN_TX_GPIO_Port GPIOA
#define USART2_SCREEN_RX_Pin GPIO_PIN_3
#define USART2_SCREEN_RX_GPIO_Port GPIOA
#define DDS_DRHOLD_Pin GPIO_PIN_8
#define DDS_DRHOLD_GPIO_Port GPIOB
#define DDS_PWRDN_Pin GPIO_PIN_9
#define DDS_PWRDN_GPIO_Port GPIOB
#define DDS_CS_Pin GPIO_PIN_10
#define DDS_CS_GPIO_Port GPIOB
#define DDS_IO_UPDATE_Pin GPIO_PIN_12
#define DDS_IO_UPDATE_GPIO_Port GPIOB
#define DDS_SCLK_Pin GPIO_PIN_13
#define DDS_SCLK_GPIO_Port GPIOB
#define DDS_SDIO_Pin GPIO_PIN_15
#define DDS_SDIO_GPIO_Port GPIOB
#define DDS_PROFILE0_Pin GPIO_PIN_2
#define DDS_PROFILE0_GPIO_Port GPIOE
#define KEY1_Pin GPIO_PIN_3
#define KEY1_GPIO_Port GPIOE
#define KEY0_Pin GPIO_PIN_4
#define KEY0_GPIO_Port GPIOE
#define DDS_OSK_Pin GPIO_PIN_5
#define DDS_OSK_GPIO_Port GPIOE
#define DDS_DRCTL_Pin GPIO_PIN_6
#define DDS_DRCTL_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
