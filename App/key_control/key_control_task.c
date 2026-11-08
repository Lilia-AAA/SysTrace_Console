#include "key_control_task.h"
#include "event_process_bsp.h"
#include "log_storage.h"

/*Utils*/
#include "print_helper.h"

#include <string.h>

TaskHandle_t xKey_Control;

static key_status_e KEY_Status  = KEY_IDLE;
static TickType_t   key_press   = 0;   
static TickType_t   key_release = 0;   

static void key_control_post(Event_type_e event, const char *msg, uint32_t beep_time)
{
    Log_t log;
    memset(&log, 0, sizeof(log));
    log.event_name = event;
    log.beep_param = beep_time;
    strncpy(log.msg, msg, sizeof(log.msg) - 1U);
    log.msg[sizeof(log.msg) - 1U] = '\0'; 

    if(xQueueSend(xEvent_Process_Queue, &log, 0U) != pdPASS)
    {
        PrintStr("event queue is full, command is lost!\r\n");
    }
}

static uint8_t key_is_pressed(void)
{
    return (HAL_GPIO_ReadPin(KEY_PORT, KEY_PIN) == GPIO_PIN_RESET) ? 1U : 0U;  
}

void vKey_Control_Task(void *pvParameters)
{
    TickType_t PreviousWakeTime = xTaskGetTickCount();

    for(;;)
    {
        uint8_t    pressed = key_is_pressed();
        TickType_t now     = xTaskGetTickCount();

        switch(KEY_Status)
        {
            case KEY_IDLE:
                if(pressed == 1)
                {
                    key_press  = now;
                    KEY_Status = KEY_DEBOUNCE;
                }
                break;

            case KEY_DEBOUNCE:
                if(pressed == 0)
                {
                    KEY_Status = KEY_IDLE;
                }
                else if(now - key_press >= DEBOUNCE)
                {
                    KEY_Status = KEY_DOWN;
                }
                break;

            case KEY_DOWN:
                if(now - key_press >= LONG_PRESS_TIME)
                {
                    key_control_post(LONG_PRESS_EVENT, "LONG PRESS\r\n", 0U);
                    KEY_Status = KEY_WAIT_UP;
                }
                else if(pressed == 0)
                {
                    /*just bounce, back to idle*/
                    key_release = now;
                    KEY_Status  = KEY_WAIT_SECOND;
                }
                break;

            case KEY_WAIT_SECOND:
                if(pressed == 1)
                {
                    /*another click happened*/
                    if(now - key_release >= DOUBLE_CLICK_WINDOW)
                    {
                       key_control_post(CLICK_EVENT, "CLICK\r\n", 0U);
                        KEY_Status = KEY_DEBOUNCE;
                    }
                    /*meant to be a double click*/
                    else
                    {  
                        key_press  = now;
                        KEY_Status = KEY_DEBOUNCE2;
                    }
                }
                else if(now - key_release >= DOUBLE_CLICK_WINDOW)
                {
                    key_control_post(CLICK_EVENT, "CLICK\r\n", 0U);
                    KEY_Status = KEY_IDLE;
                }
                break;

            case KEY_DEBOUNCE2:
                if(pressed == 0)
                {
                    /*just bounce, back to wait for second click*/
                    KEY_Status = KEY_WAIT_SECOND;
                }
                else if(now - key_press >= DEBOUNCE)
                {
                    key_control_post(DOUBLE_CLICK_EVENT, "DOUBLE CLICK\r\n", 0U);
                    KEY_Status = KEY_WAIT_UP;
                }
                break;

            case KEY_WAIT_UP:
                if(pressed == 0)
                {
                    KEY_Status = KEY_IDLE;
                }
                break;

            default:
                KEY_Status = KEY_IDLE;
                break;
        }

        vTaskDelayUntil(&PreviousWakeTime, KEY_TASK_SCAN_PERIOD);
    }
}

void vKey_Control_Task_Init(void)
{
    xTaskCreate(vKey_Control_Task, "Key_Control_Task", KEY_CONTROL_STACK_DEPTH, NULL, KEY_CONTROL_TASK_PRIO, &xKey_Control);
}
