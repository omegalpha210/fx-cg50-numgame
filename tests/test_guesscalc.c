#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE 1
#endif
#include "ng.h"
#include "ui.h"
#include "../src/games/guesscalc_math.h"
#include "../src/games/guesscalc.h"
#include "test_guesscalc_workflow.h"
#include "test_guesscalc_legacy.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
static unsigned checks,rects;
#define CHECK(x) do{++checks;assert(x);}while(0)
static const NgModule *module(const NgGame *g){return &ng_guesscalc[g->id-1];}
static unsigned old_modes(unsigned id){return id==1?6:id==6?2:ng_guesscalc[id-1].modes;}
static void start(NgGame *g,unsigned id,unsigned d,unsigned mode,uint32_t seed){memset(g,0,sizeof(*g));g->id=(uint8_t)id;g->difficulty=(uint8_t)d;g->mode=(uint8_t)mode;g->seed=g->rng=seed?seed:1;g->run_id=1;g->pack_revision=2;module(g)->init(g);CHECK(module(g)->valid(g));}
static void press(void *ctx,int key){NgGame *g=ctx;(void)module(g)->action(g,key);if(!module(g)->valid(g))fprintf(stderr,"invalid after id=%u key=%d status=%u moves=%u input=%s history=%s\n",g->id,key,g->status,(unsigned)g->moves,g->input,g->history[0]);CHECK(module(g)->valid(g));}
static void rect(void *ctx,int x,int y,int w,int h,uint16_t color){(void)ctx;(void)color;CHECK(x>=0&&y>=27&&w>0&&h>0&&x+w<=396&&y+h<=185);rects++;}
static void parser_tests(void){
 GcExpression e;int16_t cards[]={3,3,8,8};
 CHECK(gc_expression("8/(3-8/3)",false,&e));CHECK(e.value.num==24&&e.value.den==1);CHECK(gc_cards(&e,cards,4,true));
 CHECK(gc_expression("8/(3-8/3)",false,&e));int16_t wrong[]={3,8,8,9};CHECK(!gc_cards(&e,wrong,4,true));
 CHECK(gc_expression("1/3+1/6",false,&e));CHECK(e.value.num==1&&e.value.den==2);
 CHECK(gc_expression("-(-3)+8",false,&e));CHECK(e.value.num==11);
 CHECK(gc_expression("3-8+3",false,&e));CHECK(e.value.num==-2);
 const char *bad[]={"","1/0","1/(3-3)","2**3","01+2","1+","()","2(3)","2^3","3!","1 2","pi","1000001","1000000*1000000","(((((((((((((1)))))))))))))","--","1=1","0/0"};
 for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);i++)CHECK(!gc_expression(bad[i],false,&e));
 CHECK(!gc_expression("3/2*2",true,&e));CHECK(!gc_expression("2-3+4",true,&e));CHECK(!gc_expression("3-3+5",true,&e));CHECK(!gc_expression("-(-3)",true,&e));CHECK(gc_expression("(100+5)*2",true,&e));CHECK(e.value.num==210);
 int16_t cd[]={100,5,2,3,4,6};CHECK(gc_cards(&e,cd,6,false));CHECK(!gc_cards(&e,cd,6,true));
 for(int a=1;a<=40;a++)for(int b=1;b<=40;b++){char s[64];snprintf(s,sizeof(s),"%d/%d+%d/%d",a,b,b,a);CHECK(gc_expression(s,false,&e));CHECK(e.value.num*(int64_t)a*b==((int64_t)a*a+b*b)*e.value.den);}
 for(int a=-30;a<=30;a++)for(int b=-30;b<=30;b++){char s[64];snprintf(s,sizeof(s),"(%d)*(%d)",a,b);CHECK(gc_expression(s,false,&e));CHECK(e.value.den==1&&e.value.num==a*b);}
 char long_expr[110];memset(long_expr,'1',109);long_expr[109]=0;CHECK(!gc_expression(long_expr,false,&e));
 CHECK(gc_equation("12+7=19"));CHECK(gc_equation("2+2=4"));CHECK(gc_equation("8/4=2"));CHECK(gc_equation("5-9=-4"));CHECK(!gc_equation("1-1=-00"));CHECK(!gc_equation("01+2=3"));CHECK(!gc_equation("1+2=03"));CHECK(!gc_equation("2+2=5"));CHECK(!gc_equation("1--1=2"));CHECK(!gc_equation("1+1==2"));CHECK(!gc_equation("(2+2)=4"));
 CHECK(gc_factorization("5*3^2*2^3",360));CHECK(gc_factorization("2*2*2*3*3*5",360));CHECK(!gc_factorization("1*360",360));CHECK(!gc_factorization("6*60",360));CHECK(!gc_factorization("2^0*3",3));CHECK(!gc_factorization("2^99999999",8));CHECK(!gc_factorization("2^20*2^20",1000000));CHECK(!gc_factorization("02*3",6));CHECK(!gc_factorization("2*",2));CHECK(gc_factorization("9973",9973));CHECK(!gc_is_prime(1));CHECK(gc_is_prime(2));CHECK(!gc_is_prime(1000000));
}
static void feedback_tests(void){
 unsigned s,b;gc_baseball("0012","0000",4,&s,&b);CHECK(s==2&&b==0);gc_baseball("1122","2211",4,&s,&b);CHECK(s==0&&b==4);gc_baseball("0123","0123",4,&s,&b);CHECK(s==4&&b==0);
 uint8_t f[8];gc_feedback("11+1=12","12+1=11",7,f);CHECK(f[0]==2&&f[1]==1&&f[2]==2&&f[3]==2&&f[4]==2&&f[5]==2&&f[6]==1);
 for(unsigned a=0;a<10000;a+=13)for(unsigned z=0;z<10000;z+=113){char x[6],y[6];snprintf(x,sizeof(x),"%04u",a);snprintf(y,sizeof(y),"%04u",z);gc_baseball(x,y,4,&s,&b);unsigned exact=0,ax[10]={0},by[10]={0},common=0;for(unsigned i=0;i<4;i++){exact+=x[i]==y[i];ax[x[i]-'0']++;by[y[i]-'0']++;}for(unsigned i=0;i<10;i++)common+=ax[i]<by[i]?ax[i]:by[i];CHECK(s==exact&&s+b==common&&s+b<=4);}
}
static void lifecycle_tests(void){
 NgCanvas canvas={NULL,rect};
 for(unsigned id=0;id<10;id++){
  const char *r=ng_guesscalc[id].rules;unsigned width=0;
  for(unsigned i=0;;i++){if(r[i]=='\n'||!r[i]){CHECK(width<=48);width=0;if(!r[i])break;}else width++;}
 }
 for(unsigned id=1;id<=10;id++)for(unsigned mode=0;mode<old_modes(id);mode++)for(unsigned d=0;d<4;d++)for(unsigned seed=1;seed<=20;seed++){
  NgGame g,initial;start(&g,id,d,mode,seed);initial=g;module(&g)->render(&g,&canvas);press(&g,NGK_EXE);CHECK(g.status==NG_PLAYING);press(&g,'X');CHECK(g.status==NG_PLAYING);
  if(module(&g)->flags&NGF_HINT){press(&g,NGK_HINT);CHECK(g.assisted==1);}
  start(&g,id,d,mode,seed);CHECK(memcmp(&g,&initial,sizeof(g))==0);gc_test_solve(&g,&g,press);CHECK(g.status==NG_WON);module(&g)->render(&g,&canvas);NgGame copy=g;CHECK(module(&copy)->valid(&copy));CHECK(!module(&copy)->action(&copy,'1'));
 }
}
static void game_rules_tests(void){
 NgGame g;start(&g,1,0,0,2);memcpy(g.board,(int16_t[]){'0','1','2','3'},8);gc_test_type(&g,press,"0012");press(&g,NGK_EXE);CHECK(g.moves==0);while(g.input[0])press(&g,NGK_DEL);gc_test_type(&g,press,"0123");press(&g,NGK_EXE);CHECK(g.status==NG_WON);
 start(&g,1,2,3,2);while(g.status==NG_PLAYING){const char *wrong="9999";if(g.board[0]=='9'&&g.board[1]=='9'&&g.board[2]=='9'&&g.board[3]=='9')wrong="0000";gc_test_type(&g,press,wrong);press(&g,NGK_EXE);}CHECK(g.status==NG_LOST);CHECK(strstr(g.message,"Secret:")!=NULL);
 start(&g,2,0,0,3);char secret[9];for(unsigned i=0;i<(unsigned)g.data[0];i++)secret[i]=(char)g.board[i];secret[g.data[0]]=0;unsigned pivot=g.puzzle_id/30*30;for(unsigned i=pivot;i<pivot+30;i++)if(strcmp(gc_equation_pack[i],secret)){gc_test_type(&g,press,gc_equation_pack[i]);break;}press(&g,NGK_EXE);CHECK(g.status==NG_PLAYING&&g.moves==1);
 start(&g,6,2,0,1);g.puzzle_id=60;const GcCardPack *p=&gc_target_pack[60];for(unsigned i=0;i<4;i++)g.board[i]=p->cards[i];gc_test_type(&g,press,"8/(3-8/3)");press(&g,NGK_EXE);CHECK(g.status==NG_WON);
 start(&g,6,0,1,1);press(&g,NGK_HINT);CHECK(g.assisted&&!g.input[0]);press(&g,NGK_AUX);CHECK(g.assisted&&g.status==NG_PLAYING&&g.input[0]);press(&g,NGK_EXE);CHECK(g.status==NG_WON);
 start(&g,7,0,0,2);char s[30];snprintf(s,sizeof(s),"%d",g.board[0]);gc_test_type(&g,press,s);press(&g,NGK_EXE);CHECK(g.status==NG_PLAYING&&g.moves==1);CHECK(g.data[3]==abs(g.board[0]-g.data[0]));press(&g,NGK_AUX);CHECK(g.status==NG_DRAW);
 start(&g,8,2,0,1);press(&g,NGK_AUX);CHECK(g.assisted&&g.notes[0]);press(&g,NGK_DEL);CHECK(!g.notes[0]);
 start(&g,9,0,0,1);for(unsigned i=0;i<9;i++)if(g.fixed[i]){g.cursor=(uint8_t)i;int before=g.board[i];press(&g,NGK_DEL);CHECK(g.board[i]==before);press(&g,'1');CHECK(g.board[i]==before);break;}
 start(&g,10,0,0,3);gc_test_type(&g,press,"1");press(&g,NGK_EXE);CHECK(g.status==NG_PLAYING);press(&g,NGK_AUX);CHECK(g.assisted);press(&g,NGK_EXE);CHECK(g.status==NG_WON);
}
static void all_pack_engine_tests(void){
 for(unsigned id=3;id<=7;id++)for(unsigned i=0;i<(id==6?180u:90u);i++){
  NgGame g;start(&g,id,(i%90)/30,id==6?i/90:0,1);
  for(unsigned seed=2;g.puzzle_id!=i;seed++){CHECK(seed<10000);start(&g,id,(i%90)/30,id==6?i/90:0,seed);}
  gc_test_solve(&g,&g,press);CHECK(g.status==NG_WON);
 }
 for(unsigned i=0;i<90;i++){NgGame g;start(&g,9,i/30,0,1);for(unsigned seed=2;g.puzzle_id!=i;seed++){CHECK(seed<10000);start(&g,9,i/30,0,seed);}gc_test_solve(&g,&g,press);CHECK(g.status==NG_WON);}
 for(unsigned i=0;i<270;i++)CHECK(gc_equation(gc_equation_pack[i]));
}

