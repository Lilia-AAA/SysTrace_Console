#include "log_flush_task.h"
#include "log_storage.h"
#include "flash_log.h"
#include "print_helper.h"
#include "projdefs.h"
#include "stm32f1xx_hal_def.h"
#include "w25q_flash.h"
#include <stdio.h>


SemaphoreHandle_t xLog_Flush_Semphr;
TaskHandle_t      xLog_Flush;
char id_txt[30];

void vLog_Flush_Task(void *pvParameters)
{
#if FLASH_LOG_FORMAT
    for(uint8_t i = 0U; i < TOTAL_SECTOR; i++)
    {
        if(W25Q_EraseSector(&w25q_dev, START_ADDR + (uint32_t)i * SECTOR_SIZE) != HAL_OK) 
        {
            PrintStr("erase fail\r\n");
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(20U));
    }
    PrintStr("format done\r\n");
#endif

    if(Flash_Log_Init(&w25q_dev) != HAL_OK)
    {
        PrintStr("flash init fail\r\n");
    }

    for(;;)
    {
        if(xSemaphoreTake(xLog_Flush_Semphr, portMAX_DELAY) == pdPASS)
        {
            Log_flush();
        }
    }
}

void Log_Flush_Trigger(void)
{
    xSemaphoreGive(xLog_Flush_Semphr);
}

HAL_StatusTypeDef vLog_Flush_Task_Init(void)
{
    uint8_t id[3] = {0};
    HAL_StatusTypeDef st = W25Q_Read_ID(&w25q_dev, id);

    sprintf(id_txt, "W25Q ID: %02X %02X %02X (%s)\r\n", 
        id[0], id[1], id[2], (st == HAL_OK) ? "spi ok" : "spi fail");
    PrintStr(id_txt);

    xLog_Flush_Semphr = xSemaphoreCreateBinary();
    xTaskCreate(vLog_Flush_Task, "Log_Flush_Task", LOG_FLUSH_STACK_DEPTH, NULL, LOG_FLUSH_TASK_PRIO, &xLog_Flush);

    return HAL_OK;
}
