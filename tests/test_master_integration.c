#include "app.h"
#include "grids_internal.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "legacy_wire.h"
static uint8_t bytes[NG_RECORD_MAX],legacy[NG_RECORD_MAX];
static NgSession session,decoded;
static NgApp app;
static uint16_t pixels[224][396];
static void rect(void *ctx,int x,int y,int w,int h,uint16_t color)
{(void)ctx;assert(x>=0&&y>=0&&x+w<=396&&y+h<=224);for(int r=y;r<y+h;r++)for(int c=x;c<x+w;c++)pixels[r][c]=color;}
static void shot(unsigned id,const char *stage)
{(void)id;(void)stage;NgCanvas canvas={NULL,rect};ng_render(&app,&canvas);}
static void tap(int key){ng_app_event(&app,key,NG_DOWN);ng_app_event(&app,key,NG_UP);}
static void entry_workflow(unsigned id,int launch)
{
 ng_app_init(&app,(NgHooks){0},639);tap('3');tap('0'+(int)id-10);
 assert(app.screen==NG_ENTRY&&ng_entry_action(&app,app.entry_selection)==NG_ENTRY_NEW);tap(NGK_DOWN);
 assert(ng_entry_action(&app,app.entry_selection)==NG_ENTRY_LEVEL);
 tap(NGK_RIGHT);tap(NGK_RIGHT);tap(NGK_RIGHT);assert(app.settings.difficulty[id-1]==3);
 for(unsigned i=0;i<5;i++)tap(NGK_LEFT);
 assert(app.settings.difficulty[id-1]==0);
 for(unsigned i=0;i<3;i++)tap(NGK_RIGHT);
 shot(id,"master-entry");tap(launch);
 assert(app.screen==NG_PLAY&&!app.modal&&app.session.game.difficulty==3&&app.session.game.moves==0&&ng_valid(&app.session.game));
 shot(id,"master-play");tap(NGK_EXIT);tap(NGK_F4);assert(!app.modal);shot(id,"master-entry-resume");
 tap(NGK_F6);assert(app.modal==NG_MODAL_NEW&&app.session.game.difficulty==3);tap(NGK_EXIT);
 app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_RESUME);tap(NGK_F6);assert(app.screen==NG_PLAY&&app.session.game.difficulty==3);
 NgSettings copy;size_t n=ng_settings_encode(&app.settings,bytes,sizeof bytes);assert(n&&ng_settings_decode(&copy,bytes,n)&&copy.difficulty[id-1]==3);
}
static void complete(NgGame *g)
{
 const NgModule *m=ng_module(g->id);const GridsPuzzle *p=grids_puzzle(g);assert(p);
 for(unsigned i=0;i<(unsigned)g->rows*g->cols;i++)if(!g->fixed[i]){g->cursor=(uint8_t)i;assert(m->action(g,'0'+p->solution[i]));assert(ng_valid(g));}
 assert(m->action(g,NGK_EXE));assert(g->status==NG_WON&&ng_valid(g));
}
static void legacy_roundtrip(unsigned id)
{
 memset(&session,0,sizeof session);ng_new(&session.game,id,2,0,3456,1);
 for(unsigned seed=1;session.game.puzzle_id>=900 && seed<10000;seed++)ng_new(&session.game,id,2,0,seed,1);
 assert(session.game.puzzle_id<900);session.stats.started=1;complete(&session.game);ng_record_result(&session);
 size_t n=ng_encode(&session,bytes,sizeof bytes);assert(n);
 /* Independent old three-level wire view: drop each mode's new fourth bucket. */
 size_t oldlen=legacy_wire(legacy,bytes,n,3);
 assert(oldlen==3371);assert(ng_decode(&decoded,legacy,oldlen,id));
 assert(decoded.stats.started==1 && !decoded.stats.best[0][2][0].wins);
 session.game.pack_revision=1;assert(!memcmp(&session.game,&decoded.game,sizeof session.game));
}
int main(void)
{
 const unsigned ids[4]={11,12,14,15};
 for(unsigned id=11;id<=20;id++)assert(ng_difficulty_count(id)==(id<=15?5u:4u));
 for(unsigned k=0;k<4;k++){
  unsigned id=ids[k];memset(&session,0,sizeof session);ng_new(&session.game,id,3,0,1234,1);session.stats.started=1;
  assert(session.game.difficulty==3&&ng_valid(&session.game));unsigned nn=(unsigned)session.game.rows*session.game.cols;
  for(unsigned i=0;i<nn;i++){session.game.cursor=(uint8_t)i;assert(ng_valid(&session.game));}
  session.game.cursor=(uint8_t)nn;assert(!ng_valid(&session.game));session.game.cursor=0;
  session.undo_count=1;session.undo[0]=session.game;complete(&session.game);ng_record_result(&session);ng_record_result(&session);
  assert(session.stats.best[0][3][0].wins==1&&session.stats.best[0][2][0].completed==0);
  size_t n=ng_encode(&session,bytes,sizeof bytes);assert(n&&ng_decode(&decoded,bytes,n,id));
  assert(decoded.game.difficulty==3&&decoded.stats.started==1&&!decoded.stats.best[0][3][0].wins&&decoded.undo_count==1&&decoded.undo[0].difficulty==3);
  assert(!memcmp(&session.game,&decoded.game,sizeof session.game));
  NgSummary summary;ng_summarize(&decoded,&summary);assert(!summary.completed&&summary.difficulty==3);
  legacy_roundtrip(id);entry_workflow(id,k%2?NGK_EXE:NGK_F6);
 }
 puts("Independent MASTER integration: four APIs, all valid cursors, completion, compact undo codec, legacy HARD payload and inline entry/resume PASS");return 0;
}
