#include "app.h"
#include "runtime.h"
#include "diagnostics.h"
#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/drivers/keydev.h>
#include <gint/drivers/r61524.h>
#include <gint/kmalloc.h>
#include <gint/gint.h>
#include <gint/rtc.h>
#include <gint/timer.h>
#if defined(NG_DIAGNOSTIC) && defined(__sh__)
#include <gint/mmu.h>
#endif
#include <stdio.h>
static NgApp app;
static volatile int wakeup;
static NgRuntime runtime;
static int scheduler=-1;
static bool timer_active,rtc_active;
static uint16_t saved_brightness;
static bool brightness_saved;
static uint32_t idle_last,idle_fraction;
static const int native_keys[]={KEY_0,KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,KEY_6,KEY_7,KEY_8,KEY_9,
 KEY_ADD,KEY_SUB,KEY_MUL,KEY_DIV,KEY_LEFTP,KEY_RIGHTP,KEY_DOT,KEY_POWER,
 KEY_UP,KEY_RIGHT,KEY_DOWN,KEY_LEFT,KEY_EXE,KEY_DEL,
 KEY_F1,KEY_F2,KEY_F3,KEY_F4,KEY_F5,KEY_F6,KEY_EXIT,KEY_MENU,KEY_SHIFT,KEY_ALPHA,KEY_ACON,KEY_NEG};
static const int logical_keys[]={'0','1','2','3','4','5','6','7','8','9',
 '+','-','*','/','(',')','.','^',NGK_UP,NGK_RIGHT,NGK_DOWN,NGK_LEFT,NGK_EXE,NGK_DEL,
 NGK_F1,NGK_F2,NGK_F3,NGK_F4,NGK_F5,NGK_F6,NGK_EXIT,NGK_MENU,NGK_SHIFT,NGK_ALPHA,NGK_ACON,'-'};
