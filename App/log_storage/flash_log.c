#include "flash_log.h"

/*Utils*/
#include "event_process_bsp.h"
#include "log_storage.h"
#include "print_helper.h"
#include "stm32f1xx_hal_def.h"
#include "w25q_flash.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static char txt[PRINT_BUF_LEN];
static uint16_t last_saved_id = 0U;
static uint32_t curr_addr;
static uint32_t curr_sector;

HAL_StatusTypeDef Flash_Log_Init(W25Q_dev_t *dev)
{
    Log_t curr_log;
    uint8_t found       = 0U;
    uint16_t max_id     = 0U;
    uint32_t max_addr   = 0U;

    for(uint8_t sector = 0; sector < TOTAL_SECTOR; sector++)
    {
        for(uint32_t offset = 0; offset < SECTOR_SIZE; offset += LOG_SIZE)
        {
            uint32_t addr = START_ADDR + (uint32_t)sector * SECTOR_SIZE + offset;
            
            if(W25Q_Read(dev, addr, (uint8_t *)&curr_log, LOG_SIZE) != HAL_OK) return HAL_ERROR;

            if(curr_log.magic != LOG_MAGIC) continue;
            if((found == 0U) || (int16_t)(curr_log.id - max_id) > 0)
            {
                max_id      = curr_log.id;
                max_addr    = addr;
                found       = 1U;
            }
        }
    }
    
    if(found == 0U)
    {
        last_saved_id = 0U;
        curr_sector   = 0U;
        curr_addr     = START_ADDR;
        if(W25Q_EraseSector(dev, START_ADDR) != HAL_OK) return HAL_ERROR;
    } 
    else
    {
        last_saved_id = max_id;
        curr_sector   = (max_addr - START_ADDR) / SECTOR_SIZE;
        curr_addr     = max_addr + LOG_SIZE;
    }

    snprintf(txt, PRINT_BUF_LEN, "Init curr_addr: 0x%08lX\r\n", (unsigned long)curr_addr);
    PrintStr(txt);

    return HAL_OK;
}

HAL_StatusTypeDef Flash_Log_Save(W25Q_dev_t *dev, Log_t *log)
{
    uint8_t log_buf[LOG_SIZE];
    uint16_t next_id = last_saved_id + 1U;

    uint32_t curr_sector_end = START_ADDR + (uint32_t)(curr_sector + 1U) * SECTOR_SIZE;
    if (curr_addr >= curr_sector_end)
    {
        uint32_t next_sector = (curr_sector + 1U) % TOTAL_SECTOR;
        uint32_t next_addr   = START_ADDR + (uint32_t)next_sector * SECTOR_SIZE;

        if(W25Q_EraseSector(dev, next_addr) != HAL_OK) return HAL_ERROR;
        
        curr_sector = next_sector;
        curr_addr = next_addr;
    }

    log->id = next_id;
    memcpy(log_buf, log, LOG_SIZE);
    log_buf[LOG_SIZE - 1] = LOG_MAGIC;

    if(W25Q_PageProgram(dev, curr_addr, log_buf, LOG_SIZE) != HAL_OK) return HAL_ERROR;

    last_saved_id = next_id;
    curr_addr    += LOG_SIZE;

    return HAL_OK;
}

HAL_StatusTypeDef Flash_Log_Read(W25Q_dev_t *dev, Log_t *log)
{
    for(uint8_t sector = 0; (sector < TOTAL_SECTOR); sector++)
    {
        sprintf(txt, "-------------------------------sector %d-------------------------------\r\n", sector);
        PrintStr(txt);

        for(uint32_t offset = 0; offset < SECTOR_SIZE; offset += LOG_SIZE)
        {
            uint32_t addr = START_ADDR + (uint32_t)sector * SECTOR_SIZE + offset;

            if(W25Q_Read(dev, addr, (uint8_t *)log, LOG_SIZE) != HAL_OK) return HAL_ERROR;

            if(log->magic == LOG_MAGIC)
            {
                snprintf(txt, sizeof(txt), 
                "ID:%05d | time:%02d-%02d-%02d %02d:%02d:%02d evt: %s, param: %lu, msg: %s\r\n",
                log->id, 
                log->timestamp[0],
                log->timestamp[1],
                log->timestamp[2],
                log->timestamp[3],
                log->timestamp[4],
                log->timestamp[5],
                Log_eventname(log->event_name),
                (unsigned long)log->beep_param,
                log->msg);

                PrintStr(txt);
            }
            else if(log->magic == 0xFF)
            {
                break;
            }
            else
            {
                PrintStr("meet bad info");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return  HAL_OK;
}
