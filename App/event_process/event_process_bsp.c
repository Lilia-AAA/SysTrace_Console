#include "event_process_bsp.h"
#include "sys_monitor_task.h"
#include "print_helper.h"
#include "uart_console.h"
#include "log_storage.h"
#include "flash_log.h"
#include "w25q_flash.h"

#include <stdint.h>
#include <stdio.h>

void LED_ON(void)
{
    led_manual_ctrl = true;
    LED_SET(led_manual_ctrl);
}

void LED_OFF(void)
{
    led_manual_ctrl = false;
    LED_SET(led_manual_ctrl);
}

void BEEP(uint32_t time_ms)
{
    beep_active = true;
    xQueueSend(xBeep_Queue, &time_ms, 0U);
}

void STATUS(void)
{
    static Status_snapshot_t snap;
    status_get_snapshot(&snap);
}

void LOCK()
{
    is_locked = true;
}

void UNLOCK()
{
    is_locked = false;
    dev_status = DEV_NORMAL;
}

void READ_LOG(void)
{
    Log_print();
}

void CLEAR_LOG(void)
{
    Log_clear();
}

void READ_FLASHLOG(void)
{
    Log_t flash_log;
    Flash_Log_Read(&w25q_dev, &flash_log);
}

void HELP(void)
{
    char line[80];

    PrintStr("\r\n============= SysTrace Console ================\r\n");
    PrintStr("[UART commands] 115200 8N1, CR/LF terminated\r\n");

    for(uint32_t i = 0U; i < cmd_cnt; i++)
    {
        snprintf(line, sizeof(line), "  %-14s - %s\r\n",
                 CMD_Table[i].cmd_name, CMD_Table[i].cmd_desc);
        PrintStr(line);
    }

    PrintStr("[Key] PA8, active low\r\n");
    PrintStr("  single click     - toggle the LED\r\n");
    PrintStr("  double click     - toggle the alarm state\r\n");
    PrintStr("  long press > 1s  - high alarm: lock + beep\r\n");
    PrintStr("===============================================\r\n");
}

void CLICK(void)
{
    dev_status = DEV_NORMAL;

    led_manual_ctrl = !led_manual_ctrl;     
    if(led_manual_ctrl)
    {
        LED_ON();
    }
    else
    {
        LED_OFF();
    }
}
void DOUBLE_CLICK(void)
{
    if(dev_status == DEV_NORMAL)
    {
        dev_status = DEV_ALARM;
        led_manual_ctrl = true;
    }
    else if(dev_status == DEV_ALARM)
    {
        dev_status = DEV_NORMAL;
    }
}
void LONG_PRESS(void)
{
    dev_status = DEV_HIGH_ALARM;
    led_manual_ctrl = true;
    beep_active = true;
    LOCK();
}

/*---------------------------------------------*/
void LED_SET(bool on_off)
{
    if(on_off)
    {
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
    }
}