extern unsigned gc_bank_count(unsigned id,unsigned difficulty,unsigned mode);
static void master_start(NgGame *g,unsigned id,unsigned mode,unsigned ordinal){
 memset(g,0,sizeof(*g));g->id=(uint8_t)id;g->difficulty=NG_MASTER;g->mode=(uint8_t)mode;
 g->seed=g->rng=739;g->run_id=7;g->pack_revision=2;
 /* Nonzero shuffle with stride1/offset0: direct, deterministic bank ordinals. */
 g->supply_seed=30u<<16;g->supply_index=ordinal;module(g)->init(g);CHECK(module(g)->valid(g));
}
static void master_bank_tests(void){
 NgCanvas canvas={NULL,rect};unsigned visited=0,operator_alternatives=0;
 for(unsigned id=1;id<=10;id++)for(unsigned mode=0;mode<old_modes(id);mode++){
  CHECK(gc_bank_count(id,NG_HELL,mode)==0);CHECK(gc_bank_count(id,NG_MASTER,255)==0);
  unsigned count=gc_bank_count(id,NG_MASTER,mode);
  if(id==6){CHECK(count==0);count=30;}
  CHECK(count==(id==1||id==10?0u:30u));
  for(unsigned i=0;i<count;i++){
   NgGame g;master_start(&g,id,mode,i);NgGame initial=g,copy=g;
   unsigned first=id==2?270:id==6?180:id==8?0:90;
   CHECK(g.puzzle_id==first+mode*30+i);++visited;
   copy.status=NG_WON;CHECK(!module(&copy)->valid(&copy));
   copy=g;copy.pack_revision=1;CHECK(!module(&copy)->valid(&copy));
   copy=g;copy.difficulty=NG_HELL;CHECK(!module(&copy)->valid(&copy));
   copy=g;copy.puzzle_id=first+mode*30+30;CHECK(!module(&copy)->valid(&copy));
   copy=g;copy.puzzle_id=UINT32_MAX;CHECK(!module(&copy)->valid(&copy));
   module(&g)->render(&g,&canvas);gc_test_solve(&g,&g,press);CHECK(g.status==NG_WON);module(&g)->render(&g,&canvas);
   master_start(&copy,id,mode,i+30);CHECK(copy.puzzle_id==initial.puzzle_id);copy.supply_index=i;CHECK(!memcmp(&copy,&initial,sizeof(copy)));
   if(id==6){
    g=initial;char alternative[97];snprintf(alternative,sizeof(alternative),"--(%s)",gc_master_target[g.puzzle_id-180].answer);
    gc_test_type(&g,press,alternative);press(&g,NGK_EXE);CHECK(g.status==NG_WON&&!g.assisted);
   }
   if(id==7){
    g=initial;char partial[8];snprintf(partial,sizeof(partial),"%d",g.board[0]);gc_test_type(&g,press,partial);press(&g,NGK_EXE);
    CHECK(g.status==NG_PLAYING&&g.moves==1&&g.data[3]>0);press(&g,NGK_F4);CHECK(g.status==NG_DRAW);
   }
   if(id==8){
    unsigned solutions=0;
    for(unsigned bits=0;bits<1024;bits++){
     g=initial;unsigned m=bits;char expr[16];
     for(unsigned slot=0;slot<6;slot++){
      expr[slot*2]=(char)('0'+g.board[slot]);
      if(slot<5){g.cursor=(uint8_t)slot;expr[slot*2+1]="+-*/"[m%4];press(&g,expr[slot*2+1]);m/=4;}
     }
     expr[11]=0;GcExpression e;CHECK(gc_expression(expr,false,&e));bool expected=e.value.num==(int64_t)g.data[1]*e.value.den;
     press(&g,NGK_EXE);CHECK((g.status==NG_WON)==expected);
     if(expected){solutions++;bool different=false;for(unsigned slot=0;slot<5;slot++)different|=g.notes[slot]!=g.data[16+slot];operator_alternatives+=different;}
    }
    CHECK(solutions>=1&&solutions<=3);
   }
  }
 }
 CHECK(visited==330&&operator_alternatives>0);
 CHECK(gc_bank_count(0,0,0)==0&&gc_bank_count(11,0,0)==0);
}
static void master_runtime_tests(void){
 for(unsigned mode=0;mode<6;mode++)for(unsigned seed=1;seed<=100;seed++){
  NgGame g;start(&g,1,NG_MASTER,mode,seed);CHECK(g.data[0]==((int[]){6,5,7,6,5,7})[mode]&&g.data[1]==16);
  NgGame bad=g;bad.pack_revision=1;CHECK(!module(&bad)->valid(&bad));
  unsigned mask=0;for(unsigned i=0;i<(unsigned)g.data[0];i++){CHECK(g.board[i]>='0'&&g.board[i]<='9');unsigned bit=1u<<(g.board[i]-'0');if(mode<3)CHECK(!(mask&bit));mask|=bit;}
  if(seed==1){
   for(unsigned tries=0;tries<16;tries++){
    char wrong[8];for(unsigned i=0;i<(unsigned)g.data[0];i++)wrong[i]=(char)('0'+i);wrong[g.data[0]]=0;
    bool same=true;for(unsigned i=0;i<(unsigned)g.data[0];i++)same&=g.board[i]==wrong[i];if(same){char t=wrong[0];wrong[0]=wrong[1];wrong[1]=t;}
    gc_test_type(&g,press,wrong);press(&g,NGK_EXE);CHECK(g.status==(tries==15?NG_LOST:NG_PLAYING));
   }
   CHECK(g.moves==16&&g.history_count==16&&g.score==16);
  }
 }
 for(unsigned seed=1;seed<=3000;seed++){
  NgGame g;start(&g,10,NG_MASTER,0,seed);unsigned n=(unsigned)g.data[0],distinct=0,squared=0;
  for(unsigned prime=2;prime<=n;prime++)if(n%prime==0){unsigned exponent=0;do{n/=prime;exponent++;}while(n%prime==0);CHECK(prime<=13&&exponent<=2);distinct++;squared+=exponent==2;}
  CHECK(distinct==4&&squared==2&&n==1);
  NgGame bad=g;bad.data[0]=9973;CHECK(!module(&bad)->valid(&bad));bad=g;bad.data[0]=2*3*5*7;CHECK(!module(&bad)->valid(&bad));bad=g;bad.pack_revision=1;CHECK(!module(&bad)->valid(&bad));
 }
}
static void legacy_state_tests(void){
 const unsigned seeds[]={1,17,UINT32_MAX};unsigned fixture=0;
 for(unsigned id=1;id<=10;id++)for(unsigned mode=0;mode<old_modes(id);mode++)for(unsigned d=0;d<3;d++)for(unsigned s=0;s<3;s++){
  NgGame g;start(&g,id,d,mode,seeds[s]);CHECK(gc_legacy_fingerprint(&g)==gc_legacy_states[fixture++]);
  g.pack_revision=1;CHECK(module(&g)->valid(&g));gc_test_solve(&g,&g,press);CHECK(g.status==NG_WON&&module(&g)->valid(&g));
 }
 CHECK(fixture==sizeof(gc_legacy_states)/sizeof(*gc_legacy_states));
}

