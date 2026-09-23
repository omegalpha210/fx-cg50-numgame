#define _POSIX_C_SOURCE 200809L
#define _DARWIN_C_SOURCE 1
#include "support.h"
#include "diagnostics.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
static TestDisk disk;
static NgApp app,snapshot;
#ifdef NG_DIAGNOSTIC
static uint32_t ticks;
static uint32_t clock_ticks(void){return ticks++%11059200u;}
#endif
int main(void)
{
 char stack;ng_diag_reset((uintptr_t)&stack);
 disk.write_budget=-1;ng_app_init(&app,test_hooks(&disk),456);open_game(&app,21);
#ifdef NG_DIAGNOSTIC
 ng_diag_reset(1000);ng_diag_stack_range(800,1200);ng_diag_sample(1,850,0,0);
 assert(ng_diagnostics.stack_range_verified && ng_diagnostics.stack_base==1200 && ng_diagnostics.stack_low==850);
 ng_diag_sample(2,799,0,0);assert(ng_diagnostics.stack_low==850 && ng_diagnostics.stack_rejected==1);
 ng_diag_reset((uintptr_t)&stack);ng_diag_clock(clock_ticks);
 ng_diag_arena(0,1000,300,600,500,2,3);assert(ng_diagnostics.arena[0].overhead==100);
 ng_diag_emit(NGD_TIMER_START,0,0);ng_diag_clear_counters();
 assert(ng_diagnostics.timers==1 && ng_diagnostics.peak_timers==1 && ng_diagnostics.arena[0].peak_used==500);
 snapshot=app;unsigned writes=disk.saves;ng_diag_stress_start();
 for(unsigned i=0;i<37;i++)(void)ng_diag_stress_step();
 ng_diag_stress_cancel();assert(!ng_diag_stress_active() && ng_diagnostics.stress_done==37);
 ng_diag_stress_start();for(unsigned i=0;i<1000;i++)(void)ng_diag_stress_step();
 assert(!ng_diag_stress_active() && ng_diagnostics.stress_done==1000 && !ng_diagnostics.stress_failures);
 assert(!memcmp(&app,&snapshot,sizeof app) && disk.saves==writes);
 assert(ng_diagnostics.codec_peak<=NG_RECORD_MAX && ng_diagnostics.codec_peak>12000);
 assert(!ng_diagnostics.handles && ng_diagnostics.timers==1);
 for(unsigned i=0;i<NG_GAME_COUNT;i++)assert(ng_diagnostics.load[ng_visible_id(i)].count>0);
 for(unsigned i=1;i<=NG_ID_MAX;i++){
  ng_diag_ready(i,UINT32_MAX);ng_diagnostics.load[i].count=ng_diagnostics.ready[i].count=UINT32_MAX;
  ng_diagnostics.load[i].total=ng_diagnostics.load[i].max=UINT32_MAX;
 }
 /* Maximum-width export stays bounded, owns a single file, and survives repeat. */
 for(unsigned i=0;i<NG_DIAG_EVENTS;i++)ng_diag_emit(NGD_INPUT,UINT32_MAX,255);
 char folder[]="diagnostic-fixture-XXXXXX";assert(mkdtemp(folder));ng_storage_directory(folder);
 assert(ng_diag_export());char path[128];snprintf(path,sizeof path,"%s/NDDIAG.txt",folder);struct stat st;
 assert(stat(path,&st)==0 && st.st_size>1000 && st.st_size<=NG_DIAG_EXPORT_MAX);assert(ng_diag_export());
 assert(!ng_diagnostics.handles && ng_diagnostics.peak_handles==1);
 FILE *f=fopen(path,"r");assert(f);char text[NG_DIAG_EXPORT_MAX+1];size_t n=fread(text,1,NG_DIAG_EXPORT_MAX,f);text[n]=0;assert(!fclose(f));
 assert(strstr(text,"INCLUDED in _ostk") && strstr(text,"fixture_io=none") && strstr(text,"init_game=32") && strstr(text,"ready_game=32") && strstr(text,"stack_painting=none"));
 assert(!unlink(path) && !rmdir(folder));ng_storage_directory(".");
 /* App global actions cancel work before they checkpoint/leave. */
 app.screen=NG_SETTINGS;tap(&app,NGK_F3);tap(&app,NGK_F4);assert(ng_diag_stress_active());
 tap(&app,NGK_EXIT);assert(!ng_diag_stress_active());
 tap(&app,NGK_F3);tap(&app,NGK_F4);tap(&app,NGK_MENU);assert(!ng_diag_stress_active() && disk.menu==1);
 puts("Diagnostic: validated range samples, live-aware reset, 1000 RAM fixtures, no progress writes, bounded export, cancel and MENU PASS");
#else
 snapshot=app;ng_diag_stress_start();assert(!ng_diag_stress_active() && !ng_diag_stress_step());
 assert(!memcmp(&app,&snapshot,sizeof app));puts("Release: diagnostic-only stress absent, app state unchanged PASS");
#endif
 return 0;
}
