/* Compile the production adapter itself, including its actual loop and maps. */
#define main numgame_native_main
#include "../src/main.c"
#undef main
#include <gint/bfile.h>
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "legacy_wire.h"

#ifdef NG_DIAGNOSTIC
#define MOCK_PREFIX "ND"
#else
#define MOCK_PREFIX "NG"
#endif
#define MOCK_DAY 11059200u
#define MOCK_FDS 8
static uint16_t mock_vram_pixel;
uint16_t *gint_vram=&mock_vram_pixel;
static unsigned world_depth,world_calls,write_calls,close_calls,open_calls,read_calls;
static unsigned os_calls,off_calls,render_calls,clear_calls,timer_starts,timer_pauses;
static unsigned checkpoint_write_marker;
static int fail_after_create,created_open_failure;
static int close_failures,write_budget=-1,read_failure,zero_write,corrupt_on_close;
static uint32_t mock_ticks;
static bool timer_unavailable,rtc_unavailable,expect_save_failure;
static unsigned rtc_starts,rtc_stops;
static bool physical[256];
static keydev_t mock_device;
static gint_call_t timer_callback;
static keydev_transform_t transform;
typedef struct {bool exists;size_t size;uint8_t bytes[NG_ARCHIVE_BYTES];} MockFile;
typedef struct {bool open,writing;unsigned index;size_t position;} MockFD;
static MockFile files[65];
static MockFD descriptors[MOCK_FDS];
typedef struct {unsigned key,type;uint32_t advance;void (*check)(void);} Step;
static Step script[65536];
static unsigned script_count,script_at;
static jmp_buf exit_loop;

