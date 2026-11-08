/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    rtc.c
  * @brief   This file provides code for the configuration
  *          of the RTC instances.
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
#include "rtc.h"
#include "stm32f1xx_hal_def.h"
#include "stm32f1xx_hal_rtc_ex.h"
#include <stdint.h>

/* USER CODE BEGIN 0 */
/*Backup domain register flag: do not reset RTC if backup domain 
  power is present.*/
#define RTC_BKP_MAGIC   0x2005U
#define RTC_BKP_DR_YM   RTC_BKP_DR2  /*(Year << 8) | Month*/
#define RTC_BKP_DR_DW   RTC_BKP_DR3  /*(WeekDay << 8) | Day*/

void RTC_Date_Save(void)
{
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR_YM, 
                ((uint32_t)hrtc.DateToUpdate.Year << 8) | hrtc.DateToUpdate.Month);

  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR_DW, 
                ((uint32_t)hrtc.DateToUpdate.WeekDay << 8) | hrtc.DateToUpdate.Date);
}

static HAL_StatusTypeDef RTC_Date_Restore(void)
{
  uint32_t ym = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR_YM);
  uint32_t dw = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR_DW);

  uint8_t year = ym >> 8;
  uint8_t month = ym & 0xFFU;
  uint8_t weekday = dw >> 8;
  uint8_t date = dw & 0xFFU;

  if((year < 1U) || (year > 99U) || (month < 1U) || (month > 12U) || (date < 1U) || (date > 31U) || (weekday < 1U) || (weekday > 7U))
  {
    return HAL_ERROR;
  }

  hrtc.DateToUpdate.Year    = year;
  hrtc.DateToUpdate.Month   = month;
  hrtc.DateToUpdate.WeekDay = weekday;
  hrtc.DateToUpdate.Date    = date;

  return HAL_OK;
}

/* USER CODE END 0 */

RTC_HandleTypeDef hrtc;

/* RTC init function */
void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef DateToUpdate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
  hrtc.Init.OutPut = RTC_OUTPUTSOURCE_ALARM;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */
  /*If backup domain power remains valid and initialization already done: 
    return directly, keep RTC ticking.Otherwise, SetTime/SetDate will 
    reset the time on every reset.*/
  if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1) == RTC_BKP_MAGIC)
  {
    if(RTC_Date_Restore() == HAL_OK)
    {
      return;
    }
  }

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0x14;
  sTime.Minutes = 0x30;
  sTime.Seconds = 0x0;

  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  DateToUpdate.WeekDay = RTC_WEEKDAY_SUNDAY;
  DateToUpdate.Month = RTC_MONTH_OCTOBER;
  DateToUpdate.Date = 0x25;
  DateToUpdate.Year = 0x26;

  if (HAL_RTC_SetDate(&hrtc, &DateToUpdate, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */
  
  /*Set time once and store a flag. Do not reconfigure after any reset, 
    including IWDG watchdog reset.*/
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, RTC_BKP_MAGIC);
  RTC_Date_Save();
  
  /* USER CODE END RTC_Init 2 */

}

void HAL_RTC_MspInit(RTC_HandleTypeDef* rtcHandle)
{

  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspInit 0 */

  /* USER CODE END RTC_MspInit 0 */
    HAL_PWR_EnableBkUpAccess();
    /* Enable BKP CLK enable for backup registers */
    __HAL_RCC_BKP_CLK_ENABLE();
    /* RTC clock enable */
    __HAL_RCC_RTC_ENABLE();
  /* USER CODE BEGIN RTC_MspInit 1 */

  /* USER CODE END RTC_MspInit 1 */
  }
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef* rtcHandle)
{

  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspDeInit 0 */

  /* USER CODE END RTC_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_RTC_DISABLE();
  /* USER CODE BEGIN RTC_MspDeInit 1 */

  /* USER CODE END RTC_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

