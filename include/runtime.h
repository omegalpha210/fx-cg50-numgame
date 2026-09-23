#ifndef NUMGAME_RUNTIME_H
#define NUMGAME_RUNTIME_H
#include <stdbool.h>
#include <stdint.h>
#define NG_RTC_DAY 11059200u
enum {NG_POWER_DIM=1,NG_POWER_RESTORE=2,NG_POWER_OFF=4};
typedef struct {
 uint32_t last,fraction,idle_ms,backlight_ms,apo_ms;
 bool dimmed,os_settings;
} NgRuntime;
void ng_runtime_init(NgRuntime *r,uint32_t now,int backlight_half_minutes,int apo_minutes);
uint32_t ng_runtime_elapsed(NgRuntime *r,uint32_t now);
void ng_runtime_rebase(NgRuntime *r,uint32_t now);
unsigned ng_runtime_idle(NgRuntime *r,uint32_t elapsed,bool input);
/* Native LCD/OS boundary. Host tests provide these same functions. */
int ng_os_backlight_duration(void);
int ng_os_apo_minutes(void);
#endif
