#include "led_proccess.h"

void LED_Process(void)
{
    static uint32_t now;
    static uint32_t last;
    static uint32_t period;
    static bool led_switch = false;
    static Dev_status_e last_status = DEV_NORMAL;

    now = xTaskGetTickCount();
    switch(dev_status)
    {
        case DEV_ALARM:
            period = LED_ALARM_PERIOD;
            break;
        case DEV_HIGH_ALARM:
            period = LED_HIGH_ALARM_PERIOD;
            break;
        default:
            period = 0;
            break;
    }
    
    if(dev_status != last_status)
    {
        last_status = dev_status;
        
        if(period != 0)
        {
            last = now;
            led_switch = true;
            LED_SET(led_switch);          /*once alarm, turn on the led*/
        }
        else
        {
            LED_SET(led_manual_ctrl);     /*keep the led status when no alarm*/
            return;
        }
    }
    
    if((now - last >= period) && (dev_status == DEV_ALARM || dev_status == DEV_HIGH_ALARM))
    {
        led_switch = !led_switch;
        last = now;
        LED_SET(led_switch);
    }
}
