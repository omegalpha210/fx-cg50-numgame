#include "../src/games/boards.h"
#include "storage.h"
#include "test_boards_fixtures.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef BOARDS_APP_TEST
#include "app.h"
#endif

static unsigned assertions,renders;
#define CHECK(expression) do{assertions++;assert(expression);}while(0)
static uint16_t pixels[396*224];
static void paint(void *ctx,int x,int y,int w,int h,uint16_t color){(void)ctx;CHECK(x>=0&&y>=27&&x+w<=396&&y+h<=185);for(int r=y;r<y+h;r++)for(int c=x;c<x+w;c++)pixels[r*396+c]=color;}
static NgCanvas canvas={NULL,paint};
static void render(const NgGame *g){uint32_t rng=g->rng;memset(pixels,255,sizeof pixels);ng_boards[g->id-31].render(g,&canvas);CHECK(g->rng==rng);renders++;}
static NgGame fresh(unsigned id,unsigned index){
 NgGame g;for(unsigned seed=1;seed<=4096;seed++){ng_new(&g,id,index/30,0,seed,1);if(g.puzzle_id==index){CHECK(ng_valid(&g));return g;}}assert(!"missing seeded puzzle");return g;
}
static bool action(NgGame *g,int key){bool result=ng_boards[g->id-31].action(g,key);CHECK(ng_valid(g));render(g);return result;}
static void navigate_cell(NgGame *g,unsigned target){
 unsigned n=g->rows;while(g->cursor/n!=target/n)CHECK(action(g,NGK_DOWN));while(g->cursor%n!=target%n)CHECK(action(g,NGK_RIGHT));
}
static void navigate_edge(NgGame *g,unsigned target){
 unsigned n=g->rows,split=n*(n+1);if((g->cursor>=split)!=(target>=split))CHECK(action(g,NGK_AUX));unsigned base=target>=split?split:0,cols=target>=split?n+1:n;
 while((g->cursor-base)/cols!=(target-base)/cols)CHECK(action(g,NGK_DOWN));while((g->cursor-base)%cols!=(target-base)%cols)CHECK(action(g,NGK_RIGHT));CHECK(g->cursor==target);
}
static void codec(const NgGame *g,const NgGame *before){
 NgSession source={0},copy;source.game=*g;source.stats.started=1;if(before){source.undo_count=1;source.undo[0]=*before;}
 uint8_t bytes[NG_RECORD_MAX];size_t length=ng_encode(&source,bytes,sizeof bytes);CHECK(length>0);CHECK(ng_decode(&copy,bytes,length,g->id));CHECK(!memcmp(&copy.game,g,sizeof *g));if(before)CHECK(!memcmp(&copy.undo[0],before,sizeof *before));CHECK(!ng_decode(&copy,bytes,length-1,g->id));
}
static void fill_rectangle(NgGame *g,unsigned top,unsigned left,unsigned h,unsigned w,bool reverse){
 unsigned a=top*g->rows+left,b=(top+h-1)*g->rows+left+w-1;if(reverse){unsigned t=a;a=b;b=t;}navigate_cell(g,a);CHECK(action(g,NGK_EXE));CHECK(g->phase==1&&g->data[64]==(int)a);navigate_cell(g,b);NgGame before=*g;CHECK(action(g,NGK_EXE));CHECK(g->moves==before.moves+1&&g->phase==0);codec(g,&before);
}
static void all_packs(void){
 for(unsigned id=31;id<=32;id++)for(unsigned index=0;index<NB_PACK_COUNT;index++){
  NgGame g=fresh(id,index),initial=g;render(&g);codec(&g,NULL);ng_new(&g,id,index/30,0,initial.seed,initial.run_id);CHECK(!memcmp(&g,&initial,sizeof g));
  if(id==31){
   const uint8_t (*rects)[4]=boards_test_rects[index];
   for(unsigned k=1;k<=rects[0][0];k++){
    const uint8_t *r=rects[k];fill_rectangle(&g,r[0],r[1],r[2],r[3],k%2!=0);
    if(k==1&&g.status==NG_PLAYING){NgGame before=g;navigate_cell(&g,r[0]*g.rows+r[1]);CHECK(action(&g,NGK_DEL));CHECK(g.moves==before.moves+1);CHECK(nb_shikaku_partial(&g)&&!nb_shikaku_complete(&g));codec(&g,&before);g=before;CHECK(ng_valid(&g));}
   }
   CHECK(nb_shikaku_complete(&g));
  }else{
   const uint8_t *edges=boards_test_edges[index];
   for(unsigned e=0;e<nb_edge_count(g.rows);e++)if(!edges[e]){navigate_edge(&g,e);NgGame before=g;CHECK(action(&g,NGK_DEL));CHECK(nb_edge_get(&g,e)==2);CHECK(action(&g,NGK_DEL));CHECK(nb_edge_get(&g,e)==0);g=before;break;}
   for(unsigned e=0;e<nb_edge_count(g.rows);e++)if(edges[e]){navigate_edge(&g,e);NgGame before=g;CHECK(action(&g,NGK_EXE));CHECK(nb_edge_get(&g,e)==1);codec(&g,&before);}
   CHECK(nb_slither_complete(&g));
  }
  CHECK(g.status==NG_WON);codec(&g,NULL);NgGame terminal=g;CHECK(!ng_boards[id-31].action(&g,NGK_EXE));CHECK(!memcmp(&g,&terminal,sizeof g));
 }
 puts("Boards: all 240 native pack records solved through real actions/renderers; INIT and production codec+undo snapshots PASS");
}
static NgGame plain(unsigned id,unsigned n){NgGame g={0};g.id=(uint8_t)id;g.rows=g.cols=(uint8_t)n;if(id==32)for(unsigned i=0;i<n*n;i++)g.board[i]=-1;return g;}
static void shikaku_invalid(void){
 NgGame g=plain(31,2);g.data[0]=2;g.data[2]=2;g.board[0]=g.board[1]=1;g.board[2]=g.board[3]=2;CHECK(nb_shikaku_complete(&g));
 g.board[0]=g.board[1]=47;g.board[2]=g.board[3]=63;CHECK(nb_shikaku_complete(&g)); /* arbitrary player labels accepted */
 g.board[3]=0;CHECK(!nb_shikaku_complete(&g)&&!nb_shikaku_partial(&g));g.board[3]=63;
 g.data[2]=0;CHECK(!nb_shikaku_complete(&g));g.data[2]=2;g.data[3]=1;CHECK(!nb_shikaku_complete(&g));g.data[3]=0;g.data[0]=3;CHECK(!nb_shikaku_complete(&g));
 g=plain(31,3);g.data[0]=3;g.board[0]=g.board[1]=g.board[3]=1;CHECK(!nb_shikaku_partial(&g));
 g=fresh(31,0);NgGame before=g;CHECK(action(&g,NGK_EXE));codec(&g,NULL);CHECK(action(&g,NGK_EXIT));CHECK(g.phase==0&&g.data[64]==-1&&g.moves==before.moves);CHECK(!ng_boards[0].action(&g,NGK_EXIT));
 unsigned blank=0;while(g.data[blank])blank++;navigate_cell(&g,blank);CHECK(action(&g,NGK_EXE));CHECK(action(&g,NGK_EXE));CHECK(g.phase==1&&g.moves==0);CHECK(action(&g,NGK_EXIT));
 const uint8_t *r=boards_test_rects[0][1];fill_rectangle(&g,r[0],r[1],r[2],r[3],false);before=g;navigate_cell(&g,r[0]*g.rows+r[1]);CHECK(action(&g,NGK_EXE));CHECK(action(&g,NGK_EXE));CHECK(g.moves==before.moves&&g.phase==1);CHECK(!memcmp(g.board,before.board,sizeof g.board));
 puts("Shikaku: reverse corners, labels independent of witnesses, gaps, overlap, no/two clues, wrong area, nonrectangular region, cancel PASS");
}
static void square_loop(NgGame *g,unsigned row,unsigned col){unsigned n=g->rows,split=n*(n+1);CHECK(nb_edge_set(g,row*n+col,1));CHECK(nb_edge_set(g,(row+1)*n+col,1));CHECK(nb_edge_set(g,split+row*(n+1)+col,1));CHECK(nb_edge_set(g,split+row*(n+1)+col+1,1));}
static void slither_invalid(void){
 NgGame g=plain(32,5);CHECK(!nb_slither_complete(&g));square_loop(&g,0,0);CHECK(nb_slither_complete(&g));g.board[0]=0;CHECK(!nb_slither_complete(&g));g.board[0]=-1;CHECK(nb_slither_complete(&g));square_loop(&g,3,3);CHECK(!nb_slither_complete(&g));
 g=plain(32,5);square_loop(&g,0,0);square_loop(&g,1,1);CHECK(!nb_slither_complete(&g)); /* degree-four vertex */
 g=plain(32,5);square_loop(&g,0,0);CHECK(nb_edge_set(&g,0,0));CHECK(!nb_slither_complete(&g)); /* open line */
 g=plain(32,5);square_loop(&g,0,0);CHECK(nb_edge_set(&g,2,1));CHECK(!nb_slither_complete(&g)); /* separate branch */
 g=plain(32,5);square_loop(&g,0,0);CHECK(nb_edge_set(&g,2,2));CHECK(nb_slither_complete(&g)); /* X is not a line */
 for(unsigned d=0;d<3;d++){
  g=fresh(32,d*30);unsigned ne=nb_edge_count(g.rows);bool reached[NB_EDGES]={0};unsigned queue[NB_EDGES],head=0,tail=0;reached[0]=true;queue[tail++]=0;
  while(head<tail){unsigned p=queue[head++];int keys[]={NGK_UP,NGK_RIGHT,NGK_DOWN,NGK_LEFT,NGK_AUX};for(unsigned k=0;k<5;k++){g.cursor=(uint8_t)p;CHECK(action(&g,keys[k]));unsigned next=g.cursor;if(!reached[next]){reached[next]=true;queue[tail++]=next;}}}
  CHECK(tail==ne);for(unsigned e=0;e<ne;e++){g.cursor=(uint8_t)e;codec(&g,NULL);CHECK(action(&g,NGK_EXE));CHECK(nb_edge_get(&g,e)==1);CHECK(action(&g,NGK_DEL));CHECK(nb_edge_get(&g,e)==2);CHECK(action(&g,NGK_EXE));CHECK(nb_edge_get(&g,e)==1);CHECK(action(&g,NGK_EXE));CHECK(nb_edge_get(&g,e)==0);}
 }
 puts("Slitherlink: no/one/two loops, degree-four crossing, open/extra lines, zero vs blank, exclusion, all 60/84/144 edge cursors and bit boundaries PASS");
}
static bool brute_loop(unsigned mask,unsigned n){
 unsigned count=(n+1)*(n+1),degree[81]={0},adj[81][4],seen=0;bool visited[81]={0};unsigned queue[81],head=0,tail=0;
 for(unsigned r=0;r<=n;r++)for(unsigned c=0;c<n;c++)if(mask&(1u<<(r*n+c))){unsigned a=r*(n+1)+c,b=a+1;adj[a][degree[a]++]=b;adj[b][degree[b]++]=a;}
 for(unsigned r=0;r<n;r++)for(unsigned c=0;c<=n;c++)if(mask&(1u<<(n*(n+1)+r*(n+1)+c))){unsigned a=r*(n+1)+c,b=a+n+1;adj[a][degree[a]++]=b;adj[b][degree[b]++]=a;}
 for(unsigned v=0;v<count;v++)if(degree[v]){if(degree[v]!=2)return false;seen++;if(!tail){queue[tail++]=v;visited[v]=true;}}
 if(!tail)return false;
 while(head<tail){unsigned a=queue[head++];for(unsigned k=0;k<degree[a];k++){unsigned b=adj[a][k];if(!visited[b]){visited[b]=true;queue[tail++]=b;}}}
 return tail==seen;
}
static void exhaustive_small(void){
 unsigned loops=0;
 for(unsigned mask=0;mask<4096;mask++){
  NgGame g=plain(32,2);for(unsigned e=0;e<12;e++)CHECK(nb_edge_set(&g,e,(mask>>e)&1u));bool expect=brute_loop(mask,2);CHECK(nb_slither_complete(&g)==expect);loops+=expect;
  for(unsigned pattern=0;pattern<625;pattern++){
   unsigned code=pattern;bool clues=true;for(unsigned p=0;p<4;p++){int v=(int)(code%5)-1;code/=5;g.board[p]=(int16_t)v;unsigned r=p/2,c=p%2,k=((mask>>(r*2+c))&1u)+((mask>>((r+1)*2+c))&1u)+((mask>>(6+r*3+c))&1u)+((mask>>(7+r*3+c))&1u);if(v>=0&&(unsigned)v!=k)clues=false;}
   CHECK(nb_slither_complete(&g)==(expect&&clues));
  }
 }
 CHECK(loops==13);puts("Slitherlink: all 4,096 2x2 edge assignments × 625 clue patterns cross-checked with independent graph model PASS");
}

