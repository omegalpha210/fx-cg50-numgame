#include "ng.h"
#include "ui.h"
#include "../src/games/guesscalc.h"
#include "../assets/guesscalc_cryptarithm.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,rectangles;
#define CHECK(x) do{++checks;assert(x);}while(0)
static const NgModule *module(const NgGame *g){return &ng_guesscalc_extra[g->id-33];}
static void start(NgGame *g,unsigned id,unsigned d,unsigned ordinal){
 memset(g,0,sizeof(*g));g->id=(uint8_t)id;g->difficulty=(uint8_t)d;g->seed=g->rng=ordinal+1;g->pack_revision=2;g->run_id=1;
 if(id==34){g->supply_seed=30u<<16;g->supply_index=ordinal;}
 module(g)->init(g);CHECK(module(g)->valid(g));
}
static void press(NgGame *g,int key){(void)module(g)->action(g,key);if(!module(g)->valid(g))fprintf(stderr,"invalid id %u key %d moves %u\n",g->id,key,(unsigned)g->moves);CHECK(module(g)->valid(g));}
static void rect(void *ctx,int x,int y,int w,int h,uint16_t color){(void)ctx;(void)color;CHECK(x>=0&&y>=27&&w>0&&h>0&&x+w<=396&&y+h<=185);++rectangles;}
/* Independent padded-board / compass-direction model, no production helpers. */
static int reference_ray(unsigned n,const int16_t *atoms,unsigned port){
 int padded[100]={0};for(unsigned r=0;r<n;r++)for(unsigned c=0;c<n;c++)padded[(r+1)*10+c+1]=atoms[r*n+c]!=0;
 static const int delta[]={-10,1,10,-1};int at,dir;
 if(port<n){at=(int)port+1;dir=2;}
 else if(port<2*n){at=(int)(port-n+1)*10+(int)n+1;dir=3;}
 else if(port<3*n){at=(int)(n+1)*10+(int)(3*n-port);dir=0;}
 else {at=(int)(4*n-port)*10;dir=1;}
 bool entry=true;
 for(unsigned step=0;step<300;step++){
  int front=at+delta[dir];
  if(padded[front])return -1;
  bool flank_l=padded[front+delta[(dir+3)%4]]!=0,flank_r=padded[front+delta[(dir+1)%4]]!=0;
  if(entry&&(flank_l||flank_r))return -2;
  if(flank_l&&flank_r)dir=(dir+2)%4;
  else if(flank_l)dir=(dir+1)%4;
  else if(flank_r)dir=(dir+3)%4;
  else {
   at=front;entry=false;int row=at/10,col=at%10,exit;
   if(row>=1&&row<=(int)n&&col>=1&&col<=(int)n)continue;
   if(row==0)exit=col-1;else if(col==(int)n+1)exit=(int)n+row-1;
   else if(row==(int)n+1)exit=(int)(3*n)-col;else exit=(int)(4*n)-row;
   return exit==(int)port?-2:exit;
  }
 }
 return -3;
}
static unsigned bit_count(unsigned mask){unsigned count=0;while(mask){count+=mask&1;mask>>=1;}return count;}
static void ray_tests(void){
 int signatures[512][12];
 for(unsigned mask=0;mask<512;mask++){
  int16_t atoms[NG_CELLS]={0},turned[NG_CELLS]={0};
  for(unsigned i=0;i<9;i++){atoms[i]=(int16_t)((mask>>i)&1);turned[(i%3)*3+2-i/3]=atoms[i];}
  for(unsigned p=0;p<12;p++){
   int result=gc_blackbox_ray(3,atoms,p);CHECK(result==reference_ray(3,atoms,p)&&result>=-2);signatures[mask][p]=result;
   if(result>=0)CHECK(gc_blackbox_ray(3,atoms,(unsigned)result)==(int)p);
   CHECK(gc_blackbox_ray(3,turned,(p+3)%12)==(result<0?result:(result+3)%12));
  }
 }
 unsigned equivalent=0;
 for(unsigned a=0;a<512;a++)for(unsigned b=a+1;b<512;b++)if(bit_count(a)==bit_count(b)&&!memcmp(signatures[a],signatures[b],sizeof(signatures[a]))){
  NgGame g={0};g.rows=g.cols=3;g.data[0]=(int32_t)bit_count(a);
  for(unsigned i=0;i<9;i++){g.fixed[i]=(uint8_t)((a>>i)&1);g.board[i]=(int16_t)((b>>i)&1);}
  CHECK(gc_blackbox_complete(&g));++equivalent;
 }
 CHECK(equivalent>0);
 int16_t atoms[NG_CELLS]={0};CHECK(gc_blackbox_ray(1,atoms,0)==-3);CHECK(gc_blackbox_ray(9,atoms,0)==-3);CHECK(gc_blackbox_ray(3,NULL,0)==-3);CHECK(gc_blackbox_ray(3,atoms,12)==-3);
 printf("blackbox: 512 exhaustive 3x3 masks, 6144 rays; %u equivalent layout pairs\n",equivalent);
}
static void blackbox_tests(void){
 NgCanvas canvas={NULL,rect};
 for(unsigned d=0;d<4;d++)for(unsigned seed=0;seed<128;seed++){
  NgGame g,original;start(&g,33,d,seed);original=g;module(&g)->render(&g,&canvas);
  NgGame replay;start(&replay,33,d,seed);CHECK(!memcmp(&g,&replay,sizeof(g)));press(&g,NGK_F6);CHECK(g.status==NG_PLAYING&&g.score==5&&g.moves==1);
  g=original;press(&g,NGK_EXE);CHECK(g.board[0]==1&&g.moves==1);press(&g,NGK_DEL);CHECK(g.board[0]==0&&g.moves==2);
  press(&g,NGK_F4);CHECK(g.phase==1);
  for(unsigned port=0;port<4u*g.rows;port++){
   int16_t atoms[NG_CELLS];for(unsigned i=0;i<NG_CELLS;i++)atoms[i]=g.fixed[i];
   press(&g,NGK_EXE);CHECK(g.data[8+port]);int expected=reference_ray(g.rows,atoms,port);
   CHECK(g.data[8+port]==(expected==-1?1:expected==-2?2:expected+3));
   unsigned moves=g.moves;press(&g,NGK_EXE);CHECK(g.moves==moves);press(&g,NGK_RIGHT);
  }
  module(&g)->render(&g,&canvas);press(&g,NGK_F4);
  for(unsigned i=0;i<(unsigned)g.rows*g.cols;i++){g.cursor=(uint8_t)i;press(&g,g.fixed[i]?'1':'2');}
  CHECK(gc_blackbox_complete(&g));press(&g,NGK_F6);CHECK(g.status==NG_WON);module(&g)->render(&g,&canvas);
  CHECK(!module(&g)->action(&g,'1'));
  NgGame bad=g;bad.board[0]=bad.board[0]==1?0:1;CHECK(!module(&bad)->valid(&bad));bad=original;bad.fixed[0]^=1;CHECK(!module(&bad)->valid(&bad));
  bad=original;bad.status=NG_WON;CHECK(!module(&bad)->valid(&bad));bad=original;bad.data[8]=1;CHECK(!module(&bad)->valid(&bad));
  bad=original;bad.data[48]=1;CHECK(!module(&bad)->valid(&bad));bad=original;bad.data[4]=1;CHECK(!module(&bad)->valid(&bad));
  bad=original;bad.data[1]=4*g.rows;CHECK(!module(&bad)->valid(&bad));bad=original;bad.notes[0]=1;CHECK(!module(&bad)->valid(&bad));
  g=original;press(&g,NGK_HINT);CHECK(g.assisted&&g.moves==1&&g.board[g.cursor]==1&&g.fixed[g.cursor]);
 }
}
/* Independent weighted-sum/all-different solver. It has no carry search and
 * never sees the generator's witness. Bounds ignore cross-letter digit reuse,
 * so they are conservative and cannot remove a feasible assignment. */
