#include "support.h"
#include "test_guesscalc_workflow.h"
static TestDisk disk;
static NgApp app,cold;
static void app_key(void *context,int key){tap(context,key);}
static void master_lifecycles(void)
{
 for(unsigned id=1;id<=10;id++)for(unsigned mode=0;mode<ng_module(id)->modes;mode++){
  app.settings.difficulty[id-1]=NG_MASTER;app.settings.mode[id-1]=(uint8_t)mode;app.settings_dirty=true;
  open_game(&app,id);assert(app.session.game.difficulty==NG_MASTER&&app.session.game.mode==mode);
  NgGame initial=app.session.game;NgBest hard[2];memcpy(hard,app.session.stats.best[mode][NG_HARD],sizeof(hard));
  NgSupply supply=app.session.supply[mode][NG_MASTER];
  tap(&app,id==8?'+':id==9?'5':'1');ng_app_tick(&app,3210);NgGame expected=app.session.game;
  assert(ng_checkpoint(&app));ng_app_init(&cold,test_hooks(&disk),1500+id*10+mode);
  tap(&cold,'1'+(int)((id-1)/5));tap(&cold,'1'+(int)((id-1)%5));assert(cold.screen==NG_ENTRY);
  assert(ng_entry_action(&cold,cold.entry_selection)==NG_ENTRY_NEW);
  tap(&cold,'1'+(int)ng_entry_row(&cold,NG_ENTRY_RESUME));tap(&cold,NGK_F6);
  assert(cold.screen==NG_PLAY&&!memcmp(&cold.session.game,&expected,sizeof(expected)));
  assert(!memcmp(&cold.session.supply[mode][NG_MASTER],&supply,sizeof(supply)));
  tap(&cold,NGK_F1);assert(cold.modal==NG_MODAL_INIT);tap(&cold,NGK_EXE);
  assert(cold.session.game.puzzle_id==initial.puzzle_id&&cold.session.game.seed==initial.seed);
  assert(cold.session.game.run_id==initial.run_id&&cold.session.game.pack_revision==2&&cold.session.game.assisted);
  assert(!memcmp(cold.session.game.board,initial.board,sizeof(initial.board)));
  assert(!memcmp(cold.session.game.data,initial.data,sizeof(initial.data)));
  gc_test_solve(&cold.session.game,&cold,app_key);
  assert(cold.modal==NG_MODAL_RESULT&&cold.session.game.status==NG_WON&&cold.session.game.recorded);
  assert(cold.session.stats.best[mode][NG_MASTER][1].wins>=1);
  assert(!memcmp(hard,cold.session.stats.best[mode][NG_HARD],sizeof(hard)));
  NgIO io=test_io(&disk);static NgSession recovered;assert(ng_load_io(&recovered,id,&io)==NG_LOAD_OK);
  assert(recovered.game.difficulty==NG_MASTER&&recovered.game.mode==mode&&recovered.game.status==NG_WON);
  assert(!memcmp(&recovered.game,&cold.session.game,sizeof(recovered.game)));
  app=cold;
 }
}

