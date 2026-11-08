#ifndef __UART_CONSOLE_H
#define __UART_CONSOLE_H

#include "stm32f1xx_hal.h"

#define CMD_NORM_LEN    31U

typedef struct
{
    const char *cmd_name;
    const char *cmd_desc;
    int (*parse_func)(const char *cmd_arg);
}cmd_table_t;

extern const cmd_table_t CMD_Table[];
extern const uint32_t    cmd_cnt;

void uart_cmd_console(char *cmd);


#endif
