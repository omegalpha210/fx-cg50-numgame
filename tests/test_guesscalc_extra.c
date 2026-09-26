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
static void ray_boundary_tests(void){
 /* Empty paths establish each orientation independently of the reference
  * tracer. Port numbering starts at top-left and proceeds clockwise. */
 for(unsigned n=2;n<=8;n++){
  int16_t atoms[NG_CELLS]={0};
  for(unsigned p=0;p<4*n;p++){
   int exit=(p<n||(p>=2*n&&p<3*n))?(int)(3*n-1-p):(int)(5*n-1-p);
   CHECK(gc_blackbox_ray(n,atoms,p)==exit);
  }
  atoms[0]=1;
  CHECK(gc_blackbox_ray(n,atoms,0)==-1);CHECK(gc_blackbox_ray(n,atoms,4*n-1)==-1);
  CHECK(gc_blackbox_ray(n,atoms,1)==-2);CHECK(gc_blackbox_ray(n,atoms,4*n-2)==-2);
  atoms[1]=1;CHECK(gc_blackbox_ray(n,atoms,0)==-1);CHECK(gc_blackbox_ray(n,atoms,1)==-1);
 }
 int16_t atoms[NG_CELLS]={0};atoms[2*5+2]=1;
 CHECK(gc_blackbox_ray(5,atoms,2)==-1); /* central direct hit */
 CHECK(gc_blackbox_ray(5,atoms,1)==18);CHECK(gc_blackbox_ray(5,atoms,18)==1); /* bends left */
 CHECK(gc_blackbox_ray(5,atoms,3)==6);CHECK(gc_blackbox_ray(5,atoms,6)==3); /* bends right */
 memset(atoms,0,sizeof(atoms));atoms[2*5+1]=atoms[2*5+3]=1;
 CHECK(gc_blackbox_ray(5,atoms,2)==-2); /* two interior diagonal atoms reverse */
 atoms[2*5+2]=1;CHECK(gc_blackbox_ray(5,atoms,2)==-1); /* direct hit has priority */
 memset(atoms,0,sizeof(atoms));atoms[1]=atoms[3]=1;
 CHECK(gc_blackbox_ray(5,atoms,2)==-2); /* both diagonals at the entry edge */
 atoms[2]=1;CHECK(gc_blackbox_ray(5,atoms,2)==-1);
 NgGame g;start(&g,33,0,0);press(&g,NGK_F4);
 press(&g,NGK_LEFT);CHECK(g.data[1]==19);press(&g,NGK_RIGHT);CHECK(g.data[1]==0);
 press(&g,NGK_UP);CHECK(g.data[1]==15);press(&g,NGK_DOWN);CHECK(g.data[1]==0);
 int partner=-1;
 for(unsigned p=0;p<20;p++){
  int16_t hidden[NG_CELLS];for(unsigned i=0;i<NG_CELLS;i++)hidden[i]=g.fixed[i];
  partner=gc_blackbox_ray(5,hidden,p);
  if(partner>=0){g.data[1]=(int32_t)p;press(&g,NGK_EXE);break;}
 }
 CHECK(partner>=0&&g.data[2]==1&&g.score==1&&g.moves==1);
 NgGame paired=g;paired.data[8+(unsigned)partner]=0;CHECK(!module(&paired)->valid(&paired));
 paired=g;paired.data[48+(unsigned)partner]=1;CHECK(!module(&paired)->valid(&paired));
 g.data[1]=partner;press(&g,NGK_EXE);CHECK(g.data[2]==1&&g.score==1&&g.moves==1);
 press(&g,NGK_F4);unsigned count=0;
 for(unsigned i=0;i<25;i++)if(g.fixed[i]){g.board[i]=1;++count;}
 CHECK(count==3&&gc_blackbox_complete(&g));
 unsigned empty=0;while(g.fixed[empty])++empty;g.board[empty]=1;CHECK(!gc_blackbox_complete(&g));
 g.board[empty]=2;CHECK(gc_blackbox_complete(&g)); /* X is exclusion, never an atom */
 unsigned moves=g.moves;press(&g,NGK_HINT);CHECK(!g.assisted&&g.moves==moves);
 press(&g,NGK_F6);CHECK(g.status==NG_WON&&g.score==1);
 const int frozen[]={NGK_LEFT,NGK_UP,NGK_RIGHT,NGK_DOWN,NGK_AUX,NGK_F4,NGK_HINT,NGK_F3,NGK_EXE,NGK_F6,NGK_DEL,'0','1','2'};
 for(unsigned k=0;k<sizeof(frozen)/sizeof(*frozen);k++){NgGame before=g;CHECK(!module(&g)->action(&g,frozen[k]));CHECK(!memcmp(&g,&before,sizeof(g)));}
 start(&g,33,0,0);g.moves=UINT32_MAX;NgGame before=g;CHECK(!module(&g)->action(&g,'1')&&!memcmp(&g,&before,sizeof(g)));
 start(&g,33,0,0);g.data[3]=100000;g.moves=100000;g.score=500000;CHECK(module(&g)->valid(&g));before=g;
 CHECK(!module(&g)->action(&g,NGK_F6)&&!memcmp(&g,&before,sizeof(g)));
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
static unsigned word_value(const char *word,const int16_t *values){
 unsigned value=0;for(unsigned j=0;word[j];j++)value=value*10+(unsigned)values[(unsigned)(word[j]-'A')];return value;
}
static void cryptarithm_boundary_tests(void){
 NgGame g;start(&g,34,0,0);CHECK(g.puzzle_id==0);
 const GcCryptPack *p=&gc_crypt_pack[0];CHECK(!strcmp(p->words[0],"AA")&&!strcmp(p->words[1],"BC")&&!strcmp(p->words[2],"BDB"));
 const int16_t duplicate[]={9,1,2,1};memcpy(g.board,duplicate,sizeof(duplicate));
 CHECK(word_value(p->words[0],g.board)==99&&word_value(p->words[1],g.board)==12&&word_value(p->words[2],g.board)==111);
 CHECK(!gc_cryptarithm_complete(&g));press(&g,NGK_EXE);CHECK(g.status==NG_PLAYING);
 const int16_t zero_lead[]={1,0,9,2};memcpy(g.board,zero_lead,sizeof(zero_lead));
 CHECK(word_value(p->words[0],g.board)==11&&word_value(p->words[1],g.board)==9&&word_value(p->words[2],g.board)==20);
 CHECK(!gc_cryptarithm_complete(&g));press(&g,NGK_F6);CHECK(g.status==NG_PLAYING);
 /* 88+13=101 has two carries, one repeated A and repeated B in its result.
  * D=0 is legal because it is interior, not a leading letter. */
 const int16_t correct[]={8,1,3,0};memcpy(g.board,correct,sizeof(correct));
 CHECK(gc_cryptarithm_complete(&g));CHECK((g.board[0]+g.board[2])%10==g.board[1]);
 CHECK((g.board[0]+g.board[1]+1)%10==g.board[3]);CHECK((g.board[0]+g.board[1]+1)/10==g.board[1]);
 g.board[3]=2;CHECK(!gc_cryptarithm_complete(&g)); /* carry arithmetic, same distinctness */
 g.board[3]=0;g.cursor=3;uint32_t moves=g.moves;press(&g,'0');CHECK(g.moves==moves);
 press(&g,NGK_DEL);CHECK(g.board[3]==-1&&g.moves==moves+1);press(&g,NGK_DEL);CHECK(g.moves==moves+1);
 press(&g,NGK_F4);CHECK(g.cursor==3); /* only empty letter stays selected after wrap */
 press(&g,'0');NgGame before=g;press(&g,'-');CHECK(!memcmp(&g,&before,sizeof(g))); /* addition only */
 press(&g,NGK_F6);CHECK(g.status==NG_WON);
 const int frozen[]={NGK_LEFT,NGK_UP,NGK_RIGHT,NGK_DOWN,NGK_AUX,NGK_F4,NGK_EXE,NGK_F6,NGK_DEL,'0','9'};
 for(unsigned k=0;k<sizeof(frozen)/sizeof(*frozen);k++){before=g;CHECK(!module(&g)->action(&g,frozen[k]));CHECK(!memcmp(&g,&before,sizeof(g)));}
 CHECK(!gc_cryptarithm_complete(NULL));before=g;before.puzzle_id=120;CHECK(!gc_cryptarithm_complete(&before));
 for(int value=-2;value<=10;value+=12){before=g;before.board[0]=(int16_t)value;CHECK(!gc_cryptarithm_complete(&before));}
 start(&g,34,0,0);g.moves=UINT32_MAX;before=g;CHECK(!module(&g)->action(&g,'1')&&!memcmp(&g,&before,sizeof(g)));
 start(&g,34,0,0);g.data[3]=100000;g.moves=g.score=100000;CHECK(module(&g)->valid(&g));before=g;
 CHECK(!module(&g)->action(&g,NGK_EXE)&&!memcmp(&g,&before,sizeof(g)));
}
static uint16_t layout_pixels[396*224],layout_expected[396*224];
static void paint(void *context,int x,int y,int w,int h,uint16_t color){
 CHECK(x>=0&&y>=0&&w>0&&h>0&&x+w<=396&&y+h<=224);uint16_t *pixels=context;
 for(int r=y;r<y+h;r++)for(int col=x;col<x+w;col++)pixels[r*396+col]=color;
}
static void expect_glyph(NgCanvas *c,int center,int y,char value,int color){
 char glyph[2]={value,0};int width=ng_text_width(glyph,1);CHECK(width<=10);
 ng_text(c,center-6+(12-width)/2,y,glyph,color,1);
}
static void crypt_layout(const NgGame *g){
 for(unsigned i=0;i<396*224;i++)layout_pixels[i]=layout_expected[i]=NG_PAPER;
 NgCanvas actual={layout_pixels,paint},expected={layout_expected,paint};NgGame before=*g;
 module(g)->render(g,&actual);CHECK(!memcmp(g,&before,sizeof(before)));
 const GcCryptPack *p=&gc_crypt_pack[g->puzzle_id];size_t widest=0;
 for(unsigned row=0;row<3;row++){
  size_t length=strlen(p->words[row]);if(length>widest)widest=length;
  /* From units leftward: centers90,78,... and186,174,... for digit preview.
   * The right edge is invariant even when carries lengthen the result. */
  for(unsigned place=0;place<length;place++){
   char letter=p->words[row][length-1-place];unsigned index=(unsigned)(letter-'A');int center=90-(int)place*12,y=50+(int)row*22;
   if(index==g->cursor){ng_rect(&expected,center-6,y-2,12,15,NG_PALE);ng_border(&expected,center-6,y-2,12,15,NG_BLUE,1);}
   expect_glyph(&expected,center,y,letter,index==g->cursor?NG_BLUE:NG_INK);
   if(g->board[index]>=0)expect_glyph(&expected,center+96,y,(char)('0'+g->board[index]),NG_BLUE);
  }
 }
 int left=84-(int)widest*12;
 expect_glyph(&expected,left+6,72,'+',ng_operator_color('+',NG_INK));expect_glyph(&expected,left+102,72,'+',ng_operator_color('+',NG_INK));
 ng_line(&expected,left,89,95,89,NG_LINE);ng_line(&expected,left+96,89,191,89,NG_LINE);
 for(unsigned y=46;y<=108;y++)for(unsigned x=10;x<198;x++)CHECK(layout_pixels[y*396+x]==layout_expected[y*396+x]);
 /* Empty mapping cards show just their letter, with no underscore or equals
  * placeholder; assigned cards retain the visible letter=digit mapping. */
 for(unsigned i=0;i<p->letters;i++){
  int x=13+(int)(i%g->cols)*74,y=119+(int)(i/g->cols)*31;char label[20];
  ng_rect(&expected,x+2,y+2,64,23,NG_WHITE);
  if(g->board[i]<0)snprintf(label,sizeof(label),"%c",(int)('A'+i));else snprintf(label,sizeof(label),"%c = %d",(int)('A'+i),g->board[i]);
  ng_text(&expected,x+(68-ng_text_width(label,1))/2,y+8,label,i==g->cursor?NG_BLUE:NG_INK,1);
  for(int yy=y+2;yy<y+25;yy++)for(int xx=x+2;xx<x+66;xx++)CHECK(layout_pixels[yy*396+xx]==layout_expected[yy*396+xx]);
 }
}
static void capture_fixture(const NgGame *g,const char *directory,const char *name){
 for(unsigned i=0;i<396*224;i++)layout_pixels[i]=NG_PAPER;
 NgCanvas canvas={layout_pixels,paint};ng_text(&canvas,10,8,module(g)->name,NG_INK,1);module(g)->render(g,&canvas);
 ng_wrap(&canvas,10,188,376,9,2,g->message,NG_MUTED,true);
 char path[1024];int n=snprintf(path,sizeof(path),"%s/%s.ppm",directory,name);CHECK(n>0&&(size_t)n<sizeof(path));
 FILE *out=fopen(path,"wb");CHECK(out!=NULL);fprintf(out,"P6\n396 224\n255\n");
 for(unsigned i=0;i<396*224;i++){
  uint16_t pixel=layout_pixels[i];unsigned char rgb[]={(unsigned char)(((pixel>>11)&31)*255/31),(unsigned char)(((pixel>>5)&63)*255/63),(unsigned char)((pixel&31)*255/31)};
  CHECK(fwrite(rgb,1,3,out)==3);
 }
 CHECK(!fclose(out));
}
static void captures(const char *directory){
 NgGame g;start(&g,34,0,0);capture_fixture(&g,directory,"crypt-easy-empty");
 start(&g,34,3,0);g.cursor=(uint8_t)(gc_crypt_pack[g.puzzle_id].letters-1);press(&g,'7');capture_fixture(&g,directory,"crypt-master-partial");
 SumModel solution=independent_crypt(&gc_crypt_pack[g.puzzle_id]);for(unsigned i=0;i<(unsigned)g.data[1];i++){g.cursor=(uint8_t)i;press(&g,'0'+solution.answer[i]);}
 press(&g,NGK_F6);capture_fixture(&g,directory,"crypt-master-solved");
 start(&g,33,3,0);press(&g,NGK_F4);for(unsigned i=0;i<12;i++){press(&g,NGK_EXE);press(&g,NGK_RIGHT);}capture_fixture(&g,directory,"blackbox-master-rays");
 press(&g,NGK_F4);press(&g,'1');press(&g,NGK_RIGHT);press(&g,'2');capture_fixture(&g,directory,"blackbox-master-marks");
}
static void cryptarithm_tests(void){
 NgCanvas canvas={NULL,rect};uint64_t nodes=0;
 CHECK(gc_extra_bank_count(34,3,0)==30&&gc_extra_bank_count(34,4,0)==0&&gc_extra_bank_count(33,0,0)==0);
 for(unsigned d=0;d<4;d++)for(unsigned ordinal=0;ordinal<30;ordinal++){
  NgGame g,original;start(&g,34,d,ordinal);original=g;CHECK(g.puzzle_id==d*30+ordinal);const GcCryptPack *p=&gc_crypt_pack[g.puzzle_id];module(&g)->render(&g,&canvas);
  CHECK(strlen(p->words[0])==2+d&&strlen(p->words[1])==2+d);
  crypt_layout(&g);g.cursor=(uint8_t)(p->letters-1);press(&g,'7');crypt_layout(&g);g=original;
  SumModel model=independent_crypt(p);CHECK(model.count==1);nodes+=model.nodes;
  press(&g,NGK_EXE);CHECK(g.status==NG_PLAYING&&g.score==1);g=original;
  press(&g,'0');CHECK(g.board[0]==0&&g.moves==1);press(&g,NGK_DEL);CHECK(g.board[0]==-1&&g.moves==2);press(&g,NGK_F4);CHECK(g.cursor==1);
  for(unsigned i=0;i<p->letters;i++){g.cursor=(uint8_t)i;press(&g,'0'+model.answer[i]);}
  CHECK(gc_cryptarithm_complete(&g));
  NgGame bad=g;bad.board[0]=bad.board[1];CHECK(!gc_cryptarithm_complete(&bad));bad=g;bad.board[0]=-1;CHECK(!gc_cryptarithm_complete(&bad));
  for(unsigned w=0;w<=p->count;w++){bad=g;bad.board[(unsigned)(p->words[w][0]-'A')]=0;CHECK(!gc_cryptarithm_complete(&bad));}
  press(&g,NGK_F6);CHECK(g.status==NG_WON);module(&g)->render(&g,&canvas);crypt_layout(&g);CHECK(!module(&g)->action(&g,'1'));
  bad=original;bad.status=NG_WON;CHECK(!module(&bad)->valid(&bad));bad=original;bad.puzzle_id=(d+1)*30;CHECK(!module(&bad)->valid(&bad));
  bad=original;bad.data[2]=1;CHECK(!module(&bad)->valid(&bad));bad=original;bad.board[p->letters]=1;CHECK(!module(&bad)->valid(&bad));
  bad=original;bad.board[0]=10;CHECK(!module(&bad)->valid(&bad));bad=original;bad.fixed[0]=1;CHECK(!module(&bad)->valid(&bad));
  bad=original;bad.cursor=p->letters;CHECK(!module(&bad)->valid(&bad));
  for(unsigned k=0;k<100;k++){press(&original,NGK_UP+(int)(k%4));CHECK(original.cursor<p->letters);}
 }
 printf("cryptarithm: 120/120 independently unique; weighted-sum nodes %llu\n",(unsigned long long)nodes);
}
int main(int argc,char **argv){
 CHECK(argc==1||(argc==3&&!strcmp(argv[1],"--capture-dir")));CHECK(ng_text_width("I",1)!=ng_text_width("A",1));
 for(unsigned i=0;i<2;i++){unsigned length=0;const char *s=ng_guesscalc_extra[i].rules;for(unsigned j=0;;j++){if(!s[j]||s[j]=='\n'){CHECK(length<=48);length=0;if(!s[j])break;}else ++length;}}
 ray_tests();ray_boundary_tests();blackbox_tests();cryptarithm_boundary_tests();cryptarithm_tests();if(argc==3)captures(argv[2]);printf("guesscalc_extra: %u assertions, %u rectangles; PASS\n",checks,rectangles);return 0;
}
