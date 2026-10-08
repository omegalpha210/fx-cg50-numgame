#include "app.h"
#include "runtime.h"
#include "diagnostics.h"
#include "native_keys.h"
#include "usb_lifecycle.h"
#include "usb_native.h"
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
static UsbLifecycle usb;
static int scheduler=-1;
static bool timer_active,rtc_active,system_requested;
static uint16_t saved_brightness;
static bool brightness_saved;
static uint32_t idle_last,idle_fraction;
static void rect(void *ctx,int x,int y,int w,int h,uint16_t color)
{(void)ctx;drect(x,y,x+w-1,y+h-1,color);}
static int load_state(void *ctx,NgSettings *settings,NgSession *session,bool *active)
{(void)ctx;if(!ng_state_migrate(session))snprintf(app.notice,sizeof app.notice,"Old saves retained; migration incomplete.");return ng_state_load(settings,session,active);}
static bool save_state(void *ctx,NgSettings *settings,const NgSession *session,bool active)
{(void)ctx;return ng_state_save(settings,session,active);}
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
#if !defined(NG_NATIVE_TEST_GINT_H)
#include <gint/cpu.h>
int ng_os_enable_menu_return(void);
static int enable_menu_return(void *unused)
{ (void)unused; return ng_os_enable_menu_return(); }
#endif
static int read_power_settings(void *unused)
{
    (void)unused;
    ng_runtime_init(&runtime,rtc_ticks(),ng_os_backlight_duration(),ng_os_apo_minutes());
#if !defined(NG_NATIVE_TEST_GINT_H)
    /* KhiCAS Golden Rule: 5 minutes (300 seconds) auto-park on hardware */
    runtime.apo_ms = 5u * 60000u;
#endif
    return 0;
}
#if !defined(NG_NATIVE_TEST_GINT_H)
static void show_poweroff_notice(void)
{
    int box_w = 340, box_h = 96;
    int box_x = (396 - box_w) / 2;
    int box_y = (224 - box_h) / 2;
    drect_border(box_x, box_y, box_x + box_w - 1, box_y + box_h - 1, C_WHITE, 2, C_RGB(0, 16, 31));
    dtext_opt(396 / 2, box_y + 16, C_BLACK, C_NONE, DTEXT_CENTER, DTEXT_TOP, "Back to Main Menu");
    dtext_opt(396 / 2, box_y + 46, C_RGB(0, 12, 28), C_NONE, DTEXT_CENTER, DTEXT_TOP, "To shutdown, press SHIFT AC/ON");
    dtext_opt(396 / 2, box_y + 66, C_DARK, C_NONE, DTEXT_CENTER, DTEXT_TOP, "again in Main Menu");
    dupdate();
}
#endif
static void power_settings(void)
{
 (void)gint_world_switch(GINT_CALL(read_power_settings,(void *)NULL));
 app.backlight_ms=runtime.backlight_ms;app.apo_ms=runtime.apo_ms;app.power_os_settings=runtime.os_settings;
 idle_last=rtc_ticks();idle_fraction=0;
}
static void world_action(bool power)
{
 system_requested=true;
 if(!usb_handoff_begin(&usb,usb_native_sample()))return;
 restore_light();
 if(!ng_storage_cleanup()){
  snprintf(app.notice,sizeof app.notice,"Storage close failed; unsaved RAM retained.");
  usb_handoff_end(&usb,usb_native_sample());return;
 }
 clearevents();ng_diag_emit(power?NGD_OFF_ENTER:NGD_MENU_ENTER,rtc_ticks(),0);
#if !defined(NG_NATIVE_TEST_GINT_H)
 /* KhiCAS Rule: If power off, display notice, wait 1 second (128 ticks), clear events,
    and safely park in Casio OS Main Menu via 0x1EA6 + gint_osmenu(). */
 if (power) {
  show_poweroff_notice();
  uint32_t notice_t0 = rtc_ticks();
  while ((rtc_ticks() + NG_RTC_DAY - notice_t0) % NG_RTC_DAY < 128) sleep();
 } else {
  uint32_t wait_t0 = rtc_ticks();
  while ((keydown(KEY_MENU) || keydown(KEY_EXIT)) && ((rtc_ticks() + NG_RTC_DAY - wait_t0) % NG_RTC_DAY < 32)) {
   sleep();
   clearevents();
  }
 }
 clearevents();
 suspend_clock();
 (void)gint_world_switch(GINT_CALL(enable_menu_return,(void *)NULL));
 gint_osmenu();
#else
 suspend_clock();
 if(power)gint_poweroff(true);else gint_osmenu();
#endif
 ng_diag_emit(power?NGD_OFF_RETURN:NGD_MENU_RETURN,rtc_ticks(),0);
 power_settings();ng_runtime_rebase(&runtime,rtc_ticks());
 usb_handoff_end(&usb,usb_native_sample());
}
static void osmenu(void *ctx){(void)ctx;world_action(false);}
static void off(void *ctx){(void)ctx;world_action(true);}
static int repeat(int key,int duration,int count)
{(void)duration;if(key!=KEY_UP && key!=KEY_RIGHT && key!=KEY_DOWN && key!=KEY_LEFT)return -1;return count?125000:500000;}
static void barrier(void)
{
 clearevents();app.held=0;app.blocked=0;
 for(unsigned i=0;i<sizeof ng_native_keys/sizeof ng_native_keys[0];i++)
  if(keydown(ng_native_keys[i].native)) {
  int bit=ng_key_index(ng_native_keys[i].logical);
  if(bit>=0)app.held|=UINT64_C(1)<<(unsigned)bit;
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
 NgHooks hooks={.load_state=load_state,.save_state=save_state,.os_menu=osmenu,.power_off=off};
 rtc_time_t time;rtc_get_time(&time);
 uint32_t seed=rtc_ticks()^((uint32_t)time.year<<16)^((uint32_t)time.month_day<<8)^time.month;
 ng_app_init(&app,hooks,seed);keydev_set_transform(keydev_std(),(keydev_transform_t){KEYDEV_TR_REPEATS,repeat});
 scheduler=timer_configure(TIMER_ANY,250000,GINT_CALL(pulse));
 timer_active=rtc_active=brightness_saved=false;power_settings();usb_initialize(&usb,usb_native_sample());barrier();draw();ng_runtime_rebase(&runtime,rtc_ticks());
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
  usb_observe(&usb,usb_native_sample());
  if(usb_take_request(&usb)){
   /* Synchronous CPU/generator steps are atomic; service at their next
      boundary before starting another step. No partial game is persisted. */
   ng_app_osmenu(&app);barrier();draw();runtime.last=rtc_ticks();continue;
  }
  uint32_t now=rtc_ticks(),dt=ng_runtime_elapsed(&runtime,now);
  system_requested=false;
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
  int key=ng_native_key_lookup(e.key);
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
  if((power&NG_POWER_OFF) && !system_requested){ng_app_poweroff(&app);changed=true;hud_only=false;}
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
