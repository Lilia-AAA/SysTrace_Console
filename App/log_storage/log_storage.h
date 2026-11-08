#ifndef __LOG_STORAGE_H
#define __LOG_STORAGE_H

#include "stm32f1xx_hal.h"
#include "event_process_bsp.h"


#define LOG_CNT     16U
#define LOG_SIZE    32U
#define LOG_MAGIC   0x89


typedef struct
{
    uint32_t beep_param;
    Event_type_e event_name;
    uint8_t timestamp[6];
    uint16_t id;
    char msg[15];
    uint8_t magic;             /*magic number for check data integrity*/
}Log_t;

_Static_assert(sizeof(Log_t) == LOG_SIZE, "Log_t size must be 32B");

void Log_add(Event_type_e evt, char *msg, uint32_t param);
void Log_print(void);
void Log_clear(void);
void Log_flush(void);

const char *Log_eventname(Event_type_e evt);

#endif