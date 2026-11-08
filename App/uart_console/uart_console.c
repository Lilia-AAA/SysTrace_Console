#include "uart_console.h"
#include "uart_console_bsp.h"

/*Utils*/
#include "print_helper.h"

#include <string.h>

const cmd_table_t CMD_Table[] =
{
    {"LED ON",        "turn the LED on",                        Parse_led_on},
    {"LED OFF",       "turn the LED off",                       Parse_led_off},
    {"BEEP",          "beep for <ms> milliseconds",             Parse_beep},
    {"STATUS",        "print a system status snapshot",         Parse_status},
    {"LOCK",          "lock the device",                        Parse_lock },
    {"UNLOCK",        "unlock the device",                      Parse_unlock},
    {"READ LOG",      "dump the RAM log",                       Parse_read_log},
    {"CLEAR LOG",     "clear the RAM log",                      Parse_clear_log},
    {"READ FLASHLOG", "dump the flash log",                     Parse_read_flashlog},
    {"HELP",          "show this message",                      Parse_help},
    {"SET TIME",     "set time",                                Parse_set_time},
};

#define CMD_CNT     (sizeof(CMD_Table) / sizeof(CMD_Table[0]))
const uint32_t cmd_cnt = (uint32_t)CMD_CNT;

void uart_cmd_console(char *cmd)
{
    char cmd_norm[CMD_NORM_LEN];

    cmd_normalize(cmd_norm, sizeof(cmd_norm), cmd);

    if(cmd_norm[0] == '\0')
    {
        return;
    }
    
    int found = 0;
    const char *not_found = "unknown command\r\n";

    for(uint8_t i = 0; i < CMD_CNT; i++)
    {
        if(CMD_Table[i].parse_func(cmd_norm) == 1)
        {
            found = 1;
            break;
        }
    }
    if(found == 0)
    {
        PrintStr(not_found);
    }
}