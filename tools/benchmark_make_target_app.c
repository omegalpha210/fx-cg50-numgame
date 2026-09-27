/* Same-host before/after app-path sample; no device BFile or flash timings. */
#define _POSIX_C_SOURCE 200809L
#include "app.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {NgSettings settings;NgSession session;bool active,valid;unsigned saves,menus;} MemoryDisk;
static MemoryDisk disk;
static NgApp app,cold;
static double samples[256];
static volatile unsigned observed;
static double now(void)
{struct timespec t;assert(!clock_gettime(CLOCK_MONOTONIC,&t));return (double)t.tv_sec+(double)t.tv_nsec/1e9;}
static int load_state(void *context,NgSettings *settings,NgSession *session,bool *active)
{
 MemoryDisk *d=context;
 if(!d->valid)return NG_LOAD_ABSENT;
 *settings=d->settings;*session=d->session;*active=d->active;return NG_LOAD_OK;
}
static bool save_state(void *context,NgSettings *settings,const NgSession *session,bool active)
{
 MemoryDisk *d=context;
 d->settings=*settings;d->session=*session;d->active=active;d->valid=true;d->saves++;return true;
}
static void menu(void *context){((MemoryDisk *)context)->menus++;}
static NgHooks hooks(void)
{return (NgHooks){.context=&disk,.load_state=load_state,.save_state=save_state,.os_menu=menu};}
static void press(NgApp *a,int key)
{(void)ng_app_event(a,key,NG_DOWN);(void)ng_app_event(a,key,NG_UP);}
static void prepare(unsigned seed)
{
 memset(&disk,0,sizeof disk);ng_app_init(&app,hooks(),seed);
 app.settings.first_help=0;app.settings.target=200;app.settings.difficulty[5]=NG_HARD;
 app.selected_id=6;app.screen=NG_ENTRY;
 app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_NEW);
}
static void start(void)
{press(&app,NGK_F6);assert(app.screen==NG_PLAY&&!app.modal&&ng_valid(&app.session.game));observed+=app.session.game.puzzle_id;}
static void next(void)
{
 press(&app,NGK_EXIT);assert(app.screen==NG_ENTRY);
 app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_NEW);
 press(&app,NGK_F6);assert(app.screen==NG_PLAY&&!app.modal&&ng_valid(&app.session.game));
 observed+=app.session.game.puzzle_id;
}
static int cmp(const void *a,const void *b)
{double x=*(const double *)a,y=*(const double *)b;return (x>y)-(x<y);}
static void report(const char *label,unsigned n)
{
 qsort(samples,n,sizeof samples[0],cmp);
 printf("%s host ms p50=%.6f p95=%.6f n=%u\n",label,
        samples[(n-1)/2]*1000,samples[(n*95+99)/100-1]*1000,n);
}
int main(void)
{
 for(unsigned i=0;i<128;i++){
  prepare(55123+i);
  double t=now();start();samples[i]=now()-t;
 }
 report("entry_start_save",128);
 prepare(8123);start();
 for(unsigned i=0;i<128;i++){
  double t=now();next();samples[i]=now()-t;
 }
 report("repeated_new_save",128);
 for(unsigned i=0;i<128;i++){
  next();double t=now();press(&app,NGK_HINT);samples[i]=now()-t;
  assert(app.session.game.message[0]);
 }
 report("hint_app",128);
 for(unsigned i=0;i<128;i++){
  next();double t=now();press(&app,NGK_ANSWER);samples[i]=now()-t;
  assert(app.session.game.input[0]);
 }
 report("answer_app",128);
 for(unsigned i=0;i<128;i++){
  next();double t=now();assert(ng_valid(&app.session.game));samples[i]=now()-t;
 }
 report("active_validation",128);
 for(unsigned i=0;i<128;i++){
  double t=now();ng_app_init(&cold,hooks(),90123+i);press(&cold,NGK_F1);
  samples[i]=now()-t;
  assert(cold.screen==NG_PLAY&&ng_valid(&cold.session.game));
  observed+=cold.session.game.puzzle_id;
 }
 report("cold_resume_memory_disk",128);
 for(unsigned i=0;i<128;i++){
  double t=now();press(&app,NGK_MENU);samples[i]=now()-t;
  observed+=disk.menus;
 }
 report("menu_memory_disk",128);
 printf("saves=%u menus=%u observed=%u\n",disk.saves,disk.menus,observed);
 return 0;
}
