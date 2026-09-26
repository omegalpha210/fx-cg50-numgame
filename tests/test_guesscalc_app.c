#include "support.h"
#include "test_guesscalc_workflow.h"
static TestDisk disk;
static NgApp app,cold;
static NgSession recovered;
static int load_state(void *context,NgSettings *settings,NgSession *session,bool *active)
{NgIO io=test_io(context);return ng_state_load_io(settings,session,active,&io);}
static bool save_state(void *context,NgSettings *settings,const NgSession *session,bool active)
{NgIO io=test_io(context);return ng_state_save_io(settings,session,active,&io);}
static NgHooks combined_hooks(void)
{NgHooks hooks=test_hooks(&disk);hooks.load_state=load_state;hooks.save_state=save_state;return hooks;}
static void app_key(void *context,int key){tap(context,key);}
static void main_screen(NgApp *a)
{
 while(a->modal)tap(a,NGK_EXIT);
 while(a->screen!=NG_MAIN){tap(a,NGK_EXIT);assert(!a->modal);}
}
static void enter_game(NgApp *a,unsigned id)
{
 main_screen(a);tap(a,'1'+ng_catalog_index(id)/6);tap(a,'1'+ng_catalog_index(id)%6);
 assert(a->screen==NG_ENTRY&&a->selected_id==id);
}
static bool resume_row(const NgApp *a)
{for(unsigned row=0;row<ng_entry_count(a);row++)if(ng_entry_action(a,row)==NG_ENTRY_RESUME)return true;return false;}
static void read_durable(bool expected_active,unsigned expected_id)
{
 NgSettings settings;bool active=false;NgIO io=test_io(&disk);
 assert(ng_state_load_io(&settings,&recovered,&active,&io)==NG_LOAD_OK);
 assert(active==expected_active);
 if(active)assert(recovered.game.id==expected_id&&recovered.game.status==NG_PLAYING&&ng_valid(&recovered.game));
 else assert(!recovered.game.id);
}
static void frozen_result(NgApp *a)
{
 assert(a->modal==NG_MODAL_RESULT&&!a->resumable);NgGame final=a->session.game;
 /* A held EXIT closes only the modal, never both levels in one gesture. */
 ng_app_event(a,NGK_EXIT,NG_DOWN);assert(a->screen==NG_PLAY&&!a->modal&&a->result_view);
 ng_app_event(a,NGK_EXIT,NG_HOLD);assert(a->screen==NG_PLAY&&a->result_view);
 ng_app_event(a,NGK_EXIT,NG_UP);test_render(a);
 const int blocked[]={'0','9','+',NGK_EXE,NGK_DEL,NGK_UP,NGK_RIGHT,NGK_DOWN,NGK_LEFT,NGK_F1,NGK_F2,NGK_F3,NGK_F4};
 for(unsigned i=0;i<sizeof(blocked)/sizeof(*blocked);i++){tap(a,blocked[i]);assert(!memcmp(&a->session.game,&final,sizeof(final)));assert(a->screen==NG_PLAY&&!a->modal);}
 ng_app_tick(a,10000);assert(!memcmp(&a->session.game,&final,sizeof(final)));
 unsigned menus=disk.menu,offs=disk.off;tap(a,NGK_MENU);tap(a,NGK_SHIFT);tap(a,NGK_ACON);
 assert(disk.menu==menus+1&&disk.off==offs+1&&!memcmp(&a->session.game,&final,sizeof(final)));
}
static void lifecycle_drafts(void)
{
 for(unsigned id=1;id<=10;id++){
  open_game(&app,id);
  if(id==9){
   for(unsigned i=0;i<9;i++)if(!app.session.game.fixed[i]){app.session.game.cursor=(uint8_t)i;break;}
  }
  tap(&app,id==8?'+':id==9?'5':'1');ng_app_tick(&app,3210);NgGame expected=app.session.game;
  tap(&app,NGK_F5);assert(app.modal==NG_MODAL_RULES);ng_app_tick(&app,10000);assert(app.session.game.elapsed_ms==expected.elapsed_ms);
  tap(&app,NGK_EXIT);tap(&app,NGK_EXIT);assert(app.screen==NG_ENTRY&&resume_row(&app));
  assert(ng_entry_action(&app,app.entry_selection)==NG_ENTRY_RESUME);
  unsigned writes=disk.saves;tap(&app,NGK_F4);assert(!app.modal&&disk.saves==writes);
  enter_game(&app,id==10?1:id+1);assert(!resume_row(&app)&&ng_entry_action(&app,app.entry_selection)==NG_ENTRY_NEW);
  assert(app.resumable&&!memcmp(&app.session.game,&expected,sizeof(expected)));
  main_screen(&app);tap(&app,NGK_F1);assert(app.screen==NG_PLAY&&app.session.game.id==id);
  assert(!memcmp(&app.session.game,&expected,sizeof(expected)));assert(ng_checkpoint(&app));read_durable(true,id);
  assert(!memcmp(&recovered.game,&expected,sizeof(expected)));
  ng_app_init(&cold,combined_hooks(),900+id);assert(cold.resumable&&cold.screen==NG_MAIN);
  enter_game(&cold,id);assert(resume_row(&cold)&&ng_entry_action(&cold,cold.entry_selection)==NG_ENTRY_RESUME);
  NgSettings preferences=cold.settings;uint32_t seed=cold.seed;
  tap(&cold,'1'+(int)ng_entry_row(&cold,NG_ENTRY_RESUME));assert(cold.seed==seed&&!memcmp(&cold.session.game,&expected,sizeof(expected)));
  tap(&cold,id%2?NGK_EXE:NGK_F6);assert(cold.screen==NG_PLAY&&!memcmp(&cold.session.game,&expected,sizeof(expected)));
  /* OPEN on a setting row starts a fresh run without changing that setting. */
  const int settings_rows[]={NG_ENTRY_LEVEL,NG_ENTRY_MODE};
  for(unsigned row=0;row<2;row++){
   if(row==1&&ng_module(id)->modes<2)continue;
   tap(&cold,NGK_EXIT);tap(&cold,'1'+(int)ng_entry_row(&cold,settings_rows[row]));
   NgGame previous=cold.session.game;assert(ng_entry_action(&cold,cold.entry_selection)==settings_rows[row]);
   tap(&cold,row?NGK_F6:NGK_EXE);assert(cold.screen==NG_PLAY&&!cold.modal&&cold.resumable);
   assert(cold.session.game.seed!=previous.seed&&cold.session.game.run_id!=previous.run_id);
   assert(cold.settings.difficulty[id-1]==preferences.difficulty[id-1]&&cold.settings.mode[id-1]==preferences.mode[id-1]);
  }
  app=cold;
 }
}
static void master_lifecycles(void)
{
 for(unsigned id=1;id<=10;id++)for(unsigned mode=0;mode<ng_module(id)->modes;mode++){
  app.settings.difficulty[id-1]=NG_MASTER;app.settings.mode[id-1]=(uint8_t)mode;app.settings_dirty=true;
  open_game(&app,id);assert(app.session.game.difficulty==NG_MASTER&&app.session.game.mode==mode);
  NgGame initial=app.session.game;NgSupply supply=app.session.supply[mode][NG_MASTER];
  tap(&app,id==8?'+':id==9?'5':'1');ng_app_tick(&app,3210);NgGame expected=app.session.game;
  assert(ng_checkpoint(&app));ng_app_init(&cold,combined_hooks(),1500+id*10+mode);tap(&cold,NGK_F1);
  assert(cold.screen==NG_PLAY&&!memcmp(&cold.session.game,&expected,sizeof(expected)));
  assert(!memcmp(&cold.session.supply[mode][NG_MASTER],&supply,sizeof(supply)));
  tap(&cold,NGK_F1);assert(cold.modal==NG_MODAL_INIT);tap(&cold,NGK_EXE);
  assert(cold.session.game.puzzle_id==initial.puzzle_id&&cold.session.game.seed==initial.seed);
  assert(cold.session.game.run_id==initial.run_id&&cold.session.game.pack_revision==initial.pack_revision&&cold.session.game.assisted);
  assert(!memcmp(cold.session.game.board,initial.board,sizeof(initial.board))&&!memcmp(cold.session.game.data,initial.data,sizeof(initial.data)));
  gc_test_solve(&cold.session.game,&cold,app_key);assert(cold.modal==NG_MODAL_RESULT&&cold.session.game.status==NG_WON&&cold.session.game.recorded);
  assert(!cold.resumable);read_durable(false,0);ng_app_init(&app,combined_hooks(),5000+id);
  assert(!app.resumable);tap(&app,NGK_F1);assert(app.screen==NG_MAIN);
  frozen_result(&cold);NgGame completed=cold.session.game;
  tap(&cold,NGK_F6);assert(cold.screen==NG_PLAY&&cold.resumable&&!cold.result_view&&!cold.modal);
  assert(cold.session.game.id==id&&cold.session.game.difficulty==NG_MASTER&&cold.session.game.mode==mode);
  assert(cold.session.game.seed!=completed.seed&&cold.session.game.run_id!=completed.run_id);
  if(ng_bank_count(id,NG_MASTER,mode)>1)assert(cold.session.game.puzzle_id!=completed.puzzle_id);
  app=cold;
 }
}
static void master_bank_cycles(void)
{
 const unsigned ids[]={2,3,4,5,7,8,9,34};
 for(unsigned family=0;family<sizeof(ids)/sizeof(*ids);family++){
  unsigned id=ids[family];app.settings.difficulty[id-1]=NG_MASTER;app.settings.mode[id-1]=0;app.settings_dirty=true;
  open_game(&app,id);NgSupply *bag=&app.session.supply[0][NG_MASTER];
  memset(bag,0,sizeof(*bag));app.dirty=true;assert(ng_checkpoint(&app));
  unsigned seen[30]={0};uint32_t previous=UINT32_MAX;
  for(unsigned i=0;i<65;i++){
   open_game(&app,id);unsigned base=id==2?270:id==8?0:90,index=app.session.game.puzzle_id-base;
   assert(index<30&&app.session.game.puzzle_id!=previous);
   if(i%30==0)memset(seen,0,sizeof(seen));
   assert(!seen[index]++);
   if(i%30==29)for(unsigned n=0;n<30;n++)assert(seen[n]==1);
   previous=app.session.game.puzzle_id;
   if(i==12||i==42){NgSupply expected=app.session.supply[0][NG_MASTER];ng_app_init(&cold,combined_hooks(),4000+i);tap(&cold,NGK_F1);assert(cold.screen==NG_PLAY);assert(!memcmp(&cold.session.supply[0][NG_MASTER],&expected,sizeof(expected)));app=cold;}
  }
 }
}
static void undo_and_extra_flows(void)
{
 open_game(&app,8);tap(&app,'+');uint32_t moves=app.session.game.moves;tap(&app,'+');assert(app.session.game.moves==moves);
 tap(&app,NGK_DEL);assert(!app.session.game.notes[0]);tap(&app,NGK_F2);assert(app.session.game.notes[0]=='+'&&app.session.game.assisted);
 open_game(&app,8);tap(&app,NGK_F4);assert(app.session.game.notes[0]&&app.session.game.moves==1&&app.session.game.assisted);
 tap(&app,NGK_F1);assert(app.modal==NG_MODAL_INIT);tap(&app,NGK_EXIT);tap(&app,NGK_F2);assert(!app.session.game.notes[0]);
 open_game(&app,9);for(unsigned i=0;i<9;i++)if(!app.session.game.fixed[i]){app.session.game.cursor=(uint8_t)i;break;}
 unsigned cursor=app.session.game.cursor;tap(&app,NGK_F4);assert(app.session.game.board[cursor]&&app.session.game.moves==1);
 tap(&app,NGK_F2);assert(!app.session.game.board[cursor]);tap(&app,'5');tap(&app,NGK_DEL);tap(&app,NGK_F2);assert(app.session.game.board[cursor]==5);
 open_game(&app,33);tap(&app,'1');tap(&app,NGK_F2);assert(!app.session.game.board[0]&&app.session.game.assisted);
 tap(&app,NGK_F4);tap(&app,NGK_EXE);assert(app.session.game.data[8]);NgGame expected=app.session.game;
 assert(ng_checkpoint(&app));read_durable(true,33);assert(!memcmp(&recovered.game,&expected,sizeof(expected)));
 tap(&app,NGK_F4);for(unsigned i=0;i<(unsigned)app.session.game.rows*app.session.game.cols;i++){app.session.game.cursor=(uint8_t)i;tap(&app,app.session.game.fixed[i]?'1':'0');}
 tap(&app,NGK_F6);assert(app.modal==NG_MODAL_RESULT&&app.session.game.status==NG_WON);frozen_result(&app);
 tap(&app,NGK_EXIT);assert(app.screen==NG_ENTRY&&!resume_row(&app)&&ng_entry_action(&app,app.entry_selection)==NG_ENTRY_NEW);
 open_game(&app,34);tap(&app,'2');tap(&app,NGK_DEL);tap(&app,NGK_F2);assert(app.session.game.board[0]==2&&app.session.game.assisted);
 tap(&app,NGK_F4);assert(app.session.game.cursor==1);expected=app.session.game;assert(ng_checkpoint(&app));read_durable(true,34);
 assert(!memcmp(&recovered.game,&expected,sizeof(expected)));
}
static void completed_run_preferences(void)
{
 for(unsigned route=0;route<2;route++){
  app.settings.difficulty[33]=NG_EASY;app.settings.mode[33]=0;app.settings_dirty=true;open_game(&app,34);
  /* Deterministic public fixture AA+BC=BDB, then use only normal digit input. */
  NgSupply *bag=&app.session.supply[0][NG_EASY];memset(bag,0,sizeof(*bag));bag->count=30;bag->shuffle=30u<<16;app.dirty=true;
  open_game(&app,34);assert(app.session.game.puzzle_id==0&&app.session.game.difficulty==NG_EASY);
  tap(&app,NGK_EXIT);tap(&app,'1'+(int)ng_entry_row(&app,NG_ENTRY_LEVEL));
  for(unsigned step=0;step<3;step++)tap(&app,NGK_RIGHT);assert(app.settings.difficulty[33]==NG_MASTER);
  tap(&app,'1'+(int)ng_entry_row(&app,NG_ENTRY_RESUME));tap(&app,NGK_EXE);assert(app.session.game.difficulty==NG_EASY);
  const char solution[]="8130";for(unsigned i=0;i<4;i++){tap(&app,solution[i]);if(i<3)tap(&app,NGK_RIGHT);}
  tap(&app,NGK_F6);assert(app.modal==NG_MODAL_RESULT&&app.session.game.status==NG_WON&&!app.resumable);read_durable(false,0);
  if(route)frozen_result(&app);
  uint32_t previous=app.session.game.puzzle_id;tap(&app,route?NGK_F6:NGK_EXE);
  assert(app.screen==NG_PLAY&&!app.modal&&app.resumable&&app.session.game.id==34&&app.session.game.difficulty==NG_EASY&&app.session.game.mode==0);
  assert(app.session.game.puzzle_id!=previous);
 }
}
static void target_entry(void)
{
 app.settings.difficulty[5]=NG_MASTER;app.settings_dirty=true;open_game(&app,6);tap(&app,NGK_EXIT);
 tap(&app,'1'+(int)ng_entry_row(&app,NG_ENTRY_TARGET));assert(ng_entry_action(&app,app.entry_selection)==NG_ENTRY_TARGET&&!app.target_draft[0]);
 tap(&app,'1');tap(&app,'0');tap(&app,'0');tap(&app,'0');tap(&app,NGK_EXE);assert(app.screen==NG_ENTRY&&app.settings.target==1000&&!app.target_draft[0]);
 tap(&app,NGK_F6);assert(app.screen==NG_PLAY&&!app.modal&&app.session.game.data[0]==1000&&app.session.game.data[1]==6);
 NgGame expected=app.session.game;tap(&app,'1');tap(&app,NGK_F1);assert(app.modal==NG_MODAL_INIT);tap(&app,NGK_EXE);
 assert(app.session.game.data[0]==1000&&!memcmp(app.session.game.board,expected.board,sizeof(expected.board)));
 tap(&app,NGK_EXIT);tap(&app,'1'+(int)ng_entry_row(&app,NG_ENTRY_TARGET));tap(&app,'0');tap(&app,NGK_EXE);
 assert(app.screen==NG_ENTRY&&app.settings.target==1000&&!strcmp(app.target_draft,"0"));tap(&app,NGK_F6);assert(!app.modal&&app.screen==NG_ENTRY);
 tap(&app,NGK_DEL);tap(&app,'1');tap(&app,NGK_EXE);tap(&app,NGK_F6);assert(app.screen==NG_PLAY&&app.session.game.data[0]==1&&ng_valid(&app.session.game));
}
static void completed_mode_target(void)
{
 app.settings.difficulty[1]=NG_MASTER;app.settings.mode[1]=0;app.settings_dirty=true;open_game(&app,2);
 tap(&app,NGK_EXIT);tap(&app,'1'+(int)ng_entry_row(&app,NG_ENTRY_MODE));tap(&app,NGK_RIGHT);assert(app.settings.mode[1]==1);
 tap(&app,'1'+(int)ng_entry_row(&app,NG_ENTRY_RESUME));tap(&app,NGK_EXE);assert(app.session.game.mode==0);
 gc_test_solve(&app.session.game,&app,app_key);assert(app.modal==NG_MODAL_RESULT);tap(&app,NGK_EXE);
 assert(app.screen==NG_PLAY&&app.session.game.id==2&&app.session.game.mode==0&&app.session.game.difficulty==NG_MASTER);
 app.settings.target=1;app.settings.difficulty[5]=NG_MASTER;app.settings_dirty=true;open_game(&app,6);
 tap(&app,NGK_EXIT);tap(&app,'1'+(int)ng_entry_row(&app,NG_ENTRY_TARGET));
 tap(&app,'1');tap(&app,'0');tap(&app,'0');tap(&app,'0');tap(&app,NGK_EXE);assert(app.settings.target==1000);
 while(ng_entry_action(&app,app.entry_selection)!=NG_ENTRY_RESUME)tap(&app,NGK_UP);
 tap(&app,NGK_EXE);assert(app.session.game.data[0]==1);gc_test_solve(&app.session.game,&app,app_key);
 assert(app.modal==NG_MODAL_RESULT);tap(&app,NGK_EXIT);assert(app.result_view);tap(&app,NGK_F6);
 assert(app.screen==NG_PLAY&&app.session.game.id==6&&app.session.game.difficulty==NG_MASTER&&app.session.game.data[0]==1);
}
int main(void)
{
 disk.write_budget=-1;ng_app_init(&app,combined_hooks(),539);app.settings.mode[25]=0;
 lifecycle_drafts();master_lifecycles();master_bank_cycles();undo_and_extra_flows();completed_run_preferences();target_entry();completed_mode_target();
 puts("GUESS/CALC app: single-resume browsing/start, MAIN F1, cold restore, INIT, frozen results, eight65-run bank cycles, extra-game undo/save and target1/1000 PASS");return 0;
}