static void select_pack(NgGame *g,unsigned id,unsigned index){
 for(unsigned seed=1;;seed++){CHECK(seed<10000);start(g,id,(index%90)/30,id==6?index/90:0,seed);if(g->puzzle_id==index)return;}
}
static void alternative_and_mutation_tests(void){
 NgGame g;bool found=false;
 for(unsigned i=0;i<180&&!found;i++){
  const GcCardPack *p=&gc_target_pack[i];int total=0;for(unsigned k=0;k<4;k++)total+=p->cards[k];if(total!=p->target)continue;
  select_pack(&g,6,i);char expr[64];snprintf(expr,sizeof(expr),"%d+%d+%d+%d",p->cards[3],p->cards[2],p->cards[1],p->cards[0]);
  CHECK(strcmp(expr,p->answer)!=0);gc_test_type(&g,press,expr);press(&g,NGK_EXE);CHECK(g.status==NG_WON);found=true;
 }
 CHECK(found);found=false;
 for(unsigned i=0;i<90&&!found;i++)for(unsigned k=0;k<6&&!found;k++){
  const GcCardPack *p=&gc_countdown_pack[i];if(p->cards[k]!=p->target)continue;
  select_pack(&g,7,i);char expr[16];snprintf(expr,sizeof(expr),"%d",p->target);CHECK(strcmp(expr,p->answer));gc_test_type(&g,press,expr);press(&g,NGK_EXE);CHECK(g.status==NG_WON);found=true;
 }
 CHECK(found);
 /* Every combination is evaluated by the actual action for random operator puzzles. */
 unsigned alternatives=0;
 for(unsigned seed=1;seed<=30;seed++){
  start(&g,8,2,0,seed);NgGame initial=g;
  for(unsigned mask=0;mask<256;mask++){
   g=initial;unsigned m=mask;char expr[32];size_t pos=0;
   for(unsigned i=0;i<5;i++){expr[pos++]=(char)('0'+g.board[i]);if(i<4){unsigned op=m%4;m/=4;g.cursor=(uint8_t)i;press(&g,"+-*/"[op]);expr[pos++]="+-*/"[op];}}
   expr[pos]=0;GcExpression e;bool expected=gc_expression(expr,false,&e)&&e.value.num==(int64_t)g.data[1]*e.value.den;
   press(&g,NGK_EXE);CHECK((g.status==NG_WON)==expected);if(expected){bool differs=false;for(unsigned i=0;i<4;i++)differs|=g.notes[i]!=g.data[16+i];alternatives+=differs;}
  }
 }
 CHECK(alternatives>0);
 for(unsigned id=1;id<=10;id++){
  start(&g,id,1,0,71);NgGame copy=g;copy.status=NG_WON;CHECK(!module(&copy)->valid(&copy));copy=g;copy.status=NG_LOST;CHECK(!module(&copy)->valid(&copy));copy=g;copy.difficulty=255;CHECK(!module(&copy)->valid(&copy));copy=g;copy.mode=255;CHECK(!module(&copy)->valid(&copy));
  if(id!=1&&id!=8&&id!=10){copy=g;copy.puzzle_id=UINT32_MAX;CHECK(!module(&copy)->valid(&copy));}
  copy=g;if(id==1){copy.board[0]='X';CHECK(!module(&copy)->valid(&copy));}
  if(id==8){copy.notes[0]=256+'+';CHECK(!module(&copy)->valid(&copy));copy=g;copy.data[16]=INT32_MAX;CHECK(!module(&copy)->valid(&copy));}
 }
 for(unsigned id=1;id<=10;id++){
  start(&g,id,1,0,71);gc_test_solve(&g,&g,press);CHECK(module(&g)->valid(&g));
  if(id<=2)g.history[0][id==1?4:0]='X';
  else if(id==8)g.notes[0]=0;
  else if(id==9){for(unsigned i=0;i<9;i++)if(!g.fixed[i]){g.board[i]=0;break;}}
  else g.input[0]=0;
  CHECK(!module(&g)->valid(&g));
 }
 uint32_t rng=17;const char alphabet[]="0123456789+-*/()^abc=";
 for(unsigned trial=0;trial<50000;trial++){
  char expr[97];rng=rng*1664525u+1013904223u;unsigned len=rng%97;
  for(unsigned i=0;i<len;i++){rng=rng*1664525u+1013904223u;expr[i]=alphabet[rng%(sizeof(alphabet)-1)];}expr[len]=0;GcExpression e;
  bool ok=gc_expression(expr,false,&e);if(ok){CHECK(e.value.den>0&&e.value.den<=1000000000&&e.value.num>=-1000000000&&e.value.num<=1000000000&&e.count<=16&&e.operations<=31);}
 }
}


