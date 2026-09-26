#include "app.h"
#include "diagnostics.h"
#include "guesscalc.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
static uint32_t satadd(uint32_t a,uint32_t b){return b>UINT32_MAX-a?UINT32_MAX:a+b;}
static uint32_t advance_seed(uint32_t seed)
{seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed?seed:1;}
static const uint16_t target_choices[]={10,24,50,100,200,NG_TARGET_RANDOM};
static int target_choice(unsigned value)
{for(unsigned i=0;i<sizeof target_choices/sizeof target_choices[0];i++)if(target_choices[i]==value)return (int)i;return -1;}
static unsigned target_request(const NgGame *g)
{return g->pack_revision>=6 && g->data[2]==1?NG_TARGET_RANDOM:g->pack_revision>=6?(unsigned)g->data[4]:(unsigned)g->data[0];}
static void target_normalize(NgApp *a)
{if(target_choice(a->settings.target)<0){a->settings.target=24;a->settings_dirty=true;}}
static uint32_t puzzle_fingerprint(const NgGame *g)
{
 uint32_t board=ng_crc32(g->board,sizeof g->board);
 uint32_t fixed=ng_crc32(g->fixed,sizeof g->fixed);
 uint32_t data=ng_crc32(g->data,sizeof g->data);
 return board^(fixed<<1 | fixed>>31)^(data<<7 | data>>25)^
  ((uint32_t)g->rows<<24)^((uint32_t)g->cols<<16);
}
static void barrier(NgApp *a){a->epoch++;ng_app_barrier(a);}
void ng_app_barrier(NgApp *a){a->blocked|=a->held;a->shift_pending=false;a->alpha_pending=false;}
static void modal(NgApp *a,unsigned which){a->modal=(uint8_t)which;a->rules_scroll=0;barrier(a);}
int ng_key_index(int key)
{
 if(key>='0' && key<='9')return key-'0';
 const char *symbols="+-*/()=^.";const char *p=strchr(symbols,key);
 if(key>0 && key<128 && p)return 10+(int)(p-symbols);
 if(key>=NGK_UP && key<=NGK_ACON)return 19+key-NGK_UP;
 return -1;
}
static void resume_setting(NgSettings *s,unsigned id)
{
 (void)id;s->recent_count=0;memset(s->recent,0,sizeof s->recent);
 s->pending_delete=0;
}
void ng_app_init(NgApp *a,NgHooks hooks,uint32_t seed)
{
 memset(a,0,sizeof(*a));a->hooks=hooks;a->seed=seed?seed:1;a->selected_id=1;
 a->backlight_ms=60000;a->apo_ms=600000;a->power_available=true;
 memset(a->settings.difficulty,1,sizeof(a->settings.difficulty));a->settings.show_time=1;a->settings.target=24;a->settings.migration_complete=1;
 a->settings.mode[25]=1;memset(a->previous_level,1,sizeof a->previous_level);
 if(hooks.load_state){
  bool saved=false;int rc=hooks.load_state(hooks.context,&a->settings,&a->session,&saved);
  if(rc==NG_LOAD_OK || rc==NG_LOAD_RECOVERED){
   a->resumable=saved && a->session.game.status==NG_PLAYING;
   a->active=a->resumable;
   if(rc==NG_LOAD_RECOVERED)snprintf(a->notice,sizeof a->notice,"Backup recovered.");
  }else if(rc>=NG_LOAD_INVALID && !a->notice[0])snprintf(a->notice,sizeof a->notice,"Save unavailable; defaults in use.");
 }else{
  if(hooks.load_settings){
   int rc=hooks.load_settings(hooks.context,&a->settings);
   if(rc>=NG_LOAD_INVALID)snprintf(a->notice,sizeof a->notice,"Settings unavailable; defaults in use");
  }
  if(hooks.load)for(unsigned index=0;index<a->settings.recent_count;index++){
   unsigned id=a->settings.recent[index];
   int rc=hooks.load(hooks.context,&a->session,id);
   if((rc==NG_LOAD_OK || rc==NG_LOAD_RECOVERED) && a->session.game.status==NG_PLAYING){
    a->active=a->resumable=true;resume_setting(&a->settings,id);
    if(rc==NG_LOAD_RECOVERED)snprintf(a->notice,sizeof a->notice,"Backup recovered for game %u",id);
    break;
   }
  }
  if(!a->resumable)memset(&a->session,0,sizeof a->session);
 }
 for(unsigned i=0;i<NG_ID_MAX;i++)if(a->settings.difficulty[i]<NG_HELL)a->previous_level[i]=a->settings.difficulty[i];
}
bool ng_checkpoint(NgApp *a)
{
 bool writing=(a->active && a->dirty) || a->settings_dirty;
 if(writing)ng_diag_emit(NGD_SAVE_BEGIN,a->epoch,0);
 bool ok=true;
 if(writing){
  if(a->active && a->session.game.status)ng_record_result(&a->session);
  resume_setting(&a->settings,a->resumable?a->session.game.id:0);
  if(a->hooks.save_state)ok=a->hooks.save_state(a->hooks.context,&a->settings,&a->session,a->resumable);
  else {
   if(a->resumable && a->active && a->dirty && a->hooks.save)
    ok=a->hooks.save(a->hooks.context,&a->session);
   if(ok && a->hooks.save_settings)ok=a->hooks.save_settings(a->hooks.context,&a->settings);
  }
  if(ok)a->dirty=a->settings_dirty=false;
 }
 a->save_failed=!ok;if(writing)ng_diag_emit(ok?NGD_SAVE_END:NGD_SAVE_ERROR,a->epoch,0);return ok;
}
static bool enter(NgApp *a,unsigned id)
{
 if(!ng_checkpoint(a)){modal(a,NG_MODAL_SAVE_ERROR);return false;}
 a->selected_id=(uint8_t)id;
 if(id==6)target_normalize(a);
 a->screen=NG_ENTRY;a->modal=NG_MODAL_NONE;
 a->entry_selection=(uint8_t)ng_entry_row(a,a->resumable && a->session.game.id==id?NG_ENTRY_RESUME:NG_ENTRY_NEW);
 barrier(a);return true;
}
static void start(NgApp *a,bool continue_result)
{
 if(continue_result && a->selected_id==6 && a->session.game.pack_revision<=5 &&
    target_choice((unsigned)a->session.game.data[0])<0){
  target_normalize(a);a->screen=NG_ENTRY;a->modal=NG_MODAL_NONE;
  a->entry_selection=(uint8_t)ng_entry_row(a,NG_ENTRY_TARGET);
  snprintf(a->notice,sizeof a->notice,"Choose a target for the next game.");barrier(a);return;
 }
 if(!a->hooks.save_state && !ng_checkpoint(a)){modal(a,NG_MODAL_SAVE_ERROR);return;}
 unsigned id=a->selected_id;
 bool same=a->active && a->session.game.id==id;
 uint32_t run=same?satadd(a->session.stats.started,1):1;
 /* A continued game's future cycle order must survive a cold RESUME. */
 uint32_t seed=advance_seed(same?a->session.game.seed:a->seed);
 unsigned d=continue_result?a->session.game.difficulty:a->settings.difficulty[id-1];
 unsigned mode=continue_result?a->session.game.mode:a->settings.mode[id-1];
 const NgModule *module=ng_module(id);
 if(module && mode>=module->modes)mode=0;
 unsigned target=continue_result && id==6?target_request(&a->session.game):a->settings.target;
 if(id==6 && target_choice(target)<0){target_normalize(a);target=a->settings.target;}
 if(id==26 && !mode)d=NG_NORMAL;
 unsigned count=id==6?(target==NG_TARGET_RANDOM?1000u:200u):ng_bank_count(id,d,mode);
 NgSupply staged={0};
 bool target_ok=true;
 if(same && a->session.game.pack_revision==ng_pack_revision(id,mode) &&
    (id!=6 || target_request(&a->session.game)==target))
  staged=a->session.supply[mode][d];
 NgSupply *bag=&staged;
 a->before=a->session.game;
 if(count && count<=4096){
  bool cycle=bag->count!=count || bag->next>=count || !bag->shuffle;
  if(cycle){
   bag->count=(uint16_t)count;bag->next=0;
   uint32_t stride=seed&UINT32_C(0xffff0000);if(!stride)stride=UINT32_C(0x10000);
   bag->shuffle=stride|((seed&65535u)%count);
  }
  /* At a cycle boundary (including an imported save with no cycle metadata),
   * keep the next puzzle distinct from the run being replaced. */
  bool avoid_previous=same && a->before.difficulty==d &&
   a->before.mode==mode && count>1;
  for(unsigned attempt=0;attempt<count;attempt++){
   ng_new_supply(&a->session.game,id,d,mode,seed,run,bag->shuffle,bag->next);
   if(id==6 && !(target_ok=gc_target_init(&a->session.game,target)))break;
   if(!cycle || !avoid_previous ||
    a->session.game.puzzle_id!=a->before.puzzle_id)break;
   if(attempt+1<count)bag->shuffle=(bag->shuffle&UINT32_C(0xffff0000))|(((bag->shuffle&65535u)+1)%count);
  }
  bag->next++;
  for(unsigned i=3;i;i--)bag->recent[i]=bag->recent[i-1];
  bag->recent[0]=a->session.game.puzzle_id;if(bag->recent_count<4)bag->recent_count++;
 }else ng_new(&a->session.game,id,d,mode,seed,run);
 if(id==6 && !count)target_ok=gc_target_init(&a->session.game,target);
 if(!target_ok){
  a->session.game=a->before;
  snprintf(a->notice,sizeof a->notice,"Target setup failed; previous run retained.");modal(a,NG_MODAL_NONE);return;
 }
 if(!count){
  bool compare=same && a->before.difficulty==d && a->before.mode==mode &&
   staged.recent_count;
  uint32_t fingerprint=puzzle_fingerprint(&a->session.game);
  for(unsigned attempt=0;compare && fingerprint==staged.recent[0] && attempt<3;attempt++){
   seed=advance_seed(seed);ng_new(&a->session.game,id,d,mode,seed,run);
   if(id==6 && !gc_target_init(&a->session.game,target))break;
   fingerprint=puzzle_fingerprint(&a->session.game);
  }
  staged.recent[0]=fingerprint;staged.recent_count=1;
 }
 if(!ng_valid(&a->session.game)){
  a->session.game=a->before;
  snprintf(a->notice,sizeof a->notice,"Puzzle validation failed; previous run retained.");modal(a,NG_MODAL_NONE);return;
 }
 NgSettings previous_settings=a->settings;
 NgSupply previous_supply[NG_MODES][NG_LEVEL_COUNT];
 memcpy(previous_supply,a->session.supply,sizeof previous_supply);
 uint8_t previous_undo=a->session.undo_count;
 uint32_t previous_started=a->session.stats.started;
 bool previous_active=a->active,previous_resume=a->resumable;
 bool previous_dirty=a->dirty,previous_settings_dirty=a->settings_dirty;
 if(!same)memset(a->session.supply,0,sizeof a->session.supply);
 a->session.supply[mode][d]=staged;
 a->session.undo_count=0;a->session.stats.started=run;
 a->active=a->resumable=true;a->dirty=true;
 a->settings.last_game=(uint8_t)id;a->settings.mode[id-1]=(uint8_t)mode;
 if(id==6)a->settings.target=(uint16_t)target;
 resume_setting(&a->settings,id);a->settings_dirty=true;
 if(!ng_checkpoint(a)){
  a->session.game=a->before;memcpy(a->session.supply,previous_supply,sizeof previous_supply);
  a->session.undo_count=previous_undo;a->session.stats.started=previous_started;
  a->settings=previous_settings;a->active=previous_active;a->resumable=previous_resume;
  a->dirty=previous_dirty;a->settings_dirty=previous_settings_dirty;
  a->start_failed=true;a->start_failed_result=continue_result;
  modal(a,NG_MODAL_SAVE_ERROR);return;
 }
 a->start_failed=a->start_failed_result=false;a->seed=seed;
 if(!same)memset(a->session.stats.best,0,sizeof a->session.stats.best);
 a->screen=NG_PLAY;a->modal=NG_MODAL_NONE;a->result_view=false;barrier(a);
 if(a->settings.first_help && run==1)modal(a,NG_MODAL_RULES);
}
static void resume(NgApp *a)
{
 if(!a->resumable || !a->active || a->session.game.status ||
  a->session.game.id!=a->selected_id)return;
 a->screen=NG_PLAY;a->modal=NG_MODAL_NONE;
 a->result_view=false;
 barrier(a);
 if(a->session.game.id==1 && a->session.game.pack_revision>=4 && a->session.game.phase==1)
  modal(a,NG_MODAL_LAST_TRY);
}
static void init_same(NgApp *a)
{
 NgGame *g=&a->session.game;uint8_t recorded=g->recorded;
 uint8_t id=g->id,d=g->difficulty,m=g->mode;uint32_t seed=g->seed,run=g->run_id;
 uint32_t supply=g->supply_seed,index=g->supply_index,revision=g->pack_revision;
 uint32_t puzzle=g->puzzle_id;a->before=*g;
 ng_new_supply_version(g,id,d,m,seed,run,supply,index,revision);
 unsigned target=id==6?target_request(&a->before):0;
 if(id==6 && revision>=3 && !gc_target_init(g,target)){
  *g=a->before;ng_message(g,"Saved target unavailable; run retained.");modal(a,NG_MODAL_NONE);return;
 }
 g->recorded=recorded;g->assisted=1;
 /* A larger bank must never change an old run's INIT puzzle. */
 unsigned count=ng_bank_count_version(id,d,m,revision);
 if(id==6 && revision>=6 && target!=NG_TARGET_RANDOM)count=200;
 if(g->puzzle_id!=puzzle && count){
  for(unsigned ordinal=0;ordinal<count;ordinal++){
   ng_new_supply_version(g,id,d,m,seed,run,1,ordinal,revision);
   if(id==6 && revision>=3 && !gc_target_init(g,target))break;
   if(g->puzzle_id==puzzle)break;
  }
  if(g->puzzle_id!=puzzle){*g=a->before;ng_message(g,"Original puzzle unavailable; run retained.");modal(a,NG_MODAL_NONE);return;}
  g->pack_revision=revision;g->recorded=recorded;g->assisted=1;
 }
 a->session.undo_count=0;a->dirty=true;modal(a,NG_MODAL_NONE);
}
static bool finished(NgApp *a)
{
 if(!a->session.game.status)return false;
 a->resumable=false;resume_setting(&a->settings,0);
 ng_record_result(&a->session);a->dirty=true;a->settings_dirty=true;
 if(!ng_checkpoint(a))modal(a,NG_MODAL_SAVE_ERROR);
 else modal(a,NG_MODAL_RESULT);
 return true;
}
static bool perform(NgApp *a,int key)
{
 NgGame *g=&a->session.game;const NgModule *m=ng_module(g->id);
 uint8_t oldphase=g->phase;uint32_t oldmoves=g->moves;
 a->before=*g;NgDiagScope scope=ng_diag_begin(key==NGK_CPU?NGOP_CPU:key==NGK_HINT?NGOP_HINT:NGOP_INPUT,g->id);
 bool changed=m->action(g,key);ng_diag_end(scope);
 if(!changed)return false;
 /* Modules mark only a consumed hint/reveal as assisted, not an error prompt. */
 if((m->flags&NGF_UNDO) && oldmoves!=g->moves && key!=NGK_CPU && !a->before.cpu_pending) {
  if(a->session.undo_count==NG_UNDO){memmove(a->session.undo,a->session.undo+1,(NG_UNDO-1)*sizeof(NgGame));a->session.undo_count--;}
  a->session.undo[a->session.undo_count++]=a->before;
 }
 a->dirty=true;
 if(!finished(a) && oldphase!=g->phase){
  if(g->id==1 && g->pack_revision>=4 && g->phase==1)modal(a,NG_MODAL_LAST_TRY);
  else barrier(a);
 }
 return true;
}
bool ng_app_cpu(NgApp *a)
{
 if(a->screen!=NG_PLAY || a->modal || !a->session.game.cpu_pending || a->session.game.status)return false;
 return perform(a,NGK_CPU);
}
bool ng_app_tick(NgApp *a,uint32_t dt)
{
 if(a->screen!=NG_PLAY || a->modal || !a->active || a->session.game.status || !dt)return false;
 NgGame *g=&a->session.game;const NgModule *m=ng_module(g->id);
 uint32_t before=g->elapsed_ms/1000;uint8_t phase=g->phase;
 g->elapsed_ms=satadd(g->elapsed_ms,dt);a->session.stats.active_ms=satadd(a->session.stats.active_ms,dt);a->dirty=true;
 bool redraw=m->tick?m->tick(g,dt):false;
 if(finished(a))return true;
 if(phase!=g->phase){barrier(a);redraw=true;}
 return redraw || (a->settings.show_time && before!=g->elapsed_ms/1000);
}
static void difficulty(NgApp *a,int step)
{
 unsigned id=a->selected_id;if(!ng_entry_level(a))return;
 if(a->settings.difficulty[id-1]==NG_HELL){if(step<0){a->settings.difficulty[id-1]=NG_MASTER;a->previous_level[id-1]=NG_MASTER;a->settings_dirty=true;}return;}
 int next=a->settings.difficulty[id-1]+step;
 if(next>=0 && next<(int)ng_regular_difficulty_count(id)){a->settings.difficulty[id-1]=(uint8_t)next;a->previous_level[id-1]=(uint8_t)next;a->settings_dirty=true;}
}
static void mode_next(NgApp *a,int step)
{
 unsigned id=a->selected_id;const NgModule *m=ng_module(id);int next=a->settings.mode[id-1]+step;
 if(m->modes>1 && next>=0 && next<m->modes){a->settings.mode[id-1]=(uint8_t)next;a->settings_dirty=true;}
}
static void target_step(NgApp *a,int step)
{
 int index=target_choice(a->settings.target);
 if(index<0)index=1;
 int next=index+step;
 if(next>=0 && next<(int)(sizeof target_choices/sizeof target_choices[0])){
  a->settings.target=target_choices[next];a->settings_dirty=true;a->notice[0]=0;
 }
}
static bool dispatch(NgApp *a,int key)
{
 if(key==NGK_MENU || key==NGK_ACON) {
  ng_diag_stress_cancel();
  if(key==NGK_MENU)ng_diag_emit(NGD_MENU_REQUEST,a->epoch,0);
  bool ok=ng_checkpoint(a);barrier(a);
  if(key==NGK_MENU && a->hooks.os_menu)a->hooks.os_menu(a->hooks.context);
  if(key==NGK_ACON && a->hooks.power_off)a->hooks.power_off(a->hooks.context);
  if(!ok)modal(a,NG_MODAL_SAVE_ERROR);
  return true;
 }
 if(a->modal) {
  if(a->modal==NG_MODAL_LAST_TRY){
   if(key==NGK_EXE || key==NGK_F6 || key==NGK_EXIT){
    (void)perform(a,NGK_EXE);modal(a,NG_MODAL_NONE);
   }
   return true;
  }
  if(a->modal==NG_MODAL_MODE){
   unsigned count=ng_module(a->selected_id)->modes;
   if(key==NGK_EXIT)modal(a,NG_MODAL_NONE);
   else if(key==NGK_EXE || key==NGK_F6){a->settings.mode[a->selected_id-1]=a->mode_choice;a->settings_dirty=true;modal(a,NG_MODAL_NONE);a->entry_selection=(uint8_t)ng_entry_row(a,NG_ENTRY_MODE);}
   else if(key==NGK_LEFT && a->mode_choice)a->mode_choice--;
   else if(key==NGK_RIGHT && (unsigned)a->mode_choice+1u<count)a->mode_choice++;
   else if(key==NGK_UP && a->mode_choice>=2)a->mode_choice-=2;
   else if(key==NGK_DOWN && (unsigned)a->mode_choice+2u<count)a->mode_choice+=2;
   return true;
  }
  if(a->modal==NG_MODAL_DIAGNOSTICS){
   if(key==NGK_EXIT || key==NGK_EXE || key==NGK_F6){ng_diag_stress_cancel();modal(a,NG_MODAL_NONE);}
   else if(key==NGK_F2)snprintf(a->notice,sizeof a->notice,"%s",ng_diag_export()?"Diagnostic log exported":"Diagnostic export failed");
#ifdef NG_DIAGNOSTIC
   else if(key==NGK_F1){ng_diag_clear_counters();a->notice[0]=0;}
   else if(key==NGK_F3)a->diag_page^=1;
   else if(key==NGK_F4){if(ng_diag_stress_active())ng_diag_stress_cancel();else ng_diag_stress_start();}
#endif
   return true;
  }
  if(a->modal==NG_MODAL_RECORDS) {
   if(key==NGK_EXIT || key==NGK_EXE || key==NGK_F6)modal(a,NG_MODAL_NONE);
   else if(key==NGK_F1)a->record_assisted=0;
   else if(key==NGK_F2)a->record_assisted=1;
   else if(key==NGK_F3 && ng_module(a->selected_id)->modes>1){a->record_mode=(uint8_t)((a->record_mode+1)%ng_module(a->selected_id)->modes);if(a->selected_id==26 && !a->record_mode && a->record_difficulty>NG_HARD)a->record_difficulty=NG_NORMAL;}
   else if(key==NGK_F4 || key==NGK_RIGHT){unsigned n=a->selected_id==26 && !a->record_mode?3:ng_difficulty_count(a->selected_id);a->record_difficulty=(uint8_t)((a->record_difficulty+1)%n);}
   else if(key==NGK_LEFT){unsigned n=a->selected_id==26 && !a->record_mode?3:ng_difficulty_count(a->selected_id);a->record_difficulty=(uint8_t)((a->record_difficulty+n-1)%n);}
   return true;
  }
  if(a->modal==NG_MODAL_RULES) {
   if(key==NGK_DOWN && a->rules_scroll<ng_rules_max_scroll(a->selected_id))a->rules_scroll++;
   else if(key==NGK_UP && a->rules_scroll)a->rules_scroll--;
   else if(key==NGK_EXIT || key==NGK_EXE || key==NGK_F6)modal(a,a->screen==NG_PLAY && a->session.game.status && !a->result_view?NG_MODAL_RESULT:NG_MODAL_NONE);
   return true;
  }
  if(a->modal==NG_MODAL_RESULT) {
   if(key==NGK_F1 || key==NGK_EXE || key==NGK_F6)start(a,true);
   else if(key==NGK_EXIT){a->result_view=true;modal(a,NG_MODAL_NONE);}
   else if(key==NGK_F5)modal(a,NG_MODAL_RULES);
   return true;
  }
  if(a->modal==NG_MODAL_SAVE_ERROR) {
   if(key==NGK_EXE || key==NGK_F6){
    if(a->start_failed)start(a,a->start_failed_result);
    else if(ng_checkpoint(a))modal(a,a->screen==NG_PLAY && a->session.game.status && !a->result_view?NG_MODAL_RESULT:NG_MODAL_NONE);
   }
   else if(key==NGK_EXIT){a->start_failed=a->start_failed_result=false;modal(a,a->screen==NG_PLAY && a->session.game.status && !a->result_view?NG_MODAL_RESULT:NG_MODAL_NONE);}
   return true;
  }
  if(a->modal==NG_MODAL_PAUSE) {
   if(key==NGK_EXE || key==NGK_F4 || key==NGK_EXIT)modal(a,NG_MODAL_NONE);
   return true;
  }
  if(key==NGK_EXIT){modal(a,NG_MODAL_NONE);return true;}
  if(key==NGK_EXE || key==NGK_F6){if(a->modal==NG_MODAL_INIT)init_same(a);else start(a,false);}
  return true;
 }
 if(a->screen==NG_MAIN || a->screen==NG_CATEGORY) {
  if(key==NGK_LEFT)a->selection=(uint8_t)((a->selection+5)%6);
  else if(key==NGK_RIGHT)a->selection=(uint8_t)((a->selection+1)%6);
  else if(key==NGK_UP)a->selection=(uint8_t)((a->selection+4)%6);
  else if(key==NGK_DOWN)a->selection=(uint8_t)((a->selection+2)%6);
  else if(key==NGK_EXIT && a->screen==NG_CATEGORY){a->screen=NG_MAIN;a->selection=a->category;barrier(a);}
  else if(key==NGK_F2 && a->screen==NG_MAIN){a->screen=NG_SETTINGS;barrier(a);}
  else if(key==NGK_F1 && a->screen==NG_MAIN && a->resumable){a->selected_id=a->session.game.id;resume(a);}
  else if((key>='1' && key<='6') || key==NGK_EXE || key==NGK_F6) {
   unsigned selection=(key>='1' && key<='6')?(unsigned)(key-'1'):a->selection;
   if(a->screen==NG_MAIN){a->category=(uint8_t)selection;a->screen=NG_CATEGORY;a->selection=0;barrier(a);}
   else enter(a,ng_visible_id(a->category*6+selection));
  }
  return true;
 }
 if(a->screen==NG_ENTRY) {
  unsigned count=ng_entry_count(a);
  if(a->entry_selection>=count)a->entry_selection=0;
  if(key==NGK_UP)a->entry_selection=(uint8_t)((a->entry_selection+count-1)%count);
  else if(key==NGK_DOWN)a->entry_selection=(uint8_t)((a->entry_selection+1)%count);
  else if(key==NGK_F5)modal(a,NG_MODAL_RULES);
  else if(key==NGK_EXIT){a->screen=NG_CATEGORY;a->selection=(uint8_t)(ng_catalog_index(a->selected_id)%6);a->category=(uint8_t)(ng_catalog_index(a->selected_id)/6);barrier(a);}
  else {
   bool row_shortcut=ng_entry_action(a,a->entry_selection)!=NG_ENTRY_TARGET && key>='1' && key<'1'+(int)count;
   if(row_shortcut)a->entry_selection=(uint8_t)(key-'1');
   int choice=ng_entry_action(a,a->entry_selection);
   if(key==NGK_F3 && choice==NG_ENTRY_LEVEL && ng_has_hell(a->selected_id)){
    unsigned index=a->selected_id-1;
    if(a->settings.difficulty[index]==NG_HELL)a->settings.difficulty[index]=a->previous_level[index];
    else {a->previous_level[index]=a->settings.difficulty[index];a->settings.difficulty[index]=NG_HELL;}
    a->settings_dirty=true;
   }else if(key==NGK_F3 && choice==NG_ENTRY_MODE && ng_mode_chooser(a->selected_id)){
    a->mode_choice=a->settings.mode[a->selected_id-1];modal(a,NG_MODAL_MODE);
   }else if(key==NGK_LEFT || key==NGK_RIGHT){
    int step=key==NGK_LEFT?-1:1;
    if(choice==NG_ENTRY_LEVEL)difficulty(a,step);
    else if(choice==NG_ENTRY_MODE){mode_next(a,step);a->entry_selection=(uint8_t)ng_entry_row(a,NG_ENTRY_MODE);}
    else if(choice==NG_ENTRY_TARGET)target_step(a,step);
   }else if(key==NGK_EXE || key==NGK_F6){
    if(choice==NG_ENTRY_RESUME)resume(a);
    else start(a,false);
   }
  }
  return true;
 }
 if(a->screen==NG_STATS) {
  if(key==NGK_EXIT){a->screen=a->stats_category==6?NG_MAIN:NG_CATEGORY;barrier(a);}
  else if(key==NGK_RIGHT || key==NGK_DOWN)a->stats_page=(uint8_t)((a->stats_page+1)%6);
  else if(key==NGK_LEFT || key==NGK_UP)a->stats_page=(uint8_t)((a->stats_page+5)%6);
  return true;
 }
 if(a->screen==NG_SETTINGS) {
  if(key=='1' || key==NGK_F1){a->settings.first_help^=1;a->settings_dirty=true;}
  else if(key=='2' || key==NGK_F2){a->settings.show_time^=1;a->settings_dirty=true;}
  else if(key==NGK_F3)modal(a,NG_MODAL_DIAGNOSTICS);
  else if(key==NGK_EXIT){(void)ng_checkpoint(a);a->screen=NG_MAIN;barrier(a);}
  return true;
 }
 if(a->screen==NG_PLAY) {
  const NgModule *m=ng_module(a->session.game.id);
  if(a->session.game.status){
   if(key==NGK_EXIT){
    if(a->session.game.id==1){
     /* The frozen result was viewable until EXIT. Once the inactive save is
      * committed, retire its secret and 50-guess RAM record as well. Keep
      * the last seed so a subsequent NEW constructs a different run. */
     if(!ng_checkpoint(a)){modal(a,NG_MODAL_SAVE_ERROR);return true;}
     a->seed=a->session.game.seed;
     memset(&a->session,0,sizeof a->session);
     memset(&a->before,0,sizeof a->before);
     a->active=a->resumable=false;
    }
    a->screen=NG_ENTRY;a->result_view=false;
    a->entry_selection=(uint8_t)ng_entry_row(a,NG_ENTRY_NEW);barrier(a);
   }
   else if(key==NGK_F6)start(a,true);
   else if(key==NGK_F5)modal(a,NG_MODAL_RULES);
   return true;
  }
  if(key==NGK_EXIT) {
   if(a->session.game.id==31 && a->session.game.phase==1)return perform(a,key);
   if(ng_checkpoint(a)){a->screen=NG_ENTRY;a->entry_selection=(uint8_t)ng_entry_row(a,NG_ENTRY_RESUME);barrier(a);}
   else modal(a,NG_MODAL_SAVE_ERROR);
  }
  else if(key==NGK_F1){if(a->session.game.moves || a->session.game.input[0] || a->session.game.assisted || a->session.game.elapsed_ms || a->session.game.phase)modal(a,NG_MODAL_INIT);else init_same(a);}
  else if(key==NGK_F2 && (m->flags&NGF_UNDO) && a->session.undo_count) {
   NgGame *g=&a->session.game;uint8_t recorded=g->recorded;uint32_t elapsed=g->elapsed_ms;
   *g=a->session.undo[--a->session.undo_count];g->assisted=1;g->recorded=recorded;g->elapsed_ms=elapsed;a->dirty=true;barrier(a);
  }
  else if(key==NGK_F3 && (m->flags&NGF_HINT))return perform(a,NGK_HINT);
  else if(key==NGK_F4 && m->aux_label && !strcmp(m->aux_label,"PAUSE"))modal(a,NG_MODAL_PAUSE);
  else if(key==NGK_F4 && m->aux_label && m->aux_label[0])return perform(a,NGK_AUX);
  else if(key==NGK_F5)modal(a,NG_MODAL_RULES);
  else if(key==NGK_F6 && m->primary_label && m->primary_label[0])return perform(a,a->session.game.id==33?NGK_F6:NGK_EXE);
  else if(key<NGK_F1 || key>NGK_F6)return perform(a,key);
  return true;
 }
 return false;
}
void ng_app_poweroff(NgApp *a){(void)dispatch(a,NGK_ACON);}
bool ng_app_event(NgApp *a,int key,int type)
{
 int index=ng_key_index(key);
 if(index<0 || index>=64){if(type==NG_DOWN){a->shift_pending=false;a->alpha_pending=false;}return false;}
 uint64_t bit=UINT64_C(1)<<(unsigned)index;
 if(type==NG_UP){a->held&=~bit;a->blocked&=~bit;return false;}
 if(type!=NG_DOWN && type!=NG_HOLD)return false;
 bool already=(a->held&bit)!=0;if(type==NG_HOLD && !already)return false;
 a->held|=bit;if((a->blocked&bit) || (type==NG_DOWN && already))return false;
 if(key==NGK_SHIFT || key==NGK_ALPHA) {
  if(type==NG_DOWN){bool *p=key==NGK_SHIFT?&a->shift_pending:&a->alpha_pending;*p=!*p;}
  return false;
 }
 if(type==NG_HOLD) {
  bool list=a->screen==NG_PLAY && !a->modal && (a->session.game.id==1 || a->session.game.id==2 || a->session.game.id==3) && (key==NGK_UP || key==NGK_DOWN);
  if(key<NGK_UP || key>NGK_LEFT || (a->screen!=NG_MAIN && a->screen!=NG_CATEGORY && a->modal!=NG_MODAL_RULES && !list))return false;
 } else {
  uint64_t modifiers=a->held&~a->blocked;
  bool shift=a->shift_pending || (modifiers&(UINT64_C(1)<<ng_key_index(NGK_SHIFT)));
  bool alpha=a->alpha_pending || (modifiers&(UINT64_C(1)<<ng_key_index(NGK_ALPHA)));
  a->shift_pending=false;a->alpha_pending=false;
  if(key==NGK_ACON && (!shift || alpha))return false;
  if(key=='.' && shift && !alpha)key='=';
 }
 return dispatch(a,key);
}
