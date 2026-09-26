#include "support.h"
#include "test_guesscalc_workflow.h"

typedef struct {unsigned count;int x,y;} Badge;
static TestDisk disk;
static NgApp app,cold;

static void badge_rect(void *ctx,int x,int y,int w,int h,uint16_t color)
{
 Badge *b=ctx;
 assert(x>=0 && y>=0 && w>0 && h>0 && x+w<=396 && y+h<=224);
 if(w==ng_small_width("RESUME")+12 && h==12 && color==NG_RGB(24,31,31)){
  b->count++;b->x=x;b->y=y;
 }
}
static Badge badges(const NgApp *a)
{Badge b={0};NgCanvas c={&b,badge_rect};ng_render(a,&c);return b;}
static void press(void *context,int key){tap(context,key);}
static void target_row(NgApp *a)
{a->entry_selection=(uint8_t)ng_entry_row(a,NG_ENTRY_TARGET);assert(ng_entry_action(a,a->entry_selection)==NG_ENTRY_TARGET);}
static void make_target_entry(void)
{
 memset(&disk,0,sizeof disk);disk.write_budget=-1;
 ng_app_init(&app,test_hooks(&disk),3434);
 tap(&app,'2');tap(&app,'1');
 assert(app.screen==NG_ENTRY && app.selected_id==6);
 target_row(&app);
}
static void test_target_selection(void)
{
 make_target_entry();assert(app.settings.target==24);
 tap(&app,NGK_LEFT);assert(app.settings.target==10);
 tap(&app,NGK_LEFT);assert(app.settings.target==10);
 static const unsigned choices[]={24,50,100,200,NG_TARGET_RANDOM};
 for(unsigned i=0;i<5;i++){tap(&app,NGK_RIGHT);assert(app.settings.target==choices[i]);}
 tap(&app,NGK_RIGHT);assert(app.settings.target==NG_TARGET_RANDOM);
 tap(&app,'0');tap(&app,'2');tap(&app,NGK_DEL);
 assert(app.settings.target==NG_TARGET_RANDOM && app.screen==NG_ENTRY);
 assert(ng_checkpoint(&app));ng_app_init(&cold,test_hooks(&disk),22);
 assert(cold.settings.target==NG_TARGET_RANDOM);
 tap(&app,NGK_EXE);
 assert(app.screen==NG_PLAY && app.session.game.id==6 && app.session.game.data[2]==1 &&
        app.session.game.data[4]==0);
 bool found=false;for(unsigned i=0;i<5;i++)if(app.session.game.data[0]==(int32_t)(unsigned[]){10,24,50,100,200}[i])found=true;
 assert(found);
 tap(&app,NGK_EXIT);assert(app.screen==NG_ENTRY);target_row(&app);
 tap(&app,NGK_LEFT);assert(app.settings.target==200);
 tap(&app,NGK_F6);
 assert(app.screen==NG_PLAY && app.session.game.data[0]==200 && app.session.game.data[2]==0 && app.session.game.data[4]==200);
 puts("Make Target: six clamped choices, numeric keys ignored, RANDOM persisted, TARGET EXE/F6 opens PASS");
}
static void test_badge(void)
{
 memset(&disk,0,sizeof disk);disk.write_budget=-1;
 ng_app_init(&app,test_hooks(&disk),5656);
 tap(&app,'1');assert(app.screen==NG_CATEGORY && !badges(&app).count);
 open_game(&app,1);tap(&app,NGK_EXIT);tap(&app,NGK_EXIT);
 Badge b=badges(&app);assert(b.count==1 && b.x==60 && b.y==64);
 tap(&app,NGK_EXIT);assert(app.screen==NG_MAIN && !badges(&app).count);
 ng_app_init(&cold,test_hooks(&disk),55);assert(cold.resumable && cold.session.game.id==1);
 tap(&cold,'1');b=badges(&cold);assert(b.count==1 && b.x==60 && b.y==64);
 open_game(&app,33);tap(&app,NGK_EXIT);tap(&app,NGK_EXIT);
 b=badges(&app);assert(b.count==1 && b.x==255 && b.y==176);
 tap(&app,NGK_EXIT);tap(&app,'2');assert(!badges(&app).count);
 open_game(&app,1);gc_test_solve(&app.session.game,&app,press);
 assert(app.modal==NG_MODAL_RESULT && !app.resumable);
 tap(&app,NGK_EXIT);tap(&app,NGK_EXIT);tap(&app,NGK_EXIT);
 assert(app.screen==NG_CATEGORY && !badges(&app).count);
 open_game(&app,33);
 for(unsigned slot=0;slot<2;slot++)if(disk.lengths[0][slot]>40)disk.bytes[0][slot][40]^=1;
 ng_app_init(&cold,test_hooks(&disk),66);assert(!cold.resumable);
 tap(&cold,'1');assert(!badges(&cold).count);
 puts("Game tile: exactly one secondary RESUME badge; cold load, replacement, completion and corrupt-save removal PASS");
}
int main(void){test_target_selection();test_badge();return 0;}