static unsigned path_index(const uint16_t *path){
 char name[32];unsigned n=0;do{assert(n+1<sizeof name);assert(path[n]<128);name[n]=(char)path[n];}while(name[n++]);
 if(!strcmp(name,"\\\\fls0\\" MOCK_PREFIX "ARCA.dat"))return 62;
 if(!strcmp(name,"\\\\fls0\\" MOCK_PREFIX "ARCB.dat"))return 63;
 if(!strcmp(name,"\\\\fls0\\" MOCK_PREFIX "DIAG.txt"))return 64;
 assert(strlen(name)==16);assert(!memcmp(name,"\\\\fls0\\" MOCK_PREFIX,9));assert(name[9]>='0'&&name[9]<='3');assert(name[10]>='0'&&name[10]<='9');assert(name[11]=='A'||name[11]=='B');assert(!strcmp(name+12,".dat"));
 unsigned id=(unsigned)(name[9]-'0')*10u+(unsigned)(name[10]-'0');assert(id<=30);return id*2u+(name[11]=='B');
}
static MockFD *fd(int handle){assert(world_depth==1);assert(handle>=1&&handle<=MOCK_FDS);MockFD *d=&descriptors[handle-1];assert(d->open);return d;}
int BFile_Remove(const uint16_t *path){assert(world_depth==1);MockFile *f=&files[path_index(path)];if(!f->exists)return BFile_EntryNotFound;f->exists=false;f->size=0;return 0;}
int BFile_Create(const uint16_t *path,int type,int *size){assert(world_depth==1);assert(type==BFile_File&&size&&(*size==0||*size==(int)NG_ARCHIVE_BYTES));MockFile *f=&files[path_index(path)];assert(!f->exists);f->exists=true;f->size=(size_t)*size;memset(f->bytes,0xa5,f->size);created_open_failure=fail_after_create;return 0;}
int BFile_Open(const uint16_t *path,int mode){assert(world_depth==1);open_calls++;unsigned index=path_index(path);if(!files[index].exists)return BFile_EntryNotFound;if(created_open_failure){created_open_failure=0;return -5;}for(unsigned i=0;i<MOCK_FDS;i++)if(!descriptors[i].open){descriptors[i]=(MockFD){true,mode!=BFile_ReadOnly,index,0};return (int)i+1;}return -5;}
int BFile_Close(int handle){MockFD *d=fd(handle);close_calls++;if(close_failures){if(close_failures>0)close_failures--;return -5;}if(d->writing&&corrupt_on_close&&files[d->index].size){files[d->index].bytes[d->position-1]^=1;corrupt_on_close=0;}d->open=false;return 0;}
int BFile_Seek(int handle,int offset){MockFD *d=fd(handle);assert(offset>=0);d->position=(size_t)offset;return offset;}
int BFile_Size(int handle){MockFD *d=fd(handle);return (int)files[d->index].size;}
int BFile_Write(int handle,const void *data,int size){MockFD *d=fd(handle);assert(d->writing);write_calls++;if(zero_write)return 0;if(write_budget==0)return -5;if(write_budget>0&&size>write_budget)size=write_budget;assert(size>=0&&d->position+(size_t)size<=NG_ARCHIVE_BYTES);memcpy(files[d->index].bytes+d->position,data,(size_t)size);d->position+=(size_t)size;if(d->position>files[d->index].size)files[d->index].size=d->position;if(write_budget>0)write_budget-=size;return size;}
int BFile_Read(int handle,void *data,int size,int offset){MockFD *d=fd(handle);read_calls++;if(read_failure)return -5;assert(offset>=0&&size>=0);/* Fugue would return requested length even past EOF. Adapter MUST clamp. */assert((size_t)offset+(size_t)size<=files[d->index].size);memcpy(data,files[d->index].bytes+offset,(size_t)size);return size;}
int gint_world_switch(gint_call_t call){assert(!world_depth);assert(call.argfn);world_calls++;world_depth++;int result=call.argfn(call.arg);world_depth--;return result;}
void gint_osmenu(void){assert(!world_depth && !timer_active && !rtc_active && !brightness_saved);if(expect_save_failure){assert(app.dirty);os_calls++;return;}assert(!app.dirty&&!app.settings_dirty);checkpoint_write_marker=write_calls;NgSession loaded;int rc=ng_storage_load(&loaded,app.session.game.id);assert(rc==NG_LOAD_OK||rc==NG_LOAD_RECOVERED);assert(!memcmp(&loaded.game,&app.session.game,sizeof(NgGame)));os_calls++;mock_ticks=(mock_ticks+128u*36000u)%MOCK_DAY;}
void gint_poweroff(bool show_message){assert(show_message&&!world_depth && !timer_active && !rtc_active && !brightness_saved);if(expect_save_failure){assert(app.dirty);off_calls++;return;}assert(!app.dirty&&!app.settings_dirty);off_calls++;mock_ticks=(mock_ticks+128u*3600u)%MOCK_DAY;}
static uint16_t brightness=120;
static unsigned dim_calls,restore_calls,hud_calls;
int ng_os_backlight_duration(void){return 2;}
int ng_os_apo_minutes(void){return 60;}
uint16_t r61524_get(int reg){assert(reg==0x5a1);return brightness;}
void r61524_set(int reg,uint16_t value){assert(reg==0x5a1);if(value<brightness)dim_calls++;else restore_calls++;brightness=value;}
void r61524_display_rect(uint16_t *vram,int xmin,int xmax,int ymin,int ymax){assert(vram==gint_vram && xmin==0 && xmax==395 && ymin==0 && ymax==23);hud_calls++;}
kmalloc_arena_t *kmalloc_get_arena(const char *name){assert(!strcmp(name,"_uram") || !strcmp(name,"_ostk"));return NULL;}
kmalloc_gint_stats_t *kmalloc_get_gint_stats(kmalloc_arena_t *arena){(void)arena;return NULL;}
void dsetvram(uint16_t *first,uint16_t *second){assert(first==gint_vram&&!second);}
void drect(int x1,int y1,int x2,int y2,int color){(void)color;assert(x1>=0&&x2<396&&x2>=x1&&y1>=0&&y2<224&&y2>=y1);}
void dupdate(void){render_calls++;}
void rtc_get_time(rtc_time_t *time){*time=(rtc_time_t){.year=2026,.month=8,.month_day=19};}
uint32_t rtc_ticks(void){return mock_ticks;}
int timer_configure(int timer,uint64_t delay,gint_call_t call){assert(timer==TIMER_ANY&&delay==250000&&call.noarg);timer_callback=call;return timer_unavailable?-1:0;}
bool rtc_periodic_enable(int frequency,gint_call_t call){assert(frequency==RTC_16Hz&&call.noarg);timer_callback=call;rtc_starts++;return !rtc_unavailable;}
void rtc_periodic_disable(void){rtc_stops++;}
void timer_start(int timer){assert(timer==0);timer_starts++;}
void timer_pause(int timer){assert(timer==0);timer_pauses++;}
keydev_t *keydev_std(void){return &mock_device;}
void keydev_set_transform(keydev_t *device,keydev_transform_t value){assert(device==&mock_device);transform=value;assert(transform.enabled==KEYDEV_TR_REPEATS);assert(transform.repeater(KEY_EXE,0,0)==-1);assert(transform.repeater(KEY_LEFT,0,0)==500000);assert(transform.repeater(KEY_LEFT,0,1)==125000);}
void clearevents(void){clear_calls++;/* Future physical events in script are not yet queued. */}
bool keydown(int key){return key>=0&&key<256&&physical[key];}
key_event_t keydev_read(keydev_t *device,bool wait,volatile int *timeout){
 assert(device==&mock_device);if(!wait)assert(app.session.game.cpu_pending || ng_diag_stress_active());if(timeout)assert(timer_callback.noarg()==TIMER_CONTINUE&&*timeout==1);
 if(script_at==script_count)longjmp(exit_loop,1);
 Step s=script[script_at++];if(s.check)s.check();mock_ticks=(mock_ticks+s.advance)%MOCK_DAY;
 if(s.type==KEYEV_NONE && s.advance)assert(timeout);
 if(s.type==KEYEV_DOWN)physical[s.key]=true;else if(s.type==KEYEV_UP)physical[s.key]=false;
 return (key_event_t){.type=s.type,.key=s.key};
}
static void add(unsigned key,unsigned type,uint32_t ticks,void (*check)(void)){assert(script_count<65536);script[script_count++]=(Step){key,type,ticks,check};}
static void tap(unsigned key){add(key,KEYEV_DOWN,0,NULL);add(key,KEYEV_UP,0,NULL);}
static void begin(unsigned category,unsigned game){tap(category);tap(game);tap(KEY_F6);}
static void reset(void){memset(files,0,sizeof files);memset(descriptors,0,sizeof descriptors);memset(physical,0,sizeof physical);close_failures=read_failure=zero_write=corrupt_on_close=fail_after_create=created_open_failure=0;write_budget=-1;script_count=script_at=0;os_calls=off_calls=timer_starts=timer_pauses=0;checkpoint_write_marker=write_calls;mock_ticks=1000;timer_unavailable=rtc_unavailable=expect_save_failure=false;rtc_starts=rtc_stops=0;dim_calls=restore_calls=hud_calls=0;brightness=120;}
static void run_script(void){if(!setjmp(exit_loop))(void)numgame_native_main();assert(script_at==script_count);assert(!world_depth);}
static void check_nim_play(void){assert(app.screen==NG_PLAY&&app.session.game.id==21&&!app.modal);}
static void check_cpu_done(void){assert(app.session.game.moves==2&&!app.session.game.cpu_pending);}
static void check_plain_acon(void){assert(off_calls==0);}
static void fail_writes(void){zero_write=1;expect_save_failure=true;}
static void check_failed_menu(void){assert(os_calls==1&&app.modal==NG_MODAL_SAVE_ERROR&&app.active&&app.dirty&&ng_valid(&app.session.game));}
static void test_failed_power_checkpoint(void){
 reset();begin(KEY_5,KEY_1);add(KEY_MENU,KEYEV_DOWN,0,fail_writes);add(KEY_MENU,KEYEV_UP,0,NULL);add(0,KEYEV_NONE,0,check_failed_menu);tap(KEY_SHIFT);tap(KEY_ACON);run_script();assert(off_calls==1&&app.modal==NG_MODAL_SAVE_ERROR&&app.dirty);assert(ng_valid(&app.session.game));
 zero_write=0;expect_save_failure=false;assert(ng_storage_save(&app.session));
 puts("Native main: failed checkpoint still reaches MENU/OFF once, preserves RAM and reports error PASS");
}
static uint32_t elapsed_before_os;
static void remember_elapsed(void){elapsed_before_os=app.session.game.elapsed_ms;}
static void check_os_time(void){assert(os_calls==1&&app.session.game.elapsed_ms==elapsed_before_os);}
static void test_native_keys(void){
 reset();begin(KEY_5,KEY_1);add(0,KEYEV_NONE,0,check_nim_play);tap(KEY_1);tap(KEY_EXE);add(0,KEYEV_NONE,0,NULL);add(0,KEYEV_NONE,0,check_cpu_done);
 add(0,KEYEV_NONE,128,remember_elapsed);add(0,KEYEV_NONE,0,remember_elapsed);tap(KEY_MENU);add(0,KEYEV_NONE,0,check_os_time);
 tap(KEY_ACON);add(0,KEYEV_NONE,0,check_plain_acon);tap(KEY_SHIFT);tap(KEY_2);tap(KEY_ACON);add(0,KEYEV_NONE,0,check_plain_acon);tap(KEY_ALPHA);tap(KEY_SHIFT);tap(KEY_ACON);add(0,KEYEV_NONE,0,check_plain_acon);tap(KEY_SHIFT);tap(KEY_ACON);run_script();assert(os_calls==1&&off_calls==1);assert(clear_calls&&render_calls);
 puts("Native main: real key mapping, CPU event, checkpoint-before-MENU/OFF, plain AC/ON ignored, OS elapsed exclusion PASS");
}
static void legacy_memory(void){ng_new(&app.session.game,30,1,0,123,1);app.selected_id=30;app.session.undo_count=0;}
static void legacy_rush(void){ng_new(&app.session.game,29,1,0,123,1);app.selected_id=29;app.session.undo_count=0;}
static void begin_legacy(unsigned id){begin(KEY_5,KEY_1);add(0,KEYEV_NONE,0,id==30?legacy_memory:legacy_rush);}
static int32_t exposure_left;static uint32_t exposure_elapsed;
static void remember_exposure(void){assert(app.session.game.id==30&&app.session.game.phase==0);exposure_left=app.session.game.data[2];exposure_elapsed=app.session.game.elapsed_ms;}
static void check_paused(void){assert(app.modal==NG_MODAL_PAUSE&&app.session.game.data[2]==exposure_left&&app.session.game.elapsed_ms==exposure_elapsed);}
static void check_memory_input(void){assert(app.session.game.phase==2&&!app.session.game.input[0]);}
static void test_native_memory(void){
 reset();begin_legacy(30);add(0,KEYEV_NONE,128,NULL);add(0,KEYEV_NONE,0,remember_exposure);tap(KEY_F4);add(0,KEYEV_NONE,128000,NULL);add(0,KEYEV_NONE,0,check_paused);tap(KEY_EXE);
 add(KEY_7,KEYEV_DOWN,0,NULL);add(0,KEYEV_NONE,400,NULL);add(0,KEYEV_NONE,64,NULL);add(KEY_7,KEYEV_HOLD,0,NULL);add(KEY_7,KEYEV_UP,0,NULL);add(0,KEYEV_NONE,0,check_memory_input);run_script();assert(timer_starts==1&&timer_pauses==0);
 puts("Native main: 128Hz active clock, timed wakeup, pause/resume, exposure+conceal HOLD barrier PASS");
}
static void check_rush_wrap(void){assert(app.session.game.id==29&&app.session.game.elapsed_ms==2000&&app.session.game.data[9]==58000);}
static void check_long_wait(void){assert(app.session.game.id==21&&app.session.game.elapsed_ms==3000000);}
static void test_native_clock(void){
 reset();mock_ticks=MOCK_DAY-100;begin_legacy(29);add(0,KEYEV_NONE,256,NULL);add(0,KEYEV_NONE,0,check_rush_wrap);run_script();
 reset();begin(KEY_5,KEY_1);add(0,KEYEV_NONE,384000,NULL);add(0,KEYEV_NONE,0,check_long_wait);run_script();
 puts("Native main: midnight RTC wrap and 50-minute active gap PASS");
}
static void check_timer_unavailable(void){assert(app.screen==NG_ENTRY&&!app.modal&&app.active);assert(strstr(app.notice,"Timer unavailable"));assert(!app.dirty);}
static void test_timer_fallback(void){
 reset();timer_unavailable=true;begin_legacy(29);add(0,KEYEV_NONE,256,NULL);add(0,KEYEV_NONE,0,check_rush_wrap);tap(KEY_F4);run_script();assert(rtc_starts==1&&rtc_stops==0&&!timer_starts);
 reset();timer_unavailable=rtc_unavailable=true;begin(KEY_5,KEY_1);add(0,KEYEV_NONE,0,check_timer_unavailable);run_script();assert(rtc_starts==1);
 puts("Native main: RTC fallback and dual-unavailable safe checkpoint/refusal PASS");
}
static void fixture_2048(void){assert(app.session.game.id==26);memset(app.session.game.board,0,sizeof app.session.game.board);app.session.game.board[3]=1;app.session.game.board[7]=2;app.session.game.data[0]=2;}
static uint32_t held_rng;
static void remember_move(void){assert(app.session.game.moves==1);held_rng=app.session.game.rng;}
static void check_held_move(void){assert(app.session.game.moves==1&&app.session.game.rng==held_rng);}
static void test_native_hold(void){
 reset();begin(KEY_6,KEY_1);add(KEY_LEFT,KEYEV_DOWN,0,fixture_2048);add(KEY_LEFT,KEYEV_HOLD,64,remember_move);add(KEY_LEFT,KEYEV_UP,0,check_held_move);run_script();
 puts("Native main: 2048 physical HOLD cannot repeat a turn PASS");
}
static void check_idle_dim(void){assert(app.session.game.elapsed_ms==60000 && runtime.idle_ms==60000 && runtime.dimmed && brightness==20 && dim_calls==1 && hud_calls>=1);}
static void check_wake_once(void){assert(brightness==120 && restore_calls==1 && app.session.game.moves<=1 && runtime.idle_ms==0);}
static void check_apo_pending(void){assert(off_calls==1&&app.session.game.cpu_pending&&app.session.game.moves==1);}
static void test_common_idle(void){
 NgRuntime r;ng_runtime_init(&r,NG_RTC_DAY-64,-1,-1);assert(!r.os_settings && r.backlight_ms==60000 && r.apo_ms==600000);
 assert(ng_runtime_elapsed(&r,64)==1000);for(unsigned i=0;i<59;i++)assert(!ng_runtime_idle(&r,1000,false));assert(ng_runtime_idle(&r,1000,false)==NG_POWER_DIM);assert(ng_runtime_idle(&r,1,true)==NG_POWER_RESTORE && !r.idle_ms);

 reset();begin(KEY_6,KEY_1);add(0,KEYEV_NONE,128u*60u,NULL);add(KEY_LEFT,KEYEV_DOWN,0,check_idle_dim);add(KEY_LEFT,KEYEV_UP,0,check_wake_once);run_script();
 assert(app.session.game.elapsed_ms==60000 && ng_diagnostics.peak_timers==1);
 reset();begin(KEY_5,KEY_1);add(0,KEYEV_NONE,128u*3599u,NULL);tap(KEY_1);add(0,KEYEV_NONE,128u*2u,NULL);run_script();assert(off_calls==0 && runtime.idle_ms==2000);
 reset();begin(KEY_5,KEY_1);add(0,KEYEV_NONE,128u*3600u,NULL);run_script();assert(off_calls==1 && app.session.game.elapsed_ms==3600000 && runtime.idle_ms==0 && brightness==120);
 reset();begin(KEY_5,KEY_1);add(KEY_1,KEYEV_DOWN,0,NULL);add(KEY_1,KEYEV_UP,128u*3600u,NULL);run_script();assert(off_calls==1&&runtime.idle_ms==0);
 reset();begin(KEY_5,KEY_1);tap(KEY_1);tap(KEY_EXE);add(0,KEYEV_NONE,128u*3600u,NULL);add(0,KEYEV_NONE,0,check_apo_pending);add(0,KEYEV_NONE,0,check_cpu_done);run_script();assert(off_calls==1&&app.session.game.moves==2&&!app.session.game.cpu_pending);
 puts("Native common clock: idle HUD, real PWM dim/restore API, one wake key, pre-APO input, checkpointed APO PASS");
}
static void test_switch_stress(void){
 reset();
 for(unsigned i=0;i<1000;i++){
  unsigned index=i%NG_GAME_COUNT;begin(native_keys[index/5+1],native_keys[index%5+1]);
  if(i>=NG_GAME_COUNT)tap(KEY_EXE);
  add(0,KEYEV_NONE,32,NULL);tap(KEY_F5);tap(KEY_EXIT);tap(KEY_EXIT);
  tap(KEY_F4);tap(KEY_EXIT);tap(KEY_EXIT);tap(KEY_EXIT);tap(KEY_MENU);
 }
 run_script();assert(os_calls==1000 && ng_diagnostics.menu_requests==1000 && ng_diagnostics.menu_entries==1000 && ng_diagnostics.menu_returns==1000);
 assert(ng_diagnostics.peak_timers==1 && ng_diagnostics.timers==1 && !ng_diagnostics.handles && ng_diagnostics.peak_handles==1);
 unsigned count=0;for(unsigned i=0;i<65;i++)if(files[i].exists)count++;assert(count<=2);
 for(unsigned i=0;i<MOCK_FDS;i++)assert(!descriptors[i].open);
 unsigned sequence=ng_diagnostics.sequence;assert(ng_diag_export()&&files[64].exists&&files[64].size<NG_RECORD_MAX);assert(ng_diagnostics.sequence==sequence&&!ng_diagnostics.handles);size_t exported=files[64].size;assert(ng_diag_export()&&files[64].size==exported);
 printf("Native stress:1000 switches/menus, zero residual handles, one timer, %u fixed files; RAM ring bounded PASS\n",count);
}

