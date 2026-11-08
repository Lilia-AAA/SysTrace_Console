#ifndef __EVENT_PROCESS_TASK_H
#define __EVENT_PROCESS_TASK_H

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "log_storage.h"

#define EVENT_PROCESS_QUEUE_LEN       3U
#define EVENT_PROCESS_ITEM_SIZE       sizeof(Log_t)

#define EVENT_PROCESS_TASK_PRIO       3U
#define EVENT_PROCESS_STACK_DEPTH     260U

#define EVENT_TASK_SCAN_PERIOD        pdMS_TO_TICKS(10)

extern TaskHandle_t  xEvent_Process;
extern QueueHandle_t xEvent_Process_Queue;


void vEvent_Process_Task_Init(void);
void vEvent_Process_Task(void *pvParameters);

#endif