/* A protected page makes an over-read reproducible even without ASan. */
static void bounded_text_tests(void){
 size_t page=(size_t)sysconf(_SC_PAGESIZE);
 CHECK(page>=NG_INPUT);
 char *region=mmap(NULL,page*2,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);
 CHECK(region!=MAP_FAILED);CHECK(!mprotect(region+page,page,PROT_NONE));
 char *input=region+page-NG_INPUT;GcExpression e;
 memset(input,'1',NG_INPUT);CHECK(!gc_expression(input,false,&e));
 input[94]='=';input[95]='-';CHECK(!gc_equation(input));
 memset(input,'0',NG_INPUT);input[0]='2';input[1]='^';CHECK(!gc_factorization(input,8));
 input[NG_INPUT-1]=0;CHECK(!gc_factorization(input,8));
 memset(input,'1',NG_INPUT);input[NG_INPUT-1]=0;CHECK(!gc_expression(input,false,&e));
 CHECK(!munmap(region,page*2));
 CHECK(!gc_expression(NULL,false,&e));CHECK(!gc_expression("1",false,NULL));
 CHECK(!gc_equation(NULL));CHECK(!gc_factorization(NULL,4));
}
static void replace_input(NgGame *g,const char *text){
 while(g->input[0])press(g,NGK_DEL);
 gc_test_type(g,press,text);
}
static void countdown_score_tests(void){
 NgGame g;select_pack(&g,7,60);CHECK(g.data[0]==100);
 NgGame bad=g;bad.data[3]=1;bad.data[4]=101;bad.score=7;bad.status=NG_DRAW;
 CHECK(!module(&bad)->valid(&bad));
 bad=g;bad.moves=1;bad.data[3]=0;bad.data[4]=100;bad.score=10;
 CHECK(!module(&bad)->valid(&bad));
 const char *expressions[]={"100+1","100+3+2","100+3+2+1","100+(75-25)/(3+2)","100+(75-25)/(3+2)+1"};
 const int distances[]={1,5,6,10,11};const unsigned scores[]={7,7,5,5,0};
 for(unsigned i=0;i<5;i++){
  select_pack(&g,7,60);gc_test_type(&g,press,expressions[i]);press(&g,NGK_EXE);
  CHECK(g.data[3]==distances[i]&&g.score==scores[i]&&g.moves==1&&g.status==NG_PLAYING);
  press(&g,NGK_AUX);CHECK(g.status==NG_DRAW&&module(&g)->valid(&g));
 }
 select_pack(&g,7,60);gc_test_type(&g,press,"100+1");press(&g,NGK_EXE);
 const char *invalid[]={"100/3*3","25-75+100","100-75-25+1","100+1-1","100+100"};
 for(unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);i++){
  replace_input(&g,invalid[i]);press(&g,NGK_EXE);
  CHECK(g.status==NG_PLAYING&&g.data[3]==1&&g.data[4]==101&&g.score==7&&g.moves==1);
 }
 replace_input(&g,"100+(75-25)/(3+2)+1");press(&g,NGK_EXE);
 CHECK(g.data[3]==1&&g.score==7&&g.moves==2);
 replace_input(&g,"75+25");press(&g,NGK_EXE);
 CHECK(g.status==NG_WON&&g.score==10&&g.data[3]==0&&g.moves==3);
 NgGame finished=g;press(&g,NGK_EXE);CHECK(!memcmp(&finished,&g,sizeof(g)));
}
static void additional_metamorphic_tests(void){
 NgGame g;GcExpression e;
 CHECK(gc_expression("8/(3-8/3)",false,&e));
 int16_t cards[4]={3,3,8,8};
 for(unsigned a=0;a<4;a++)for(unsigned b=0;b<4;b++)if(a!=b)
 for(unsigned c=0;c<4;c++)if(c!=a&&c!=b)for(unsigned d=0;d<4;d++)if(d!=a&&d!=b&&d!=c){
  int16_t permuted[]={cards[a],cards[b],cards[c],cards[d]};CHECK(gc_cards(&e,permuted,4,true));
 }
 const char *cheats[]={"8/(3-8/3)+0","8/(3-8/3)+3-3","8*3"};
 for(unsigned i=0;i<3;i++){select_pack(&g,6,60);gc_test_type(&g,press,cheats[i]);press(&g,NGK_EXE);CHECK(g.status==NG_PLAYING&&!g.moves);}
 /* Relabel every character bijectively; feedback positions must be invariant. */
 const char alphabet[]="0123+-*/=",relabeled[]="=*/-+3210";uint32_t rng=987;
 for(unsigned trial=0;trial<2000;trial++){
  char secret[16],guess[16],mapped_secret[16],mapped_guess[16];uint8_t original[16],mapped[16];unsigned n=1+trial%16;
  for(unsigned i=0;i<n;i++){rng=rng*1664525u+1013904223u;unsigned a=rng%9;rng=rng*1664525u+1013904223u;unsigned b=rng%9;secret[i]=alphabet[a];guess[i]=alphabet[b];mapped_secret[i]=relabeled[a];mapped_guess[i]=relabeled[b];}
  gc_feedback(secret,guess,n,original);gc_feedback(mapped_secret,mapped_guess,n,mapped);CHECK(!memcmp(original,mapped,n));
 }
 /* A different, equally true equation cannot win the hidden-text game. */
 bool checked=false;
 for(unsigned seed=1;seed<200&&!checked;seed++){
  start(&g,2,0,0,seed);char secret[9]={0};for(int i=0;i<g.data[0];i++)secret[i]=(char)g.board[i];
  int a,b,r;if(sscanf(secret,"%d+%d=%d",&a,&b,&r)!=3||a==b)continue;
  char alternative[32];snprintf(alternative,sizeof(alternative),"%d+%d=%d",b,a,r);
  CHECK(strcmp(secret,alternative)&&gc_equation(alternative));gc_test_type(&g,press,alternative);press(&g,NGK_EXE);
  CHECK(g.status==NG_PLAYING&&g.moves==1);gc_test_type(&g,press,secret);press(&g,NGK_EXE);CHECK(g.status==NG_WON&&g.moves==2);checked=true;
 }
 CHECK(checked);
 /* Mutating any one code position must violate at least one public clue. */
 for(unsigned index=0;index<90;index++){
  select_pack(&g,3,index);NgGame initial=g;const GcMindPack *p=&gc_mind_pack[index];
  for(unsigned col=0;col<p->n;col++)for(unsigned digit=0;digit<p->alphabet;digit++)if(digit!=p->solution[col]){
   g=initial;char guess[6]={0};for(unsigned i=0;i<p->n;i++)guess[i]=(char)('0'+p->solution[i]);guess[col]=(char)('0'+digit);
   gc_test_type(&g,press,guess);press(&g,NGK_EXE);CHECK(g.status==NG_PLAYING&&g.moves==1);
  }
 }
 for(unsigned index=0;index<90;index++){
  select_pack(&g,5,index);char answer[16];snprintf(answer,sizeof(answer),"%d",gc_sequence_pack[index].seq[6]+1);
  gc_test_type(&g,press,answer);press(&g,NGK_EXE);CHECK(g.status==NG_PLAYING);
 }
 static unsigned char composite[10001];
 for(unsigned p=2;p*p<10001;p++)if(!composite[p])for(unsigned v=p*p;v<10001;v+=p)composite[v]=1;
 for(unsigned n=0;n<10001;n++)CHECK(gc_is_prime(n)==(n>=2&&!composite[n]));
 start(&g,10,2,0,19);g.data[0]=994009;CHECK(module(&g)->valid(&g));press(&g,NGK_AUX);CHECK(!strcmp(g.input,"997^2"));press(&g,NGK_EXE);CHECK(g.status==NG_WON);
 for(unsigned id=1;id<=10;id++){
  start(&g,id,0,0,23);CHECK(!module(&g)->action(&g,'.'));
  if(id==2){press(&g,'=');CHECK(!strcmp(g.input,"="));}
  else {CHECK(!module(&g)->action(&g,'='));CHECK(!g.input[0]);}
 }
}


