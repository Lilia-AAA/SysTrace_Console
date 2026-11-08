#include "get_rtc.h"
#include "rtc.h"

void RTC_Get_Timestamp(uint8_t ts[RTC_TS_LEN])
{
    static RTC_DateTypeDef last  = {0};
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    if((sDate.Year != last.Year) || (sDate.Month != last.Month) || (sDate.Date != last.Date))
    {
        last = sDate;
        RTC_Date_Save();
    }

    ts[0] = (uint8_t)sDate.Year;
    ts[1] = (uint8_t)sDate.Month;
    ts[2] = (uint8_t)sDate.Date;
    ts[3] = (uint8_t)sTime.Hours;
    ts[4] = (uint8_t)sTime.Minutes;
    ts[5] = (uint8_t)sTime.Seconds;
}
