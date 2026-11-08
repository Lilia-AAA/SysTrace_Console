#ifndef __KEY_CONTROL_TASK_H
#define __KEY_CONTROL_TASK_H

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "event_process_task.h"


#define KEY_PORT                   GPIOA
#define KEY_PIN                    GPIO_PIN_8

#define DEBOUNCE                   pdMS_TO_TICKS(20U)
#define LONG_PRESS_TIME            pdMS_TO_TICKS(1000U)
#define DOUBLE_CLICK_WINDOW        pdMS_TO_TICKS(300U)
#define KEY_TASK_SCAN_PERIOD       pdMS_TO_TICKS(10U)

#define KEY_CONTROL_STACK_DEPTH    80U
#define KEY_CONTROL_TASK_PRIO      1U

extern TaskHandle_t  xKey_Control;

typedef enum
{
    KEY_IDLE,             /*wait for key down or up*/
    KEY_DEBOUNCE,         /*first key down debounce*/
    KEY_DOWN,             /*confirm key down, start timing*/
    KEY_WAIT_SECOND,      /*confirm key down, wait for second click*/
    KEY_DEBOUNCE2,        /*second key down debounce*/
    KEY_WAIT_UP           /*wait for key up*/
}key_status_e;

void vKey_Control_Task_Init(void);
void vKey_Control_Task(void *pvParameters);

#endif