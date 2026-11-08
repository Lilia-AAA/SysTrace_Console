#include "event_process_task.h"
#include "event_process_bsp.h"
#include "led_proccess.h"
#include "beep_process.h"

#include <stdbool.h>

QueueHandle_t xEvent_Process_Queue;
TaskHandle_t  xEvent_Process;

Log_t                evt_log;
Dev_status_e         dev_status;
bool led_manual_ctrl = false;
bool is_locked       = false;
bool beep_active     = false;

void vEvent_Process_Task(void *pvParameters)
{
    dev_status = DEV_NORMAL;
    for(;;)
    {
        if(xQueueReceive(xEvent_Process_Queue, &evt_log, EVENT_TASK_SCAN_PERIOD) == pdPASS)
        {
            switch(evt_log.event_name)
            {
                /*-- uart event--*/
                case LED_ON_EVENT:
                    LED_ON();
                    Log_add(evt_log.event_name, evt_log.msg, evt_log.beep_param);
                    break;
                case LED_OFF_EVENT:
                    LED_OFF();
                    Log_add(evt_log.event_name, evt_log.msg, evt_log.beep_param);
                    break;
                case BEEP_EVENT:
                    BEEP(evt_log.beep_param);//LOG
                    Log_add(evt_log.event_name, evt_log.msg, evt_log.beep_param);
                    break;
                case STATUS_EVENT:
                    STATUS();
                    Log_add(evt_log.event_name, evt_log.msg, evt_log.beep_param);
                    break;
                case LOCK_EVENT:
                    LOCK();
                    Log_add(evt_log.event_name, evt_log.msg, evt_log.beep_param);
                    break;
                case UNLOCK_EVENT:
                    UNLOCK();
                    Log_add(evt_log.event_name, evt_log.msg, evt_log.beep_param);
                    break;
                case READ_LOG_EVENT:
                    READ_LOG();
                    Log_add(evt_log.event_name, evt_log.msg, evt_log.beep_param);
                    break;
                case CLEAR_LOG_EVENT:
                    CLEAR_LOG();
                    break;
                case READ_FLASHLOG_EVENT:
                    READ_FLASHLOG();
                    Log_add(evt_log.event_name, evt_log.msg, evt_log.beep_param);
                    break;
                case HELP_EVENT:
                    HELP();
                    Log_add(evt_log.event_name, evt_log.msg, evt_log.beep_param);
                    break;

                /*---key event---*/
                case CLICK_EVENT:
                    if(is_locked == false)
                    {
                        CLICK();
                        Log_add(evt_log.event_name, evt_log.msg, evt_log.beep_param);
                    }
                    break;
                case DOUBLE_CLICK_EVENT:
                    if(is_locked == false)
                    {
                        DOUBLE_CLICK();
                        Log_add(evt_log.event_name, evt_log.msg, evt_log.beep_param);
                    } 
                    break;
                case LONG_PRESS_EVENT:
                    if(is_locked == false)
                    {
                        LONG_PRESS();
                        Log_add(evt_log.event_name, evt_log.msg, evt_log.beep_param);
                    }
                    break;
                default:
                    break;
            }

        }
        LED_Process();
        Beep_Process();
    }
}

void vEvent_Process_Task_Init(void)
{
    xEvent_Process_Queue = xQueueCreate(EVENT_PROCESS_QUEUE_LEN, EVENT_PROCESS_ITEM_SIZE);
    xTaskCreate(vEvent_Process_Task, "Event_Process_Task", EVENT_PROCESS_STACK_DEPTH, NULL, EVENT_PROCESS_TASK_PRIO, &xEvent_Process);
}