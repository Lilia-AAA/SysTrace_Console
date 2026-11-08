#include "beep_process.h"

#include <stdbool.h>

TaskHandle_t xBeep;
QueueHandle_t xBeep_Queue;


void Beep_Process(void)
{
    static uint32_t now;
    static uint32_t last;
    static Dev_status_e last_status = DEV_NORMAL;

    now = xTaskGetTickCount();
    if(dev_status != last_status)
    {
        last_status = dev_status;
        last = now - BEEP_HIGH_ALARM_PERIOD;       /*once high_alarm, beep immediately*/
    }

    if(dev_status != DEV_HIGH_ALARM)
    {
        return;
    }

    if(now - last >= BEEP_HIGH_ALARM_PERIOD)
    {
        last = now;
        BEEP(BEEP_ON_HIGH_ALARM);
    }
}

void vBeep_Task(void *pvParameters)
{
    uint32_t time_ms;
    uint32_t beep_start;
    bool beep_switch = false;

    for(;;)
    {
        if(xQueueReceive(xBeep_Queue, &time_ms, BEEP_TASK_SCAN_PERIOD) == pdPASS)
        {
            beep_switch = true;
            beep_start = xTaskGetTickCount();
        }
        if(beep_switch == true && (xTaskGetTickCount() - beep_start) <= time_ms)
        {
            HAL_GPIO_WritePin(BEEP_PORT, BEEP_PIN, GPIO_PIN_RESET);          
        }
        else
        {
            if(beep_switch == true)
            {
                beep_switch = false;
                beep_active = false;
            }
            HAL_GPIO_WritePin(BEEP_PORT, BEEP_PIN, GPIO_PIN_SET);          
        }
    }
}

void vBeep_Task_Init(void)
{
    xBeep_Queue = xQueueCreate(BEEP_QUEUE_LEN, BEEP_ITEM_SIZE);
    xTaskCreate(vBeep_Task, "Beep_Task", BEEP_STACK_DEPTH, NULL, BEEP_TASK_PRIO, &xBeep);
}