static NgSession session_new(unsigned id){NgSession s;memset(&s,0,sizeof s);ng_new(&s.game,id,1,0,123,1);s.stats.started=1;return s;}
static void test_native_storage(void){
 reset();NgSession s=session_new(26),loaded;assert(ng_storage_save(&s)&&s.generation==1);assert(files[62].exists&&!files[63].exists);s.game.elapsed_ms=17;assert(ng_storage_save(&s)&&s.generation==2);assert(files[63].exists);assert(ng_storage_load(&loaded,26)==NG_LOAD_OK);assert(loaded.game.elapsed_ms==17);
 files[63].bytes[NG_ARCHIVE_HEADER+26*NG_RECORD_MAX+40]^=1;assert(ng_storage_load(&loaded,26)==NG_LOAD_RECOVERED);assert(!loaded.game.elapsed_ms);
 /* Partial replacement never destroys the other valid slot. */
 write_budget=100;s.game.elapsed_ms=29;assert(!ng_storage_save(&s));write_budget=-1;assert(ng_storage_load(&loaded,26)==NG_LOAD_RECOVERED);assert(!loaded.game.elapsed_ms);assert(ng_storage_save(&s));
 /* Readback must check the bytes actually accepted by Fugue. */
 unsigned generation=s.generation;corrupt_on_close=1;s.game.elapsed_ms=31;assert(!ng_storage_save(&s));assert(s.generation==generation);assert(ng_storage_load(&loaded,26)==NG_LOAD_RECOVERED);assert(loaded.game.elapsed_ms==29);
 zero_write=1;unsigned writes=write_calls;assert(!ng_storage_save(&s));assert(write_calls-writes<=1);zero_write=0;
 read_failure=1;assert(ng_storage_load(&loaded,26)==NG_LOAD_IO_ERROR);read_failure=0;
 /* Failed close is bounded, descriptor retained, and retried later. */
 close_failures=-1;unsigned closes=close_calls;assert(!ng_storage_save(&s));assert(close_calls-closes<=4);close_failures=0;int rc=ng_storage_load(&loaded,26);assert(rc==NG_LOAD_OK||rc==NG_LOAD_RECOVERED);for(unsigned i=0;i<MOCK_FDS;i++)assert(!descriptors[i].open);
 assert(ng_storage_save(&s));NgSettings settings={.last_game=26,.show_time=1};assert(ng_settings_save(&settings));NgSettings recovered={0};assert(ng_settings_load(&recovered)==NG_LOAD_OK);assert(recovered.last_game==26&&recovered.show_time);
 assert(ng_storage_load(&loaded,0)==NG_LOAD_INVALID);assert(ng_storage_load(&loaded,33)==NG_LOAD_INVALID);
 printf("Native Fugue adapter: namespace, two slots, exact EOF clamp, partial/zero writes, readback corruption, failed-close recovery PASS (%u world switches)\n",world_calls);
}



