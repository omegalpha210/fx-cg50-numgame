#include "support.h"
#include "test_guesscalc_workflow.h"
#include "grids_internal.h"
#include "strategyquick.h"
#include "boards.h"
#include "test_boards_fixtures.h"
#include <stdlib.h>
static TestDisk disk;
static NgApp app,cold;
static void press(void *ctx,int key){NgApp *a=ctx;tap(a,key);assert(ng_valid(&a->session.game));}
static void navigate(NgApp *a,unsigned i)
{
 NgGame *g=&a->session.game;
 if(g->id==13) {
  int parent[81],key[81],queue[81],head=0,tail=0;
  for(unsigned n=0;n<81;n++)parent[n]=-1;
  parent[g->cursor]=g->cursor;queue[tail++]=g->cursor;
  while(head<tail && parent[i]<0){int p=queue[head++];for(int k=NGK_UP;k<=NGK_LEFT;k++){
   NgGame probe=*g;probe.cursor=(uint8_t)p;ng_module(g->id)->action(&probe,k);
   int q=probe.cursor;if(parent[q]<0){parent[q]=p;key[q]=k;queue[tail++]=q;}
  }}
  assert(parent[i]>=0);int path[81],n=0;for(int p=(int)i;p!=g->cursor;p=parent[p])path[n++]=key[p];
  while(n)tap(a,path[--n]);return;
 }
 while(g->cursor/g->cols!=i/g->cols)tap(a,NGK_DOWN);
 while(g->cursor%g->cols!=i%g->cols)tap(a,NGK_RIGHT);
}
static void grid_solve(NgApp *a)
{
 NgGame *g=&a->session.game;const GridsPuzzle *p=grids_puzzle(g);
 for(unsigned i=0;i<(unsigned)p->n*p->n;i++) {
  if(g->fixed[i])continue;navigate(a,i);
  if(g->id==16 || g->id==20){if(g->board[i]!=p->solution[i])tap(a,NGK_EXE);}
  else if(g->id==18 || g->id==19){char n[8];snprintf(n,sizeof(n),"%u",p->solution[i]);gc_test_type(a,press,n);tap(a,NGK_EXE);}
  else tap(a,'0'+p->solution[i]);
 }
 tap(a,(g->id==16 || g->id>=18)?NGK_F4:NGK_EXE);
}
static void strategy_solve(NgApp *a)
{
 NgGame *g=&a->session.game;unsigned moves=0;
 while(!g->status && moves++<400) {
  if(g->cpu_pending){assert(ng_app_cpu(a));test_render(a);continue;}
  NgGame copy=*g;SqMove m=sq_strategy_pick(&copy,true);assert(sq_strategy_legal(g,m));
  if(g->id==24)tap(a,'1'+m.choice);
  else {if(g->id==21 || g->id==22)navigate(a,(unsigned)m.choice);
   char n[8];snprintf(n,sizeof(n),"%d",m.amount);gc_test_type(a,press,n);tap(a,NGK_EXE);}
 }
 assert(g->status && moves<400);
}
static unsigned distance8(const int16_t b[9])
{unsigned n=0;for(unsigned i=0;i<9;i++)if(b[i]){int p=b[i]-1;n+=(unsigned)abs((int)(i%3)-p%3)+(unsigned)abs((int)(i/3)-p/3);}return n;}
static int path8[80];static unsigned solution_depth;
static bool solve8(int16_t b[9],unsigned blank,unsigned depth,unsigned limit,int prev)
{
 unsigned h=distance8(b);if(!h){solution_depth=depth;return true;}if(depth+h>limit)return false;
 for(int k=0;k<4;k++) {
  if(prev>=0 && (k+2)%4==prev)continue;
  if((k==0 && blank<3)||(k==1 && blank%3==2)||(k==2 && blank>=6)||(k==3 && blank%3==0))continue;
  unsigned q=(unsigned)((int)blank+(k==0?-3:k==1?1:k==2?3:-1));
  b[blank]=b[q];b[q]=0;path8[depth]=NGK_UP+k;
  if(solve8(b,q,depth+1,limit,k))return true;
  b[q]=b[blank];b[blank]=0;
 }
 return false;
}
static void quick_solve(NgApp *a)
{
 NgGame *g=&a->session.game;
 if(g->id==26){uint32_t rng=123;for(unsigned i=0;!g->status && i<10000;i++){rng=rng*1664525+1013904223;tap(a,NGK_UP+(int)(rng%4));}assert(g->status==NG_LOST);}
 else if(g->id==27){int16_t b[9];memcpy(b,g->board,sizeof(b));unsigned bound=distance8(b);for(;bound<40;bound++)if(solve8(b,g->cursor,0,bound,-1))break;assert(bound<40);for(unsigned i=0;i<solution_depth;i++)tap(a,path8[i]);}
 else if(g->id==28){uint32_t solution;assert(sq_lights_solution(g,&solution));for(unsigned i=0;i<(unsigned)g->rows*g->cols && !g->status;i++)if(solution&(1u<<i)){navigate(a,i);tap(a,NGK_EXE);}}
 else if(g->id==29){for(unsigned i=0;i<5;i++){char n[16];snprintf(n,sizeof(n),"%ld",(long)g->data[4]);gc_test_type(a,press,n);tap(a,NGK_EXE);ng_app_tick(a,300);tap(a,NGK_EXE);}ng_app_tick(a,60000);}
 else if(g->id==30){for(unsigned round=0;round<20;round++) {
  ng_app_tick(a,(uint32_t)g->data[2]);assert(g->phase==1);ng_app_tick(a,350);assert(g->phase==2);
  char n[20];for(int i=0;i<g->data[0];i++)n[i]=(char)('0'+g->board[i]);n[g->data[0]]=0;gc_test_type(a,press,n);tap(a,NGK_EXE);
  if(!g->status){ng_app_tick(a,300);tap(a,NGK_EXE);}
 }}
}
static void board_solve(NgApp *a)
{
 NgGame *g=&a->session.game;unsigned n=g->rows,index=g->puzzle_id;
 if(g->id==31){const uint8_t (*rects)[4]=boards_test_rects[index];for(unsigned i=1;i<=rects[0][0];i++){const uint8_t *r=rects[i];navigate(a,r[0]*n+r[1]);tap(a,NGK_EXE);navigate(a,(r[0]+r[2]-1)*n+r[1]+r[3]-1);tap(a,NGK_EXE);}}
 else for(unsigned e=0;e<nb_edge_count(n);e++)if(boards_test_edges[index][e]){
  unsigned split=n*(n+1),base=e>=split?split:0,cols=e>=split?n+1:n;
  if((g->cursor>=split)!=(e>=split))tap(a,NGK_F4);
  while((g->cursor-base)/cols!=(e-base)/cols)tap(a,NGK_DOWN);
  while((g->cursor-base)%cols!=(e-base)%cols)tap(a,NGK_RIGHT);
  tap(a,NGK_EXE);
 }
}
static void lifecycle_all(void)
{
 for(unsigned index=0;index<NG_GAME_COUNT;index++) {
  unsigned id=ng_visible_id(index);
  if(id>=33)continue; /* Six new engines have independent lifecycle suites. */
  open_game(&app,id);const NgModule *m=ng_module(id);
  tap(&app,NGK_F5);assert(app.modal==NG_MODAL_RULES);tap(&app,NGK_EXIT);
  NgGame initial=app.session.game;
  tap(&app,NGK_F1);assert(!app.modal);assert(app.session.game.seed==initial.seed);
  assert(!memcmp(app.session.game.board,initial.board,sizeof(initial.board)));
  /* INIT is practice; count this run once only after completion. */
  if(id<=10)gc_test_solve(&app.session.game,&app,press);
  else if(id<=20)grid_solve(&app);
  else if(id<=25)strategy_solve(&app);
  else if(id<=28)quick_solve(&app);else board_solve(&app);
  if(!app.session.game.status){fprintf(stderr,"UI game %u did not finish: %s\n",id,app.session.game.message);abort();}
  assert(app.modal==NG_MODAL_RESULT);assert(ng_valid(&app.session.game));assert(app.session.game.recorded);
  tap(&app,NGK_F5);assert(app.modal==NG_MODAL_RULES);tap(&app,NGK_EXIT);assert(app.modal==NG_MODAL_RESULT);
  assert(!app.resumable);NgGame completed=app.session.game;
  assert(ng_checkpoint(&app));assert(ng_checkpoint(&app));
  tap(&app,NGK_EXIT);assert(app.screen==NG_PLAY && app.result_view && !app.modal);
  tap(&app,'1');tap(&app,NGK_EXE);assert(!memcmp(&app.session.game,&completed,sizeof completed));
  test_render(&app);tap(&app,NGK_EXIT);assert(app.screen==NG_ENTRY && !app.result_view);
  ng_app_init(&cold,test_hooks(&disk),900+id);assert(!cold.resumable);
  tap(&cold,NGK_F1);assert(cold.screen==NG_MAIN);
  tap(&cold,'1'+(int)(ng_catalog_index(id)/6));tap(&cold,'1'+(int)(ng_catalog_index(id)%6));
  assert(ng_entry_action(&cold,0)==NG_ENTRY_NEW);
  printf("UI complete/result view/cold no-resume: %02u %s\n",id,m->name);
 }
}
static void controls(void)
{
 while(app.modal)tap(&app,NGK_EXIT);while(app.screen!=NG_MAIN)tap(&app,NGK_EXIT);
 unsigned start=app.selection;tap(&app,NGK_LEFT);assert(app.selection==(start+5)%6);tap(&app,NGK_RIGHT);assert(app.selection==start);
 tap(&app,NGK_UP);assert(app.selection==(start+4)%6);tap(&app,NGK_DOWN);assert(app.selection==start);
 unsigned writes=disk.saves;tap(&app,NGK_F1);test_render(&app);tap(&app,NGK_EXIT);assert(disk.saves==writes);
 tap(&app,NGK_EXIT);assert(app.screen==NG_MAIN);
 open_game(&app,26);tap(&app,NGK_LEFT);NgGame moved=app.session.game;
 ng_app_event(&app,NGK_RIGHT,NG_DOWN);uint32_t move=app.session.game.moves;for(unsigned i=0;i<5;i++)ng_app_event(&app,NGK_RIGHT,NG_HOLD);assert(app.session.game.moves==move);ng_app_event(&app,NGK_RIGHT,NG_UP);
 if(app.session.game.moves!=moved.moves){tap(&app,NGK_F2);assert(!memcmp(app.session.game.board,moved.board,sizeof(moved.board)));assert(app.session.game.rng==moved.rng);}
 tap(&app,NGK_F1);assert(app.modal==NG_MODAL_INIT);ng_app_event(&app,NGK_EXE,NG_DOWN);uint32_t seed=app.session.game.seed;for(unsigned i=0;i<5;i++)ng_app_event(&app,NGK_EXE,NG_HOLD);assert(!app.modal && app.session.game.seed==seed);ng_app_event(&app,NGK_EXE,NG_UP);
 unsigned off=disk.off,menu=disk.menu;tap(&app,NGK_ACON);assert(disk.off==off);tap(&app,NGK_SHIFT);tap(&app,NGK_ACON);assert(disk.off==off+1);tap(&app,NGK_MENU);assert(disk.menu==menu+1);
 disk.write_budget=1;app.dirty=true;tap(&app,NGK_EXIT);assert(app.modal==NG_MODAL_SAVE_ERROR && app.screen==NG_PLAY);tap(&app,NGK_MENU);assert(disk.menu==menu+2);disk.write_budget=-1;tap(&app,NGK_EXE);assert(!app.modal);
 open_game(&app,21);ng_app_tick(&app,500);uint32_t elapsed=app.session.game.elapsed_ms;tap(&app,NGK_F5);ng_app_tick(&app,90000);assert(app.session.game.elapsed_ms==elapsed);tap(&app,NGK_EXIT);tap(&app,NGK_MENU);
 ng_app_init(&cold,test_hooks(&disk),32);tap(&cold,NGK_F1);assert(cold.screen==NG_PLAY && cold.session.game.id==21 && cold.session.game.elapsed_ms==elapsed);
}
static void cross_game(void)
{
 open_game(&app,11);tap(&app,'5');tap(&app,NGK_F4);tap(&app,NGK_RIGHT);tap(&app,'3');ng_app_tick(&app,1234);tap(&app,NGK_EXIT);
 open_game(&app,26);tap(&app,NGK_LEFT);ng_app_tick(&app,2134);tap(&app,NGK_EXIT);
 open_game(&app,21);tap(&app,'1');tap(&app,NGK_EXE);assert(app.session.game.cpu_pending);NgGame nim=app.session.game;tap(&app,NGK_MENU);
 ng_app_init(&cold,test_hooks(&disk),32);assert(cold.resumable && cold.session.game.id==21);
 tap(&cold,NGK_F1);assert(cold.screen==NG_PLAY && !memcmp(&cold.session.game,&nim,sizeof nim));
 assert(ng_app_cpu(&cold));uint32_t moves=cold.session.game.moves;
 assert(!ng_app_cpu(&cold) && cold.session.game.moves==moves);
 tap(&cold,NGK_F2);assert(cold.session.game.turn==0 && !cold.session.game.cpu_pending);
 tap(&cold,NGK_EXIT);while(cold.screen!=NG_MAIN)tap(&cold,NGK_EXIT);
 tap(&cold,'3');tap(&cold,'1');assert(ng_entry_action(&cold,0)==NG_ENTRY_NEW);
}
int main(void)
{
 setbuf(stdout,NULL);
 disk.write_budget=-1;ng_app_init(&app,test_hooks(&disk),54321);test_render(&app);
 lifecycle_all();controls();cross_game();puts("30 renderer/input workflows, single resume, result view, barriers, OS checkpoints PASS");return 0;
}