typedef struct {int coefficient[10],order[10],value[10],answer[10];unsigned letters,leading,count;uint64_t nodes;} SumModel;
static void sum_search(SumModel *m,unsigned depth,unsigned used,int sum){
 ++m->nodes;
 if(m->count>=2)return;
 if(depth==m->letters){if(!sum){++m->count;memcpy(m->answer,m->value,sizeof(m->answer));}return;}
 int low=sum,high=sum;
 for(unsigned k=depth;k<m->letters;k++){
  unsigned letter=(unsigned)m->order[k];int minimum=10,maximum=-1;
  for(int digit=0;digit<10;digit++)if(!(used&(1u<<digit))&&(digit||!(m->leading&(1u<<letter)))){if(digit<minimum)minimum=digit;maximum=digit;}
  if(maximum<0)return;
  int coeff=m->coefficient[letter];low+=coeff*(coeff>=0?minimum:maximum);high+=coeff*(coeff>=0?maximum:minimum);
 }
 if(low>0||high<0)return;
 unsigned letter=(unsigned)m->order[depth];
 for(int digit=0;digit<10;digit++)if(!(used&(1u<<digit))&&(digit||!(m->leading&(1u<<letter)))){
  m->value[letter]=digit;sum_search(m,depth+1,used|(1u<<digit),sum+m->coefficient[letter]*digit);
 }
}
static SumModel independent_crypt(const GcCryptPack *p){
 SumModel model={0};model.letters=p->letters;
 for(unsigned w=0;w<=p->count;w++){
  const char *word=p->words[w];size_t length=strlen(word);if(length>1)model.leading|=1u<<(word[0]-'A');int place=w<p->count?1:-1;
  for(size_t j=length;j>0;j--){model.coefficient[(unsigned)(word[j-1]-'A')]+=place;place*=10;}
 }
 for(unsigned i=0;i<model.letters;i++)model.order[i]=(int)i;
 for(unsigned i=0;i<model.letters;i++)for(unsigned j=i+1;j<model.letters;j++)if(abs(model.coefficient[model.order[j]])>abs(model.coefficient[model.order[i]])){int swap=model.order[i];model.order[i]=model.order[j];model.order[j]=swap;}
 sum_search(&model,0,0,0);return model;
}
static void cryptarithm_tests(void){
 NgCanvas canvas={NULL,rect};uint64_t nodes=0;
 CHECK(gc_extra_bank_count(34,3,0)==30&&gc_extra_bank_count(34,4,0)==0&&gc_extra_bank_count(33,0,0)==0);
 for(unsigned d=0;d<4;d++)for(unsigned ordinal=0;ordinal<30;ordinal++){
  NgGame g,original;start(&g,34,d,ordinal);original=g;CHECK(g.puzzle_id==d*30+ordinal);const GcCryptPack *p=&gc_crypt_pack[g.puzzle_id];module(&g)->render(&g,&canvas);
  CHECK(strlen(p->words[0])==2+d&&strlen(p->words[1])==2+d);
  SumModel model=independent_crypt(p);CHECK(model.count==1);nodes+=model.nodes;
  press(&g,NGK_EXE);CHECK(g.status==NG_PLAYING&&g.score==1);g=original;
  press(&g,'0');CHECK(g.board[0]==0&&g.moves==1);press(&g,NGK_DEL);CHECK(g.board[0]==-1&&g.moves==2);press(&g,NGK_F4);CHECK(g.cursor==1);
  for(unsigned i=0;i<p->letters;i++){g.cursor=(uint8_t)i;press(&g,'0'+model.answer[i]);}
  CHECK(gc_cryptarithm_complete(&g));
  NgGame bad=g;bad.board[0]=bad.board[1];CHECK(!gc_cryptarithm_complete(&bad));bad=g;bad.board[0]=-1;CHECK(!gc_cryptarithm_complete(&bad));
  for(unsigned w=0;w<=p->count;w++){bad=g;bad.board[(unsigned)(p->words[w][0]-'A')]=0;CHECK(!gc_cryptarithm_complete(&bad));}
  press(&g,NGK_F6);CHECK(g.status==NG_WON);module(&g)->render(&g,&canvas);CHECK(!module(&g)->action(&g,'1'));
  bad=original;bad.status=NG_WON;CHECK(!module(&bad)->valid(&bad));bad=original;bad.puzzle_id=(d+1)*30;CHECK(!module(&bad)->valid(&bad));
  bad=original;bad.data[2]=1;CHECK(!module(&bad)->valid(&bad));bad=original;bad.board[p->letters]=1;CHECK(!module(&bad)->valid(&bad));
  bad=original;bad.board[0]=10;CHECK(!module(&bad)->valid(&bad));bad=original;bad.fixed[0]=1;CHECK(!module(&bad)->valid(&bad));
  bad=original;bad.cursor=p->letters;CHECK(!module(&bad)->valid(&bad));
  for(unsigned k=0;k<100;k++){press(&original,NGK_UP+(int)(k%4));CHECK(original.cursor<p->letters);}
 }
 printf("cryptarithm: 120/120 independently unique; weighted-sum nodes %llu\n",(unsigned long long)nodes);
}
int main(void){
 for(unsigned i=0;i<2;i++){unsigned length=0;const char *s=ng_guesscalc_extra[i].rules;for(unsigned j=0;;j++){if(!s[j]||s[j]=='\n'){CHECK(length<=48);length=0;if(!s[j])break;}else ++length;}}
 ray_tests();blackbox_tests();cryptarithm_tests();printf("guesscalc_extra: %u assertions, %u rectangles; PASS\n",checks,rectangles);return 0;
}