static void mock_put32(uint8_t *p,uint32_t value);
/* Version1 fixtures serialize the old three-level wire order, not struct memory. */
static void legacy_fixture(const NgSession *source,unsigned slot)
{
 uint8_t current[NG_RECORD_MAX];size_t length=ng_encode(source,current,sizeof current);assert(length>=2057);
 MockFile *f=&files[2*source->game.id+slot];f->exists=true;uint8_t *p=f->bytes+32;
 f->size=32+legacy_wire(p,current,length,3);
 memcpy(f->bytes,"NGSAVE01",8);mock_put32(f->bytes+8,source->game.id);mock_put32(f->bytes+12,1);mock_put32(f->bytes+16,slot+1);mock_put32(f->bytes+20,(uint32_t)f->size-32);mock_put32(f->bytes+24,ng_crc32(p,f->size-32));mock_put32(f->bytes+28,ng_crc32(f->bytes,28));
}
static void mock_put32(uint8_t *p,uint32_t value);
static void test_archive_migration(void)
{
 NgSession source=session_new(26),workspace,loaded;
 reset();source.game.elapsed_ms=17;legacy_fixture(&source,0);source.game.elapsed_ms=99;legacy_fixture(&source,1);
 assert(ng_storage_migrate(&workspace));assert(!files[52].exists&&!files[53].exists&&files[62].exists&&files[63].exists);
 assert(ng_storage_load(&loaded,26)==NG_LOAD_OK&&loaded.game.elapsed_ms==99);
 assert(loaded.stats.started==1&&!loaded.stats.best[0][3][0].completed);
 /* Restart/migration is idempotent, and disagreeing old/new progress is retained. */
 assert(ng_storage_migrate(&workspace));source.game.elapsed_ms=111;legacy_fixture(&source,1);
 assert(!ng_storage_migrate(&workspace)&&files[53].exists);assert(ng_storage_load(&loaded,26)==NG_LOAD_OK&&loaded.game.elapsed_ms==99);
 /* Retired IDs remain decodable and are never reinterpreted as new boards. */
 reset();for(unsigned id=29;id<=30;id++){NgSession retired=session_new(id);legacy_fixture(&retired,0);}
 MockFile *preferences=&files[0];preferences->exists=true;preferences->size=95;uint8_t *p=preferences->bytes;
 memcpy(p,"NGSAVE01",8);mock_put32(p+8,0);mock_put32(p+12,1);mock_put32(p+16,1);mock_put32(p+20,63);p[32]=30;memset(p+33,1,30);p[32+31+28]=1;p[94]=1;mock_put32(p+24,ng_crc32(p+32,63));mock_put32(p+28,ng_crc32(p,28));
 assert(ng_storage_migrate(&workspace));NgSettings options;assert(ng_settings_load(&options)==NG_LOAD_OK&&options.last_game==30&&options.mode[28]==1&&options.difficulty[31]==1);
 for(unsigned id=29;id<=30;id++){assert(ng_storage_load(&loaded,id)==NG_LOAD_OK&&loaded.game.id==id);assert(!files[id*2].exists);}
 assert(ng_storage_load(&loaded,31)==NG_LOAD_ABSENT&&ng_storage_load(&loaded,32)==NG_LOAD_ABSENT);
 /* A corrupt/foreign filename is never ownership permission to delete it. */
 reset();legacy_fixture(&source,0);files[52].bytes[0]='X';assert(ng_storage_migrate(&workspace)&&files[52].exists&&!files[62].exists);
 reset();legacy_fixture(&source,0);files[62].exists=true;files[62].size=NG_ARCHIVE_BYTES;memset(files[62].bytes,0x53,NG_ARCHIVE_BYTES);
 assert(!ng_storage_migrate(&workspace)&&files[52].exists&&files[62].bytes[0]==0x53);
 assert(ng_storage_load(&loaded,26)==NG_LOAD_OK&&loaded.game.elapsed_ms==111);
 const int cuts[]={0,1,15,31,32,64,100,1087,1088,1100,1119,1120,1250,5000,6000,9000};
 for(unsigned i=0;i<sizeof cuts/sizeof cuts[0];i++){
  reset();legacy_fixture(&source,0);write_budget=cuts[i];bool ok=ng_storage_migrate(&workspace);write_budget=-1;
  if(!ok)assert(files[52].exists);
  assert(ng_storage_load(&loaded,26)<=NG_LOAD_RECOVERED&&loaded.game.elapsed_ms==111);
  assert(ng_storage_migrate(&workspace));assert(!files[52].exists);
  assert(ng_storage_load(&loaded,26)==NG_LOAD_OK&&loaded.game.elapsed_ms==111);
 }
 reset();legacy_fixture(&source,0);fail_after_create=1;assert(!ng_storage_migrate(&workspace)&&files[52].exists&&!files[62].exists);fail_after_create=0;assert(ng_storage_migrate(&workspace));
 /* Complete initialization ownership marker survives a process interruption. */
 reset();files[62].exists=true;files[62].size=NG_ARCHIVE_BYTES;uint8_t *h=files[62].bytes;
 memcpy(h,"NGINIT02",8);mock_put32(h+8,2);mock_put32(h+12,33);mock_put32(h+16,NG_RECORD_MAX);mock_put32(h+20,NG_ARCHIVE_BYTES);mock_put32(h+28,ng_crc32(h,28));
 assert(ng_storage_save(&source));assert(!memcmp(h,"NGARCH02",8));assert(ng_storage_load(&loaded,26)==NG_LOAD_OK);
 /* Corrupting one record leaves unrelated games readable. */
 NgSession other=session_new(21);assert(ng_storage_save(&other));h[NG_ARCHIVE_HEADER+26*NG_RECORD_MAX+40]^=1;
 assert(ng_storage_load(&loaded,26)==NG_LOAD_INVALID);assert(ng_storage_load(&loaded,21)==NG_LOAD_OK);
 puts("Archive: v1 latest-slot migration, independent workspace, two-copy verification, conflict/foreign preservation, 16 interrupted-write retries, init recovery, per-record isolation PASS");
}
static void block_shift(void){app.blocked|=UINT64_C(1)<<ng_key_index(NGK_SHIFT);app.shift_pending=false;}
static void test_global_phase_boundary(void)
{
 reset();begin_legacy(30);add(KEY_MENU,KEYEV_DOWN,1280,NULL);add(KEY_MENU,KEYEV_UP,0,NULL);run_script();assert(os_calls==1);
 reset();begin_legacy(30);tap(KEY_SHIFT);add(KEY_ACON,KEYEV_DOWN,1280,NULL);add(KEY_ACON,KEYEV_UP,0,NULL);run_script();assert(off_calls==1);
 reset();begin_legacy(30);add(KEY_SHIFT,KEYEV_DOWN,0,NULL);add(KEY_ACON,KEYEV_DOWN,1280,block_shift);add(KEY_ACON,KEYEV_UP,0,NULL);run_script();assert(off_calls==0);
 reset();timer_unavailable=true;begin(KEY_5,KEY_1);for(unsigned i=0;i<160;i++)add(0,KEYEV_NONE,8,NULL);run_script();assert(runtime.idle_ms==10000&&app.session.game.elapsed_ms==10000);
 puts("Native boundaries: MENU/OFF survive phase barrier; 160 fractional RTC idle wakeups equal exactly 10 seconds PASS");
}

