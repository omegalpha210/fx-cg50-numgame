#include "grids_extra.h"
#include "ui.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static NgGame fresh(unsigned id,unsigned difficulty,unsigned ordinal)
{
 NgGame g={0};
 /* Use the public bag contract, without assuming one shuffle algorithm. */
 for(unsigned index=0;index<GRIDS_EXTRA_BANK;index++){
  memset(&g,0,sizeof g);g.id=(uint8_t)id;g.difficulty=(uint8_t)difficulty;g.seed=g.rng=1;g.pack_revision=2;
  g.supply_seed=1;g.supply_index=index;ng_grids_extra[id-35].init(&g);
  if(g.puzzle_id==difficulty*GRIDS_EXTRA_BANK+ordinal)return g;
 }
 assert(!"bank cycle omitted an ordinal");return g;
}
typedef struct {unsigned pixels;uint16_t *image;} Pixels;
static void rect(void *context,int x,int y,int w,int h,uint16_t color)
{
 Pixels *p=context;assert(x>=0&&y>=27&&w>0&&h>0&&x+w<=396&&y+h<=185);p->pixels+=(unsigned)(w*h);
 if(p->image)for(int row=y;row<y+h;row++)for(int col=x;col<x+w;col++)p->image[row*396+col]=color;
}
static void solve_actions(NgGame *g)
{
 const NgModule *m=&ng_grids_extra[g->id-35];int16_t solution[NG_CELLS];assert(grids_extra_witness(g->id,g->puzzle_id,solution));
 if(g->id==35){
  const GridsHashiPuzzle *p=&grids_hashi_pack[g->puzzle_id];
  for(unsigned i=0;i<p->count;i++){
   unsigned cell=p->pos[i];g->cursor=(uint8_t)cell;
   for(unsigned k=0;k<(unsigned)(solution[cell]%3);k++)assert(m->action(g,'6'));
   for(unsigned k=0;k<(unsigned)(solution[cell]/3);k++)assert(m->action(g,'2'));
   assert(m->valid(g));
  }
 }else for(unsigned i=0;i<(unsigned)g->rows*g->cols;i++){g->cursor=(uint8_t)i;assert(m->action(g,'0'+solution[i]));assert(m->valid(g));}
 assert(!memcmp(g->board,solution,sizeof solution));assert(m->action(g,NGK_AUX));assert(g->status==NG_WON&&m->valid(g));
}
static void all_records(void)
{
 for(unsigned id=35;id<=36;id++)for(unsigned d=0;d<(id==35?5u:4u);d++)for(unsigned ordinal=0;ordinal<GRIDS_EXTRA_BANK;ordinal++){
  const NgModule *m=&ng_grids_extra[id-35];NgGame g=fresh(id,d,ordinal);assert(m->valid(&g)&&!grids_extra_complete(&g));
  NgGame again=fresh(id,d,ordinal);assert(!memcmp(&g,&again,sizeof g));
  Pixels pixels={0,NULL};NgCanvas canvas={&pixels,rect};m->render(&g,&canvas);assert(pixels.pixels>1000);
  assert(m->action(&g,NGK_AUX));assert(g.status==NG_PLAYING);solve_actions(&g);m->render(&g,&canvas);
  if(id==35){GridsHashiPuzzle p=grids_hashi_pack[g.puzzle_id];memset(p.solution,255,sizeof p.solution);assert(grids_hashi_rules(&g,&p));}
  else {GridsNonoPuzzle p=grids_nono_pack[g.puzzle_id];memset(p.solution,255,sizeof p.solution);assert(grids_nono_rules(&g,&p));}
  const int frozen_keys[]={NGK_UP,NGK_RIGHT,NGK_DOWN,NGK_LEFT,NGK_EXE,NGK_AUX,NGK_HINT,NGK_DEL,'0','1','2','4','6','8'};
  NgGame completed=g;
  for(unsigned k=0;k<sizeof frozen_keys/sizeof frozen_keys[0];k++){assert(!m->action(&g,frozen_keys[k]));assert(!memcmp(&g,&completed,sizeof g));}
  if(id==35){unsigned cell=grids_hashi_pack[g.puzzle_id].pos[0];int v=g.board[cell];g.board[cell]=(int16_t)(v%3?v-1:v-3);}
  else g.board[g.cursor]=-1;
  assert(!m->valid(&g));
 }
}
static void rule_boundaries(void)
{
 NgGame g={0};GridsHashiPuzzle p={3,4,{0,2,6,8},{2,2,2,2},{255,255,255,255}};g.id=35;g.rows=g.cols=3;
 g.board[0]=4;g.board[2]=3;g.board[6]=1;assert(grids_hashi_rules(&g,&p));
 /* Degree-correct disconnected components are forbidden. */
 memset(g.board,0,sizeof g.board);g.board[0]=2;g.board[6]=2;assert(!grids_hashi_rules(&g,&p));
 /* This graph has exact degrees and is connected, but crosses at cell12. */
 GridsHashiPuzzle cross={5,8,{0,2,4,10,14,20,22,24},{2,3,2,3,3,2,3,2},{0}};
 g.rows=g.cols=5;memset(g.board,0,sizeof g.board);g.board[0]=4;g.board[2]=4;g.board[4]=3;g.board[10]=4;g.board[14]=3;g.board[20]=1;g.board[22]=1;
 assert(!grids_hashi_rules(&g,&cross));g.board[12]=1;assert(!grids_hashi_rules(&g,&cross));
 p.count=26;assert(!grids_hashi_rules(&g,&p));
 GridsNonoPuzzle n={5,{0x21,0,0,0,0,1,0,0,1,1},{65535}};memset(&g,0,sizeof g);g.id=36;g.rows=g.cols=5;g.board[0]=g.board[3]=g.board[4]=1;
 assert(grids_nono_rules(&g,&n));n.clue[0]=0x12;assert(!grids_nono_rules(&g,&n));n.clue[0]=0x21;
 g.board[1]=-1;assert(!grids_nono_rules(&g,&n));g.board[1]=0;g.board[0]=2;assert(!grids_nono_rules(&g,&n));g.board[0]=1;n.clue[0]=0x201;assert(!grids_nono_rules(&g,&n));
 /* Zero clues mean an empty line, never a missing/unconstrained clue. */
 memset(&n,0,sizeof n);n.n=3;memset(&g,0,sizeof g);g.rows=g.cols=3;
 assert(grids_nono_rules(&g,&n));g.board[4]=1;assert(!grids_nono_rules(&g,&n));g.board[4]=-1;assert(!grids_nono_rules(&g,&n));g.board[4]=0;
 n.clue[0]=0x11;n.clue[3]=n.clue[5]=1;g.board[0]=g.board[2]=1;assert(grids_nono_rules(&g,&n));
 g.board[1]=1;g.board[2]=0;n.clue[4]=1;n.clue[5]=0;assert(!grids_nono_rules(&g,&n)); /* Adjacent singles merge. */
 n.clue[0]=2;assert(grids_nono_rules(&g,&n));n.clue[0]=4;assert(!grids_nono_rules(&g,&n));
 n.clue[0]=2;n.clue[6]=1;assert(!grids_nono_rules(&g,&n)); /* Unused clue data. */
 memset(&n,0,sizeof n);n.n=9;memset(&g,0,sizeof g);g.rows=g.cols=9;n.clue[0]=0x11111;
 for(unsigned c=0;c<9;c+=2){g.board[c]=1;n.clue[9+c]=1;}
 assert(grids_nono_rules(&g,&n));g.board[1]=1;n.clue[10]=1;assert(!grids_nono_rules(&g,&n));
}
static unsigned leader(unsigned parent[8],unsigned i)
{while(parent[i]!=i)i=parent[i];return i;}
static void exhaustive_hashi_geometry(void)
{
 /* Independent raster + union/find truth model, 3^10 assignments. */
 static const uint8_t pos[8]={0,2,4,10,14,20,22,24};
 static const uint8_t endpoints[10][2]={{0,1},{1,2},{3,4},{5,6},{6,7},{0,3},{3,5},{1,6},{2,4},{4,7}};
 unsigned valid_count=0,crossing_count=0,disconnected_count=0;
 for(unsigned code=0;code<59049;code++){
  NgGame g={0};GridsHashiPuzzle p={0};p.n=5;p.count=8;memcpy(p.pos,pos,sizeof pos);g.rows=g.cols=5;
  uint8_t raster[25]={0};unsigned parent[8],value=code;bool crossing=false;
  for(unsigned i=0;i<8;i++)parent[i]=i;
  for(unsigned edge=0;edge<10;edge++){
   unsigned count=value%3;value/=3;unsigned a=endpoints[edge][0],b=endpoints[edge][1],from=pos[a],to=pos[b];
   p.clue[a]+=(uint8_t)count;p.clue[b]+=(uint8_t)count;
   bool horizontal=from/5==to/5;g.board[from]+=(int16_t)(count*(horizontal?1:3));
   if(!count)continue;
   parent[leader(parent,a)]=leader(parent,b);
   unsigned direction=horizontal?1u:2u,step=horizontal?1u:5u;
   for(unsigned cell=from+step;cell<to;cell+=step){raster[cell]|=(uint8_t)direction;if(raster[cell]==3)crossing=true;}
  }
  bool connected=true;for(unsigned i=1;i<8;i++)if(leader(parent,i)!=leader(parent,0))connected=false;
  bool expected=connected&&!crossing;assert(grids_hashi_rules(&g,&p)==expected);
  valid_count+=expected;crossing_count+=crossing;disconnected_count+=!connected;
 }
 assert(valid_count&&crossing_count&&disconnected_count);
 printf("HASHI raster/union-find: 59049 assignments, %u connected noncrossing, %u crossing, %u disconnected PASS\n",valid_count,crossing_count,disconnected_count);
}
static int scan_island(const GridsHashiPuzzle *p,unsigned start,unsigned step)
{
 for(unsigned cell=start+step;cell<(unsigned)p->n*p->n;cell+=step){
  if(step==1&&cell/p->n!=start/p->n)break;
  for(unsigned i=0;i<p->count;i++)if(p->pos[i]==cell)return (int)cell;
 }
 return -1;
}
static NgGame crossing_case(unsigned *horizontal,unsigned *vertical,unsigned *intersection)
{
 for(unsigned index=0;index<150;index++){
  const GridsHashiPuzzle *p=&grids_hashi_pack[index];unsigned n=p->n;
  for(unsigned i=0;i<p->count;i++)for(unsigned j=0;j<p->count;j++){
   unsigned a=p->pos[i],c=p->pos[j];int b=scan_island(p,a,1),d=scan_island(p,c,n);
   if(b>=0&&d>=0&&a%n<c%n&&c%n<(unsigned)b%n&&c/n<a/n&&a/n<(unsigned)d/n){
    *horizontal=a;*vertical=c;*intersection=(a/n)*n+c%n;return fresh(35,index/GRIDS_EXTRA_BANK,index%GRIDS_EXTRA_BANK);
   }
  }
 }
 assert(!"No crossing geometry in HASHI bank");return fresh(35,0,0);
}
typedef struct {int x,y;uint16_t color;unsigned hits;} PixelProbe;
static void probe_rect(void *context,int x,int y,int w,int h,uint16_t color)
{
 PixelProbe *p=context;Pixels bounds={0,NULL};rect(&bounds,x,y,w,h,color);
 if(color==p->color&&p->x>=x&&p->x<x+w&&p->y>=y&&p->y<y+h)p->hits++;
}
static void hashi_feedback(void)
{
 unsigned h,v,cross;NgGame g=crossing_case(&h,&v,&cross);const NgModule *m=&ng_grids_extra[0];int pitch=128/(g.cols-1);
 g.cursor=(uint8_t)h;g.data[0]=1;PixelProbe probe={48+(int)(h%g.cols)*pitch+9,40+(int)(h/g.cols)*pitch,NG_BLUE,0};NgCanvas canvas={&probe,probe_rect};
 m->render(&g,&canvas);assert(probe.hits); /* Dashed empty candidate. */
 assert(m->action(&g,'6')&&g.moves==1);probe.hits=0;m->render(&g,&canvas);assert(probe.hits); /* Single blue edge. */
 assert(m->action(&g,NGK_EXE)&&g.moves==2);probe.y-=2;probe.hits=0;m->render(&g,&canvas);assert(probe.hits); /* First double stroke. */
 probe.y+=4;probe.hits=0;m->render(&g,&canvas);assert(probe.hits); /* Second double stroke. */
 g.cursor=(uint8_t)v;assert(m->action(&g,'2'));assert(strstr(g.message,"Crossing")&&m->valid(&g));
 probe=(PixelProbe){48+(int)(cross%g.cols)*pitch,40+(int)(cross/g.cols)*pitch,NG_RED,0};m->render(&g,&canvas);assert(probe.hits);
 assert(m->action(&g,NGK_AUX));assert(g.status==NG_PLAYING&&strstr(g.message,"cannot cross"));
 assert(m->action(&g,NGK_DEL));assert(!g.message[0]&&m->valid(&g));
}
static void controls_and_states(void)
{
 for(unsigned id=35;id<=36;id++){
  const NgModule *m=&ng_grids_extra[id-35];NgGame original=fresh(id,id==35?4:3,0),g=original;
  const int keys[]={NGK_UP,NGK_RIGHT,NGK_DOWN,NGK_LEFT};
  for(unsigned i=0;i<4;i++){assert(m->action(&g,keys[i]));assert(m->valid(&g));}
  assert(!m->action(&g,'+'));assert(!m->action(&g,'9'));assert(!m->action(&g,'3'));
  g=original;
  if(id==35){
   const GridsHashiPuzzle *p=&grids_hashi_pack[g.puzzle_id];bool edited=false;
   for(unsigned i=0;i<p->count&&!edited;i++){
    g.cursor=p->pos[i];uint32_t before=g.moves;m->action(&g,'6');if(g.moves==before)m->action(&g,'2');
    if(g.moves>before){edited=true;assert(m->action(&g,NGK_EXE));assert(m->action(&g,NGK_DEL));assert(g.moves==before+3);assert(m->action(&g,NGK_HINT));assert(g.assisted&&strstr(g.message,"REVEAL"));}
   }
   assert(edited);
  }else{
   assert(m->action(&g,NGK_EXE)&&g.board[0]==1);assert(m->action(&g,NGK_EXE)&&g.board[0]==0);assert(m->action(&g,NGK_EXE)&&g.board[0]==-1);
   assert(m->action(&g,'1'));assert(m->action(&g,NGK_DEL));assert(g.board[0]==-1);assert(m->action(&g,NGK_HINT)&&g.assisted&&strstr(g.message,"REVEAL"));
  }
  assert(m->valid(&g));
  g=original;g.puzzle_id=UINT32_MAX;assert(!m->valid(&g));
  g=original;g.pack_revision=1;assert(!m->valid(&g));
  g=original;g.cursor=81;assert(!m->valid(&g));
  g=original;g.rows=0;assert(!m->valid(&g));
  g=original;g.notes[0]=1;assert(!m->valid(&g));
  g=original;g.fixed[0]=1;assert(!m->valid(&g));
  g=original;g.data[127]=1;assert(!m->valid(&g));
  g=original;g.data[0]=4;assert(!m->valid(&g));
  g=original;g.status=NG_WON;assert(!m->valid(&g));
  g=original;g.status=NG_LOST;assert(!m->valid(&g));
  g=original;g.history[0][0]='x';assert(!m->valid(&g));
  g=original;g.input[NG_INPUT-1]='x';assert(!m->valid(&g));
  g=original;g.mode=1;assert(!m->valid(&g));
  g=original;g.difficulty=5;assert(!m->valid(&g));
 }
 assert(!grids_extra_bank_count(35,5,0)&&!grids_extra_bank_count(36,4,0));
 assert(grids_extra_bank_id(35,0,30)==UINT32_MAX);
}
void test_grids_extra(void)
{all_records();rule_boundaries();exhaustive_hashi_geometry();hashi_feedback();controls_and_states();puts("grids_extra: 270 witnesses/actions, independent completion, states, content bounds PASS");}

