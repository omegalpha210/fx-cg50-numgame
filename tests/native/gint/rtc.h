#ifndef NG_NATIVE_TEST_RTC_H
#define NG_NATIVE_TEST_RTC_H
#include <stdint.h>
#include <gint/gint.h>
typedef struct {uint16_t year;uint8_t week_day,month,month_day,hours,minutes,seconds,ticks;} rtc_time_t;
void rtc_get_time(rtc_time_t *time);
uint32_t rtc_ticks(void);
enum {RTC_16Hz=3};
bool rtc_periodic_enable(int frequency,gint_call_t callback);
void rtc_periodic_disable(void);
#endif
