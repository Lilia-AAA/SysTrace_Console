#include "print_helper.h"
#include "dma_task.h"

#include <string.h>

TaskHandle_t xUART_Gatekeeper;
QueueHandle_t xUART_Gatekeeper_Queue;

volatile uint32_t g_uart_tx_ok  =0;
volatile uint32_t g_uart_tx_err =0;
volatile uint32_t g_uart_q_drop =0;

void PrintStr(const char* str)
{
    PrintStr_Wait(str, pdMS_TO_TICKS(PRINT_WAIT_TIMEOUT));
}

void PrintStr_Wait(const char *str, uint32_t timeout_ms)
{
    char send_str[PRINT_BUF_LEN];
    strncpy(send_str, str, sizeof(send_str) - 1);
    send_str[sizeof(send_str) - 1] = '\0'; 

    BaseType_t ret = xQueueSend(xUART_Gatekeeper_Queue, send_str, timeout_ms);

    if(ret != pdPASS)
    {
        g_uart_q_drop++;
    }
}

void vUART_Gatekeeper_Task(void *pvParameters)
{
    char recv_str[PRINT_BUF_LEN];
    for(;;)
    {
        if(xQueueReceive(xUART_Gatekeeper_Queue, recv_str, portMAX_DELAY) == pdPASS)
        {       
            TX_LED_ON();
            HAL_StatusTypeDef status = HAL_UART_Transmit(&PRINT_UART_HANDLE, (uint8_t *)recv_str, strlen(recv_str), PRINT_TRANS_TIMEOUT);
            TX_LED_OFF();

            if(status == HAL_OK)
            {
                g_uart_tx_ok++;
            }
            else
            {
                g_uart_tx_err++;
            }
        }
    }
}

void vUART_Gatekeeper_Task_Init(void)
{
    xUART_Gatekeeper_Queue = xQueueCreate(UART_GATEKEEPER_QUEUE_LEN, UART_GATEKEEPER_ITEM_SIZE);
    xTaskCreate(vUART_Gatekeeper_Task, "UART_Gatekeeper_Task", UART_GATEKEEPER_STACK_DEPTH, NULL, UART_GATEKEEPER_TASK_PRIO, &xUART_Gatekeeper);
}