static void check_equation_no_dot(void){assert(app.session.game.id==2&&!strcmp(app.session.game.input,"12+7"));}
static void check_equation_equal(void){assert(!strcmp(app.session.game.input,"12+7=")&&!app.shift_pending);}
static void check_equation_complete(void){assert(!strcmp(app.session.game.input,"12+7=19")&&!app.shift_pending);}
static void check_no_equal(void){assert(!strchr(app.session.game.input,'=')&&!app.shift_pending&&!app.alpha_pending);}
static unsigned global_stage;
static void check_global_stage(void)
{
 static const unsigned screens[]={NG_PLAY,NG_PLAY,NG_PLAY,NG_ENTRY,NG_ENTRY,NG_ENTRY,NG_STATS,NG_SETTINGS,NG_SETTINGS,NG_MAIN};
 static const unsigned modals[]={0,NG_MODAL_RULES,NG_MODAL_INIT,0,NG_MODAL_RECORDS,NG_MODAL_NEW,0,0,NG_MODAL_DIAGNOSTICS,0};
 assert(global_stage<10);assert(app.screen==screens[global_stage]&&app.modal==modals[global_stage]);global_stage++;
}
static void checked_menu(void){add(KEY_MENU,KEYEV_DOWN,0,check_global_stage);add(KEY_MENU,KEYEV_UP,0,NULL);}
static void test_global_screens(void)
{
 global_stage=0;reset();begin(KEY_5,KEY_1);checked_menu(); /* playing */
 tap(KEY_F5);checked_menu();tap(KEY_EXIT); /* rules */
 add(0,KEYEV_NONE,32,NULL);tap(KEY_F1);checked_menu();tap(KEY_EXIT); /* init confirmation */
 tap(KEY_EXIT);checked_menu(); /* entry */
 tap(KEY_F4);checked_menu();tap(KEY_EXIT); /* records */
 tap(KEY_F6);checked_menu();tap(KEY_EXIT); /* new confirmation */
 tap(KEY_EXIT);add(0,KEYEV_NONE,32,NULL);tap(KEY_F1);checked_menu();tap(KEY_EXIT); /* category stats */
 tap(KEY_EXIT);tap(KEY_F2);checked_menu();tap(KEY_F3);checked_menu(); /* settings + diagnostics */
 tap(KEY_SHIFT);tap(KEY_ACON);tap(KEY_EXIT);tap(KEY_EXIT);checked_menu(); /* main */
 run_script();assert(os_calls==10&&off_calls==1&&!ng_diagnostics.handles&&ng_diagnostics.peak_timers==1);
 puts("Native global dispatch: PLAY/RULES/INIT/ENTRY/RECORDS/NEW/STATS/SETTINGS/DIAGNOSTICS/MAIN MENU and modal OFF PASS");
}
static void test_physical_equals(void)
{
 reset();begin(KEY_1,KEY_2);tap(KEY_1);tap(KEY_2);tap(KEY_ADD);tap(KEY_7);tap(KEY_DOT);add(0,KEYEV_NONE,0,check_equation_no_dot);
 tap(KEY_SHIFT);add(KEY_DOT,KEYEV_DOWN,0,NULL);add(KEY_DOT,KEYEV_HOLD,0,check_equation_equal);add(KEY_DOT,KEYEV_UP,0,check_equation_equal);
 tap(KEY_1);tap(KEY_9);tap(KEY_FD);add(0,KEYEV_NONE,0,check_equation_complete);run_script();
 reset();begin(KEY_1,KEY_2);tap(KEY_1);tap(KEY_2);tap(KEY_ADD);tap(KEY_7);add(KEY_SHIFT,KEYEV_DOWN,0,NULL);tap(KEY_DOT);add(KEY_SHIFT,KEYEV_UP,0,check_equation_equal);run_script();
 reset();begin(KEY_1,KEY_2);tap(KEY_SHIFT);tap(KEY_ALPHA);tap(KEY_DOT);add(0,KEYEV_NONE,0,check_no_equal);run_script();
 reset();begin(KEY_2,KEY_1);tap(KEY_SHIFT);tap(KEY_DOT);add(0,KEYEV_NONE,0,check_no_equal);run_script();
 puts("Native SHIFT+DOT: latched/held modifiers, exactly one equal, DOT grammar unchanged, consumed modifier, alpha conflict, removed FD alias PASS");
}