static void rect(void *ctx,int x,int y,int w,int h,uint16_t color)
{(void)ctx;drect(x,y,x+w-1,y+h-1,color);}
static int load(void *ctx,NgSession *s,unsigned id){(void)ctx;return ng_storage_load(s,id);}
static bool save(void *ctx,NgSession *s){(void)ctx;return ng_storage_save(s);}
static int load_settings(void *ctx,NgSettings *s){(void)ctx;if(!ng_storage_migrate(&app.session))snprintf(app.notice,sizeof app.notice,"Legacy saves retained; migration incomplete.");return ng_settings_load(s);}
static bool save_settings(void *ctx,NgSettings *s){(void)ctx;return ng_settings_save(s);}
static int pulse(void) __attribute__((no_instrument_function));
static int pulse(void){wakeup=1;return TIMER_CONTINUE;}
static void restore_light(void)
{
 if(brightness_saved){r61524_set(0x5a1,saved_brightness);brightness_saved=false;ng_diag_emit(NGD_RESTORE,saved_brightness,0);}
 runtime.dimmed=false;
}
static void suspend_clock(void)
{
 if(timer_active){timer_pause(scheduler);timer_active=false;ng_diag_emit(NGD_TIMER_STOP,0,0);}
 if(rtc_active){rtc_periodic_disable();rtc_active=false;ng_diag_emit(NGD_TIMER_STOP,0,1);}
 wakeup=0;
}
static int read_power_settings(void *unused)
{(void)unused;ng_runtime_init(&runtime,rtc_ticks(),ng_os_backlight_duration(),ng_os_apo_minutes());return 0;}
static void power_settings(void)
{
 (void)gint_world_switch(GINT_CALL(read_power_settings,(void *)NULL));
 app.backlight_ms=runtime.backlight_ms;app.apo_ms=runtime.apo_ms;app.power_os_settings=runtime.os_settings;
 idle_last=rtc_ticks();idle_fraction=0;
}
static void world_action(bool power)
{
 suspend_clock();restore_light();
 if(!ng_storage_cleanup())snprintf(app.notice,sizeof app.notice,"Storage close failed; unsaved RAM retained.");
 clearevents();ng_diag_emit(power?NGD_OFF_ENTER:NGD_MENU_ENTER,rtc_ticks(),0);
 if(power)gint_poweroff(true);else gint_osmenu();
 ng_diag_emit(power?NGD_OFF_RETURN:NGD_MENU_RETURN,rtc_ticks(),0);
 power_settings();ng_runtime_rebase(&runtime,rtc_ticks());
}
static void osmenu(void *ctx){(void)ctx;world_action(false);}
static void off(void *ctx){(void)ctx;world_action(true);}
static int repeat(int key,int duration,int count)
{(void)duration;if(key!=KEY_UP && key!=KEY_RIGHT && key!=KEY_DOWN && key!=KEY_LEFT)return -1;return count?125000:500000;}
static void barrier(void)
{
 clearevents();app.held=0;app.blocked=0;
 for(unsigned i=0;i<sizeof(native_keys)/sizeof(native_keys[0]);i++)if(keydown(native_keys[i])) {
  int bit=ng_key_index(logical_keys[i]);if(bit>=0)app.held|=UINT64_C(1)<<(unsigned)bit;
 }
 ng_app_barrier(&app);
}
static void draw(void){NgCanvas canvas={NULL,rect};ng_render(&app,&canvas);dupdate();}
static void draw_hud(void)
{NgCanvas canvas={NULL,rect};ng_render_hud(&app,&canvas);r61524_display_rect(gint_vram,0,395,0,23);}
static void sample(void)
{
 char marker;kmalloc_arena_t *arena=kmalloc_get_arena("_uram");
 kmalloc_gint_stats_t *stats=arena?kmalloc_get_gint_stats(arena):NULL;
 ng_diag_sample(rtc_ticks(),(uintptr_t)&marker,arena?arena->stats.live_blocks:-1,stats?(int32_t)stats->free_memory:-1);
 ng_diag_context(app.screen,app.active?app.session.game.id:app.selected_id);
#ifdef NG_DIAGNOSTIC
 const char *names[2]={"_uram","_ostk"};
 for(unsigned i=0;i<2;i++){
  kmalloc_arena_t *a=kmalloc_get_arena(names[i]);
  kmalloc_gint_stats_t *s=a?kmalloc_get_gint_stats(a):NULL;
  if(a && s)ng_diag_arena(i,(uint32_t)((uintptr_t)a->end-(uintptr_t)a->start),s->used_memory,s->free_memory,s->peak_used_memory,a->stats.live_blocks,a->stats.peak_live_blocks);
 }
#endif
}
int main(void)
{
 char stack_marker;ng_diag_reset((uintptr_t)&stack_marker);
 ng_diag_clock(rtc_ticks);
#if defined(NG_DIAGNOSTIC) && defined(__sh__)
 extern void *gint_stack_top;
 ng_diag_stack_range((uintptr_t)gint_stack_top,(uintptr_t)mmu_uram()+mmu_uram_size());
#endif
 dsetvram(gint_vram,NULL);
 NgHooks hooks={NULL,load,save,load_settings,save_settings,osmenu,off};
 rtc_time_t time;rtc_get_time(&time);
 uint32_t seed=rtc_ticks()^((uint32_t)time.year<<16)^((uint32_t)time.month_day<<8)^time.month;
 ng_app_init(&app,hooks,seed);keydev_set_transform(keydev_std(),(keydev_transform_t){KEYDEV_TR_REPEATS,repeat});
 scheduler=timer_configure(TIMER_ANY,250000,GINT_CALL(pulse));
 timer_active=rtc_active=brightness_saved=false;power_settings();barrier();draw();ng_runtime_rebase(&runtime,rtc_ticks());
 for(;;) {
  sample();
  if(!timer_active && scheduler>=0){timer_start(scheduler);timer_active=true;ng_diag_emit(NGD_TIMER_START,0,0);}
  if(scheduler<0 && !rtc_active && app.power_available) {
   rtc_active=rtc_periodic_enable(RTC_16Hz,GINT_CALL(pulse));
   if(rtc_active)ng_diag_emit(NGD_TIMER_START,0,1);
   if(!rtc_active) {
    app.power_available=false;snprintf(app.notice,sizeof app.notice,"Timer unavailable. Reopen the app to retry.");
   }
  }
  if(!app.power_available && app.screen==NG_PLAY){(void)ng_checkpoint(&app);app.screen=NG_ENTRY;app.modal=NG_MODAL_NONE;barrier();draw();ng_runtime_rebase(&runtime,rtc_ticks());}
  wakeup=0;
  bool cpu=app.screen==NG_PLAY && !app.modal && app.session.game.cpu_pending;
  bool stress=app.modal==NG_MODAL_DIAGNOSTICS && ng_diag_stress_active();
  key_event_t e=keydev_read(keydev_std(),!cpu && !stress,app.power_available?&wakeup:NULL);
  uint32_t now=rtc_ticks(),dt=ng_runtime_elapsed(&runtime,now);
  uint32_t idle_ticks=now>=idle_last?now-idle_last:NG_RTC_DAY-idle_last+now;idle_last=now;
  /* Only actual key DOWN/HOLD resets idle; wake flags, draws and saves do not. */
  bool input=e.type==KEYEV_DOWN || e.type==KEYEV_HOLD;
  uint64_t idle_units=(uint64_t)idle_ticks*1000+idle_fraction;
  idle_fraction=input?0:(uint32_t)(idle_units%128);
  unsigned power=ng_runtime_idle(&runtime,(uint32_t)(idle_units/128),input);
  if(power&NG_POWER_RESTORE)restore_light();
  if(power&NG_POWER_DIM){saved_brightness=r61524_get(0x5a1);brightness_saved=true;r61524_set(0x5a1,saved_brightness<0x14?saved_brightness:0x14);ng_diag_emit(NGD_DIM,saved_brightness,0);}
  uint64_t modifiers=app.held & ~app.blocked;
  bool global_shift=app.shift_pending || (modifiers&(UINT64_C(1)<<ng_key_index(NGK_SHIFT)));
  bool global_alpha=app.alpha_pending || (modifiers&(UINT64_C(1)<<ng_key_index(NGK_ALPHA)));
  unsigned epoch=app.epoch;bool changed=ng_app_tick(&app,dt);
  bool hud_only=changed && app.screen==NG_PLAY && !app.modal && !ng_module(app.session.game.id)->tick;
  if(app.epoch!=epoch){
   /* A phase barrier discards stale game input, but must retain global exit. */
   bool global=e.type==KEYEV_DOWN && (e.key==KEY_MENU || (e.key==KEY_ACON && global_shift && !global_alpha));
   barrier();
   if(global){uint64_t bit=UINT64_C(1)<<ng_key_index(e.key==KEY_MENU?NGK_MENU:NGK_ACON);app.held&=~bit;app.blocked&=~bit;app.shift_pending=global_shift;app.alpha_pending=global_alpha;}else e.type=KEYEV_NONE;
  }
  epoch=app.epoch;
  int key=0;for(unsigned i=0;i<sizeof(native_keys)/sizeof(native_keys[0]);i++)if(e.key==(unsigned)native_keys[i]){key=logical_keys[i];break;}
#ifdef NG_DIAGNOSTIC
  bool ready_candidate=app.screen==NG_ENTRY;
  uint32_t ready_start=rtc_ticks(),previous_seed=app.session.game.seed,previous_run=app.session.game.run_id;
#endif
  if(e.type==KEYEV_DOWN || e.type==KEYEV_UP || e.type==KEYEV_HOLD){
   ng_diag_emit(NGD_INPUT,(uint32_t)key,e.type);bool event_changed=ng_app_event(&app,key,e.type==KEYEV_DOWN?NG_DOWN:e.type==KEYEV_UP?NG_UP:NG_HOLD);
   if(event_changed)hud_only=false;
   changed=event_changed||changed;
  }else if(stress && !(power&NG_POWER_OFF)){bool progress=ng_diag_stress_step();if(progress)hud_only=false;changed=progress||changed;}
  else if(cpu && !(power&NG_POWER_OFF)){bool moved=ng_app_cpu(&app);if(moved)hud_only=false;changed=moved||changed;}
  if(power&NG_POWER_OFF){ng_app_poweroff(&app);changed=true;hud_only=false;}
  if(app.epoch!=epoch){barrier();runtime.fraction=0;}
  /* World-switch/OS/off, drawing and storage duration never inflate active time. */
  if(changed){if(hud_only)draw_hud();else draw();}
#ifdef NG_DIAGNOSTIC
  if(ready_candidate && app.screen==NG_PLAY && (app.session.game.seed!=previous_seed || app.session.game.run_id!=previous_run)){
   uint32_t ready_end=rtc_ticks();ng_diag_ready(app.session.game.id,ready_end>=ready_start?ready_end-ready_start:NG_RTC_DAY-ready_start+ready_end);
  }
#endif
  runtime.last=rtc_ticks();
 }
}
