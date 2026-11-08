#include "dma_task.h"
#include "print_helper.h"
#include "stm32f1xx_hal_def.h"
#include "stm32f1xx_hal_uart.h"
#include "uart_dma_impl.h"
#include "uart_console.h"

#include <stddef.h>
#include <string.h>
#include <stdio.h>

QueueHandle_t xDMA_Queue;
TaskHandle_t  xDMA; 
char uart_recv[RX_BUF_LEN + 1U];

void vDMA_Task(void *pvParameters)
{
    char chunk[RX_BUF_LEN + 1U];
    char line [RX_BUF_LEN + 1U];
    size_t len = 0U;

    HAL_UARTEx_ReceiveToIdle_DMA(&RECV_UART_HANDLE, (uint8_t *)uart_recv, RX_BUF_LEN);
    for(;;)
    {
        if(xQueueReceive(xDMA_Queue, chunk, portMAX_DELAY) == pdPASS)
        { 
            for(const char *p = chunk; (*p != '\0') && (len < RX_BUF_LEN); p++)
            {
                /* Frame boundary: CR / LF indicates end of command */
                if((*p == '\r') || *p == '\n')
                {
                    line[len] = '\0';
                    if(len > 0U)
                    {
                        uart_cmd_console(line);
                    }
                    len = 0U;
                }
                else 
                {
                    line[len++] = *p;
                }
            }
        }
    }
}

void vDMA_Task_Init(void)
{
    xDMA_Queue = xQueueCreate(DMA_QUEUE_LEN, DMA_ITEM_SIZE);
    xTaskCreate(vDMA_Task, "DMA_Task", DMA_STACK_DEPTH, NULL, DMA_TASK_PRIO, &xDMA);
}