static bool fail_settings_save(void *context,NgSettings *settings){(void)context;(void)settings;return false;}
static void direct_press(NgApp *state,int key){ng_app_event(state,key,NG_DOWN);ng_app_event(state,key,NG_UP);}
static void test_review_transitions(void){
 NgApp state;ng_app_init(&state,(NgHooks){0},9);state.session=session_new(25);ng_new(&state.session.game,25,0,2,123,1);state.session.game.board[0]=20;state.screen=NG_PLAY;state.selected_id=25;state.active=true;
 direct_press(&state,'1');direct_press(&state,NGK_EXE);assert(state.modal==NG_MODAL_RESULT);direct_press(&state,NGK_F5);assert(state.modal==NG_MODAL_RULES);direct_press(&state,NGK_EXIT);assert(state.modal==NG_MODAL_RESULT);
 ng_app_init(&state,(NgHooks){.save_settings=fail_settings_save},9);state.session=session_new(21);state.screen=NG_MAIN;state.selected_id=21;state.active=true;state.settings.last_game=26;state.summary[25].exists=1;state.settings_dirty=true;
 NgGame preserved=state.session.game;direct_press(&state,NGK_F3);assert(state.modal==NG_MODAL_SAVE_ERROR&&state.screen==NG_MAIN&&state.selected_id==21);assert(!memcmp(&preserved,&state.session.game,sizeof preserved));
 puts("Review regressions: RESULT/RULES return and failed-checkpoint Main RESUME preserve state/error PASS");
}


