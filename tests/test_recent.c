#include "support.h"

static TestDisk disk;
static NgApp app,cold;

static void main_menu(NgApp *a)
{
 while(a->modal)tap(a,NGK_EXIT);
 while(a->screen!=NG_MAIN)tap(a,NGK_EXIT);
}
static void entry(NgApp *a,unsigned id)
{
 main_menu(a);
 int index=ng_catalog_index(id);assert(index>=0);
 tap(a,'1'+index/6);tap(a,'1'+index%6);
 assert(a->screen==NG_ENTRY && a->selected_id==id);
}
static void test_one_resume(void)
{
 disk.write_budget=-1;ng_app_init(&app,test_hooks(&disk),9001);
 assert(!app.resumable);tap(&app,NGK_F1);assert(app.screen==NG_MAIN);
 open_game(&app,1);NgGame first=app.session.game;
 tap(&app,NGK_EXIT);assert(app.screen==NG_ENTRY);
 assert(ng_entry_action(&app,0)==NG_ENTRY_RESUME);
 tap(&app,NGK_EXIT);assert(app.screen==NG_CATEGORY);
 tap(&app,NGK_F1);assert(app.screen==NG_CATEGORY);
 tap(&app,NGK_EXIT);assert(app.screen==NG_MAIN);
 tap(&app,NGK_F1);assert(app.screen==NG_PLAY);
 assert(!memcmp(&app.session.game,&first,sizeof first));
 entry(&app,33);assert(ng_entry_action(&app,0)==NG_ENTRY_NEW);
 for(unsigned row=0;row<ng_entry_count(&app);row++)assert(ng_entry_action(&app,row)!=NG_ENTRY_RESUME);
 tap(&app,NGK_F6);assert(app.screen==NG_PLAY && !app.modal && app.session.game.id==33);
 assert(!disk.lengths[1][0] && !disk.lengths[33][0]);
 ng_app_init(&cold,test_hooks(&disk),42);assert(cold.resumable && cold.session.game.id==33);
 assert(!cold.settings.recent_count);
 tap(&cold,NGK_F1);assert(cold.screen==NG_PLAY && cold.session.game.id==33);
 entry(&cold,1);assert(ng_entry_action(&cold,0)==NG_ENTRY_NEW);
 entry(&cold,33);assert(ng_entry_action(&cold,0)==NG_ENTRY_RESUME);
 for(unsigned id=1;id<=36;id++){
  unsigned selected=ng_visible_id((id-1)%NG_GAME_COUNT);
  entry(&cold,selected);tap(&cold,NGK_F6);
  assert(cold.screen==NG_PLAY && cold.session.game.id==selected);
 }
 assert(disk.lengths[0][0] && disk.lengths[0][1]);
 for(unsigned id=1;id<=NG_ID_MAX;id++)assert(!disk.lengths[id][0] && !disk.lengths[id][1]);
 puts("Single RESUME: Main F1, same-game row, different-game START, cold load, two files after 36 switches PASS");
}
static void test_failed_replacement(void)
{
 memset(&disk,0,sizeof disk);disk.write_budget=-1;
 ng_app_init(&app,test_hooks(&disk),9011);open_game(&app,1);
 NgGame old=app.session.game;entry(&app,33);
 disk.write_budget=0;tap(&app,NGK_F6);
 assert(app.modal==NG_MODAL_SAVE_ERROR && app.screen==NG_ENTRY && app.start_failed);
 assert(app.resumable && !memcmp(&app.session.game,&old,sizeof old));
 ng_app_init(&cold,test_hooks(&disk),42);assert(cold.resumable && cold.session.game.id==1);
 disk.write_budget=-1;tap(&app,NGK_EXE);
 assert(app.screen==NG_PLAY && !app.modal && app.session.game.id==33);
 ng_app_init(&cold,test_hooks(&disk),43);assert(cold.resumable && cold.session.game.id==33);
 puts("Single RESUME: failed replacement preserves old valid run; retry commits new run PASS");
}
static void test_cycle(void)
{
 for(unsigned n=1;n<=5;n++)if(n==1 || n==2 || n==5)
  for(unsigned seed=1;seed<=101;seed++){
   NgGame g={.supply_seed=(seed<<16)|(seed%n)};bool seen[5]={0};
   for(unsigned index=0;index<n;index++){
    g.supply_index=index;unsigned ordinal=ng_bank_pick(&g,n);
    assert(ordinal<n && !seen[ordinal]);seen[ordinal]=true;
   }
  }
 memset(&disk,0,sizeof disk);disk.write_budget=-1;
 ng_app_init(&app,test_hooks(&disk),661);
 unsigned count=ng_bank_count(2,NG_NORMAL,0);assert(count==30);
 static uint32_t seen[4096];unsigned used=0;NgApp *current=&app;
 for(unsigned i=0;i<count;i++){
  open_game(current,2);uint32_t puzzle=current->session.game.puzzle_id;
  for(unsigned j=0;j<used;j++)assert(seen[j]!=puzzle);
  seen[used++]=puzzle;
  if(i==count/2){ng_app_init(&cold,test_hooks(&disk),997);
   assert(cold.resumable && cold.session.game.puzzle_id==puzzle);
   current=&cold;}
 }
 uint32_t last=seen[count-1];open_game(current,2);
 assert(current->session.game.puzzle_id!=last);
 puts("Puzzle cycles: N=1/2/5 coprime permutations; 30 unique app bank draws, cold continuation and no boundary repeat PASS");
}
int main(void){test_one_resume();test_failed_replacement();test_cycle();return 0;}
