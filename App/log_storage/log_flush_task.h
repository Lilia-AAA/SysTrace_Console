#ifndef __LOG_FLUSH_TASK_H
#define __LOG_FLUSH_TASK_H

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "stm32f1xx_hal_def.h"
#include "task.h"
#include "semphr.h"

#define FLASH_LOG_FORMAT            0U

#define LOG_FLUSH_TASK_PRIO         1U
#define LOG_FLUSH_STACK_DEPTH       160U

#define LOG_FLUSH_THRESHOLD         4U
extern TaskHandle_t xLog_Flush;

HAL_StatusTypeDef vLog_Flush_Task_Init(void);
void vLog_Flush_Task(void *pvParameters);
void Log_Flush_Trigger(void);

#endif