typedef struct {uint16_t pixel[396*224],writes[396*224];} RenderProbe;
static RenderProbe actual_view,expected_view;
static void probe_rect(void *ctx,int x,int y,int w,int h,uint16_t color){
 RenderProbe *view=ctx;
 for(int r=y;r<y+h;r++)for(int col=x;col<x+w;col++){unsigned pos=(unsigned)(r*396+col);view->pixel[pos]=color;view->writes[pos]++;}
}
static NgCanvas probe_canvas(RenderProbe *view){
 memset(view,0,sizeof(*view));for(unsigned i=0;i<396*224;i++)view->pixel[i]=NG_PAPER;
 return (NgCanvas){view,probe_rect};
}
static void compare_region(int x,int y,int w,int h){
 for(int r=y;r<y+h;r++)for(int col=x;col<x+w;col++){
  unsigned pos=(unsigned)(r*396+col);CHECK(actual_view.pixel[pos]==expected_view.pixel[pos]);CHECK(actual_view.writes[pos]==expected_view.writes[pos]);
 }
}
static void expression_render_tests(void){
 NgGame g;
 /* Card label is drawn once, preserving its normal glyph mask/advance. */
 for(unsigned i=0;i<4;i++){
  start(&g,8,2,0,11);press(&g,"+-*/"[i]);
  NgCanvas actual=probe_canvas(&actual_view),expected=probe_canvas(&expected_view);module(&g)->render(&g,&actual);
  char text[]={"+-*/"[i],0};ng_card(&expected,43,80,29,30,"",true,false);
  int scale=ng_text_width(text,2)<21?2:1;
  ng_expression(&expected,43+(29-ng_text_width(text,scale))/2,80+(30-10*scale)/2,text,NG_INK,scale);
  compare_region(43,80,29,30);
  bool colored=false;for(unsigned p=0;p<396*224;p++)if(expected_view.pixel[p]!=NG_PAPER&&expected_view.pixel[p]!=NG_WHITE&&expected_view.pixel[p]!=NG_LINE&&expected_view.pixel[p]!=NG_BLUE&&expected_view.pixel[p]!=NG_INK)colored=true;
  CHECK(colored);
 }
 const unsigned ids[]={2,6,7,10};const char *drafts[]={"8/4+2=4","8+6-4*2/1","100+3-2*1/5","2^3*3^2"};
 for(unsigned i=0;i<4;i++){
  start(&g,ids[i],0,0,51);gc_test_type(&g,press,drafts[i]);
  NgCanvas actual=probe_canvas(&actual_view),expected=probe_canvas(&expected_view);module(&g)->render(&g,&actual);
  int y=ids[i]==2?150:151;ng_input_expression(&expected,10,y,376,g.input);compare_region(10,y,376,21);
 }
 /* Long input is cropped once by the same width/cursor path as the common input. */
 start(&g,6,0,0,51);memset(g.input,'1',96);g.input[95]='+';g.input[96]=0;
 NgCanvas actual=probe_canvas(&actual_view),expected=probe_canvas(&expected_view);module(&g)->render(&g,&actual);
 ng_input_expression(&expected,10,151,376,g.input);compare_region(10,151,376,21);
 /* Feedback result tiles retain positional colors, including an operator. */
 start(&g,2,0,0,31);gc_test_type(&g,press,"12+7=19");press(&g,NGK_EXE);
 actual=probe_canvas(&actual_view);expected=probe_canvas(&expected_view);module(&g)->render(&g,&actual);
 int state=g.history[0][g.data[0]+3]-'0',background=state==2?NG_GREEN:state==1?NG_YELLOW:NG_LINE;
 ng_rect(&expected,102,48,32,21,background);ng_center(&expected,102,53,32,"+",state==2?NG_WHITE:NG_INK,1);compare_region(102,48,32,21);
 /* Every horizontal/vertical arithmetic clue follows the common operator path. */
 start(&g,9,1,0,25);actual=probe_canvas(&actual_view);expected=probe_canvas(&expected_view);module(&g)->render(&g,&actual);
 for(unsigned row=0;row<3;row++)for(unsigned col=0;col<2;col++){
  char op[]={"+-*"[g.data[2*row+col]],0};int x=58+(int)col*64,y=39+(int)row*46;
  ng_expression(&expected,x,y,op,NG_INK,1);compare_region(x,y,ng_text_width(op,1),11);
 }
 for(unsigned col=0;col<3;col++)for(unsigned row=0;row<2;row++){
  char op[]={"+-*"[g.data[6+2*col+row]],0};int x=27+(int)col*64,y=64+(int)row*46;
  ng_expression(&expected,x,y,op,NG_INK,1);compare_region(x,y,ng_text_width(op,1),11);
 }
}