static void shikaku_exhaustive(void){
 static const unsigned rectangles[]={1,2,4,8,3,12,5,10,15};
 for(unsigned labels=0;labels<625;labels++){
  NgGame g=plain(31,2);unsigned code=labels,masks[5]={0};bool covered=true;
  for(unsigned p=0;p<4;p++){unsigned tag=code%5;code/=5;g.board[p]=(int16_t)tag;masks[tag]|=1u<<p;if(!tag)covered=false;}
  for(unsigned clues=0;clues<625;clues++){
   code=clues;for(unsigned p=0;p<4;p++){g.data[p]=(int)(code%5);code/=5;}bool expected=covered;
   for(unsigned tag=1;tag<=4&&expected;tag++)if(masks[tag]){
    bool rectangular=false;for(unsigned k=0;k<9;k++)rectangular|=masks[tag]==rectangles[k];if(!rectangular){expected=false;break;}
    unsigned count=0,area=0,clue_count=0;for(unsigned p=0;p<4;p++)if(masks[tag]&(1u<<p)){count++;if(g.data[p]){area=(unsigned)g.data[p];clue_count++;}}
    expected=clue_count==1&&area==count;
   }
   CHECK(nb_shikaku_complete(&g)==expected);
  }
 }
 puts("Shikaku: all 625 labelled/gapped 2x2 boards × 625 clue patterns against independently enumerated rectangle masks PASS");
}

