#include "uart_console_bsp.h"
#include "beep_process.h"
#include "log_storage.h"
/*Utils*/
#include "print_helper.h"
#include "rtc.h"
#include "stm32f1xx_hal_def.h"
#include "stm32f1xx_hal_rtc.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void uart_cmd_post(Event_type_e event, const char *msg, uint32_t beep_time)
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

static char cmd_norm_char(char c)
{
    if(c >= 'a' && c <= 'z')
    {
        c += ('A' - 'a');
    }
    if(c == '-' || c == '_' || c == '\r' || c == '\n' || c == '\t')
    {
        c = ' ';
    }
    return (char)c;
}

void cmd_normalize(char *dst, size_t dst_size, const char *src)
{
    size_t i = 0;
    if(dst_size == 0)
    {
        return;
    }

    for(; *src != '\0'; src++)
    {
        char c = cmd_norm_char(*src);
        /*drop leading blanks and multiple spaces between words*/
        if((c == ' ') && (i == 0 || dst[i - 1] == ' '))
        {
            continue;
        }
        /*truncate if dst is full*/
        if((i + 1) >= dst_size)
        {
            break;
        }
        dst[i] = c;
        i++;
    }
    while((i > 0) && (dst[i -1] == ' '))
    {
        i--;
    }
    dst[i] = '\0';
}

int Parse_led_on(const char *cmd_arg)
{
    if(strcmp(cmd_arg, "LED ON") == 0)
    {
        uart_cmd_post(LED_ON_EVENT, "LED ON\r\n", 0U);
        return 1; 
    }
    return 0;
}
int Parse_led_off(const char *cmd_arg)
{
    if(strcmp(cmd_arg, "LED OFF") == 0)
    {
        uart_cmd_post(LED_OFF_EVENT, "LED OFF\r\n", 0U);
        return 1;
    }
    return 0;
}
int Parse_beep(const char *cmd_arg)
{
    char msg[20];
    if(strncmp(cmd_arg, "BEEP ", 5U) == 0)
    {
        const char *p = &cmd_arg[5];
        char *endptr = NULL;
        unsigned long val = strtoul(p, &endptr, 10);

        if((val > pdMS_TO_TICKS(BEEP_MAX_MS)) || (endptr == p) || (*endptr != '\0'))
        {
            uart_cmd_post(BEEP_EVENT, "BEEP FAILED\r\n", 0U);
            return 1;
        }
        else
        {
            uint32_t beep_ms = (uint32_t)val;
            snprintf(msg, sizeof(msg), "BEEP %lu ms\r\n", (unsigned long)beep_ms);
            uart_cmd_post(BEEP_EVENT, msg, beep_ms);
            return 1;
        }
    }
    else if(strcmp(cmd_arg, "BEEP") == 0)
    {
        PrintStr("Correct usage: BEEP <ms>\r\n");
        return 1;
    }
    return 0;
}
int Parse_status(const char *cmd_arg)
{
    if(strcmp(cmd_arg, "STATUS") == 0)
    {
        uart_cmd_post(STATUS_EVENT, "STATUS\r\n", 0U);
        return 1;
    }
    return 0;
}
int Parse_lock(const char *cmd_arg)
{
    if(strcmp(cmd_arg, "LOCK") == 0)
    {
        uart_cmd_post(LOCK_EVENT, "LOCK\r\n", 0U);        
        return 1;
    }
    return 0;
}
int Parse_unlock(const char *cmd_arg)
{
    if(strcmp(cmd_arg, "UNLOCK") == 0)
    {
        uart_cmd_post(UNLOCK_EVENT, "UNLOCK\r\n", 0U);        
        return 1;
    }
    return 0;
}
int Parse_read_log(const char *cmd_arg)
{
    if(strcmp(cmd_arg, "READ LOG") == 0)
    {
        uart_cmd_post(READ_LOG_EVENT, "READRAM LOG\r\n", 0U);        
        return 1;
    }
    return 0;
}
int Parse_clear_log(const char *cmd_arg)
{
    if(strcmp(cmd_arg, "CLEAR LOG") == 0)
    {
        uart_cmd_post(CLEAR_LOG_EVENT, "CLEAR RAM LOG\r\n", 0U);        
        return 1;
    }
    return 0;
}
int Parse_read_flashlog(const char *cmd_arg)
{
    if(strcmp(cmd_arg, "READ FLASHLOG") == 0)
    {
        uart_cmd_post(READ_FLASHLOG_EVENT, "GET FLASHLOG\r\n", 0U);        
        return 1;
    }
    return 0;
}
int Parse_help(const char *cmd_arg)
{
    if(strcmp(cmd_arg, "HELP") == 0)
    {
        uart_cmd_post(HELP_EVENT, "HELP\r\n", 0U);        
        return 1;
    }    
    return 0;
}

int Parse_set_time(const char *cmd_arg)
{
    if(strncmp(cmd_arg, "SET TIME ", 9U) == 0)
    {
        unsigned y = 0, mon = 0, d = 0, h = 0, min = 0, s = 0;

        if(sscanf(&cmd_arg[9], "%u/%u/%u %u:%u:%u", &y, &mon, &d, &h, &min, &s) == 6)
        {
            RTC_TimeTypeDef time = {0};
            RTC_DateTypeDef date = {0};

            time.Hours   = (uint8_t)h;
            time.Minutes = (uint8_t)min;
            time.Seconds = (uint8_t)s;

            date.Year    = (uint8_t)(y % 100U);
            date.Month   = (uint8_t)mon;
            date.Date    = (uint8_t)d;

            if((HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN) == HAL_OK) &&
               (HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN) == HAL_OK))
            {
                PrintStr("RTC SET\r\n");
            }
            else 
            {
                PrintStr("rtc set failed\r\n");
            }
            return 1;
        }
        PrintStr("usage: SET TIME 2026/12/15 8:30:20\r\n");
    }
    return 0;
}