static void expression_prose_tests(void){
 /* Wrapping must not turn prose separators or exponent signs into operators. */
 const char *plain[]={"Zero-match clues", "NORMAL / ASSISTED", "expression / zero divisor", "1e-3", "2E+4", "Next explanation"};
 const int widths[]={31,47,63,95,127,191,376};
 unsigned failures=0;
 for(unsigned t=0;t<sizeof(plain)/sizeof(*plain);t++)for(unsigned w=0;w<sizeof(widths)/sizeof(*widths);w++)for(unsigned small=0;small<2;small++){
  NgCanvas actual=probe_canvas(&actual_view),expected=probe_canvas(&expected_view);
  ng_wrap_expression(&actual,10,30,widths[w],12,6,plain[t],NG_BLUE,small!=0);
  ng_wrap(&expected,10,30,widths[w],12,6,plain[t],NG_BLUE,small!=0);
  if(memcmp(&actual_view,&expected_view,sizeof(actual_view))){fprintf(stderr,"Wrapped prose differs: text=%s width=%d small=%u\n",plain[t],widths[w],small);failures++;}
  else compare_region(10,30,widths[w],72);
 }
 for(unsigned t=0;t<sizeof(plain)/sizeof(*plain);t++)for(unsigned w=0;w<sizeof(widths)/sizeof(*widths);w++){
  NgCanvas actual=probe_canvas(&actual_view),expected=probe_canvas(&expected_view);
  ng_input_expression(&actual,10,30,widths[w],plain[t]);ng_input(&expected,10,30,widths[w],plain[t]);
  if(memcmp(&actual_view,&expected_view,sizeof(actual_view))){fprintf(stderr,"Clipped prose differs: text=%s width=%d\n",plain[t],widths[w]);failures++;}
  else compare_region(10,30,widths[w],21);
 }
 CHECK(!failures);
 /* Mathematical tokens use the four colors without repainting base glyphs. */
 const char *math="12+3-4*5/6=7";
 for(unsigned small=0;small<2;small++){
  NgCanvas actual=probe_canvas(&actual_view),expected=probe_canvas(&expected_view);
  ng_wrap_expression(&actual,10,30,376,12,2,math,NG_BLUE,small!=0);
  int x=10;
  for(unsigned i=0;math[i];i++){
   char glyph[]={math[i],0};int color=NG_BLUE;
   if(math[i]=='+')color=NG_RGB(24,0,23);
   else if(math[i]=='-')color=NG_RGB(25,12,0);
   else if(math[i]=='*')color=NG_RGB(0,24,4);
   else if(math[i]=='/')color=NG_RGB(0,18,23);
   if(small)ng_small(&expected,x,30,glyph,color);else ng_text(&expected,x,30,glyph,color,1);
   x+=(small?ng_small_width(glyph):ng_text_width(glyph,1))+1;
  }
  compare_region(10,30,376,24);
 }
 /* Sequence's variable products/sums are math even between alphabetic tokens. */
 const char *variables[]={"n*n","3n*n","2n+n"};const int variable_widths[]={31,47,376};
 for(unsigned t=0;t<3;t++)for(unsigned w=0;w<3;w++)for(unsigned small=0;small<2;small++){
  NgCanvas actual=probe_canvas(&actual_view),expected=probe_canvas(&expected_view);
  ng_wrap_expression(&actual,10,30,variable_widths[w],12,6,variables[t],NG_BLUE,small!=0);
  const char *remaining=variables[t];char line[32];
  for(unsigned row=0;row<6&&ng_text_line(line,sizeof(line),&remaining,variable_widths[w],small!=0);row++){
   int x=10,y=30+(int)row*12;
   for(unsigned i=0;line[i];i++){
    char glyph[]={line[i],0};int color=line[i]=='*'?NG_RGB(0,24,4):line[i]=='+'?NG_RGB(24,0,23):NG_BLUE;
    if(small)ng_small(&expected,x,y,glyph,color);else ng_text(&expected,x,y,glyph,color,1);
    x+=(small?ng_small_width(glyph):ng_text_width(glyph,1))+1;
   }
  }
  CHECK(!*remaining);compare_region(10,30,variable_widths[w],72);
 }
}


