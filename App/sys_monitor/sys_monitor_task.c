#include "sys_monitor_task.h"
/*ALL TASKS*/
#include "dma_task.h"
#include "print_helper.h"
#include "beep_process.h"
#include "key_control_task.h"
#include "event_process_task.h"
#include "log_flush_task.h"

#include <stdio.h>

TaskHandle_t xSys_Monitor;
QueueHandle_t xSys_Monitor_Queue;

static const struct
{
    const char   *name;
    TaskHandle_t *handle;
    uint16_t      stack_words;
}
tasks_status [TASK_TOTAL_CNT] = 
{
    {"DMA_Task",             &xDMA,             DMA_STACK_DEPTH},
    {"UART_Gatekeeper_Task", &xUART_Gatekeeper, UART_GATEKEEPER_STACK_DEPTH},
    {"Beep_Task",            &xBeep,            BEEP_STACK_DEPTH},
    {"Sys_Monitor_Task",     &xSys_Monitor,     SYS_MONITOR_STACK_DEPTH},
    {"Key_Control_Task",     &xKey_Control,     KEY_CONTROL_STACK_DEPTH},
    {"Event_Process_Task",   &xEvent_Process,   EVENT_PROCESS_STACK_DEPTH},
    {"Log_Flush_Task",       &xLog_Flush,       LOG_FLUSH_STACK_DEPTH},
};


void status_get_snapshot(Status_snapshot_t *snap)
{
    /*--------Business Layer-----------*/    
    taskENTER_CRITICAL();
    snap->dev_status = dev_status;
    snap->locked = is_locked;
    snap->led = led_manual_ctrl;
    snap->beep =beep_active;
    taskEXIT_CRITICAL();
    //get rtc
    /*--------FreeRTOS Kernel Info-----*/
    snap->sys_tick = xTaskGetTickCount();
    snap->free_heap = xPortGetFreeHeapSize();
    snap->min_free_heap = xPortGetMinimumEverFreeHeapSize();
    snap->heap_total = configTOTAL_HEAP_SIZE;

    for(uint8_t i = 0; i < TASK_TOTAL_CNT; i++)
    {
        snap->task[i].name        = tasks_status[i].name;
        snap->task[i].stack_words = tasks_status[i].stack_words;
        
        if(*tasks_status[i].handle == NULL)
        {
            snap->task[i].handle_ok        = false;
            snap->task[i].prio             = 0U;
            snap->task[i].stack_free_words = 0U;
        }
        else
        {
            snap->task[i].handle_ok        = true;
            snap->task[i].prio             = (uint8_t)uxTaskPriorityGet(*tasks_status[i].handle);
            snap->task[i].stack_free_words = (uint16_t)uxTaskGetStackHighWaterMark(*tasks_status[i].handle);
        }
    }
    /*--------Queue occupancy-----------*/
    snap->q_dma.wait                = uxQueueMessagesWaiting(xDMA_Queue);
    snap->q_dma.depth               = DMA_QUEUE_LEN;
    snap->q_uart_gatekeeper.wait    = uxQueueMessagesWaiting(xUART_Gatekeeper_Queue);
    snap->q_uart_gatekeeper.depth   = UART_GATEKEEPER_QUEUE_LEN;
    snap->q_beep.wait               = uxQueueMessagesWaiting(xBeep_Queue);
    snap->q_beep.depth              = BEEP_QUEUE_LEN;
    snap->q_sys_monitor.wait        = uxQueueMessagesWaiting(xSys_Monitor_Queue);
    snap->q_sys_monitor.depth       = SYS_MONITOR_QUEUE_LEN;
    snap->q_event.wait              = uxQueueMessagesWaiting(xEvent_Process_Queue);
    snap->q_event.depth             = EVENT_PROCESS_QUEUE_LEN;

    if(xQueueSend(xSys_Monitor_Queue, snap, 0U) != pdPASS)
    {
        /*Queue is full, drop the snapshot*/
        //led warning
        return; 
    }
}

