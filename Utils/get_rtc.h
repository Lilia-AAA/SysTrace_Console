#ifndef __GET_RTC_H
#define __GET_RTC_H

#include <stdint.h>

#define RTC_TS_LEN      6U

void RTC_Get_Timestamp(uint8_t ts[RTC_TS_LEN]);

#endif