static void assert_bad_stats(NgSession *source){uint8_t payload[NG_RECORD_MAX];NgSession decoded;size_t length=ng_encode(source,payload,sizeof payload);assert(length);assert(!ng_decode(&decoded,payload,length,source->game.id));}
static void mock_put32(uint8_t *p,uint32_t value){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(value>>(8*i));}
static void test_malformed_statistics(void){
 NgSession source=session_new(26);NgBest *b=&source.stats.best[0][1][0];b->completed=1;b->losses=1;
 const uint32_t invalid[]={31,255,UINT32_MAX};for(unsigned i=0;i<3;i++){b->best_aux=invalid[i];assert_bad_stats(&source);}b->best_aux=15;
 b->wins=1;assert_bad_stats(&source);b->wins=0;
 source.stats.best[0][0][0].completed=1;source.stats.best[0][0][0].losses=1;assert_bad_stats(&source);
 source=session_new(26);source.stats.best[0][1][0].best_score=1;assert_bad_stats(&source);
 source=session_new(25);source.stats.best[0][1][0].completed=source.stats.best[0][1][0].losses=1;source.stats.best[0][1][0].best_aux=1;assert_bad_stats(&source);
 /* Same invalid field through the real native slot parser, with valid CRCs. */
 reset();source=session_new(26);assert(ng_storage_save(&source));uint8_t *record=files[62].bytes+NG_ARCHIVE_HEADER+26*NG_RECORD_MAX;
 unsigned best_offset=32+1+8+2*32; /* header, undo-count, stats header, mode0/level1/normal */
 mock_put32(record+best_offset,1);mock_put32(record+best_offset+8,1);mock_put32(record+best_offset+28,255);
 mock_put32(record+24,ng_crc32(record+32,(size_t)record[20]|((size_t)record[21]<<8)));mock_put32(record+28,ng_crc32(record,28));NgSession loaded;assert(ng_storage_load(&loaded,26)==NG_LOAD_INVALID);
 source=session_new(25);source.undo_count=1;source.undo[0]=source.game;source.undo[0].board[0]=30;const NgModule *module=ng_module(25);strcpy(source.undo[0].input,"1");assert(module->action(&source.undo[0],NGK_EXE));assert(source.undo[0].status==NG_WON);assert_bad_stats(&source);
 source=session_new(21);source.undo_count=1;source.undo[0]=source.game;source.undo[0].board[0]--;source.undo[0].moves=1;source.undo[0].turn=1;source.undo[0].cpu_pending=1;assert(ng_valid(&source.undo[0]));assert_bad_stats(&source);
 puts("Review regressions: malformed stats/tile exponent, valid-CRC malicious slot, terminal/CPU undo rejected PASS");
}

