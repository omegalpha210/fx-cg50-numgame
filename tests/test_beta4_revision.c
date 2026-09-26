#include "support.h"
#include "guesscalc.h"

static TestDisk disk;
static NgApp app,cold;

static void legacy_run(unsigned id,unsigned difficulty,unsigned mode,unsigned revision,unsigned target)
{
 memset(&disk,0,sizeof disk);disk.write_budget=-1;
 ng_app_init(&app,test_hooks(&disk),UINT32_C(20260926));
 open_game(&app,id);
 ng_new_supply_version(&app.session.game,id,difficulty,mode,UINT32_C(971413),1,
                       UINT32_C(0x20003),2,revision);
 if(id==6 && revision>=3)assert(gc_target_init(&app.session.game,target));
 assert(ng_valid(&app.session.game));
 NgGame original=app.session.game;
 app.session.stats.started=1;
 app.active=app.resumable=app.dirty=true;
 app.screen=NG_PLAY;app.selected_id=(uint8_t)id;
 app.settings.target=(uint16_t)target;
 unsigned count=ng_bank_count_version(id,difficulty,mode,revision);
 app.session.supply[mode][difficulty].count=(uint16_t)count;
 app.session.supply[mode][difficulty].next=(uint16_t)(count>3?3:count);
 app.session.supply[mode][difficulty].shuffle=count?UINT32_C(0x10000):0;
 assert(ng_checkpoint(&app));
 ng_app_init(&cold,test_hooks(&disk),UINT32_C(77));
 assert(cold.resumable && cold.active);
 assert(cold.session.game.pack_revision==revision);
 assert(!memcmp(&cold.session.game,&original,sizeof original));
 cold.selected_id=(uint8_t)id;cold.screen=NG_ENTRY;
 cold.entry_selection=(uint8_t)ng_entry_row(&cold,NG_ENTRY_RESUME);
 tap(&cold,NGK_F6);
 assert(cold.screen==NG_PLAY && !memcmp(&cold.session.game,&original,sizeof original));
 tap(&cold,NGK_F1);
 if(cold.modal==NG_MODAL_INIT)tap(&cold,NGK_EXE);
 assert(!cold.modal && cold.session.game.pack_revision==revision);
 assert(cold.session.game.puzzle_id==original.puzzle_id);
 assert(!memcmp(cold.session.game.board,original.board,sizeof original.board));
 assert(ng_valid(&cold.session.game));
 tap(&cold,NGK_EXIT);
 assert(cold.screen==NG_ENTRY);
 cold.entry_selection=(uint8_t)ng_entry_row(&cold,NG_ENTRY_NEW);
 tap(&cold,NGK_F6);
 if(cold.modal==NG_MODAL_NEW)tap(&cold,NGK_EXE);
 assert(cold.screen==NG_PLAY && !cold.modal);
 assert(cold.session.game.pack_revision==4);
 assert(ng_valid(&cold.session.game));
 if(id==6)assert(cold.session.game.data[0]==(int32_t)target);
}

static void target_cycle_reset(void)
{
 memset(&disk,0,sizeof disk);disk.write_budget=-1;
 ng_app_init(&app,test_hooks(&disk),UINT32_C(42));
 open_game(&app,6);
 assert(app.session.game.pack_revision==4 && app.session.game.data[0]==24);
 assert(app.session.supply[0][NG_NORMAL].count==2 &&
        app.session.supply[0][NG_NORMAL].next==1);
 tap(&app,NGK_EXIT);
 app.settings.target=247;app.settings_dirty=true;
 app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_NEW);
 tap(&app,NGK_F6);
 assert(app.screen==NG_PLAY && app.session.game.data[0]==247);
 assert(app.session.supply[0][NG_NORMAL].count==2 &&
        app.session.supply[0][NG_NORMAL].next==1);
 uint32_t first=app.session.game.puzzle_id;
 assert(app.session.supply[0][NG_NORMAL].recent[0]==first);
 tap(&app,NGK_EXIT);
 app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_NEW);
 tap(&app,NGK_F6);
 assert(app.screen==NG_PLAY && app.session.game.data[0]==247);
 assert(app.session.game.puzzle_id!=first);
 assert(app.session.supply[0][NG_NORMAL].next==2);
 for(unsigned i=0;i<32;i++){
  uint32_t previous=app.session.game.puzzle_id;
  tap(&app,NGK_EXIT);
  app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_NEW);
  tap(&app,NGK_F6);
  assert(app.screen==NG_PLAY && app.session.game.data[0]==247);
  assert(app.session.game.puzzle_id!=previous);
  assert(app.session.supply[0][NG_NORMAL].recent[0]==app.session.game.puzzle_id);
 }
}

static void legacy_target_mode_result(void)
{
 memset(&disk,0,sizeof disk);disk.write_budget=-1;
 ng_app_init(&app,test_hooks(&disk),UINT32_C(12345));
 open_game(&app,6);
 ng_new_supply_version(&app.session.game,6,NG_NORMAL,1,UINT32_C(12345),1,
                       UINT32_C(0x10000),0,2);
 assert(ng_valid(&app.session.game) && app.session.game.data[0]==10);
 app.active=app.resumable=app.dirty=true;
 assert(ng_checkpoint(&app));
 ng_app_init(&cold,test_hooks(&disk),UINT32_C(77));
 assert(cold.resumable && cold.session.game.pack_revision==2 &&
        cold.session.game.mode==1);
 tap(&cold,NGK_F1);
 assert(cold.screen==NG_PLAY && cold.selected_id==6);
 cold.modal=NG_MODAL_RESULT;
 cold.session.game.status=NG_WON;
 cold.resumable=false;cold.dirty=true;
 tap(&cold,NGK_F6);
 assert(cold.screen==NG_PLAY && cold.modal==NG_MODAL_NONE);
 assert(cold.session.game.pack_revision==4 && cold.session.game.mode==0 &&
        cold.session.game.data[0]==10 && ng_valid(&cold.session.game));
 assert(cold.settings.mode[5]==0);
 assert(cold.session.supply[0][NG_NORMAL].count==2 &&
        cold.session.supply[0][NG_NORMAL].next==1 &&
        cold.session.supply[0][NG_NORMAL].shuffle!=0);
 assert(cold.session.supply[0][NG_NORMAL].recent[0]==cold.session.game.puzzle_id);
}

int main(void)
{
 for(unsigned d=0;d<2;d++)legacy_run(5,d,0,2,24);
 for(unsigned d=0;d<4;d++)legacy_run(6,d,0,3,247);
 legacy_run(7,NG_HARD,0,2,24);
 legacy_run(10,NG_MASTER,0,2,24);
 for(unsigned mode=0;mode<2;mode++)for(unsigned d=0;d<4;d++)
  legacy_run(27,d,mode,2,24);
 target_cycle_reset();
 legacy_target_mode_result();
 puts("beta.4 revisions: old active puzzles, cold RESUME, INIT and NEW migration PASS");
 return 0;
}
