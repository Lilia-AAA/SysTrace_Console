#include "uart_dma_impl.h"
#include "dma_task.h"
#include "stm32f1xx_hal_uart.h"

#include <string.h>

static char dma_buf[DMA_ITEM_SIZE];

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    BaseType_t lastwake = pdFALSE;
    if(huart->Instance != USART1)
    {
        return;
    }
    if(HAL_UARTEx_GetRxEventType(huart) == HAL_UART_RXEVENT_HT)
    {
        return;
    }

    if((Size > 0U) && (Size <= RX_BUF_LEN))
    {
        memcpy(dma_buf, uart_recv, Size);
        dma_buf[Size] = '\0';

        memset(uart_recv, 0, RX_BUF_LEN);

        HAL_UARTEx_ReceiveToIdle_DMA(&RECV_UART_HANDLE, (uint8_t *)uart_recv, RX_BUF_LEN);
        xQueueSendFromISR(xDMA_Queue, dma_buf, &lastwake);
    }
    portYIELD_FROM_ISR(lastwake);
}