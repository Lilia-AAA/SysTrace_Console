#ifndef __EVENT_PROCESS_BSP_H
#define __EVENT_PROCESS_BSP_H

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include <stdbool.h>

#define LED_PORT                   GPIOB
#define LED_PIN                    GPIO_PIN_1


typedef enum
{
    /*-- uart event--*/
    LED_ON_EVENT,
    LED_OFF_EVENT,
    BEEP_EVENT,
    STATUS_EVENT,
    LOCK_EVENT,
    UNLOCK_EVENT,
    READ_LOG_EVENT,
    CLEAR_LOG_EVENT,
    READ_FLASHLOG_EVENT,
    HELP_EVENT,
    /*---key event---*/
    CLICK_EVENT,
    DOUBLE_CLICK_EVENT,
    LONG_PRESS_EVENT,
}Event_type_e;

typedef enum
{
    DEV_NORMAL,
    DEV_ALARM,
    DEV_HIGH_ALARM,
}Dev_status_e;

extern QueueHandle_t xBeep_Queue;
extern Dev_status_e  dev_status;
extern bool led_manual_ctrl;
extern bool is_locked;
extern bool beep_active;

void LED_SET(bool on_off);
/*----- uart event-----*/
void LED_ON(void);
void LED_OFF(void);
void BEEP(uint32_t time_ms);
void STATUS(void);
void LOCK(void);
void UNLOCK(void);
void READ_LOG(void);
void CLEAR_LOG(void);
void READ_FLASHLOG(void);
void HELP(void);
/*------key event------*/
void CLICK(void);
void DOUBLE_CLICK(void);
void LONG_PRESS(void);

#endif