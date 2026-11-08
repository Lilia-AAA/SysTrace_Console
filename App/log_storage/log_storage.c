#include "log_storage.h"
#include "event_process_bsp.h"
#include "flash_log.h"
#include "log_flush_task.h"
/*Utils*/
#include "print_helper.h"
#include "stm32f1xx_hal_def.h"
#include "w25q_flash.h"
#include "get_rtc.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static Log_t   log_ram_buf[LOG_CNT];
static uint8_t wr_index      = 0U;   
static uint8_t log_count     = 0U;   
static uint8_t flush_pending = 0U;      /*Count of logs waiting to be written to flash*/
static uint32_t log_drop_cnt = 0U;      /*Counter for dropped logs (overflow, not flushed in time)*/

static char log_txt[PRINT_BUF_LEN];

static uint8_t log_oldest(void)
{
    return (uint8_t)((wr_index + LOG_CNT - log_count) % LOG_CNT);
}

const char *Log_eventname(Event_type_e evt)
{
    switch (evt)
    {
        case LED_ON_EVENT:          return "LED_ON";
        case LED_OFF_EVENT:         return "LED_OFF";
        case BEEP_EVENT:            return "BEEP";
        case STATUS_EVENT:          return "STATUS";
        case LOCK_EVENT:            return "LOCK";
        case UNLOCK_EVENT:          return "UNLOCK";
        case READ_LOG_EVENT:        return "READ_LOG";
        case CLEAR_LOG_EVENT:       return "CLEAR_LOG";
        case READ_FLASHLOG_EVENT:   return "READ_FALSH_LOG";
        case HELP_EVENT:            return "HELP";
        case CLICK_EVENT:           return "KEY_CLICK";
        case DOUBLE_CLICK_EVENT:    return "KEY_DOUBLE_CLICK";
        case LONG_PRESS_EVENT:      return "KEY_LONG_PRESS";
        default:                    return "UNKNOWN";
    }
}

void Log_add(Event_type_e evt, char *msg, uint32_t param)
{
    Log_t log;
    memset(&log, 0U, sizeof(Log_t));
    
    log.event_name = evt;
    log.beep_param = param;
    RTC_Get_Timestamp(log.timestamp);  

    strncpy(log.msg, msg, sizeof(log.msg) - 1U);
    log.msg[sizeof(log.msg) - 1U] = '\0';
    log.magic = LOG_MAGIC;          /*Mark this log as pending, waiting for flash save*/

    uint8_t pending;

    taskENTER_CRITICAL();

    /*buffer full, will overwrite oldest entry*/
    if (log_count == LOG_CNT)
    {
        /*the overwritten entry has not been flushed yet -> this log is lost*/
        if (log_ram_buf[wr_index].magic == LOG_MAGIC)
        {
            log_drop_cnt++;
            flush_pending--;
        }
    }
    else
    {
        log_count++;
    }

    log_ram_buf[wr_index] = log;
    wr_index = (uint8_t)((wr_index + 1U) % LOG_CNT);
    flush_pending++;

    pending = flush_pending;

    taskEXIT_CRITICAL();

    if (pending >= LOG_FLUSH_THRESHOLD)
    {
        Log_Flush_Trigger();
    }
}

void Log_print(void)
{
    static Log_t ram_log;
    uint8_t start;          /* oldest log index */
    uint8_t count;

    taskENTER_CRITICAL();
    count = log_count;
    start = log_oldest();
    taskEXIT_CRITICAL();

    if (count == 0U)
    {
        PrintStr("No logs\r\n");
        return;
    }

    for(uint8_t n = 0U; n < count; n++)
    {
        uint8_t i = (uint8_t)((start + n) % LOG_CNT);

        taskENTER_CRITICAL();
        ram_log = log_ram_buf[i];
        taskEXIT_CRITICAL();

        snprintf(log_txt, sizeof(log_txt),
        "time:%02d-%02d-%02d %02d:%02d:%02d evt: %s, param: %lu, msg:%s\r\n",
        ram_log.timestamp[0],
        ram_log.timestamp[1],
        ram_log.timestamp[2],
        ram_log.timestamp[3],
        ram_log.timestamp[4],
        ram_log.timestamp[5],
        Log_eventname(ram_log.event_name),
        (unsigned long)ram_log.beep_param,
        ram_log.msg);

        PrintStr(log_txt);
    }
}

void Log_clear(void)
{
    taskENTER_CRITICAL();
    wr_index      = 0U;
    log_count     = 0U;
    flush_pending = 0U;
    taskEXIT_CRITICAL();
}


void Log_flush(void)
{
    Log_t log_flush;
    Log_t snap;

    for (;;)
    {
        uint8_t start;
        uint8_t count;
        uint8_t slot  = 0U;
        uint8_t found = 0U;

        taskENTER_CRITICAL();

        count = log_count;
        start = log_oldest();

        /*scan from oldest to newest log*/
        for (uint8_t n = 0U; n < count; n++)
        {
            uint8_t i = (uint8_t)((start + n) % LOG_CNT);

            if (log_ram_buf[i].magic == LOG_MAGIC)
            {
                slot  = i;
                found = 1U;
                break;
            }
        }
        if(found)
        {
            log_flush = log_ram_buf[slot];
        }
        taskEXIT_CRITICAL();

        /*No more pending logs, exit flush loop*/
        if (found == 0U) return;

        snap = log_flush;
        if(Flash_Log_Save(&w25q_dev, &log_flush) != HAL_OK) return;

        taskENTER_CRITICAL();
        if(memcmp(&log_ram_buf[slot], &snap, sizeof(Log_t)) == 0)
        {
            log_ram_buf[slot].magic = 0U;            /*Mark this entry: already saved to flash*/ 
            flush_pending--;
        }
        taskEXIT_CRITICAL();
    }
}