void vSys_Monitor_Task(void *pvParameters)
{
    Status_snapshot_t snap;
    const char *dev_state;
    char buf[PRINT_BUF_LEN];

    for(;;)
    {
        if(xQueueReceive(xSys_Monitor_Queue, &snap, SYS_TASK_SCAN_PERIOD) == pdPASS)
        {
            switch(snap.dev_status)
            {
                case DEV_NORMAL:      dev_state = "NORMAL";      break;
                case DEV_ALARM:       dev_state = "ALARM";       break;
                case DEV_HIGH_ALARM:  dev_state = "HIGH_ALARM";  break;
                default:              dev_state = "UNKNOWN";     break;
            }            
            PrintStr_Wait("\r\n===================DEVICE STATUS REPORT===================\r\n", portMAX_DELAY);
            
            /*--------Business Layer-----------*/
            //rtc

            snprintf(buf, sizeof(buf), 
            "State: %s | LED: %s | Beep: %s | Lock: %s\r\n",
            dev_state,
            snap.led ? "ON" : "OFF",
            snap.beep ? "ACTIVE" : "IDLE",
            snap.locked ? "LOCKED" : "UNLOCKED");
            PrintStr_Wait(buf, portMAX_DELAY);

            /*--------FreeRTOS Kernel Info-----*/
            snprintf(buf, sizeof(buf), 
            "Uptime: %lu ticks (%lu s) | tick rate: %u Hz\r\n",
            (long unsigned)snap.sys_tick,
            (long unsigned)snap.sys_tick / (uint32_t)configTICK_RATE_HZ,
            (unsigned)configTICK_RATE_HZ);
            PrintStr_Wait(buf, portMAX_DELAY);

            snprintf(buf, sizeof(buf), 
            "Heap: free %lu B | min_ever free %lu B | total %lu B\r\n",
            (long unsigned)snap.free_heap,
            (long unsigned)snap.min_free_heap,
            (long unsigned)snap.heap_total);
            PrintStr_Wait(buf, portMAX_DELAY);

            PrintStr_Wait("----------- Task stacks (free/allocated, words) ----------\r\n", portMAX_DELAY);
            for(uint8_t i = 0; i < TASK_TOTAL_CNT; i++)
            {
                if(snap.task[i].handle_ok == false)
                {
                    snprintf(buf, sizeof(buf),
                    " %-22s NOT CREATED (xTaskCreate failed)\r\n",
                    snap.task[i].name);
                }
                else
                {
                    snprintf(buf, sizeof(buf),
                    " %-22s prio %u free %u/%u w (%lu/%lu B)\r\n",
                    snap.task[i].name,
                    snap.task[i].prio,
                    snap.task[i].stack_free_words,
                    snap.task[i].stack_words,
                    snap.task[i].stack_free_words * 4UL,
                    snap.task[i].stack_words * 4UL);
                }
                PrintStr_Wait(buf, portMAX_DELAY);
            }

            /*--------Queue occupancy-----------*/
            snprintf(buf, sizeof(buf), 
            "Queues (waiting/depth): dma %lu/%lu | uart_gatekeeper %lu/%lu | beep %lu/%lu\r\n",
            snap.q_dma.wait, snap.q_dma.depth,
            snap.q_uart_gatekeeper.wait, snap.q_uart_gatekeeper.depth,
            snap.q_beep.wait, snap.q_beep.depth);
            PrintStr_Wait(buf, portMAX_DELAY);
            
            snprintf(buf, sizeof(buf), 
            "                        sys_monitor %lu/%lu | event %lu/%lu\r\n",
            snap.q_sys_monitor.wait, snap.q_sys_monitor.depth,
            snap.q_event.wait, snap.q_event.depth);
            PrintStr_Wait(buf, portMAX_DELAY);

            PrintStr_Wait("==========================================================\r\n", portMAX_DELAY);
        }
    }
}

void vSys_Monitor_Task_Init(void)
{
    xSys_Monitor_Queue = xQueueCreate(SYS_MONITOR_QUEUE_LEN, SYS_MONITOR_ITEM_SIZE);
    xTaskCreate(vSys_Monitor_Task, "Sys_Monitor_Task", SYS_MONITOR_STACK_DEPTH, NULL, SYS_MONITOR_TASK_PRIO, &xSys_Monitor);
}