#ifdef NG_GRIDS_EXTRA_TEST_MAIN
static void capture_one(const NgGame *g,const char *directory,const char *name,uint16_t *image)
{
 for(unsigned k=0;k<396*224;k++)image[k]=NG_PAPER;
 Pixels pixels={0,image};NgCanvas canvas={&pixels,rect};ng_grids_extra[g->id-35].render(g,&canvas);
 char path[512];snprintf(path,sizeof path,"%s/%s.ppm",directory,name);FILE *f=fopen(path,"wb");assert(f);fprintf(f,"P6\n396 224\n255\n");
 for(unsigned k=0;k<396*224;k++){unsigned v=image[k];unsigned char rgb[3]={(unsigned char)(((v>>11)&31)*255/31),(unsigned char)(((v>>5)&63)*255/63),(unsigned char)((v&31)*255/31)};assert(fwrite(rgb,1,3,f)==3);}assert(!fclose(f));
}
static void audit_capture(const char *directory)
{
 uint16_t *image=malloc(396*224*sizeof(*image));assert(image);unsigned h,v,intersection;NgGame g=crossing_case(&h,&v,&intersection);
 const NgModule *m=&ng_grids_extra[0];g.cursor=(uint8_t)h;g.data[0]=1;capture_one(&g,directory,"hashi-empty",image);
 assert(m->action(&g,'6'));capture_one(&g,directory,"hashi-single",image);
 assert(m->action(&g,NGK_EXE));capture_one(&g,directory,"hashi-double",image);
 g.cursor=(uint8_t)v;assert(m->action(&g,'2'));capture_one(&g,directory,"hashi-crossing",image);
 bool found=false;
 for(unsigned index=90;index<120&&!found;index++){
  const GridsNonoPuzzle *p=&grids_nono_pack[index];bool zero=false;
  for(unsigned i=0;i<2*p->n;i++)if(!p->clue[i])zero=true;
  if(!zero)continue;
  g=fresh(36,index/GRIDS_EXTRA_BANK,index%GRIDS_EXTRA_BANK);g.board[0]=1;g.board[1]=0;
  capture_one(&g,directory,"nonogram-zero-marks",image);found=true;
 }
 assert(found);free(image);
}
static void capture(const char *directory)
{
 uint16_t *image=malloc(396*224*sizeof(*image));assert(image);
 for(unsigned id=35;id<=36;id++)for(unsigned d=0;d<(id==35?5u:4u);d++){
  unsigned ordinal=0,best=0;
  for(unsigned q=0;q<GRIDS_EXTRA_BANK;q++){
   unsigned score=0;
   if(id==36){const GridsNonoPuzzle *p=&grids_nono_pack[d*GRIDS_EXTRA_BANK+q];unsigned longest=0;
    for(unsigned i=0;i<2*p->n;i++){unsigned digits=0;uint32_t v=p->clue[i];while(v){digits++;v>>=4;}if(digits>longest)longest=digits;score+=digits;}score+=longest*100;
   }else {const GridsHashiPuzzle *p=&grids_hashi_pack[d*GRIDS_EXTRA_BANK+q];for(unsigned i=0;i<p->count;i++)score+=p->clue[i];}
   if(score>best){best=score;ordinal=q;}
  }
  NgGame g=fresh(id,d,ordinal);char name[32];snprintf(name,sizeof name,"capture-%u-%u",id,d);capture_one(&g,directory,name,image);
 }
 free(image);
}
int main(int argc,char **argv)
{test_grids_extra();if(argc==3&&!strcmp(argv[1],"--capture"))capture(argv[2]);if(argc==3&&!strcmp(argv[1],"--audit-capture"))audit_capture(argv[2]);return 0;}
#endif
