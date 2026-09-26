#define NG_TEST_SUPPORT_IO_ONLY
#include "support.h"

static TestDisk disk;
static NgSettings settings,loaded_settings;
static NgSession session,loaded_session;
static bool active,loaded_active;

static void defaults(void)
{
 memset(&disk,0,sizeof disk);disk.write_budget=-1;
 memset(&settings,0,sizeof settings);memset(settings.difficulty,1,sizeof settings.difficulty);
 settings.mode[25]=1;settings.show_time=1;settings.target=24;
 memset(&session,0,sizeof session);
 active=false;
}
static void run(unsigned id,uint32_t seed)
{
 unsigned d=settings.difficulty[id-1],mode=settings.mode[id-1];
 memset(&session,0,sizeof session);
 ng_new(&session.game,id,d,mode,seed,1);
 assert(ng_valid(&session.game) && session.game.status==NG_PLAYING);
 session.stats.started=1;settings.last_game=(uint8_t)id;active=true;
}
static int load(void)
{
 NgIO io=test_io(&disk);return ng_state_load_io(&loaded_settings,&loaded_session,&loaded_active,&io);
}
static bool save(void)
{
 NgIO io=test_io(&disk);return ng_state_save_io(&settings,&session,active,&io);
}
static void basic(void)
{
 defaults();assert(load()==NG_LOAD_ABSENT);
 assert(save());assert(disk.lengths[0][0]==126);assert(!disk.lengths[0][1]);
 assert(save());assert(disk.lengths[0][1]==126);
 assert(load()==NG_LOAD_OK && !loaded_active && loaded_settings.show_time);
 run(1,1001);assert(save());size_t first=disk.lengths[0][0];
 assert(first==1998 && disk.lengths[0][1]==126);
 assert(load()==NG_LOAD_OK && loaded_active && loaded_session.game.id==1);
 assert(!loaded_session.supply[0][0].count);
 run(33,1002);assert(save());assert(load()==NG_LOAD_OK);
 assert(loaded_active && loaded_session.game.id==33 && loaded_settings.last_game==33);
 assert(disk.lengths[0][0]==first && disk.lengths[0][1]==1998);
 assert(!disk.lengths[1][0] && !disk.lengths[33][0]);
 active=false;assert(save());assert(load()==NG_LOAD_OK && !loaded_active);
 assert(disk.lengths[0][0]==126 && disk.lengths[0][1]==1998);
 puts("Single state: empty/active/completed exact lengths, A/B replacement, one logical resume PASS");
}
static void interruption(void)
{
 defaults();run(1,711);assert(save());assert(save());
 uint8_t old[NG_RECORD_MAX];size_t old_len=disk.lengths[0][1];
 memcpy(old,disk.bytes[0][1],old_len);
 run(33,712);disk.write_budget=100;assert(!save());disk.write_budget=-1;
 assert(load()==NG_LOAD_RECOVERED && loaded_active && loaded_session.game.id==1);
 assert(disk.lengths[0][1]==old_len && !memcmp(old,disk.bytes[0][1],old_len));
 assert(save());assert(load()==NG_LOAD_OK && loaded_session.game.id==33);
 disk.bytes[0][0][disk.lengths[0][0]-1]^=1;
 assert(load()==NG_LOAD_RECOVERED && loaded_session.game.id==1);
 assert(save());assert(load()==NG_LOAD_OK && loaded_session.game.id==33);
 puts("Single state: partial write, intact partner, corrupt newest recovery and retry PASS");
}
static void undo_and_cold(void)
{
 defaults();run(26,940);
 session.undo[0]=session.game;session.undo_count=1;
 assert(save() && save() && load()==NG_LOAD_OK);
 assert(loaded_active && loaded_session.undo_count==1);
 assert(!memcmp(&loaded_session.undo[0],&session.undo[0],sizeof(NgGame)));
 assert(disk.lengths[0][0]==3840 && disk.lengths[0][1]==3840);
 assert(ng_state_slot_valid(&(NgIO){&disk,test_open,test_read,test_write,test_close,NULL},0));
 puts("Single state: bounded undo and cold load PASS");
}
int main(void){basic();interruption();undo_and_cold();return 0;}
