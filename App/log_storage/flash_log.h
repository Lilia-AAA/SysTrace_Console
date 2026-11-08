#ifndef __FLASH_LOG_H
#define __FLASH_LOG_H

#include "stm32f1xx_hal.h"
#include "log_storage.h"
#include "stm32f1xx_hal_def.h"
#include "w25q_flash.h"

#define START_ADDR              0x00000000
#define SECTOR_SIZE             4096U
#define PAGE_SIZE               256U
#define TOTAL_SECTOR            4U

HAL_StatusTypeDef Flash_Log_Init(W25Q_dev_t *dev);
HAL_StatusTypeDef Flash_Log_Save(W25Q_dev_t *dev, Log_t *log);
HAL_StatusTypeDef Flash_Log_Read(W25Q_dev_t *dev, Log_t *log);

#endif