#ifndef ___BEEP_PROCESS_H
#define ___BEEP_PROCESS_H

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "event_process_task.h"
#include "event_process_bsp.h"

#include <stdbool.h>

#define BEEP_PORT                  GPIOB
#define BEEP_PIN                   GPIO_PIN_0

#define BEEP_HIGH_ALARM_PERIOD     pdMS_TO_TICKS(300)
#define BEEP_ON_HIGH_ALARM         200U
#define BEEP_TASK_SCAN_PERIOD      pdMS_TO_TICKS(10)
#define BEEP_MAX_MS                1000000UL  

#define BEEP_QUEUE_LEN       3U
#define BEEP_ITEM_SIZE       sizeof(uint32_t)

#define BEEP_TASK_PRIO       1U
#define BEEP_STACK_DEPTH     70U

extern bool beep_active;
extern Dev_status_e dev_status;
extern TaskHandle_t xBeep;
extern QueueHandle_t xBeep_Queue;

void vBeep_Task_Init(void);
void vBeep_Task(void *pvParameters);
void Beep_Process(void);

#endif
