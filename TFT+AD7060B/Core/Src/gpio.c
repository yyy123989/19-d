/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
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

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, AD_CONVST_Pin|AD_CS_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(AD_SCLK_GPIO_Port, AD_SCLK_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(DDS_RESET_GPIO_Port, DDS_RESET_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, DDS_DRHOLD_Pin|DDS_PWRDN_Pin|DDS_IO_UPDATE_Pin|DDS_SCLK_Pin
                          |DDS_SDIO_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(DDS_CS_GPIO_Port, DDS_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, AD_RESET_Pin|AD_RANGE_Pin|AD_OS0_Pin|AD_OS1_Pin
                          |AD_OS2_Pin|AD_PAR_SER_Pin|AD_STBY_Pin|AD_WR_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, DDS_PROFILE0_Pin|DDS_OSK_Pin|DDS_DRCTL_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : AD_CONVST_Pin AD_CS_Pin AD_SCLK_Pin */
  GPIO_InitStruct.Pin = AD_CONVST_Pin|AD_CS_Pin|AD_SCLK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  /* Note: AD_SCLK (PA5) is later reconfigured as AF_PP by SPI1 MspInit. */

  /*Configure GPIO pin : DDS_RESET_Pin */
  GPIO_InitStruct.Pin = DDS_RESET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(DDS_RESET_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : DDS_DRHOLD_Pin DDS_PWRDN_Pin DDS_CS_Pin DDS_IO_UPDATE_Pin
                           DDS_SCLK_Pin DDS_SDIO_Pin */
  GPIO_InitStruct.Pin = DDS_DRHOLD_Pin|DDS_PWRDN_Pin|DDS_CS_Pin|DDS_IO_UPDATE_Pin
                          |DDS_SCLK_Pin|DDS_SDIO_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : AD_DOUTA_Pin */
  GPIO_InitStruct.Pin = AD_DOUTA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(AD_DOUTA_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : AD_BUSY_Pin */
  GPIO_InitStruct.Pin = AD_BUSY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(AD_BUSY_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : AD_RESET_Pin AD_RANGE_Pin AD_OS0_Pin AD_OS1_Pin
                           AD_OS2_Pin AD_PAR_SER_Pin AD_STBY_Pin AD_WR_Pin */
  GPIO_InitStruct.Pin = AD_RESET_Pin|AD_RANGE_Pin|AD_OS0_Pin|AD_OS1_Pin
                          |AD_OS2_Pin|AD_PAR_SER_Pin|AD_STBY_Pin|AD_WR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : AD_FRSTDATA_Pin */
  GPIO_InitStruct.Pin = AD_FRSTDATA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(AD_FRSTDATA_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : DDS_PROFILE0_Pin DDS_OSK_Pin DDS_DRCTL_Pin */
  GPIO_InitStruct.Pin = DDS_PROFILE0_Pin|DDS_OSK_Pin|DDS_DRCTL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : KEY1_Pin KEY0_Pin */
  GPIO_InitStruct.Pin = KEY1_Pin|KEY0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  __HAL_GPIO_EXTI_CLEAR_IT(AD_BUSY_Pin);
  HAL_NVIC_SetPriority(EXTI1_IRQn, 1, 1);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
