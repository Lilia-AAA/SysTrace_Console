#include "w25q_flash.h"
#include "projdefs.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_def.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_spi.h"
#include <stdint.h>
#include <string.h>

W25Q_dev_t w25q_dev = 
{
    .hspi    = &hspi1,
    .cs_port = GPIOA,
    .cs_pin  = GPIO_PIN_4
};

static HAL_StatusTypeDef W25Q_Transfer(W25Q_dev_t *dev, const uint8_t *cmd, uint16_t cmd_len,
                                       const uint8_t *tx, uint8_t *rx, uint16_t data_len)
{
    HAL_StatusTypeDef st;

    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);

    st = HAL_SPI_Transmit(dev->hspi, cmd, cmd_len, W25Q_SPI_TIMEOUT);
    if((st == HAL_OK) && (data_len > 0U))
    {
        if(rx != NULL)
        {
            st = HAL_SPI_Receive(dev->hspi, rx, data_len, W25Q_SPI_TIMEOUT);
        }
        else 
        {
            st = HAL_SPI_Transmit(dev->hspi, tx, data_len, W25Q_SPI_TIMEOUT);
        }
    }

    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);
    return st;
}

HAL_StatusTypeDef W25Q_Read_ID(W25Q_dev_t *dev, uint8_t id[3])
{
    const uint8_t cmd = CMD_READ_JEDED_ID;
    return W25Q_Transfer(dev, &cmd, 1U, NULL, id, 3U);
}

HAL_StatusTypeDef W25Q_Write_Enable(W25Q_dev_t *dev)
{
    const uint8_t cmd = CMD_WRITE_ENABLE;

    return W25Q_Transfer(dev, &cmd, 1U, NULL, NULL, 0U);
}

HAL_StatusTypeDef W25Q_Wait_Busy(W25Q_dev_t *dev)
{
    const uint8_t cmd = CMD_READ_STATUS_REG1;
    uint32_t start = HAL_GetTick();
    uint8_t status_reg;
    
    do
    {
        HAL_StatusTypeDef st = W25Q_Transfer(dev, &cmd, 1U, NULL, &status_reg, 1U);

        if (st != HAL_OK) 
        {
            return st;
        }
        if((status_reg & 0x01U) == 0U)
        {
            return  HAL_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(1U));
    }
    while((HAL_GetTick() - start) < W25Q_BUSY_TIMEOUT);

    return HAL_TIMEOUT;
}

HAL_StatusTypeDef W25Q_Read(W25Q_dev_t *dev, uint32_t addr, uint8_t *buf, uint32_t len)
{
    uint8_t cmd[4];
    cmd[0] = CMD_READ_DATA;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8)  & 0xFF;
    cmd[3] = addr         & 0xFF;

    if ((buf == NULL) || (len == 0U) || (len > 0xFFFFU)) 
    {
        return  HAL_ERROR;
    }

    return W25Q_Transfer(dev, cmd, 4U, NULL, buf, (uint16_t)len);

}

HAL_StatusTypeDef W25Q_PageProgram(W25Q_dev_t *dev, uint32_t addr, const uint8_t *buf, uint32_t len)
{
    HAL_StatusTypeDef st;

    uint8_t cmd[4];
    cmd[0] = CMD_PAGE_PROGRAM;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8)  & 0xFF;
    cmd[3] = addr         & 0xFF;

    st = W25Q_Write_Enable(dev);    
    if(st != HAL_OK) return st;

    st = W25Q_Transfer(dev, cmd, 4U, buf, NULL, (uint16_t)len);
    if (st != HAL_OK) return st;

    return W25Q_Wait_Busy(dev);
}

HAL_StatusTypeDef W25Q_EraseSector(W25Q_dev_t *dev, uint32_t addr)
{
    HAL_StatusTypeDef st;

    uint8_t cmd[4];
    cmd[0] = CMD_SECTOR_ERASE;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8)  & 0xFF;
    cmd[3] = addr         & 0xFF;

    st = W25Q_Write_Enable(dev);    
    if(st != HAL_OK) return st;

    st = W25Q_Transfer(dev, cmd, 4U, NULL, NULL, 0U);
    if (st != HAL_OK) return st;

    return W25Q_Wait_Busy(dev);

}
