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
static void test_target_editor(void)
{
 make_target_entry();assert(app.settings.target==24 && !app.target_editing);
 tap(&app,NGK_LEFT);assert(app.target_editing && app.target_cursor==0 && !strcmp(app.target_draft,"24"));
 tap(&app,'1');assert(app.target_cursor==1 && !strcmp(app.target_draft,"124"));
 tap(&app,NGK_RIGHT);assert(app.target_cursor==2);
 tap(&app,NGK_DEL);assert(app.target_cursor==1 && !strcmp(app.target_draft,"14"));
 tap(&app,NGK_EXE);assert(!app.target_editing && app.settings.target==14 && app.screen==NG_ENTRY);
 tap(&app,NGK_RIGHT);assert(app.target_editing && app.target_cursor==2 && !strcmp(app.target_draft,"14"));
 tap(&app,NGK_EXIT);assert(!app.target_editing && app.screen==NG_ENTRY && app.settings.target==14);
 tap(&app,'0');assert(app.target_editing && !strcmp(app.target_draft,"0"));
 tap(&app,NGK_EXE);assert(app.target_editing && app.notice[0] && app.settings.target==14);
 tap(&app,NGK_F6);assert(app.screen==NG_ENTRY && app.target_editing && app.settings.target==14);
 tap(&app,NGK_EXIT);assert(!app.target_editing && !app.notice[0] && app.settings.target==14);
 tap(&app,NGK_RIGHT);tap(&app,NGK_DEL);tap(&app,NGK_DEL);
 assert(app.target_editing && !app.target_draft[0] && app.target_cursor==0);
 tap(&app,NGK_EXE);assert(app.target_editing && app.notice[0] && app.settings.target==14);
 tap(&app,NGK_EXIT);assert(!app.target_editing && app.settings.target==14);
 tap(&app,'1');tap(&app,NGK_EXE);assert(!app.target_editing && app.settings.target==1);
 tap(&app,'1');tap(&app,'0');tap(&app,'0');tap(&app,'1');tap(&app,NGK_EXE);
 assert(app.target_editing && app.settings.target==1 && !strcmp(app.target_draft,"1001"));
 tap(&app,NGK_DEL);tap(&app,'0');tap(&app,NGK_EXE);
 assert(!app.target_editing && app.settings.target==1000 && app.screen==NG_ENTRY);
 assert(ng_checkpoint(&app));ng_app_init(&cold,test_hooks(&disk),22);assert(cold.settings.target==1000);
 tap(&app,NGK_EXE);
 assert(app.screen==NG_PLAY && app.session.game.id==6 && app.session.game.data[0]==1000);
 tap(&app,NGK_EXIT);assert(app.screen==NG_ENTRY);target_row(&app);
 tap(&app,NGK_F6);assert(app.screen==NG_PLAY && app.session.game.data[0]==1000);
 puts("Make Target: DIFF EQ-style insert/backspace cursor, invalid retain, EXIT cancel, F6 edit guard, EXE commit then EXE start, 1..1000 persistence PASS");
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
int main(void){test_target_editor();test_badge();return 0;}
