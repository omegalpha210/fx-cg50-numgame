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
static void test_five_recent(void)
{
 const unsigned first[5]={1,2,3,4,5};
 disk.write_budget=-1;ng_app_init(&app,test_hooks(&disk),9001);
 for(unsigned i=0;i<5;i++){
  open_game(&app,first[i]);tap(&app,NGK_EXIT);
  assert(app.settings.recent_count==i+1 && app.settings.recent[0]==first[i]);
 }
 entry(&app,33);tap(&app,NGK_F6);
 assert(app.modal==NG_MODAL_EVICT && app.settings.recent[4]==1);
 tap(&app,NGK_EXIT);assert(!app.modal && app.settings.recent_count==5);
 tap(&app,NGK_F6);tap(&app,NGK_EXE);
 assert(app.screen==NG_PLAY && app.session.game.id==33);
 tap(&app,NGK_EXIT);
 assert(app.settings.recent_count==5 && app.settings.recent[0]==33);
 for(unsigned i=1;i<5;i++)assert(app.settings.recent[i]==6-i);
 assert(!disk.lengths[1][0] && !disk.lengths[1][1]);
 ng_app_init(&cold,test_hooks(&disk),42);
 assert(cold.settings.recent_count==5 && !cold.summary[0].exists && cold.summary[32].exists);
 for(unsigned i=0;i<5;i++)assert(cold.settings.recent[i]==app.settings.recent[i]);
 entry(&cold,5);assert(cold.active);tap(&cold,'1');tap(&cold,NGK_F6);
 assert(cold.screen==NG_PLAY && cold.session.game.id==5);
 assert(cold.settings.recent[0]==5);
 puts("Recent saves: sixth-game confirmation, five-entry LRU, deleted files and cold resume PASS");
}
static void test_failed_eviction(void)
{
 memset(&disk,0,sizeof disk);disk.write_budget=-1;
 ng_app_init(&app,test_hooks(&disk),9011);
 for(unsigned id=1;id<=5;id++){open_game(&app,id);tap(&app,NGK_EXIT);}
 entry(&app,33);tap(&app,NGK_F6);tap(&app,NGK_EXE);
 assert(app.screen==NG_PLAY);
 disk.write_budget=0;tap(&app,NGK_EXIT);
 assert(app.modal==NG_MODAL_SAVE_ERROR && disk.lengths[1][0]);
 assert(app.settings.recent_count==5 && app.settings.recent[4]==1);
 disk.write_budget=-1;tap(&app,NGK_EXE);
 assert(!app.modal && app.settings.recent[0]==33);
 assert(!disk.lengths[1][0] && !disk.lengths[1][1]);
 puts("Recent saves: failed sixth-game write preserves oldest save until retry PASS");
}
int main(void){test_five_recent();test_failed_eviction();return 0;}
