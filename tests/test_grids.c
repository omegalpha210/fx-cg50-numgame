#include "ng.h"
#include "ui.h"
#include "grids_internal.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static NgGame fresh(unsigned id,unsigned difficulty,unsigned mode,uint32_t seed)
{
 NgGame g={0};g.id=(uint8_t)id;g.difficulty=(uint8_t)difficulty;g.mode=(uint8_t)mode;g.seed=seed;g.rng=seed;
 ng_grids[id-11].init(&g);return g;
}
static NgGame from_pack(unsigned index)
{
 const GridsPuzzle *p=grids_record(index);assert(p);NgGame g={0};g.id=p->id;g.difficulty=p->difficulty;g.puzzle_id=index;g.rows=p->n;g.cols=p->n;g.rng=1;g.seed=1;
 for(unsigned i=0;i<(unsigned)p->n*p->n;i++){
  if(p->id==16||p->id==20)g.board[i]=p->id==20;
  else if(p->id==13)g.fixed[i]=p->cells[i]!=255;
  else {g.board[i]=(p->id==17&&p->cells[i]==255)?-1:p->cells[i];g.fixed[i]=g.board[i]!=(p->id==17?-1:0);}
 }
 return g;
}
static void solve_actions(NgGame *g)
{
 const NgModule *m=&ng_grids[g->id-11];const GridsPuzzle *p=grids_puzzle(g);unsigned n=(unsigned)p->n*p->n;
 for(unsigned i=0;i<n;i++){
  if(g->fixed[i])continue;
  g->cursor=(uint8_t)i;int v=p->solution[i];
  if(g->id==16||g->id==20){if(g->board[i]!=v)assert(m->action(g,NGK_EXE));}
  else if(g->id==18||g->id==19){if(v>=10)assert(m->action(g,'0'+v/10));assert(m->action(g,'0'+v%10));assert(m->action(g,NGK_EXE));}
  else assert(m->action(g,'0'+v));
  assert(m->valid(g));
 }
 assert(m->action(g,(g->id==16||g->id==18||g->id==19||g->id==20)?NGK_AUX:NGK_EXE));
 assert(g->status==NG_WON&&grids_complete(g)&&m->valid(g));
}
static void pixel(void *ctx,int x,int y,int w,int h,uint16_t color)
{
 unsigned *count=ctx;(void)color;assert(x>=0&&y>=0&&w>0&&h>0&&x+w<=396&&y+h<=224);*count+=(unsigned)(w*h);
}
static void lifecycle(void)
{
 for(unsigned id=11;id<=20;id++)for(unsigned d=0;d<5;d++)for(unsigned mode=0;mode<ng_grids[id-11].modes;mode++){
  if(!grids_bank_count(id,d,mode))continue;
  const NgModule *m=&ng_grids[id-11];NgGame g=fresh(id,d,mode,1000+id*33+d);assert(m->valid(&g));
  NgGame restart=fresh(id,d,mode,1000+id*33+d);assert(!memcmp(&g,&restart,sizeof g));
  unsigned pixels=0;NgCanvas canvas={&pixels,pixel};m->render(&g,&canvas);assert(pixels>1000);
  assert(m->action(&g,NGK_RIGHT));assert(m->action(&g,NGK_DOWN));assert(m->action(&g,NGK_LEFT));assert(m->action(&g,NGK_UP));
  for(unsigned i=0;i<(unsigned)g.rows*g.cols;i++)if(g.fixed[i]){
   g.cursor=(uint8_t)i;int old=g.board[i];m->action(&g,'1');m->action(&g,NGK_DEL);assert(g.board[i]==old&&g.fixed[i]);break;
  }
  for(unsigned i=0;i<(unsigned)g.rows*g.cols;i++)if(!g.fixed[i]){g.cursor=(uint8_t)i;break;}
  if(id!=16&&id!=20){int16_t before=g.board[g.cursor];m->action(&g,id==17?'2':'0');assert(g.board[g.cursor]==before);if(id==18||id==19){m->action(&g,NGK_EXE);assert(g.board[g.cursor]==before);g.input[0]=0;}}
  if(id!=19){assert(m->action(&g,NGK_HINT));assert(g.assisted&&strstr(g.message,"REVEAL"));assert(g.board[g.cursor]==grids_puzzle(&g)->solution[g.cursor]);}
  NgGame restored=g;assert(m->valid(&restored));assert(!memcmp(&restored,&g,sizeof g));
  if(id==19&&mode==1){solve_actions(&g);}else{g=restart;solve_actions(&g);}
  pixels=0;m->render(&g,&canvas);assert(pixels>1000);
 }
}
static void all_records(void)
{
 for(unsigned i=0;i<GRIDS_PACK_COUNT;i++){
  NgGame g=from_pack(i);const GridsPuzzle *p=grids_puzzle(&g);const NgModule *m=&ng_grids[g.id-11];assert(p&&m->valid(&g));assert(!grids_complete(&g));
  g.pack_revision=1;assert(m->valid(&g)==(i<980));g.pack_revision=2;assert(m->valid(&g));
  for(unsigned j=0;j<(unsigned)p->n*p->n;j++)g.board[j]=p->solution[j];
  assert(grids_complete(&g));GridsPuzzle corrupt=*p;memset(corrupt.solution,255,sizeof corrupt.solution);assert(grids_rules_complete(&g,&corrupt));
  for(unsigned j=0;j<(unsigned)p->n*p->n;j++)if(!g.fixed[j]){
   int16_t old=g.board[j];g.board[j]=(g.id==16||g.id==20)?!old:(g.id==17?-1:0);assert(!grids_complete(&g));g.board[j]=old;break;
  }
  g.status=NG_WON;assert(m->valid(&g));
  g.puzzle_id=GRIDS_PACK_COUNT;assert(!m->valid(&g));
 }
}
static void rule_probes(void)
{
 /* Hitori diagonal contact is legal, but isolated white islands are not. */
 NgGame g={0};GridsPuzzle p={0};g.id=16;g.rows=g.cols=3;p.n=3;p.id=16;
 const uint8_t latin[9]={1,2,3,2,3,1,3,1,2};memcpy(p.cells,latin,9);
 assert(grids_rules_complete(&g,&p));g.board[0]=g.board[4]=1;assert(grids_rules_complete(&g,&p));
 memset(g.board,0,sizeof g.board);g.board[1]=g.board[3]=g.board[5]=g.board[7]=1;assert(!grids_rules_complete(&g,&p));
 memset(g.board,0,sizeof g.board);g.board[0]=g.board[1]=1;assert(!grids_rules_complete(&g,&p));
 /* Skyscraper directions are independently laid out top,bottom,left,right. */
 memset(&g,0,sizeof g);memset(&p,0,sizeof p);g.id=15;g.rows=g.cols=4;p.n=4;p.id=15;
 const int16_t board[16]={1,2,3,4,2,3,4,1,3,4,1,2,4,1,2,3};memcpy(g.board,board,sizeof board);
 const int16_t clues[16]={4,3,2,1,1,2,2,2,4,3,2,1,1,2,2,2};memcpy(p.a,clues,sizeof clues);assert(grids_rules_complete(&g,&p));
 for(unsigned i=0;i<16;i++){int16_t old=p.a[i];p.a[i]=old%4+1;assert(!grids_rules_complete(&g,&p));p.a[i]=old;}
 /* Futoshiki top < below maps to +1; tip points to top. */
 g.id=14;p.id=14;memset(p.a,0,sizeof p.a);p.b[0]=1;assert(grids_rules_complete(&g,&p));p.b[0]=-1;assert(!grids_rules_complete(&g,&p));
 /* Calcudoku permits a repeated value within a cage in different lines. */
 g.id=12;p.id=12;memset(p.a,0,sizeof p.a);memset(p.b,0,sizeof p.b);
 for(unsigned i=0;i<16;i++){p.a[i]=(int16_t)i;p.b[2*i]=0;p.b[2*i+1]=g.board[i];}
 p.a[0]=p.a[1]=p.a[4]=p.a[5]=0;p.b[0]=1;p.b[1]=8;assert(grids_rules_complete(&g,&p));
 p.b[0]=2;p.b[1]=12;assert(grids_rules_complete(&g,&p));p.b[1]=13;assert(!grids_rules_complete(&g,&p));
 /* A Latin square that violates Sudoku boxes must fail. */
 memset(&g,0,sizeof g);memset(&p,0,sizeof p);g.id=11;g.rows=g.cols=9;p.n=9;p.id=11;
 for(unsigned r=0;r<9;r++)for(unsigned c=0;c<9;c++)g.board[r*9+c]=(int16_t)((r+c)%9+1);
 assert(!grids_rules_complete(&g,&p));
 /* Every Kakuro white cell has across/down membership; 0 and repeats fail. */
 memset(&g,0,sizeof g);memset(&p,0,sizeof p);g.id=13;g.rows=g.cols=5;p.n=5;p.id=13;
 p.cells[6]=p.cells[7]=p.cells[11]=p.cells[12]=255;g.board[6]=1;g.board[7]=2;g.board[11]=3;g.board[12]=1;
 p.a[5]=3;p.a[10]=4;p.b[1]=4;p.b[2]=3;assert(grids_rules_complete(&g,&p));
 g.board[6]=0;assert(!grids_rules_complete(&g,&p));g.board[6]=1;g.board[7]=1;p.a[5]=2;p.b[2]=2;assert(!grids_rules_complete(&g,&p));
 /* Balanced lines without triples still fail if whole rows repeat. */
 memset(&g,0,sizeof g);memset(&p,0,sizeof p);g.id=17;g.rows=g.cols=6;p.n=6;p.id=17;memset(p.cells,255,36);
 const unsigned pattern[6]={0,0,1,1,0,1};for(unsigned r=0;r<6;r++)for(unsigned c=0;c<6;c++)g.board[r*6+c]=(int16_t)((pattern[r]+c)%2);
 assert(!grids_rules_complete(&g,&p));
 /* Alternative Numbrix paths work; a diagonal shortcut does not. */
 memset(&g,0,sizeof g);memset(&p,0,sizeof p);g.id=18;g.rows=g.cols=4;p.n=4;p.id=18;
 for(unsigned r=0;r<4;r++)for(unsigned c=0;c<4;c++)g.board[r*4+c]=(int16_t)(r*4+(r%2?4-c:c+1));
 assert(grids_rules_complete(&g,&p));g.board[1]=3;g.board[2]=2;assert(!grids_rules_complete(&g,&p));
 /* Zero Sum Grid targets require every positive number removed. */
 memset(&g,0,sizeof g);memset(&p,0,sizeof p);g.id=20;g.rows=g.cols=5;p.n=5;p.id=20;memset(p.cells,1,25);
 assert(grids_rules_complete(&g,&p));g.board[0]=1;assert(!grids_rules_complete(&g,&p));
 /* Magic square free mode accepts a different valid rotated answer. */
 g=fresh(19,0,1,887);const NgModule *m=&ng_grids[8];const int16_t magic[9]={2,7,6,9,5,1,4,3,8};memcpy(g.board,magic,sizeof magic);assert(grids_complete(&g));assert(m->action(&g,NGK_AUX));assert(g.status==NG_WON);
 /* MASTER explicitly adds wrap diagonals in both modes. A normal magic
    square remains valid on HARD, but must fail PAN without consulting witness. */
 memset(&g,0,sizeof g);memset(&p,0,sizeof p);g.id=p.id=19;g.rows=g.cols=p.n=4;
 const int16_t normal4[16]={16,2,3,13,5,11,10,8,9,7,6,12,4,14,15,1};
 const int16_t pan4[16]={9,6,15,4,16,3,10,5,2,13,8,11,7,12,1,14};
 memcpy(g.board,normal4,sizeof normal4);g.difficulty=2;assert(grids_rules_complete(&g,&p));
 g.difficulty=3;assert(!grids_rules_complete(&g,&p));g.mode=1;assert(!grids_rules_complete(&g,&p));
 memcpy(g.board,pan4,sizeof pan4);assert(grids_rules_complete(&g,&p));g.mode=0;assert(grids_rules_complete(&g,&p));
 p.cells[0]=1;assert(!grids_rules_complete(&g,&p));g.mode=1;assert(grids_rules_complete(&g,&p));
 /* Binary zero is a value; DEL restores -1. */
 g=fresh(17,2,0,445);m=&ng_grids[6];for(unsigned i=0;i<(unsigned)g.rows*g.cols;i++)if(!g.fixed[i]){g.cursor=(uint8_t)i;break;}
 assert(m->action(&g,'0'));assert(g.board[g.cursor]==0);assert(m->action(&g,NGK_DEL));assert(g.board[g.cursor]==-1);
 /* Notes are real toggles and never overwrite fixed clues or answers. */
 g=fresh(11,0,0,887);m=&ng_grids[0];for(unsigned i=0;i<81;i++)if(!g.fixed[i]){g.cursor=(uint8_t)i;break;}
 assert(m->action(&g,NGK_AUX));assert(m->action(&g,'9'));assert(g.board[g.cursor]==0&&g.notes[g.cursor]==256);assert(m->action(&g,'1'));assert(g.notes[g.cursor]==257);assert(m->action(&g,'9'));assert(g.notes[g.cursor]==1);assert(m->action(&g,NGK_DEL));assert(!g.notes[g.cursor]);
 /* Full draft, range errors and cursor draft cancellation are bounded. */
 g=fresh(18,2,0,79);m=&ng_grids[7];for(unsigned i=0;i<(unsigned)g.rows*g.cols;i++)if(!g.fixed[i]){g.cursor=(uint8_t)i;break;}
 assert(m->action(&g,'9'));assert(m->action(&g,'9'));assert(m->action(&g,'9'));assert(!strcmp(g.input,"99"));assert(m->action(&g,NGK_EXE));assert(!g.board[g.cursor]);assert(m->action(&g,NGK_RIGHT));assert(!g.input[0]);
 /* Malformed persisted fields reject rather than indexing outside arrays. */
 g=fresh(11,0,0,19);m=&ng_grids[0];g.data[127]=1;assert(!m->valid(&g));g.data[127]=0;g.rows=8;assert(!m->valid(&g));g.rows=9;g.cursor=81;assert(!m->valid(&g));g.cursor=0;memset(g.input,'9',sizeof g.input);assert(!m->valid(&g));
}
static void bank_ranges(void)
{
 const unsigned ids[4]={11,12,14,15};
 assert(!grids_record(UINT32_MAX)&&!grids_record(GRIDS_PACK_COUNT));
 assert(!grids_bank_count(10,0,0)&&!grids_bank_count(11,5,0)&&!grids_bank_count(11,0,1));
 assert(grids_bank_id(11,0,100000)==UINT32_MAX);
 for(unsigned id=11;id<=20;id++){
  for(unsigned d=0;d<3;d++)for(unsigned k=0;k<30;k++)assert(grids_bank_id(id,d,k)==(id-11)*90+d*30+k);
  for(unsigned d=0;d<5;d++){
   unsigned count=grids_bank_count(id,d,0);if(!count)continue;assert(count<=256);
   bool seen[256]={0};
   for(unsigned ordinal=0;ordinal<count;ordinal++){
    NgGame g={0};g.id=(uint8_t)id;g.difficulty=(uint8_t)d;g.seed=g.rng=1;g.supply_seed=3932039;g.supply_index=ordinal;
    ng_grids[id-11].init(&g);assert(ng_grids[id-11].valid(&g));
    unsigned found=0;while(found<count&&grids_bank_id(id,d,found)!=g.puzzle_id)found++;
    assert(found<count&&!seen[found]);seen[found]=true;
    NgGame bad=g;bad.cursor=(uint8_t)(g.rows*g.cols);assert(!ng_grids[id-11].valid(&bad));
   }
   for(unsigned k=0;k<count;k++)assert(seen[k]);
  }
 }
 for(unsigned group=0;group<4;group++)for(unsigned k=0;k<20;k++)assert(grids_bank_id(ids[group],3,k)==900+group*20+k);
 /* Independent caller-owned decode survives a different shared cache record. */
 GridsPuzzle saved;assert(grids_decode(0,&saved));const GridsPuzzle *other=grids_record(GRIDS_PACK_COUNT-1);assert(other);NgGame g=from_pack(0);
 for(unsigned j=0;j<81;j++)g.board[j]=saved.solution[j];
 assert(grids_rules_complete(&g,&saved));
}
void test_grids(void)
{
 lifecycle();all_records();rule_probes();bank_ranges();printf("grids: %u witnesses, completion independence, all supported lifecycles, compact/bag/rules/input/render PASS\n",GRIDS_PACK_COUNT);
}
#ifdef NG_GRIDS_TEST_MAIN
int main(void){test_grids();return 0;}
#endif
