#include "runtime.h"
#include <limits.h>
void ng_runtime_init(NgRuntime *r,uint32_t now,int backlight,int apo)
{
 bool valid_backlight=backlight==1 || backlight==2 || backlight==6;
 bool valid_apo=apo==10 || apo==60;
 *r=(NgRuntime){.last=now,.backlight_ms=valid_backlight?(uint32_t)backlight*30000u:60000u,
  .apo_ms=valid_apo?(uint32_t)apo*60000u:600000u,.os_settings=valid_backlight && valid_apo};
}
uint32_t ng_runtime_elapsed(NgRuntime *r,uint32_t now)
{
 uint32_t ticks=now>=r->last?now-r->last:NG_RTC_DAY-r->last+now;
 uint64_t units=(uint64_t)ticks*1000u+r->fraction;
 r->last=now;r->fraction=(uint32_t)(units%128u);return (uint32_t)(units/128u);
}
void ng_runtime_rebase(NgRuntime *r,uint32_t now){r->last=now;r->fraction=0;}
unsigned ng_runtime_idle(NgRuntime *r,uint32_t elapsed,bool input)
{
 if(input){r->idle_ms=0;if(r->dimmed){r->dimmed=false;return NG_POWER_RESTORE;}return 0;}
 r->idle_ms=elapsed>UINT32_MAX-r->idle_ms?UINT32_MAX:r->idle_ms+elapsed;
 if(r->idle_ms>=r->apo_ms){r->idle_ms=0;unsigned flags=NG_POWER_OFF|(r->dimmed?NG_POWER_RESTORE:0);r->dimmed=false;return flags;}
 if(!r->dimmed && r->idle_ms>=r->backlight_ms){r->dimmed=true;return NG_POWER_DIM;}
 return 0;
}
