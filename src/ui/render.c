#include "app.h"
#include "diagnostics.h"
#include <stdio.h>
#include <string.h>
static const char *const categories[6]={"GUESS","CALC","LOGIC","PUZZLE","STRATEGY","BOARD"};
static const char *const levels[NG_LEVEL_COUNT]={"EASY","NORMAL","HARD","MASTER","HELL"};
static const int pale[6]={NG_RGB(25,29,31),NG_RGB(31,28,21),NG_RGB(25,30,25),NG_RGB(29,26,31),NG_RGB(31,26,25),NG_RGB(25,30,29)};
static const int colors[6]={NG_RGB(3,12,23),NG_RGB(19,12,1),NG_RGB(3,16,8),NG_RGB(15,7,22),NG_RGB(22,5,5),NG_RGB(2,15,15)};
static unsigned long tile_value(uint32_t exponent){return exponent>=1 && exponent<=30?(unsigned long)(UINT32_C(1)<<exponent):0;}
static void header(NgCanvas *c,const char *name,const char *right)
{
 int rw=right?ng_text_width(right,1):0;
 ng_rect(c,0,0,396,24,NG_INK);ng_text_fit(c,8,7,380-(right?rw+12:0),name,NG_WHITE,1);
 if(right)ng_text(c,388-rw,7,right,NG_RGB(24,28,30),1);
}
static void soft(NgCanvas *c,const char *const labels[6],bool play)
{
 ng_rect(c,0,204,396,20,NG_WHITE);
 for(unsigned i=0;i<6;i++)if(labels[i] && labels[i][0]) {
  if(!strcmp(labels[i],"ENHM")){
   int x=(int)i*66+1;static const int level_colors[4]={NG_BLUE,NG_RGB(25,12,0),NG_RED,NG_MAGENTA};
   ng_rect(c,x,205,64,18,NG_WHITE);ng_border(c,x,205,64,18,NG_LINE,1);
   for(unsigned j=0;j<4;j++){char letter[2]={labels[i][j],0};ng_text(c,x+5+(int)j*14,209,letter,level_colors[j],1);}
   continue;
  }
  int bg=!strcmp(labels[i],"HELL")?NG_RED:play && i==0?NG_YELLOW:play && i==1?NG_MAGENTA:NG_INK;
  ng_rect(c,(int)i*66+1,205,64,18,bg);
  ng_center(c,(int)i*66+1,209,64,labels[i],play && i<2?NG_BLACK:NG_WHITE,1);
 }
}
static void menu(NgCanvas *c,const NgApp *a)
{
 #ifdef NG_DIAGNOSTIC
 const char *app_name="NUM DIAG";
#else
 const char *app_name="NUM GAME";
#endif
 bool main=a->screen==NG_MAIN;header(c,main?app_name:categories[a->category],main?"36 GAMES":"6 GAMES");
 for(unsigned i=0;i<6;i++) {
  int x=7+(int)(i%2)*195,y=30+(int)(i/2)*56,w=187,h=50;
  unsigned color=main?i:a->category;
  ng_rect(c,x,y,w,h,pale[color]);ng_border(c,x,y,w,h,i==a->selection?NG_INK:NG_LINE,i==a->selection?3:1);
  ng_number(c,x+w-15,y+5,(int)i+1,colors[color],1);
  const char *s=main?categories[i]:ng_module(ng_visible_id(a->category*6+i))->short_name;
  static const unsigned category_icons[6]={1,6,11,18,21,31};
  unsigned icon=main?category_icons[i]:ng_visible_id(a->category*6+i);
  ng_game_icon(c,x+10,y+9,icon,colors[color]);
  ng_center(c,x+52,y+20,w-59,s,NG_INK,1);
 }
 const char *labels[6]={main && a->resumable?"RESUME":"",main?"SET":"","","","","OPEN"};
 soft(c,labels,false);
}
static void difficulty_options(NgCanvas *c,const NgApp *a,int y)
{
 static const int colors[4]={NG_RGB(0,17,23),NG_RGB(25,12,0),NG_RGB(26,3,4),NG_RGB(18,5,25)};
 unsigned count=ng_regular_difficulty_count(a->selected_id),selected=a->settings.difficulty[a->selected_id-1];
 if(selected==NG_HELL){ng_text(c,151,y+9,"HELL",NG_RED,1);return;}
 const char *names[4]={"EASY","NORMAL","HARD","MASTER"};int total=0;
 for(unsigned i=0;i<count;i++)total+=ng_text_width(names[i],1)+10;
 int gap=count>1?(228-total)/(int)(count-1):0,x=146;
 for(unsigned i=0;i<count;i++){
  int width=ng_text_width(names[i],1)+10;
  if(i==selected){ng_rect(c,x,y+4,width,21,NG_WHITE);ng_border(c,x,y+4,width,21,colors[i],1);ng_rect(c,x+3,y+23,width-6,2,colors[i]);}
  ng_text(c,x+5,y+9,names[i],colors[i],1);x+=width+gap;
 }
}
static void mode_options(NgCanvas *c,const NgApp *a,int y)
{
 const NgModule *m=ng_module(a->selected_id);unsigned selected=a->settings.mode[a->selected_id-1];
 bool first=(a->selected_id>=21 && a->selected_id<=25) || a->selected_id==37;
 int total=0;for(unsigned i=0;i<m->modes;i++)total+=ng_text_width(first?(i?"CPU":"YOU"):m->mode_name(i),1)+10;
 if(total+(int)(m->modes-1)*6>228){ng_text_fit(c,146,y+9,228,m->mode_name(selected),NG_BLUE,1);return;}
 int gap=m->modes>1?(228-total)/(m->modes-1):0,x=146;
 for(unsigned i=0;i<m->modes;i++){
  const char *label=first?(i?"CPU":"YOU"):m->mode_name(i);
  int w=ng_text_width(label,1)+10;
  if(i==selected){ng_rect(c,x,y+4,w,21,NG_WHITE);ng_border(c,x,y+4,w,21,NG_BLUE,1);ng_rect(c,x+3,y+23,w-6,2,NG_BLUE);}
  ng_text(c,x+5,y+9,label,NG_BLUE,1);x+=w+gap;
 }
}
static void entry(NgCanvas *c,const NgApp *a)
{
 const NgModule *m=ng_module(a->selected_id);header(c,m->name,"GAME MENU");
 unsigned count=ng_entry_count(a);
 for(unsigned i=0;i<count;i++){
  int action=ng_entry_action(a,i);char number[4];snprintf(number,sizeof number,"%u",i+1);
  const char *name=action==NG_ENTRY_RESUME?"RESUME":action==NG_ENTRY_NEW?"START GAME":action==NG_ENTRY_LEVEL?(a->selected_id==27?"SCRAMBLE":"DIFFICULTY"):action==NG_ENTRY_TARGET?"TARGET":((a->selected_id>=21 && a->selected_id<=25) || a->selected_id==37)?"FIRST":"MODE";
  int y=34+(int)i*34;ng_rect(c,10,y,376,29,i==a->entry_selection?pale[ng_catalog_index(a->selected_id)/6]:NG_WHITE);
  ng_border(c,10,y,376,29,i==a->entry_selection?NG_BLUE:NG_LINE,i==a->entry_selection?2:1);
  ng_text(c,20,y+9,number,NG_MUTED,1);ng_text_fit(c,43,y+9,98,name,NG_INK,1);
  if(action==NG_ENTRY_LEVEL)difficulty_options(c,a,y);
  else if(action==NG_ENTRY_MODE)mode_options(c,a,y);
  else if(action==NG_ENTRY_TARGET){char value[16];snprintf(value,sizeof value,"%s%s",a->target_draft[0]?a->target_draft:"",a->target_draft[0]?"_":"");if(!a->target_draft[0])snprintf(value,sizeof value,"%u",a->settings.target);ng_text(c,151,y+9,value,NG_BLUE,1);}
 }
 int selected=ng_entry_action(a,a->entry_selection);
  const char *hint=selected==NG_ENTRY_RESUME?"Continue the saved game with its original settings.":selected==NG_ENTRY_NEW?"Start a game with these settings; replaces old resume.":selected==NG_ENTRY_LEVEL?"LEFT/RIGHT: difficulty for the next START GAME.":selected==NG_ENTRY_TARGET?"Type 1..1000, EXE: apply, F6: open.":"LEFT/RIGHT: mode for the next START GAME.";
 if(selected==NG_ENTRY_LEVEL && a->selected_id==27)hint="LEFT/RIGHT: choose how much to shuffle a new board.";
 if(selected==NG_ENTRY_MODE && (a->selected_id==27 || a->selected_id==28))hint="LEFT/RIGHT: board size for the next START GAME.";
 if(selected==NG_ENTRY_MODE && ((a->selected_id>=21 && a->selected_id<=25) || a->selected_id==37))hint="LEFT/RIGHT: YOU or CPU starts.";
 if(selected==NG_ENTRY_LEVEL && ng_has_hell(a->selected_id))hint=a->settings.difficulty[a->selected_id-1]==NG_HELL?"HELL selected. F3: return to E/N/H/M.":"LEFT/RIGHT: level. F3: separate HELL challenge.";
 if(a->notice[0])ng_small_fit(c,12,174,372,a->notice,NG_RED);
 else if(a->resumable && a->session.game.id==a->selected_id){char b[96];bool classic=a->selected_id==26 && !a->session.game.mode;snprintf(b,sizeof(b),"Saved: %s%s%s%s",classic?"CLASSIC":levels[a->session.game.difficulty],m->modes>1 && !classic?" / ":"",m->modes>1 && !classic?m->mode_name(a->session.game.mode):"",a->session.game.assisted?" / ASSISTED":"");ng_small_fit(c,12,174,372,b,NG_MUTED);}
 ng_wrap(c,12,187,372,9,2,hint,NG_MUTED,true);
 const char *context=selected==NG_ENTRY_LEVEL && ng_has_hell(a->selected_id)?a->settings.difficulty[a->selected_id-1]==NG_HELL?"ENHM":"HELL":"";
 const char *keys[6]={"","",context,"","RULES","OPEN"};soft(c,keys,false);
}
static bool arithmetic_message(unsigned id){return id==2 || (id>=5 && id<=10);}
static void message(NgCanvas *c,const char *s,int color,bool arithmetic)
{
 if(arithmetic)ng_wrap_expression(c,7,186,382,9,2,s,color,true);else ng_wrap(c,7,186,382,9,2,s,color,true);
}
void ng_render_hud(const NgApp *a,NgCanvas *c)
{
 const NgGame *g=&a->session.game;const NgModule *m=ng_module(g->id);char info[40];
 if(g->cpu_pending)snprintf(info,sizeof(info),"CPU TURN");
 else if(a->settings.show_time)snprintf(info,sizeof(info),"%s%lu:%02lu",g->assisted?"A ":"",(unsigned long)(g->elapsed_ms/60000),(unsigned long)((g->elapsed_ms/1000)%60));
 else snprintf(info,sizeof(info),"%s",g->assisted?"ASSISTED":levels[g->difficulty]);
 header(c,m->name,info);
}
static void play(NgCanvas *c,const NgApp *a)
{
 const NgGame *g=&a->session.game;const NgModule *m=ng_module(g->id);
 ng_render_hud(a,c);
 if(m->render)m->render(g,c);
 message(c,g->message,a->save_failed?NG_RED:NG_MUTED,!a->save_failed && arithmetic_message(g->id));
 if(g->status){
  const char *keys[6]={"","","","","RULES","NEW"};soft(c,keys,true);
  return;
 }
 const char *primary=m->primary_label;
 if(g->cpu_pending || (g->id==30 && g->phase<2))primary="";
 else if((g->id==29 && g->phase==1) || (g->id==30 && g->phase==3))primary="NEXT";
 bool cpu_wait=g->id==37 && g->cpu_pending;
 const char *keys[6]={"INIT",(m->flags&NGF_UNDO) && a->session.undo_count?"UNDO":"",!cpu_wait && (m->flags&NGF_HINT)?((g->id>=11 && g->id<=20) || g->id==38?"REVEAL":"HINT"):"",cpu_wait?"":m->aux_label,"RULES",primary};
 soft(c,keys,true);
}
static void stats(NgCanvas *c,const NgApp *a)
{
 unsigned cat=a->stats_category==6?a->stats_page:a->stats_category;
 header(c,"STATISTICS",categories[cat]);
 uint64_t played=0,finished=0,time=0;
 for(unsigned i=0;i<NG_GAME_COUNT;i++)if(a->stats_category==6 || i/5==cat){const NgSummary *v=&a->summary[ng_visible_id(i)-1];played+=v->played;finished+=v->completed+v->assisted;time+=v->active_ms;}
 char b[96];snprintf(b,sizeof(b),"PLAYS %llu   FINISHED %llu   TIME %llum",(unsigned long long)played,(unsigned long long)finished,(unsigned long long)(time/60000));
 ng_text_fit(c,10,34,376,b,NG_INK,1);
 ng_small(c,205,55,"PLAYS",NG_MUTED);ng_small(c,267,55,"NORMAL",NG_MUTED);ng_small(c,329,55,"ASSIST",NG_MUTED);
 for(unsigned i=0;i<5;i++) {
  unsigned id=ng_visible_id(cat*5+i);const NgSummary *s=&a->summary[id-1];int y=70+(int)i*23;
  ng_rect(c,7,y-5,382,23,i%2?NG_PAPER:NG_WHITE);ng_text_fit(c,12,y,184,ng_module(id)->short_name,NG_INK,1);
  uint32_t values[3]={s->played,s->completed,s->assisted};
  for(unsigned j=0;j<3;j++){snprintf(b,sizeof(b),"%lu",(unsigned long)values[j]);ng_small(c,264+(int)j*62-ng_small_width(b),y+1,b,NG_BLUE);}
 }
 ng_small(c,12,192,a->stats_category==6?"LEFT/RIGHT: CATEGORY     EXIT: MAIN":"EXIT: CATEGORY",NG_MUTED);
 const char *keys[6]={"","","","","",""};soft(c,keys,false);
}
static void settings(NgCanvas *c,const NgApp *a)
{
 header(c,"SETTINGS",NULL);ng_text(c,16,48,"1  FIRST GAME RULES",NG_INK,1);ng_text(c,287,48,a->settings.first_help?"ON":"OFF",NG_BLUE,1);
 ng_text(c,16,81,"2  SHOW ACTIVE TIME",NG_INK,1);ng_text(c,287,81,a->settings.show_time?"ON":"OFF",NG_BLUE,1);
 char power[64];snprintf(power,sizeof power,"POWER: DIM %lus / OFF %lum",(unsigned long)(a->backlight_ms/1000),(unsigned long)(a->apo_ms/60000));ng_text_fit(c,16,119,364,power,NG_INK,1);
 ng_small(c,16,141,!a->power_available?"Clock unavailable: reopen app before play.":a->power_os_settings?"Uses OS timeout settings; never changes them.":"NUM GAME fallback: 60s dim / 10min auto-off.",NG_MUTED);
 ng_small(c,16,174,"EXIT saves settings. F3: runtime diagnostics.",NG_MUTED);
 const char *keys[6]={"HELP","TIME","DIAG","","",""};soft(c,keys,false);
}
static void rules(NgCanvas *c,const NgApp *a)
{
 unsigned id=a->selected_id;const NgModule *m=ng_module(id);header(c,"RULES",m->short_name);
 ng_rect(c,0,25,396,179,NG_WHITE);
 const char *texts[2]={m->rules,ng_has_hell(id)?"Entry: focus DIFFICULTY, F3 selects HELL.\nF3 again restores the prior normal level.\nHELL + LEFT returns to MASTER. RIGHT stays.\nOPEN starts HELL; RESUME keeps its own level.":""};
 unsigned line=0,drawn=0;
 for(unsigned part=0;part<2;part++){const char *p=texts[part];while(*p && drawn<10) {
  char b[128];if(!ng_text_line(b,sizeof b,&p,376,false))break;
  if(line++<a->rules_scroll)continue;
  ng_text(c,10,32+(int)drawn*16,b,NG_INK,1);drawn++;
 }}
 const char *keys[6]={"","","","","","OK"};soft(c,keys,false);
}
static void dialog(NgCanvas *c,const NgApp *a)
{
 if(a->modal==NG_MODAL_MODE){
  const NgModule *m=ng_module(a->selected_id);ng_rect(c,0,0,396,204,NG_PAPER);header(c,"CHOOSE MODE",m->short_name);
  for(unsigned i=0;i<m->modes;i++){
   int x=10+(int)(i%2)*193,y=35+(int)(i/2)*34;
   ng_rect(c,x,y,183,28,i==a->mode_choice?NG_WHITE:NG_PAPER);
   ng_border(c,x,y,183,28,i==a->mode_choice?NG_BLUE:NG_LINE,i==a->mode_choice?2:1);
   ng_center(c,x+5,y+9,173,m->mode_name(i),i==a->mode_choice?NG_BLUE:NG_INK,1);
  }
  ng_small(c,12,184,"Arrows: select. OK: use setting. EXIT: cancel.",NG_MUTED);
  const char *keys[6]={"","","","","","OK"};soft(c,keys,false);return;
 }
 if(a->modal==NG_MODAL_DIAGNOSTICS){
#ifdef NG_DIAGNOSTIC
  const NgDiagnostics *d=&ng_diagnostics;char b[96];ng_rect(c,0,0,396,204,NG_PAPER);header(c,a->diag_page?"RUNTIME SUMMARY":"MEMORY SUMMARY","NUM DIAG");
  if(!a->diag_page){
   snprintf(b,sizeof b,"Observed stack: %lu B%s",(unsigned long)(d->stack_base-d->stack_low),d->stack_range_verified?" / 16K region":" / range unavailable");ng_text_fit(c,10,34,376,b,NG_INK,1);
   snprintf(b,sizeof b,"Deepest: game %u / %s",d->peak_game,ng_diag_operation_name(d->peak_operation));ng_text_fit(c,10,55,376,b,NG_MUTED,1);
   for(unsigned i=0;i<2;i++){
    const NgDiagArena *ar=&d->arena[i];
    if(ar->available)snprintf(b,sizeof b,"%s used/peak: %lu / %lu B",i?"_ostk":"_uram",(unsigned long)ar->used,(unsigned long)ar->peak_used);
    else snprintf(b,sizeof b,"%s heap: unavailable",i?"_ostk":"_uram");
    ng_text_fit(c,10,76+(int)i*21,376,b,NG_INK,1);
   }
   snprintf(b,sizeof b,"Codec high-water: %lu / %u B",(unsigned long)d->codec_peak,NG_RECORD_MAX);ng_text_fit(c,10,118,376,b,NG_INK,1);
   snprintf(b,sizeof b,"Handles %u (peak %u) / Timers %u (peak %u)",d->handles,d->peak_handles,d->timers,d->peak_timers);ng_text_fit(c,10,139,376,b,NG_INK,1);
   ng_small(c,10,160,"SP samples are observed, not a worst-case bound.",NG_MUTED);
   ng_small(c,10,171,"VRAM is included in _ostk. Heap peaks are lifetime.",NG_MUTED);
  }else{
   snprintf(b,sizeof b,"MENU request / enter / return: %lu / %lu / %lu",(unsigned long)d->menu_requests,(unsigned long)d->menu_entries,(unsigned long)d->menu_returns);ng_text_fit(c,10,36,376,b,NG_INK,1);
   snprintf(b,sizeof b,"Save errors: %lu / Sampled calls: %lu",(unsigned long)d->save_errors,(unsigned long)d->instrument_calls);ng_text_fit(c,10,61,376,b,NG_INK,1);
   snprintf(b,sizeof b,"Stress: %lu / 1000   Errors: %lu",(unsigned long)d->stress_done,(unsigned long)d->stress_failures);ng_text_fit(c,10,87,376,b,NG_INK,1);
   ng_text(c,10,113,ng_diag_stress_active()?"Running RAM fixtures; EXIT cancels.":"F4 starts a bounded, cancellable RAM test.",NG_BLUE,1);
   ng_small(c,10,139,"Fixtures reuse the codec workspace; no save writes.",NG_MUTED);
   ng_small(c,10,151,"EXPORT includes init timing samples at 128 ticks/s.",NG_MUTED);
   ng_small(c,10,163,"Library/OS stack between samples is not observed.",NG_MUTED);
  }
  ng_small_fit(c,10,186,376,a->notice[0]?a->notice:"Export the summary after tests and before leaving.",NG_MUTED);
  const char *keys[6]={"RESET","EXPORT","PAGE",ng_diag_stress_active()?"STOP":"STRESS","","OK"};soft(c,keys,false);return;
#else
  const NgDiagnostics *d=&ng_diagnostics;char b[96];ng_rect(c,0,0,396,204,NG_PAPER);header(c,"RUNTIME DIAGNOSTICS",NULL);
  snprintf(b,sizeof b,"MENU request/enter/return: %lu/%lu/%lu",(unsigned long)d->menu_requests,(unsigned long)d->menu_entries,(unsigned long)d->menu_returns);ng_text_fit(c,10,36,376,b,NG_INK,1);
  snprintf(b,sizeof b,"Open handles: %u   Peak: %u",d->handles,d->peak_handles);ng_text(c,10,60,b,NG_INK,1);
  snprintf(b,sizeof b,"Active timers: %u   Peak: %u",d->timers,d->peak_timers);ng_text(c,10,84,b,NG_INK,1);
  snprintf(b,sizeof b,"Heap blocks: %ld   Free: %ld",(long)d->heap_live,(long)d->heap_free);ng_text_fit(c,10,108,376,b,NG_INK,1);
  snprintf(b,sizeof b,"Stack sampled: %lu bytes",(unsigned long)(d->stack_base-d->stack_low));ng_text(c,10,132,b,NG_INK,1);
  ng_small(c,10,159,"Samples only; -1 = unavailable. RAM ring: 96 events.",NG_MUTED);
  ng_small_fit(c,10,181,376,a->notice[0]?a->notice:"EXPORT writes one diagnostic file when requested.",NG_MUTED);
  const char *keys[6]={"","EXPORT","","","","OK"};soft(c,keys,false);return;
#endif
 }
 if(a->modal==NG_MODAL_RULES){rules(c,a);return;}
 if(a->modal==NG_MODAL_RECORDS) {
  const NgModule *m=ng_module(a->selected_id);
  const NgBest *b=&a->session.stats.best[a->record_mode][a->record_difficulty][a->record_assisted];
  ng_rect(c,0,0,396,204,NG_PAPER);header(c,m->short_name,"RECORDS");char text[80];
  snprintf(text,sizeof(text),"%s / %s",a->record_assisted?"ASSISTED":"UNASSISTED",levels[a->record_difficulty]);ng_text(c,12,34,text,NG_BLUE,1);
  ng_text(c,12,54,m->mode_name?m->mode_name(a->record_mode):"STANDARD",NG_MUTED,1);
  snprintf(text,sizeof(text),"FINISHED %lu   W %lu / L %lu / D %lu",(unsigned long)b->completed,(unsigned long)b->wins,(unsigned long)b->losses,(unsigned long)b->draws);ng_wrap(c,12,81,372,13,2,text,NG_INK,false);
  if(b->completed) {
   if(a->selected_id==26)snprintf(text,sizeof(text),"BEST SCORE %lu / TILE %lu",(unsigned long)b->best_score,tile_value(b->best_aux));
   else if(a->selected_id>=21 && a->selected_id<=25)snprintf(text,sizeof(text),"Completed games by outcome above");
   else if(a->selected_id==29 || a->selected_id==30)snprintf(text,sizeof(text),"BEST SCORE %lu",(unsigned long)b->best_score);
   else snprintf(text,sizeof(text),"BEST %s %lu",a->selected_id<=5?"TRIES":"MOVES",(unsigned long)b->best_moves);
   ng_text_fit(c,12,113,372,text,NG_INK,1);
   if(b->wins){snprintf(text,sizeof(text),"FASTEST CLEAR %lu.%03lu s",(unsigned long)(b->best_ms/1000),(unsigned long)(b->best_ms%1000));ng_text_fit(c,12,139,372,text,NG_INK,1);}
  }else ng_text(c,12,113,"No finished run in this category.",NG_MUTED,1);
  ng_small(c,12,182,"Separate records per game, mode, difficulty and assistance.",NG_MUTED);
  const char *keys[6]={"NORMAL","ASSIST",m->modes>1?"MODE":"","LEVEL","","OK"};soft(c,keys,false);return;
 }
 if(a->modal==NG_MODAL_PAUSE)ng_rect(c,0,25,396,179,NG_PAPER);
 int x=61,y=63,w=274,h=104;
 if(a->modal==NG_MODAL_RESULT){x=27;y=40;w=342;h=150;}
 ng_rect(c,x+3,y+3,w,h,NG_MUTED);ng_rect(c,x,y,w,h,NG_WHITE);ng_border(c,x,y,w,h,NG_INK,2);
 ng_rect(c,x+2,y+2,w-4,4,NG_BLUE);
 const char *title="",*line="",*sub="";
 switch(a->modal){
 case NG_MODAL_EVICT:{
  const char *keys[6]={"","","","","","YES"};soft(c,keys,false);
  unsigned old=a->settings.recent[NG_RECENT_LIMIT-1];
  title="REMOVE OLDEST SAVE?";line=ng_module(old)?ng_module(old)->short_name:"OLD GAME";
  sub="EXE: REPLACE   EXIT: CANCEL";break;
 }
 case NG_MODAL_INIT:case NG_MODAL_NEW:{
  const char *keys[6]={"","","","","","YES"};soft(c,keys,false);
  title=a->modal==NG_MODAL_INIT?"Restart this game?":"Start a new game?";line="EXE: YES";sub="EXIT: NO";break;
 }
 case NG_MODAL_PAUSE:{
  const char *keys[6]={"","","","RESUME","",""};soft(c,keys,false);
  title="PAUSED";line="EXE: CONTINUE";sub="Active timer stopped";break;
 }
 case NG_MODAL_SAVE_ERROR:{
  const char *keys[6]={"","","","","","RETRY"};soft(c,keys,false);
  title="SAVE FAILED";line="EXE: RETRY";sub="EXIT: KEEP PLAYING";break;
 }
 case NG_MODAL_RESULT: {
  const char *result_keys[6]={"","","","","RULES","NEW"};soft(c,result_keys,false);
  const NgGame *g=&a->session.game;title=g->status==NG_WON?"COMPLETE":g->status==NG_LOST?"GAME OVER":"DRAW";
  if(g->id==7 && g->status==NG_DRAW)title="FINISHED";
  if(g->id>=21 && g->id<=25)title=g->status==NG_DRAW?"DRAW":g->mode==2?(g->turn==0?"PLAYER 1 WINS":"PLAYER 2 WINS"):(g->status==NG_WON?"YOU WIN":"CPU WINS");
  if(g->id==37)title=g->status==NG_DRAW?"DRAW":g->status==NG_WON?"YOU WIN":"CPU WINS";
  ng_center(c,x+12,y+17,w-24,title,NG_INK,2);
  char b[96];
  if(g->id<=5)snprintf(b,sizeof(b),"TRIES %lu    %s",(unsigned long)g->moves,g->assisted?"ASSISTED":"NORMAL");
  else if(g->id>=21 && g->id<=25)snprintf(b,sizeof(b),"TURNS %lu    %s / %s",(unsigned long)g->moves,levels[g->difficulty],g->assisted?"ASSISTED":"NORMAL");
  else if(g->id==37)snprintf(b,sizeof(b),"TURNS %lu    %s / %s",(unsigned long)g->moves,levels[g->difficulty],g->assisted?"ASSISTED":"UNASSISTED");
  else if(g->id==29)snprintf(b,sizeof(b),"CORRECT %ld/%lu    SCORE %lu",(long)g->data[5],(unsigned long)g->moves,(unsigned long)g->score);
  else if(g->id==30)snprintf(b,sizeof(b),"ROUNDS %lu/20    SCORE %lu",(unsigned long)g->moves,(unsigned long)g->score);
  else snprintf(b,sizeof(b),"MOVES %lu    SCORE %lu    %s",(unsigned long)g->moves,(unsigned long)g->score,g->assisted?"ASSISTED":"NORMAL");
  ng_center(c,x+12,y+48,w-24,b,NG_MUTED,1);
  char first[80]="",second[80]="";
  if(g->id==30) {
   char actual[32];unsigned count=(unsigned)g->data[0];if(count>20)count=20;
   for(unsigned i=0;i<count;i++)actual[i]=(char)('0'+g->board[i]);
   actual[count]=0;
   snprintf(first,sizeof(first),"ACTUAL: %s",actual);snprintf(second,sizeof(second),"YOURS: %.40s",g->input);
  }
  if(g->id==26) {
   snprintf(first,sizeof(first),"MAX TILE %lu",tile_value((uint32_t)g->data[0]));
   snprintf(second,sizeof(second),"SCORE %lu",(unsigned long)g->score);
  }
  if(g->id==26 || g->id==30){ng_center(c,x+12,y+72,w-24,first,NG_INK,1);ng_center(c,x+12,y+87,w-24,second,NG_INK,1);}
  else if(arithmetic_message(g->id))ng_wrap_expression(c,x+12,y+70,w-24,12,3,g->message,NG_INK,false);
  else ng_wrap(c,x+12,y+70,w-24,12,3,g->message,NG_INK,false);
  ng_center(c,x,y+111,w,"EXE: NEW GAME",NG_BLUE,1);ng_center(c,x,y+129,w,"EXIT: VIEW RESULT",NG_INK,1);return;
 }
 default:break;
 }
 ng_center(c,x,y+22,w,title,NG_INK,1);ng_center(c,x,y+51,w,line,NG_BLUE,1);ng_center(c,x,y+76,w,sub,NG_INK,1);
}
void ng_render(const NgApp *a,NgCanvas *c)
{
 ng_rect(c,0,0,396,224,NG_PAPER);
 switch(a->screen){case NG_MAIN:case NG_CATEGORY:menu(c,a);break;case NG_ENTRY:entry(c,a);break;
 case NG_PLAY:play(c,a);break;case NG_STATS:stats(c,a);break;case NG_SETTINGS:settings(c,a);break;}
 if(a->notice[0] && a->screen==NG_MAIN)ng_small_fit(c,8,196,380,a->notice,NG_RED);
 if(a->modal)dialog(c,a);
}