static void mutations(void){
 for(unsigned id=31;id<=32;id++)for(unsigned d=0;d<4;d++){
  NgGame good=fresh(id,d*30),g=good;g.cursor=(uint8_t)(id==31?g.rows*g.cols:nb_edge_count(g.rows));CHECK(!ng_valid(&g));g=good;g.puzzle_id=NB_PACK_COUNT;CHECK(!ng_valid(&g));g=good;g.difficulty=4;CHECK(!ng_valid(&g));g=good;g.status=NG_WON;CHECK(!ng_valid(&g));g=good;g.rows=9;CHECK(!ng_valid(&g));g=good;g.fixed[0]^=1;CHECK(!ng_valid(&g));g=good;g.phase=3;CHECK(!ng_valid(&g));g=good;g.data[127]=1;CHECK(!ng_valid(&g));g=good;g.notes[0]=1;CHECK(!ng_valid(&g));g=good;g.board[80]=1;CHECK(!ng_valid(&g));
  if(id==31){g=good;g.data[0]++;CHECK(!ng_valid(&g));g=good;g.phase=1;g.data[64]=64;CHECK(!ng_valid(&g));g=good;g.board[0]=65;CHECK(!ng_valid(&g));}
  else{g=good;g.board[0]=4;CHECK(!ng_valid(&g));g=good;g.data[0]=3;CHECK(!ng_valid(&g));if(d<2){g=good;unsigned e=nb_edge_count(g.rows);g.data[e/16]|=(int32_t)(1u<<((e%16)*2));CHECK(!ng_valid(&g));}}
 }
 puts("Boards: malformed pack identity, immutable clues, dimensions, status, phase, corner, region, packed edge and unused data rejected PASS");
}
static void capture(const char *path,const NgGame *g){
 render(g);FILE *file=fopen(path,"wb");CHECK(file);fprintf(file,"P6\n396 224\n255\n");for(unsigned i=0;i<396*224;i++){unsigned v=pixels[i];uint8_t rgb[3]={(uint8_t)(((v>>11)&31)*255/31),(uint8_t)(((v>>5)&63)*255/63),(uint8_t)((v&31)*255/31)};CHECK(fwrite(rgb,1,3,file)==3);}CHECK(!fclose(file));
}
static void captures(void){
 const char *dir=getenv("NB_CAPTURE_DIR");if(!dir)return;
 char path[512];for(unsigned id=31;id<=32;id++)for(unsigned d=0;d<4;d++){
  NgGame g=fresh(id,d*30);snprintf(path,sizeof path,"%s/%u-%u-start.ppm",dir,id,d);capture(path,&g);
  if(id==31){
   const uint8_t (*rects)[4]=boards_test_rects[d*30];const uint8_t *r=rects[1];navigate_cell(&g,r[0]*g.rows+r[1]);action(&g,NGK_EXE);navigate_cell(&g,(r[0]+r[2]-1)*g.rows+r[1]+r[3]-1);snprintf(path,sizeof path,"%s/%u-%u-corner.ppm",dir,id,d);capture(path,&g);CHECK(action(&g,NGK_EXIT));
   for(unsigned k=1;k<=rects[0][0];k++){r=rects[k];fill_rectangle(&g,r[0],r[1],r[2],r[3],k%2!=0);}
  }else{
   g.cursor=(uint8_t)(nb_edge_count(g.rows)-1);action(&g,NGK_DEL);snprintf(path,sizeof path,"%s/%u-%u-edge.ppm",dir,id,d);capture(path,&g);
   const uint8_t *edges=boards_test_edges[d*30];for(unsigned e=0;e<nb_edge_count(g.rows);e++)if(edges[e]){navigate_edge(&g,e);CHECK(action(&g,NGK_EXE));}
  }
  CHECK(g.status==NG_WON);snprintf(path,sizeof path,"%s/%u-%u-complete.ppm",dir,id,d);capture(path,&g);
 }
}
#ifdef BOARDS_APP_TEST
/* Actual core/app/codec/renderer integration; the standalone module runner omits
 * this section. Hooks keep only three encoded records and never copy NgGame as
 * the persistence mechanism. Archive/native I/O is covered by the root suites. */
