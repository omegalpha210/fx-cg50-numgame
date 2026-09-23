#include "app.h"
#include "diagnostics.h"
#include "guesscalc.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
static uint32_t satadd(uint32_t a,uint32_t b){return b>UINT32_MAX-a?UINT32_MAX:a+b;}
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
static int recent_position(const NgSettings *s,unsigned id)
{for(unsigned i=0;i<s->recent_count;i++)if(s->recent[i]==id)return (int)i;return -1;}
static void recent_touch(NgSettings *s,unsigned id)
{
 int found=recent_position(s,id);if(found==0)return;
 unsigned end=found>=0?(unsigned)found:s->recent_count;
 if(found<0 && end==NG_RECENT_LIMIT){s->pending_delete=s->recent[NG_RECENT_LIMIT-1];end--;}
 else if(found<0)s->recent_count++;
 for(unsigned i=end;i>0;i--)s->recent[i]=s->recent[i-1];
 s->recent[0]=(uint8_t)id;
}
void ng_app_init(NgApp *a,NgHooks hooks,uint32_t seed)
{
 memset(a,0,sizeof(*a));a->hooks=hooks;a->seed=seed?seed:1;a->selected_id=1;
 a->backlight_ms=60000;a->apo_ms=600000;a->power_available=true;
 memset(a->settings.difficulty,1,sizeof(a->settings.difficulty));a->settings.show_time=1;a->settings.target=24;a->settings.migration_complete=1;
 a->settings.mode[25]=1;memset(a->previous_level,1,sizeof a->previous_level);
 if(hooks.load_settings) {
  int rc=hooks.load_settings(hooks.context,&a->settings);
  if(rc>=NG_LOAD_INVALID)snprintf(a->notice,sizeof(a->notice),"Settings unavailable; defaults in use");
 }
 if(hooks.load)for(unsigned index=0;index<a->settings.recent_count;index++) {
  unsigned id=a->settings.recent[index];
  int rc=hooks.load(hooks.context,&a->session,id);
  if(rc==NG_LOAD_OK || rc==NG_LOAD_RECOVERED)ng_summarize(&a->session,&a->summary[id-1]);
  if(rc==NG_LOAD_RECOVERED)snprintf(a->notice,sizeof(a->notice),"Backup recovered for game %u",id);
  else if(rc>=NG_LOAD_INVALID)snprintf(a->notice,sizeof(a->notice),"Game %u save unavailable; others retained",id);
 }
 memset(&a->session,0,sizeof(a->session));
 for(unsigned i=0;i<NG_ID_MAX;i++)if(a->settings.difficulty[i]<NG_HELL)a->previous_level[i]=a->settings.difficulty[i];
}
bool ng_checkpoint(NgApp *a)
{
 bool writing=(a->active && a->dirty) || a->settings_dirty || a->settings.pending_delete;
 if(writing)ng_diag_emit(NGD_SAVE_BEGIN,a->epoch,0);
 bool ok=true;
 if(a->active && a->dirty) {
  ng_record_result(&a->session);
  if(a->hooks.save)ok=a->hooks.save(a->hooks.context,&a->session);
  if(ok){a->dirty=false;ng_summarize(&a->session,&a->summary[a->session.game.id-1]);
   if(ng_catalog_index(a->session.game.id)>=0){recent_touch(&a->settings,a->session.game.id);a->settings_dirty=true;}
  }
 }
 if(ok && a->settings_dirty) {
  if(a->hooks.save_settings)ok=a->hooks.save_settings(a->hooks.context,&a->settings);
  if(ok)a->settings_dirty=false;
 }
 if(ok && a->settings.pending_delete){
  unsigned id=a->settings.pending_delete;
  if(a->hooks.remove_game)ok=a->hooks.remove_game(a->hooks.context,id);
  if(ok){a->summary[id-1].exists=0;a->settings.pending_delete=0;a->settings_dirty=true;}
 }
 if(ok && a->settings_dirty){
  if(a->hooks.save_settings)ok=a->hooks.save_settings(a->hooks.context,&a->settings);
  if(ok)a->settings_dirty=false;
 }
 a->save_failed=!ok;if(writing)ng_diag_emit(ok?NGD_SAVE_END:NGD_SAVE_ERROR,a->epoch,0);return ok;
}
static bool enter(NgApp *a,unsigned id)
{
 if(!ng_checkpoint(a)){modal(a,NG_MODAL_SAVE_ERROR);return false;}
 a->selected_id=(uint8_t)id;a->entry_selection=0;
 a->target_draft[0]=0;
 a->screen=NG_ENTRY;a->modal=NG_MODAL_NONE;a->active=false;
 memset(&a->session,0,sizeof(a->session));
 if(a->hooks.load && a->summary[id-1].exists) {
  int rc=a->hooks.load(a->hooks.context,&a->session,id);
  if(rc==NG_LOAD_OK || rc==NG_LOAD_RECOVERED)a->active=true;
  else {a->summary[id-1].exists=0;snprintf(a->notice,sizeof(a->notice),"Saved run unavailable");}
 }
 a->entry_selection=(uint8_t)ng_entry_row(a,NG_ENTRY_NEW);barrier(a);return true;
}
static void start(NgApp *a)
{
 if(!ng_checkpoint(a)){modal(a,NG_MODAL_SAVE_ERROR);return;}
 unsigned id=a->selected_id;
 if(recent_position(&a->settings,id)<0 && a->settings.recent_count==NG_RECENT_LIMIT && !a->evict_confirmed){modal(a,NG_MODAL_EVICT);return;}
 a->evict_confirmed=false;
 uint32_t run=a->session.stats.started+1;
 a->seed^=a->seed<<13;a->seed^=a->seed>>17;a->seed^=a->seed<<5;
 unsigned d=a->settings.difficulty[id-1],mode=a->settings.mode[id-1];
 if(id==26 && !mode)d=NG_NORMAL;
 unsigned count=ng_bank_count(id,d,mode);
 NgSupply staged=a->session.supply[mode][d],*bag=&staged;a->before=a->session.game;
 if(count && count<=4096){
  bool cycle=bag->count!=count || bag->next>=count || !bag->shuffle;
  if(cycle){
   bag->count=(uint16_t)count;bag->next=0;
   uint32_t stride=a->seed&UINT32_C(0xffff0000);if(!stride)stride=UINT32_C(0x10000);
   bag->shuffle=stride|((a->seed&65535u)%count);
  }
  /* Rotate a fresh permutation away from up to four recent puzzle IDs.
   * At most count candidates; no time-dependent solver or partial puzzle. */
  for(unsigned attempt=0;attempt<count;attempt++){
   ng_new_supply(&a->session.game,id,d,mode,a->seed,run,bag->shuffle,bag->next);
   bool recent=false;
   unsigned remembered=bag->recent_count<count?bag->recent_count:count-1;
   for(unsigned i=0;i<remembered;i++)if(bag->recent[i]==a->session.game.puzzle_id)recent=true;
   if(!cycle || !recent)break;
   if(attempt+1<count)bag->shuffle=(bag->shuffle&UINT32_C(0xffff0000))|(((bag->shuffle&65535u)+1)%count);
  }
  bag->next++;
  for(unsigned i=3;i;i--)bag->recent[i]=bag->recent[i-1];
  bag->recent[0]=a->session.game.puzzle_id;if(bag->recent_count<4)bag->recent_count++;
 }else ng_new(&a->session.game,id,d,mode,a->seed,run);
 if(id==6 && !gc_target_init(&a->session.game,a->settings.target)){
  a->session.game=a->before;snprintf(a->notice,sizeof a->notice,"Target setup failed; previous run retained.");modal(a,NG_MODAL_NONE);return;
 }
 if(!ng_valid(&a->session.game)){
  a->session.game=a->before;snprintf(a->notice,sizeof a->notice,"Puzzle validation failed; previous run retained.");modal(a,NG_MODAL_NONE);return;
 }
 a->session.supply[mode][d]=staged;
 a->session.undo_count=0;a->session.stats.started=satadd(a->session.stats.started,1);
 a->active=true;a->dirty=true;a->screen=NG_PLAY;a->modal=NG_MODAL_NONE;
 a->settings.last_game=(uint8_t)id;a->settings_dirty=true;barrier(a);
 if(a->settings.first_help && run==1)modal(a,NG_MODAL_RULES);
}
static void resume(NgApp *a)
{
 if(!a->active){start(a);return;}
 a->screen=NG_PLAY;a->modal=NG_MODAL_NONE;
 a->settings.last_game=a->selected_id;recent_touch(&a->settings,a->selected_id);a->settings_dirty=true;barrier(a);
 if(a->session.game.status)modal(a,NG_MODAL_RESULT);
}
static void init_same(NgApp *a)
{
 NgGame *g=&a->session.game;uint8_t recorded=g->recorded;
 uint8_t id=g->id,d=g->difficulty,m=g->mode;uint32_t seed=g->seed,run=g->run_id;
 uint32_t supply=g->supply_seed,index=g->supply_index,revision=g->pack_revision;
 uint32_t puzzle=g->puzzle_id;a->before=*g;
 ng_new_supply_version(g,id,d,m,seed,run,supply,index,revision);
 if(id==6 && revision>=3 && !gc_target_init(g,(unsigned)a->before.data[0])){
  *g=a->before;ng_message(g,"Saved target unavailable; run retained.");modal(a,NG_MODAL_NONE);return;
 }
 g->recorded=recorded;g->assisted=1;
 /* A larger bank must never change an old run's INIT puzzle. */
 if(g->puzzle_id!=puzzle && ng_bank_count(id,d,m)){
  unsigned count=ng_bank_count(id,d,m);
  for(unsigned ordinal=0;ordinal<count;ordinal++){
   ng_new_supply_version(g,id,d,m,seed,run,1,ordinal,revision);if(g->puzzle_id==puzzle)break;
  }
  if(g->puzzle_id!=puzzle){*g=a->before;ng_message(g,"Original puzzle unavailable; run retained.");modal(a,NG_MODAL_NONE);return;}
  g->pack_revision=revision;g->recorded=recorded;g->assisted=1;
 }
 a->session.undo_count=0;a->dirty=true;modal(a,NG_MODAL_NONE);
}
static bool finished(NgApp *a)
{
 if(!a->session.game.status)return false;
 ng_record_result(&a->session);a->dirty=true;
 (void)ng_checkpoint(a);modal(a,NG_MODAL_RESULT);return true;
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
 if(!finished(a) && oldphase!=g->phase)barrier(a);
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
static bool target_commit(NgApp *a)
{
 if(!a->target_draft[0])return true;
 unsigned value=0;
 for(unsigned i=0;a->target_draft[i];i++)value=value*10u+(unsigned)(a->target_draft[i]-'0');
 if(value<1 || value>1000){snprintf(a->notice,sizeof a->notice,"Target must be 1..1000.");return false;}
 a->settings.target=(uint16_t)value;a->settings_dirty=true;a->target_draft[0]=0;a->notice[0]=0;return true;
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
  if(a->modal==NG_MODAL_EVICT){
   if(key==NGK_EXIT)modal(a,NG_MODAL_NONE);
   else if(key==NGK_EXE || key==NGK_F6){a->evict_confirmed=true;modal(a,NG_MODAL_NONE);start(a);}
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
   if(key==NGK_DOWN && a->rules_scroll<100)a->rules_scroll++;
   else if(key==NGK_UP && a->rules_scroll)a->rules_scroll--;
   else if(key==NGK_EXIT || key==NGK_EXE || key==NGK_F6)modal(a,a->screen==NG_PLAY && a->session.game.status?NG_MODAL_RESULT:NG_MODAL_NONE);
   return true;
  }
  if(a->modal==NG_MODAL_RESULT) {
   if(key==NGK_F1 || key==NGK_EXE || key==NGK_F6)start(a);
   else if(key==NGK_EXIT){a->screen=NG_ENTRY;a->entry_selection=(uint8_t)ng_entry_row(a,NG_ENTRY_NEW);modal(a,NG_MODAL_NONE);}
   else if(key==NGK_F5)modal(a,NG_MODAL_RULES);
   return true;
  }
  if(a->modal==NG_MODAL_SAVE_ERROR) {
   if(key==NGK_EXE || key==NGK_F6){if(ng_checkpoint(a))modal(a,a->screen==NG_PLAY && a->session.game.status?NG_MODAL_RESULT:NG_MODAL_NONE);}
   else if(key==NGK_EXIT)modal(a,a->screen==NG_PLAY && a->session.game.status?NG_MODAL_RESULT:NG_MODAL_NONE);
   return true;
  }
  if(a->modal==NG_MODAL_PAUSE) {
   if(key==NGK_EXE || key==NGK_F4 || key==NGK_EXIT)modal(a,NG_MODAL_NONE);
   return true;
  }
  if(key==NGK_EXIT){modal(a,NG_MODAL_NONE);return true;}
  if(key==NGK_EXE || key==NGK_F6){if(a->modal==NG_MODAL_INIT)init_same(a);else start(a);}
  return true;
 }
 if(a->screen==NG_MAIN || a->screen==NG_CATEGORY) {
  if(key==NGK_LEFT)a->selection=(uint8_t)((a->selection+5)%6);
  else if(key==NGK_RIGHT)a->selection=(uint8_t)((a->selection+1)%6);
  else if(key==NGK_UP)a->selection=(uint8_t)((a->selection+4)%6);
  else if(key==NGK_DOWN)a->selection=(uint8_t)((a->selection+2)%6);
  else if(key==NGK_EXIT && a->screen==NG_CATEGORY){a->screen=NG_MAIN;a->selection=a->category;barrier(a);}
  else if(key==NGK_F2 && a->screen==NG_MAIN){a->screen=NG_SETTINGS;barrier(a);}
  else if(key==NGK_F3 && a->screen==NG_MAIN && ng_catalog_index(a->settings.last_game)>=0 && a->summary[a->settings.last_game-1].exists){if(enter(a,a->settings.last_game))resume(a);}
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
  if((key==NGK_UP || key==NGK_DOWN) && a->target_draft[0] && !target_commit(a))return true;
  if(key==NGK_UP)a->entry_selection=(uint8_t)((a->entry_selection+count-1)%count);
  else if(key==NGK_DOWN)a->entry_selection=(uint8_t)((a->entry_selection+1)%count);
  else if(key==NGK_F5)modal(a,NG_MODAL_RULES);
  else if(key==NGK_EXIT){a->screen=NG_CATEGORY;a->selection=(uint8_t)(ng_catalog_index(a->selected_id)%6);a->category=(uint8_t)(ng_catalog_index(a->selected_id)/6);barrier(a);}
  else {
   bool row_shortcut=ng_entry_action(a,a->entry_selection)!=NG_ENTRY_TARGET && key>='1' && key<'1'+(int)count;
   if(row_shortcut)a->entry_selection=(uint8_t)(key-'1');
   int choice=ng_entry_action(a,a->entry_selection);
   if(choice==NG_ENTRY_TARGET && !row_shortcut && key>='0' && key<='9'){
    size_t n=strlen(a->target_draft);
    if(n<4){a->target_draft[n]=(char)key;a->target_draft[n+1]=0;}
    return true;
   }
   if(choice==NG_ENTRY_TARGET && key==NGK_DEL){
    size_t n=strlen(a->target_draft);if(n)a->target_draft[n-1]=0;return true;
   }
   if(choice==NG_ENTRY_TARGET && key==NGK_EXE){(void)target_commit(a);return true;}
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
    else if(choice==NG_ENTRY_TARGET){
     if(!target_commit(a))return true;
     int value=(int)a->settings.target+step;if(value>=1 && value<=1000){a->settings.target=(uint16_t)value;a->settings_dirty=true;}
    }
   }else if(key==NGK_EXE || key==NGK_F6){
    if(!target_commit(a))return true;
    if(choice==NG_ENTRY_RESUME)resume(a);
    else if(a->active && !a->session.game.status)modal(a,NG_MODAL_NEW);
    else start(a);
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
  if(key==NGK_EXIT) {
   if(a->session.game.id==31 && a->session.game.phase==1)return perform(a,key);
   if(ng_checkpoint(a)){a->screen=NG_ENTRY;a->entry_selection=(uint8_t)ng_entry_row(a,NG_ENTRY_NEW);barrier(a);}
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
  if(key<NGK_UP || key>NGK_LEFT || (a->screen!=NG_MAIN && a->screen!=NG_CATEGORY && a->modal!=NG_MODAL_RULES))return false;
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
