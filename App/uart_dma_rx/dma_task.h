#ifndef __DMA_TASK_H
#define __DMA_TASK_H

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

extern UART_HandleTypeDef huart1;
extern TaskHandle_t xDMA;
extern QueueHandle_t xDMA_Queue;

#define RECV_UART_HANDLE    huart1
#define DMA_QUEUE_LEN       3U
#define DMA_ITEM_SIZE       (RX_BUF_LEN + 1U)   //'\0' is included

#define DMA_TASK_PRIO       2U
#define DMA_STACK_DEPTH     400U

#define RX_BUF_LEN          40U


void vDMA_Task_Init(void);
void vDMA_Task(void *pvParameters);

#endif