typedef struct {
 uint8_t bytes[3][NG_RECORD_MAX];size_t lengths[3];
 NgSettings settings;bool has_settings;unsigned saves,menus,offs;
} BoardAppDisk;
static BoardAppDisk app_disk;
static NgApp board_app,board_cold;
static unsigned app_frames;
static int app_slot(unsigned id){return id==26?0:id==31?1:id==32?2:-1;}
static int app_load(void *ctx,NgSession *s,unsigned id){
 BoardAppDisk *d=ctx;int slot=app_slot(id);if(slot<0||!d->lengths[slot])return NG_LOAD_ABSENT;
 return ng_decode(s,d->bytes[slot],d->lengths[slot],id)?NG_LOAD_OK:NG_LOAD_INVALID;
}
static bool app_save(void *ctx,NgSession *s){
 BoardAppDisk *d=ctx;int slot=app_slot(s->game.id);CHECK(slot>=0);CHECK(ng_valid(&s->game));
 d->lengths[slot]=ng_encode(s,d->bytes[slot],sizeof d->bytes[slot]);CHECK(d->lengths[slot]);d->saves++;return true;
}
static int app_settings_load(void *ctx,NgSettings *s){BoardAppDisk *d=ctx;if(!d->has_settings)return NG_LOAD_ABSENT;*s=d->settings;return NG_LOAD_OK;}
static bool app_settings_save(void *ctx,NgSettings *s){BoardAppDisk *d=ctx;d->settings=*s;d->has_settings=true;return true;}
static void app_menu(void *ctx){((BoardAppDisk *)ctx)->menus++;}
static void app_off(void *ctx){((BoardAppDisk *)ctx)->offs++;}
static NgHooks app_hooks(void){return (NgHooks){&app_disk,app_load,app_save,app_settings_load,app_settings_save,app_menu,app_off,NULL};}
static void app_paint(void *ctx,int x,int y,int w,int h,uint16_t color){
 (void)ctx;CHECK(x>=0&&y>=0&&w>0&&h>0&&x+w<=396&&y+h<=224);for(int r=y;r<y+h;r++)for(int c=x;c<x+w;c++)pixels[r*396+c]=color;
}
static void app_draw(NgApp *a){NgCanvas c={NULL,app_paint};ng_render(a,&c);app_frames++;if(a->active)CHECK(ng_valid(&a->session.game));}
static void app_press(NgApp *a,int key){ng_app_event(a,key,NG_DOWN);ng_app_event(a,key,NG_UP);app_draw(a);}
static void app_focus(NgApp *a,int choice){
 CHECK(a->screen==NG_ENTRY&&!a->modal);unsigned target=ng_entry_row(a,choice);CHECK(ng_entry_action(a,target)==choice);
 for(unsigned k=0;a->entry_selection!=target&&k<ng_entry_count(a);k++)app_press(a,NGK_DOWN);CHECK(a->entry_selection==target);
}
static void app_entry(NgApp *a,unsigned id){
 for(unsigned k=0;(a->modal||a->screen!=NG_MAIN)&&k<16;k++)app_press(a,NGK_EXIT);CHECK(!a->modal&&a->screen==NG_MAIN);
 int index=ng_catalog_index(id);CHECK(index>=0&&ng_visible_id((unsigned)index)==id);app_press(a,'1'+index/6);app_press(a,'1'+index%6);
 CHECK(a->screen==NG_ENTRY&&a->selected_id==id);CHECK(ng_entry_action(a,a->entry_selection)==NG_ENTRY_NEW);
}
static void app_resume(NgApp *a){app_focus(a,NG_ENTRY_RESUME);app_press(a,NGK_EXE);CHECK(a->screen==NG_PLAY);}
static void app_cell(NgApp *a,unsigned target){
 NgGame *g=&a->session.game;for(unsigned k=0;g->cursor/g->cols!=target/g->cols&&k<g->rows;k++)app_press(a,NGK_DOWN);for(unsigned k=0;g->cursor%g->cols!=target%g->cols&&k<g->cols;k++)app_press(a,NGK_RIGHT);CHECK(g->cursor==target);
}
static void app_edge(NgApp *a,unsigned target){
 NgGame *g=&a->session.game;unsigned n=g->rows,split=n*(n+1);if((g->cursor>=split)!=(target>=split))app_press(a,NGK_F4);unsigned base=target>=split?split:0,cols=target>=split?n+1:n;
 for(unsigned k=0;(g->cursor-base)/cols!=(target-base)/cols&&k<=n;k++)app_press(a,NGK_DOWN);for(unsigned k=0;(g->cursor-base)%cols!=(target-base)%cols&&k<=n;k++)app_press(a,NGK_RIGHT);CHECK(g->cursor==target);
}
static NgGame app_rectangle(NgApp *a,const uint8_t *r){
 unsigned n=a->session.game.rows;app_cell(a,r[0]*n+r[1]);app_press(a,NGK_EXE);app_cell(a,(r[0]+r[2]-1)*n+r[1]+r[3]-1);NgGame before=a->session.game;app_press(a,NGK_EXE);CHECK(a->session.game.moves==before.moves+1);return before;
}
static void app_undo_equal(NgApp *a,NgGame expected){
 expected.assisted=1;expected.elapsed_ms=a->session.game.elapsed_ms;expected.recorded=a->session.game.recorded;app_press(a,NGK_F2);CHECK(!memcmp(&a->session.game,&expected,sizeof expected));
}
static void app_finish(NgApp *a){
 NgGame *g=&a->session.game;unsigned index=g->puzzle_id;
 if(g->id==31){const uint8_t (*rects)[4]=boards_test_rects[index];for(unsigned k=1;k<=rects[0][0];k++){const uint8_t *r=rects[k];if(!g->board[r[0]*g->cols+r[1]])(void)app_rectangle(a,r);}}
 else for(unsigned e=0;e<nb_edge_count(g->rows);e++)if(boards_test_edges[index][e]&&nb_edge_get(g,e)!=1){app_edge(a,e);app_press(a,NGK_EXE);}
 CHECK(g->status==NG_WON&&g->recorded&&a->modal==NG_MODAL_RESULT);
}
static void app_workflows(void){
 CHECK(ng_catalog_index(29)==-1&&ng_catalog_index(30)==-1);unsigned cases=0;
 for(unsigned id=31;id<=32;id++)for(unsigned difficulty=0;difficulty<4;difficulty++){
  memset(&app_disk,0,sizeof app_disk);ng_app_init(&board_app,app_hooks(),7300+id+difficulty);NgApp *a=&board_app;app_entry(a,id);app_focus(a,NG_ENTRY_LEVEL);
  for(unsigned k=0;a->settings.difficulty[id-1]!=difficulty&&k<3;k++)app_press(a,a->settings.difficulty[id-1]>difficulty?NGK_LEFT:NGK_RIGHT);CHECK(a->settings.difficulty[id-1]==difficulty);
  app_press(a,NGK_F6);CHECK(a->screen==NG_PLAY&&a->session.game.id==id&&a->session.game.difficulty==difficulty);NgGame initial=a->session.game;
  app_press(a,NGK_F5);CHECK(a->modal==NG_MODAL_RULES);CHECK(!ng_app_tick(a,1000));CHECK(a->session.game.elapsed_ms==initial.elapsed_ms);app_press(a,NGK_EXIT);
  if(id==31){
   ng_app_event(a,NGK_EXE,NG_DOWN);CHECK(a->session.game.phase==1);CHECK(!ng_app_event(a,NGK_EXE,NG_HOLD));ng_app_event(a,NGK_EXE,NG_UP);app_press(a,NGK_RIGHT);
  }else{app_edge(a,nb_edge_count(a->session.game.rows)-1);app_press(a,NGK_DEL);CHECK(nb_edge_get(&a->session.game,a->session.game.cursor)==2);}
  NgGame selected=a->session.game;app_press(a,NGK_MENU);CHECK(app_disk.menus==1);ng_app_init(&board_cold,app_hooks(),8800);a=&board_cold;app_entry(a,id);app_resume(a);CHECK(!memcmp(&a->session.game,&selected,sizeof selected));
  if(id==31){
   unsigned writes=app_disk.saves;ng_app_event(a,NGK_EXIT,NG_DOWN);CHECK(a->screen==NG_PLAY&&!a->session.game.phase&&!a->session.game.moves);CHECK(!ng_app_event(a,NGK_EXIT,NG_HOLD));ng_app_event(a,NGK_EXIT,NG_UP);CHECK(a->screen==NG_PLAY&&app_disk.saves==writes);CHECK(!memcmp(a->session.game.board,initial.board,sizeof initial.board));
   const uint8_t *r=boards_test_rects[a->session.game.puzzle_id][1];NgGame before=app_rectangle(a,r);CHECK(ng_app_tick(a,1234));app_undo_equal(a,before);CHECK(a->session.game.phase==1);app_press(a,NGK_EXIT);CHECK(a->screen==NG_PLAY&&!a->session.game.phase);
   (void)app_rectangle(a,r);before=a->session.game;app_press(a,NGK_DEL);CHECK(a->session.game.moves==before.moves+1);app_undo_equal(a,before);
  }else{
   NgGame before=selected;CHECK(a->session.undo_count==1);before=a->session.undo[0];app_undo_equal(a,before);before=a->session.game;
   ng_app_event(a,NGK_EXE,NG_DOWN);CHECK(a->session.game.moves==before.moves+1);CHECK(!ng_app_event(a,NGK_EXE,NG_HOLD));ng_app_event(a,NGK_EXE,NG_UP);CHECK(ng_app_tick(a,1234));app_undo_equal(a,before);before=a->session.game;app_press(a,NGK_DEL);CHECK(nb_edge_get(&a->session.game,a->session.game.cursor)==2);app_undo_equal(a,before);
  }
  NgGame saved=a->session.game;app_press(a,NGK_EXIT);CHECK(a->screen==NG_ENTRY&&ng_entry_action(a,a->entry_selection)==NG_ENTRY_NEW);
  unsigned writes=app_disk.saves;app_press(a,NGK_F4);CHECK(!a->modal&&a->screen==NG_ENTRY);CHECK(app_disk.saves==writes&&!memcmp(&a->session.game,&saved,sizeof saved));
  app_press(a,NGK_F6);CHECK(a->modal==NG_MODAL_NEW&&!memcmp(&a->session.game,&saved,sizeof saved));app_press(a,NGK_EXIT);app_resume(a);CHECK(!memcmp(&a->session.game,&saved,sizeof saved));
  app_press(a,NGK_EXIT);app_focus(a,NG_ENTRY_LEVEL);unsigned new_level=(difficulty+1)%4;for(unsigned k=0;a->settings.difficulty[id-1]!=new_level&&k<3;k++)app_press(a,a->settings.difficulty[id-1]>new_level?NGK_LEFT:NGK_RIGHT);CHECK(a->settings.difficulty[id-1]==new_level);app_press(a,NGK_F6);CHECK(a->modal==NG_MODAL_NEW&&!memcmp(&a->session.game,&saved,sizeof saved));app_press(a,NGK_EXIT);app_resume(a);CHECK(!memcmp(&a->session.game,&saved,sizeof saved));
  app_press(a,NGK_EXIT);app_focus(a,NG_ENTRY_NEW);app_press(a,NGK_EXE);CHECK(a->modal==NG_MODAL_NEW);app_press(a,NGK_EXE);CHECK(a->screen==NG_PLAY&&!a->modal&&a->session.game.run_id==saved.run_id+1&&a->session.game.seed!=saved.seed&&a->session.game.difficulty==new_level&&!a->session.game.moves&&!a->session.game.assisted&&!a->session.undo_count);
  if(id==31)(void)app_rectangle(a,boards_test_rects[a->session.game.puzzle_id][1]);else{app_edge(a,nb_edge_count(a->session.game.rows)-1);app_press(a,NGK_DEL);}
  saved=a->session.game;app_press(a,NGK_EXIT);app_entry(a,26);app_press(a,NGK_F6);CHECK(a->session.game.id==26);app_entry(a,id);app_resume(a);CHECK(!memcmp(&a->session.game,&saved,sizeof saved));
  app_finish(a);unsigned level=a->session.game.difficulty,assisted=a->session.game.assisted;uint32_t completed=a->session.stats.best[0][level][assisted].completed;CHECK(completed==1);NgGame terminal=a->session.game;
  app_press(a,NGK_F5);CHECK(a->modal==NG_MODAL_RULES);app_press(a,NGK_EXIT);CHECK(a->modal==NG_MODAL_RESULT);CHECK(!ng_app_tick(a,5000));CHECK(!memcmp(&terminal,&a->session.game,sizeof terminal));CHECK(ng_checkpoint(a)&&ng_checkpoint(a));app_press(a,NGK_MENU);app_press(a,NGK_SHIFT);app_press(a,NGK_ACON);CHECK(app_disk.menus==2&&app_disk.offs==1);
  app_press(a,NGK_EXIT);CHECK(ng_entry_action(a,a->entry_selection)==NG_ENTRY_NEW);app_resume(a);CHECK(a->modal==NG_MODAL_RESULT&&a->session.stats.best[0][level][assisted].completed==completed);
  ng_app_init(&board_app,app_hooks(),9900);a=&board_app;app_entry(a,id);app_resume(a);CHECK(a->modal==NG_MODAL_RESULT&&!memcmp(&a->session.game,&terminal,sizeof terminal));CHECK(!a->session.stats.best[0][level][assisted].completed);cases++;
 }
 printf("Boards actual NgApp: %u level workflows, real renderer %u frames; phase-cancel/HOLD, edge144, undo, MENU codec reload, NEW/RESUME and cold result PASS\n",cases,app_frames);
}
#endif
int test_boards(void){setvbuf(stdout,NULL,_IONBF,0);all_packs();shikaku_invalid();slither_invalid();exhaustive_small();shikaku_exhaustive();mutations();captures();
#ifdef BOARDS_APP_TEST
 app_workflows();
#endif
 printf("Boards host: %u assertions and %u real module renderer frames PASS\n",assertions,renders);return 0;}
#ifdef BOARDS_TEST_MAIN
int main(void){return test_boards();}
#endif