static void reproduce_malformed_stats(void){
 reset();NgSession source=session_new(26),decoded;source.stats.best[0][1][0].completed=1;source.stats.best[0][1][0].losses=1;source.stats.best[0][1][0].best_aux=255;
 uint8_t payload[NG_RECORD_MAX];size_t length=ng_encode(&source,payload,sizeof payload);assert(length);
 if(!ng_decode(&decoded,payload,length,26)){puts("Malformed 2048 best_aux=255 rejected by decoder PASS");return;}
 puts("BUG REPRODUCED: decoder accepted best_aux=255; invoking actual record renderer under UBSan");
 memset(&app,0,sizeof app);app.session=decoded;app.selected_id=26;app.screen=NG_ENTRY;app.modal=NG_MODAL_RECORDS;app.record_difficulty=1;
 draw();
}

int main(void){if(getenv("NG_NATIVE_REPRO_STATS")){setvbuf(stdout,NULL,_IONBF,0);reproduce_malformed_stats();return 0;}setvbuf(stdout,NULL,_IONBF,0);test_native_storage();test_global_screens();test_physical_equals();test_archive_migration();test_global_phase_boundary();test_native_keys();test_failed_power_checkpoint();test_native_memory();test_native_hold();test_native_clock();test_timer_fallback();test_common_idle();test_switch_stress();test_review_transitions();test_malformed_statistics();return 0;}