static void master_bank_cycles(void)
{
 const unsigned ids[]={2,3,4,5,6,7,8,9};
 for(unsigned family=0;family<sizeof(ids)/sizeof(*ids);family++){
  unsigned id=ids[family];app.settings.difficulty[id-1]=NG_MASTER;app.settings.mode[id-1]=0;app.settings_dirty=true;
  open_game(&app,id);NgSupply *bag=&app.session.supply[0][NG_MASTER];
  /* Restart a known complete cycle; this changes the test fixture only. */
  memset(bag,0,sizeof(*bag));app.dirty=true;assert(ng_checkpoint(&app));
  unsigned seen[30]={0};uint32_t previous=UINT32_MAX;
  for(unsigned i=0;i<35;i++){
   open_game(&app,id);unsigned base=id==2?270:id==6?180:id==8?0:90,index=app.session.game.puzzle_id-base;
   assert(index<30&&app.session.game.puzzle_id!=previous);
   if(i<30)assert(!seen[index]++);
   else for(unsigned recent=1;recent<app.session.supply[0][NG_MASTER].recent_count;recent++)
    assert(app.session.game.puzzle_id!=app.session.supply[0][NG_MASTER].recent[recent]);
   previous=app.session.game.puzzle_id;
  }
  for(unsigned i=0;i<30;i++)assert(seen[i]==1);
 }
}
static void lifecycle_drafts(void)
{
 for(unsigned id=1;id<=10;id++){
  open_game(&app,id);
  if(id==9){
   for(unsigned i=0;i<9;i++)if(!app.session.game.fixed[i]){
    while(app.session.game.cursor/3!=i/3)tap(&app,NGK_DOWN);
    while(app.session.game.cursor%3!=i%3)tap(&app,NGK_RIGHT);
    break;
   }
  }
  tap(&app,id==8?'+':id==9?'5':'1');
  ng_app_tick(&app,3210);NgGame expected=app.session.game;
  tap(&app,NGK_F5);assert(app.modal==NG_MODAL_RULES);
  ng_app_tick(&app,10000);assert(app.session.game.elapsed_ms==expected.elapsed_ms);
  tap(&app,NGK_EXIT);tap(&app,NGK_EXIT);assert(app.screen==NG_ENTRY);
  unsigned writes=disk.saves;tap(&app,NGK_F4);assert(app.modal==NG_MODAL_RECORDS);
  tap(&app,NGK_EXIT);assert(disk.saves==writes);
  open_game(&app,id==10?1:id+1);
  unsigned menus=disk.menu,offs=disk.off;tap(&app,NGK_MENU);assert(disk.menu==menus+1);
  tap(&app,NGK_SHIFT);tap(&app,NGK_ACON);assert(disk.off==offs+1);
  ng_app_init(&cold,test_hooks(&disk),900+id);
  tap(&cold,'1'+(int)((id-1)/5));tap(&cold,'1'+(int)((id-1)%5));assert(cold.screen==NG_ENTRY);
  assert(ng_entry_action(&cold,cold.entry_selection)==NG_ENTRY_NEW);
  NgSettings preferences=cold.settings;uint32_t seed=cold.seed;
  /* Digits select a row. Settings-row OPEN requests NEW, never RESUME. */
  const int settings_rows[]={NG_ENTRY_LEVEL,NG_ENTRY_MODE};
  for(unsigned row=0;row<2;row++){
   if(row==1&&ng_module(id)->modes<2)continue;
   tap(&cold,'1'+(int)ng_entry_row(&cold,settings_rows[row]));
   assert(cold.screen==NG_ENTRY&&!cold.modal);
   assert(ng_entry_action(&cold,cold.entry_selection)==settings_rows[row]);
   assert(!memcmp(&cold.settings,&preferences,sizeof(preferences)));
   assert(!memcmp(&cold.session.game,&expected,sizeof(expected))&&cold.seed==seed);
   tap(&cold,row?NGK_F6:NGK_EXE);assert(cold.modal==NG_MODAL_NEW&&cold.screen==NG_ENTRY);
   assert(!memcmp(&cold.settings,&preferences,sizeof(preferences)));
   assert(!memcmp(&cold.session.game,&expected,sizeof(expected))&&cold.seed==seed);
   tap(&cold,NGK_EXIT);assert(!cold.modal&&cold.screen==NG_ENTRY);
  }
  tap(&cold,'1'+(int)ng_entry_row(&cold,NG_ENTRY_RESUME));
  assert(cold.screen==NG_ENTRY&&!cold.modal&&ng_entry_action(&cold,cold.entry_selection)==NG_ENTRY_RESUME);
  assert(!memcmp(&cold.session.game,&expected,sizeof(expected))&&cold.seed==seed);
  tap(&cold,id%2?NGK_EXE:NGK_F6);assert(cold.screen==NG_PLAY&&cold.session.game.id==id);
  assert(!memcmp(&cold.session.game,&expected,sizeof(expected)));assert(ng_valid(&cold.session.game));
 }
}

int main(void)
{
 disk.write_budget=-1;ng_app_init(&app,test_hooks(&disk),539);
 /* Explicit unrelated CLASSIC setting keeps this fixture independent of 2048 modes. */
 app.settings.mode[25]=0;
 open_game(&app,8);assert(app.session.game.moves==0);
 tap(&app,'+');assert(app.session.game.notes[0]=='+');
 uint32_t moves=app.session.game.moves;tap(&app,'+');assert(app.session.game.moves==moves);
 tap(&app,NGK_DEL);assert(!app.session.game.notes[0]);
 tap(&app,NGK_F2);assert(app.session.game.notes[0]=='+');assert(app.session.game.assisted);
 open_game(&app,8);assert(!app.session.game.moves);
 tap(&app,NGK_F4);assert(app.session.game.notes[0]&&app.session.game.moves==1&&app.session.game.assisted);
 tap(&app,NGK_F1);assert(app.modal==NG_MODAL_INIT);tap(&app,NGK_EXIT);
 tap(&app,NGK_F2);assert(!app.session.game.notes[0]&&app.session.game.assisted);
 open_game(&app,9);
 for(unsigned i=0;i<9;i++)if(!app.session.game.fixed[i]){
  while(app.session.game.cursor/3!=i/3)tap(&app,NGK_DOWN);
  while(app.session.game.cursor%3!=i%3)tap(&app,NGK_RIGHT);
  break;
 }
 unsigned cursor=app.session.game.cursor;assert(!app.session.game.fixed[cursor]&&!app.session.game.board[cursor]);
 tap(&app,NGK_F4);assert(app.session.game.board[cursor]&&app.session.game.moves==1&&app.session.game.assisted);
 tap(&app,NGK_F1);assert(app.modal==NG_MODAL_INIT);tap(&app,NGK_EXIT);
 tap(&app,NGK_F2);assert(!app.session.game.board[cursor]&&app.session.game.assisted);
 tap(&app,'5');moves=app.session.game.moves;tap(&app,'5');assert(app.session.game.moves==moves);
 tap(&app,NGK_DEL);assert(!app.session.game.board[cursor]);tap(&app,NGK_F2);assert(app.session.game.board[cursor]==5);
 lifecycle_drafts();
 master_lifecycles();master_bank_cycles();
 puts("GUESS/CALC app: old10 workflows, all18 MASTER mode save/RESUME/INIT/result/stats, eight35-run bank cycles PASS");return 0;
}
