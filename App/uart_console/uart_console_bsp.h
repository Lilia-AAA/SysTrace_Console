#ifndef __UART_CONSOLE_BSP_H
#define __UART_CONSOLE_BSP_H

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "event_process_task.h"

extern QueueHandle_t xEvent_Process_Queue;

void cmd_normalize(char *dst, size_t dst_size, const char *src);

/* CMD_Parse_function prototypes */
int Parse_led_on(const char *cmd_arg);
int Parse_led_off(const char *cmd_arg);
int Parse_beep(const char *cmd_arg);
int Parse_status(const char *cmd_arg);
int Parse_lock(const char *cmd_arg);
int Parse_unlock(const char *cmd_arg);
int Parse_read_log(const char *cmd_arg);
int Parse_clear_log(const char *cmd_arg);
int Parse_read_flashlog(const char *cmd_arg);
int Parse_help(const char *cmd_arg);
int Parse_set_time(const char *cmd_arg);

#endif