static void revised_baseball_target_tests(void){
 CHECK(ng_guesscalc[0].modes==1&&ng_guesscalc[5].modes==1);NgCanvas canvas={NULL,rect};
 for(unsigned d=0;d<4;d++)for(unsigned seed=1;seed<=128;seed++){
  NgGame g={0};g.id=1;g.difficulty=(uint8_t)d;g.seed=g.rng=seed;g.run_id=1;g.pack_revision=3;module(&g)->init(&g);
  CHECK(module(&g)->valid(&g)&&g.data[0]==(int)(4+d));module(&g)->render(&g,&canvas);
  for(unsigned i=0;i<4+d;i++)g.board[i]='0';
  for(unsigned i=0;i<4+d;i++)press(&g,'0');press(&g,NGK_EXE);CHECK(g.status==NG_WON&&g.moves==1);
  NgGame corrupt=g;corrupt.mode=1;CHECK(!module(&corrupt)->valid(&corrupt));
 }
 for(unsigned d=0;d<4;d++)for(unsigned target=1;target<=1000;target++){
  NgGame g={0};g.id=6;g.difficulty=(uint8_t)d;g.seed=g.rng=target*7001+d+1;g.run_id=1;g.pack_revision=3;module(&g)->init(&g);
  CHECK(gc_target_init(&g,target));CHECK(module(&g)->valid(&g));CHECK(g.data[0]==(int)target&&g.data[1]==(int)(d<2?4:d+3));
  NgGame original=g,copy=g;CHECK(gc_target_init(&copy,target)&&!memcmp(&copy,&g,sizeof(g)));
  CHECK(!gc_target_init(&copy,0)&&!memcmp(&copy,&g,sizeof(g)));CHECK(!gc_target_init(&copy,1001)&&!memcmp(&copy,&g,sizeof(g)));
  press(&g,NGK_ANSWER);GcExpression e;CHECK(gc_expression(g.input,false,&e)&&gc_cards(&e,g.board,(unsigned)g.data[1],true));CHECK(e.value.num==(int64_t)target*e.value.den);
  char equivalent[97];snprintf(equivalent,sizeof(equivalent),"--(%.80s)",g.input);strcpy(g.input,equivalent);press(&g,NGK_EXE);CHECK(g.status==NG_WON);
  if(target%100==0)module(&g)->render(&g,&canvas);
  copy=original;copy.board[0]++;CHECK(!module(&copy)->valid(&copy));copy=original;copy.data[0]=1001;CHECK(!module(&copy)->valid(&copy));
  copy=original;copy.data[1]++;CHECK(!module(&copy)->valid(&copy));copy=original;copy.status=NG_WON;CHECK(!module(&copy)->valid(&copy));
  copy=original;copy.mode=1;CHECK(!module(&copy)->valid(&copy));copy=original;copy.data[8]=1;CHECK(!module(&copy)->valid(&copy));
  copy=original;snprintf(copy.input,sizeof(copy.input),"%d",copy.board[0]);press(&copy,NGK_EXE);CHECK(copy.status==NG_PLAYING&&!copy.moves);
 }
}

int main(void){revised_baseball_target_tests();expression_prose_tests();expression_render_tests();countdown_score_tests();bounded_text_tests();additional_metamorphic_tests();parser_tests();feedback_tests();lifecycle_tests();game_rules_tests();all_pack_engine_tests();master_bank_tests();master_runtime_tests();legacy_state_tests();alternative_and_mutation_tests();printf("guesscalc: %u assertions, %u rendered rectangles; PASS\n",checks,rects);return 0;}
