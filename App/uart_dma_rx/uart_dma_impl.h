#ifndef __UART_DMA_IMPL_H
#define __UART_DMA_IMPL_H

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "queue.h"

extern UART_HandleTypeDef huart1;
extern char uart_recv[];
extern QueueHandle_t xDMA_Queue;


#endif