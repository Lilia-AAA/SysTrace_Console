#ifndef __LED_PROCESS_H
#define __LED_PROCESS_H

#include "stm32f1xx_hal.h"
#include "event_process_task.h"
#include "event_process_bsp.h"

#include <stdbool.h>

#define LED_ALARM_PERIOD           pdMS_TO_TICKS(400)
#define LED_HIGH_ALARM_PERIOD      pdMS_TO_TICKS(250)

extern bool led_manual_ctrl;
extern Dev_status_e dev_status;

void LED_Process(void); 

#endif