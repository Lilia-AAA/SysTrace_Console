#ifndef __W25Q_FLASH_H
#define __W25Q_FLASH_H

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

#define CMD_READ_JEDED_ID       0x9F
#define CMD_WRITE_DISABLE       0x04
#define CMD_WRITE_ENABLE        0x06
#define CMD_READ_STATUS_REG1    0x05
#define CMD_READ_DATA           0x03
#define CMD_PAGE_PROGRAM        0x02
#define CMD_SECTOR_ERASE        0x20

#define W25Q_SPI_TIMEOUT        10U
#define W25Q_BUSY_TIMEOUT       1000U

typedef struct
{
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef      *cs_port;
    uint16_t           cs_pin;
}W25Q_dev_t;

extern SPI_HandleTypeDef hspi1;
extern W25Q_dev_t w25q_dev;

HAL_StatusTypeDef W25Q_Read_ID(W25Q_dev_t *dev, uint8_t id[3]);
HAL_StatusTypeDef W25Q_Write_Enable(W25Q_dev_t *dev);
HAL_StatusTypeDef W25Q_Wait_Busy(W25Q_dev_t *dev);
HAL_StatusTypeDef W25Q_Read(W25Q_dev_t *dev, uint32_t addr, uint8_t *buf, uint32_t len);
HAL_StatusTypeDef W25Q_PageProgram(W25Q_dev_t *dev, uint32_t addr, const uint8_t *buf, uint32_t len);
HAL_StatusTypeDef W25Q_EraseSector(W25Q_dev_t *dev, uint32_t addr);

#endif