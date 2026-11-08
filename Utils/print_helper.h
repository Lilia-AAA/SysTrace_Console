#ifndef __PRINT_HELPER_H
#define __PRINT_HELPER_H

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define TRANS_LED_GPIO_PORT  GPIOA
#define TRANS_LED_PIN        GPIO_PIN_1

#define TX_LED_ON()          HAL_GPIO_WritePin(TRANS_LED_GPIO_PORT, TRANS_LED_PIN, GPIO_PIN_RESET)
#define TX_LED_OFF()         HAL_GPIO_WritePin(TRANS_LED_GPIO_PORT, TRANS_LED_PIN, GPIO_PIN_SET)

extern UART_HandleTypeDef huart1;
extern TaskHandle_t xUART_Gatekeeper;
extern QueueHandle_t xUART_Gatekeeper_Queue;

#define PRINT_UART_HANDLE             huart1
#define UART_GATEKEEPER_QUEUE_LEN     3U
#define UART_GATEKEEPER_ITEM_SIZE     PRINT_BUF_LEN

#define UART_GATEKEEPER_STACK_DEPTH   100U
#define UART_GATEKEEPER_TASK_PRIO     1U

#define PRINT_WAIT_TIMEOUT            100U
#define PRINT_BUF_LEN                 96U
#define PRINT_TRANS_TIMEOUT           100U

void PrintStr(const char* str);
void PrintStr_Wait(const char *str, uint32_t timeout_ms);
void vUART_Gatekeeper_Task_Init(void);
void vUART_Gatekeeper_Task(void *pvParameters);

#endif