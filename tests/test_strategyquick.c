#include "../src/games/strategyquick.h"
#include "app.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

static NgGame fresh(unsigned id,unsigned difficulty,unsigned mode,uint32_t seed){
 NgGame g;ng_new(&g,id,difficulty,mode,seed,1);assert(ng_valid(&g));return g;
}
static int after_value(NgGame g,SqMove m){assert(sq_strategy_legal(&g,m));sq_strategy_apply(&g,m);if(g.status==NG_DRAW)return 0;if(g.status!=NG_PLAYING)return 1;return -sq_strategy_value(g.id,&g);}
static void audit_nim(void){
 /* Independent complete normal-play DAG, indexed by base-32 pile counts. */
 unsigned states=1u<<20;uint8_t *win=calloc(states,1);assert(win);NgGame g=fresh(21,2,0,19);
 for(unsigned code=1;code<states;code++){
  for(unsigned p=0;p<4;p++){unsigned n=(code>>(p*5))&31u;g.board[p]=(int16_t)n;for(unsigned k=1;k<=n;k++)if(!win[code-(k<<(p*5))])win[code]=1;}
  assert(sq_strategy_value(21,&g)==(win[code]?1:-1));SqMove m=sq_strategy_pick(&g,true);assert(sq_strategy_legal(&g,m));unsigned child=code-((unsigned)m.amount<<((unsigned)m.choice*5));if(win[code])assert(!win[child]);
 }
 free(win);puts("Nim: 1,048,575 nonterminal four-pile states, 0..31 exhaustive PASS");
}
static void audit_wythoff(void){
 uint8_t ref[41][41]={{0}};NgGame g=fresh(22,2,0,9);
 for(unsigned a=0;a<=40;a++)for(unsigned b=0;b<=40;b++){
  if(!a&&!b)continue;
  for(unsigned x=0;x<a;x++)if(!ref[x][b])ref[a][b]=1;
  for(unsigned y=0;y<b;y++)if(!ref[a][y])ref[a][b]=1;
  for(unsigned k=1;k<=a&&k<=b;k++)if(!ref[a-k][b-k])ref[a][b]=1;
  g.board[0]=(int16_t)a;g.board[1]=(int16_t)b;assert(sq_strategy_value(22,&g)==(ref[a][b]?1:-1));assert(after_value(g,sq_strategy_pick(&g,true))==(ref[a][b]?1:-1));
 }
 puts("Wythoff: 1,680 nonterminal pairs, 0..40 exhaustive PASS");
}
static void audit_euclid(void){
 uint8_t ref[100][100]={{0}};NgGame g=fresh(23,2,0,12);unsigned count=0;
 /* Increasing sum order is independent of the generator's memo recursion. */
 for(unsigned total=2;total<=198;total++)for(unsigned a=1;a<=99;a++){
  if(total<=a)continue;unsigned b=total-a;if(b<a||b>99)continue;
  for(unsigned k=1;k*a<=b;k++){unsigned r=b-k*a,x=r<a?r:a,y=r<a?a:r;if(!ref[x][y])ref[a][b]=1;}
  g.board[0]=(int16_t)a;g.board[1]=(int16_t)b;assert(sq_strategy_value(23,&g)==(ref[a][b]?1:-1));assert(after_value(g,sq_strategy_pick(&g,true))==(ref[a][b]?1:-1));count++;
 }
 printf("Euclid: %u positive unordered pairs, 1..99 exhaustive PASS\n",count);
}
static bool ref_fifteen_win(unsigned mask){
 /* Independent Lo Shu rows, columns, and diagonals mapping. */
 static const unsigned lines[8][3]={{8,1,6},{3,5,7},{4,9,2},{8,3,4},{1,5,9},{6,7,2},{8,5,2},{6,5,4}};
 for(unsigned i=0;i<8;i++){unsigned m=0;for(unsigned j=0;j<3;j++)m|=1u<<(lines[i][j]-1);if((mask&m)==m)return true;}return false;
}
static int8_t fifteen_ref[512][512];
static int ref_fifteen(unsigned own,unsigned other){
 if(ref_fifteen_win(own))return 1;if(ref_fifteen_win(other))return -1;
 if((own|other)==511)return 0;
 int8_t *cache=&fifteen_ref[own][other];if(*cache!=2)return *cache;
 int best=-1;for(unsigned i=0;i<9;i++)if(!((own|other)&(1u<<i))){int v=-ref_fifteen(other,own|(1u<<i));if(v>best)best=v;}*cache=(int8_t)best;return best;
}
static void audit_fifteen(void){
 memset(fifteen_ref,2,sizeof fifteen_ref);NgGame g=fresh(24,2,0,11);unsigned count=0;
 for(unsigned code=0;code<19683;code++){
  unsigned n=code,own=0,other=0;g.moves=0;
  for(unsigned i=0;i<9;i++){unsigned p=n%3;n/=3;g.board[i]=(int16_t)p;if(p==1)own|=1u<<i;if(p==2)other|=1u<<i;g.moves+=p!=0;}
  if(ref_fifteen_win(own)||ref_fifteen_win(other)||(own|other)==511)continue;
  int value=ref_fifteen(own,other);assert(sq_strategy_value(24,&g)==value);SqMove m=sq_strategy_pick(&g,true);assert(sq_strategy_legal(&g,m));assert(after_value(g,m)==value);count++;
 }
 printf("Make Fifteen: %u nonterminal disjoint ownership states, exact minimax PASS\n",count);
}
static void audit_race(void){
 unsigned count=0;for(unsigned d=0;d<3;d++){
  NgGame g=fresh(25,d,0,7);int target=g.data[1],max=g.data[2];bool ref[32]={0};
  for(int r=1;r<=target;r++)for(int k=1;k<=max&&k<=r;k++)if(!ref[r-k])ref[r]=true;
  for(int total=0;total<target;total++){g.board[0]=(int16_t)total;int expect=ref[target-total]?1:-1;assert(sq_strategy_value(25,&g)==expect);assert(after_value(g,sq_strategy_pick(&g,true))==expect);count++;}
 }
 printf("Race: %u nonterminal totals across all three presets PASS\n",count);
 unsigned extended=0;NgGame g=fresh(25,2,0,7);
 for(unsigned maximum=2;maximum<=8;maximum++){
  bool ref[64]={0};for(unsigned r=1;r<64;r++)for(unsigned k=1;k<=maximum&&k<=r;k++)if(!ref[r-k])ref[r]=true;
  for(unsigned remain=1;remain<64;remain++){g.data[1]=(int32_t)remain;g.data[2]=(int32_t)maximum;g.board[0]=0;int expected=ref[remain]?1:-1;assert(sq_strategy_value(25,&g)==expected);assert(after_value(g,sq_strategy_pick(&g,true))==expected);extended++;}
 }
 printf("Race MASTER domain: %u remaining-distance/add-limit states match independent DP PASS\n",extended);
}
static void strategy_lifecycle(void){
 for(unsigned id=21;id<=25;id++)for(unsigned d=0;d<3;d++)for(unsigned mode=0;mode<2;mode++)for(unsigned seed=1;seed<=20;seed++){
  NgGame g=fresh(id,d,mode,seed);const NgModule *m=&ng_strategyquick[id-21];
  assert(!sq_strategy_legal(&g,(SqMove){-1,1}));assert(!sq_strategy_legal(&g,(SqMove){0,0}));
  for(unsigned step=0;g.status==NG_PLAYING&&step<180;step++){
   if(g.cpu_pending){NgGame before=g;assert(!m->action(&g,'1'));assert(!memcmp(&before,&g,sizeof g));assert(m->action(&g,NGK_CPU));}
   else{SqMove p=sq_strategy_pick(&g,d==2);assert(sq_strategy_apply(&g,p));}
   assert(m->valid(&g));
  }
  assert(g.status!=NG_PLAYING);NgGame ended=g;assert(!m->action(&g,NGK_CPU));assert(!memcmp(&ended,&g,sizeof g));
 }
 NgGame g=fresh(24,2,0,1);int sequence[]={8,1,3,2,4};for(unsigned i=0;i<5;i++)assert(sq_strategy_apply(&g,(SqMove){sequence[i]-1,1}));assert(g.status==NG_WON);assert(g.data[0]==1);
 g=fresh(24,2,0,1);int fourth[]={2,1,4,3,8,6,5};for(unsigned i=0;i<7;i++)assert(sq_strategy_apply(&g,(SqMove){fourth[i]-1,1}));assert(g.status==NG_WON&&g.moves==7);
 g=fresh(24,2,0,1);int draw[]={8,1,6,5,3,7,9,4,2};for(unsigned i=0;i<9;i++)assert(sq_strategy_apply(&g,(SqMove){draw[i]-1,1}));assert(g.status==NG_DRAW);
 g=fresh(23,2,0,1);g.board[0]=7;g.board[1]=7;assert(sq_strategy_apply(&g,(SqMove){0,1}));assert(g.status==NG_WON);
 g=fresh(23,2,0,1);g.board[0]=3;g.board[1]=12;assert(!sq_strategy_legal(&g,(SqMove){0,5}));assert(sq_strategy_apply(&g,(SqMove){0,4}));assert(g.status==NG_WON);
 puts("Strategy: 600 seed/difficulty/mode complete legal playthroughs PASS");
}
static void quick_2048(void){
 int16_t line[4]={1,1,1,1};uint32_t score=0;assert(sq_2048_line(line,&score));assert(line[0]==2&&line[1]==2&&!line[2]&&!line[3]&&score==8);
 int16_t b[4]={1,1,2,0};score=0;assert(sq_2048_line(b,&score));assert(b[0]==2&&b[1]==2&&!b[2]&&score==4);
 int16_t c[4]={2,0,2,2};score=0;assert(sq_2048_line(c,&score));assert(c[0]==3&&c[1]==2&&!c[2]&&score==8);
 int16_t cap[4]={30,30,29,29};score=UINT32_MAX-2;assert(sq_2048_line(cap,&score));assert(cap[0]==30&&cap[1]==30&&cap[2]==30&&score==UINT32_MAX);
 NgGame g=fresh(26,0,0,1);memset(g.board,0,sizeof g.board);g.board[0]=1;g.data[0]=1;uint32_t rng=g.rng;assert(!sq_quick_action(&g,NGK_LEFT));assert(g.rng==rng&&g.moves==0);
 NgGame before=g;assert(sq_quick_action(&g,NGK_RIGHT));NgGame after=g;g=before;assert(sq_quick_action(&g,NGK_RIGHT));assert(!memcmp(&g,&after,sizeof g));
 for(unsigned i=0;i<16;i++)g.board[i]=(int16_t)(1+(i+i/4)%2);assert(!sq_2048_can_move(&g));g.board[1]=g.board[0];assert(sq_2048_can_move(&g));
 unsigned exponents2=0,exponents4=0;for(unsigned seed=1;seed<=5000;seed++){g=fresh(26,0,0,seed);unsigned count=0;for(unsigned i=0;i<16;i++){count+=g.board[i]!=0;exponents2+=g.board[i]==1;exponents4+=g.board[i]==2;}assert(count==2);}
 assert(exponents4>800&&exponents4<1200);printf("2048: merge-once, no-op RNG, replay, overflow, terminal PASS; spawns 2:%u 4:%u\n",exponents2,exponents4);
}
static void quick_sliding(void){
 for(unsigned mode=0;mode<2;mode++)for(unsigned d=0;d<3;d++)for(unsigned seed=1;seed<=200;seed++){
  NgGame g=fresh(27,d,mode,seed);assert(sq_sliding_solvable(&g));unsigned n=g.rows*g.cols;bool goal=true;for(unsigned i=0;i<n;i++)goal&=g.board[i]==(int)((i+1)%n);assert(!goal);
  for(int k=NGK_UP;k<=NGK_LEFT;k++){NgGame before=g;if(sq_quick_action(&g,k)){assert(sq_quick_valid(&g));g=before;assert(!memcmp(&g,&before,sizeof g));}}
 }
 NgGame g=fresh(27,0,0,1);for(unsigned i=0;i<9;i++)g.board[i]=(int16_t)((i+1)%9);g.board[8]=8;g.board[7]=0;g.cursor=7;assert(sq_quick_action(&g,NGK_RIGHT));assert(g.status==NG_WON);
 int16_t tmp=g.board[0];g.board[0]=g.board[1];g.board[1]=tmp;assert(!sq_sliding_solvable(&g));assert(!sq_quick_valid(&g));
 puts("Sliding: 1,200 seeded solvable non-goal boards, parity and completion PASS");
}
static void quick_lights(void){
 for(unsigned mode=0;mode<2;mode++)for(unsigned d=0;d<3;d++)for(unsigned seed=1;seed<=200;seed++){
  NgGame g=fresh(28,d,mode,seed);uint32_t solution=0;assert(sq_lights_solution(&g,&solution));assert(solution);
  NgGame hinted=g;assert(sq_quick_action(&hinted,NGK_HINT));assert(hinted.assisted);assert(solution&(1u<<hinted.cursor));
  for(unsigned i=0;i<g.rows*g.cols;i++)if(solution&(1u<<i)){g.cursor=(uint8_t)i;assert(sq_quick_action(&g,NGK_EXE));}
  for(unsigned i=0;i<g.rows*g.cols;i++)assert(!g.board[i]);assert(g.status==NG_WON);assert(sq_quick_valid(&g));
 }
 /* Cross-check every 4x4 generated toggle subset with an independent stencil. */
 NgGame g=fresh(28,0,0,1);for(uint32_t mask=1;mask<65536;mask++){
  memset(g.board,0,sizeof g.board);g.status=NG_PLAYING;
  for(unsigned r=0;r<4;r++)for(unsigned c=0;c<4;c++){unsigned bit=r*4+c;int parity=(mask>>bit)&1u;if(r)parity^=(mask>>(bit-4))&1u;if(r<3)parity^=(mask>>(bit+4))&1u;if(c)parity^=(mask>>(bit-1))&1u;if(c<3)parity^=(mask>>(bit+1))&1u;g.board[bit]=(int16_t)parity;}
  uint32_t sol;assert(sq_lights_solution(&g,&sol));for(unsigned i=0;i<16;i++)if(sol&(1u<<i)){g.cursor=(uint8_t)i;sq_quick_action(&g,NGK_EXE);}for(unsigned i=0;i<16;i++)assert(!g.board[i]);
 }
 puts("Lights: 1,200 seeded boards and all 65,535 nonempty 4x4 press sets solved PASS");
}
static void quick_time_memory(void){
 for(unsigned d=0;d<3;d++)for(unsigned seed=1;seed<=200;seed++){
  NgGame g=fresh(29,d,0,seed);int a=g.data[0],b=g.data[1],op=g.data[3];int value=(op==0?a+b:op==1?a-b:op==2?a*b:a/b)+g.data[2];assert(value==g.data[4]);if(op==3)assert(a%b==0);
  snprintf(g.input,sizeof g.input,"%d",value);assert(sq_quick_action(&g,NGK_EXE));assert(g.data[5]==1&&g.data[7]==1&&g.phase==1);assert(!sq_quick_action(&g,'1'));assert(!sq_quick_action(&g,NGK_EXE));sq_quick_tick(&g,300);assert(sq_quick_action(&g,NGK_EXE));assert(g.phase==0);assert(sq_quick_valid(&g));
  sq_quick_tick(&g,UINT32_MAX);assert(g.status==NG_WON && !g.data[9]);assert(sq_quick_valid(&g));
  g=fresh(29,d,1,seed);sq_quick_tick(&g,UINT32_MAX);assert(g.status==NG_PLAYING&&g.assisted);assert(sq_quick_action(&g,'='));assert(g.status==NG_WON);
 }
 for(unsigned d=0;d<3;d++)for(unsigned seed=1;seed<=100;seed++){
  NgGame g=fresh(30,d,0,seed);
  for(unsigned round=0;round<20;round++){
   assert(g.phase==0);assert(!sq_quick_action(&g,'1'));int original=g.data[2];sq_quick_tick(&g,1000);assert(g.data[2]==original-1000);NgGame saved=g;g=saved;assert(g.data[2]==original-1000);sq_quick_tick(&g,UINT32_MAX);assert(g.phase==1);assert(!sq_quick_action(&g,'1'));sq_quick_tick(&g,350);assert(g.phase==2);
   for(int i=0;i<g.data[0];i++)assert(sq_quick_action(&g,'0'+g.board[i]));assert(sq_quick_action(&g,NGK_EXE));assert(g.status==NG_WON||g.phase==3);assert(sq_quick_valid(&g));if(round<19){assert(!sq_quick_action(&g,NGK_EXE));sq_quick_tick(&g,300);assert(sq_quick_action(&g,NGK_EXE));}
  }
  assert(g.status==NG_WON&&g.moves==20);
 }
 NgGame g=fresh(30,0,0,1);g.board[0]=0;sq_quick_tick(&g,100000);sq_quick_tick(&g,350);strcpy(g.input,"999");if(g.board[0]==9)g.board[0]=0;assert(sq_quick_action(&g,NGK_EXE));assert(g.status==NG_LOST&&g.phase==4);
 puts("Rush: 600 timed+practice runs; Memory: 300 full 20-round runs, phase barriers/leading zero/exposure/large delta PASS");
}
static void reject_corrupt(void){
 for(unsigned id=21;id<=30;id++){
  NgGame g=fresh(id,2,0,1);const NgModule *m=&ng_strategyquick[id-21];g.cursor=80;assert(!m->valid(&g));g=fresh(id,2,0,1);g.rows=0;assert(!m->valid(&g));g=fresh(id,2,0,1);memset(g.input,'1',sizeof g.input);assert(!m->valid(&g));g=fresh(id,2,0,1);g.board[0]=INT16_MAX;assert(!m->valid(&g));
 }
 puts("All 10 modules: corrupted cursor/dimensions/input/board rejected PASS");
}

static void codec_roundtrip(NgSession *session){
 uint8_t bytes[NG_RECORD_MAX];NgSession decoded;size_t length=ng_encode(session,bytes,sizeof bytes);assert(length);assert(ng_decode(&decoded,bytes,length,session->game.id));assert(!memcmp(&decoded.game,&session->game,sizeof(NgGame)));assert(decoded.undo_count==session->undo_count);for(unsigned i=0;i<session->undo_count;i++)assert(!memcmp(&decoded.undo[i],&session->undo[i],sizeof(NgGame)));assert(!ng_decode(&decoded,bytes,length-1,session->game.id));assert(!ng_decode(&decoded,bytes,length,session->game.id==30?29:30));
}
static void press(NgApp *app,int key){ng_app_event(app,key,NG_DOWN);ng_app_event(app,key,NG_UP);}
static void type_int(NgApp *app,int value){char text[20];snprintf(text,sizeof text,"%d",value);for(unsigned i=0;text[i];i++)press(app,text[i]);}
static void integrated_matrix(void){
 unsigned count=0;
 for(unsigned id=21;id<=28;id++)for(unsigned d=0;d<4;d++)for(unsigned mode=0;mode<ng_module(id)->modes;mode++){
  NgApp app;ng_app_init(&app,(NgHooks){0},123);app.settings.difficulty[id-1]=(uint8_t)d;app.settings.mode[id-1]=(uint8_t)mode;
  press(&app,'1'+ng_catalog_index(id)/6);press(&app,'1'+ng_catalog_index(id)%6);assert(app.screen==NG_ENTRY);press(&app,NGK_F6);assert(app.screen==NG_PLAY&&app.session.game.id==id);assert(app.session.game.difficulty==(id==26&&mode==0?1:d));assert(app.settings.difficulty[id-1]==d);assert(ng_valid(&app.session.game));codec_roundtrip(&app.session);
  NgGame initial=app.session.game;
  /* INIT must regenerate exactly the same board and gameplay RNG. */
  press(&app,NGK_F1);if(app.modal==NG_MODAL_INIT)press(&app,NGK_EXE);assert(app.session.game.seed==initial.seed);assert(app.session.game.rng==initial.rng);assert(!memcmp(app.session.game.board,initial.board,sizeof initial.board));assert(app.session.game.assisted);
  if(app.session.game.cpu_pending){codec_roundtrip(&app.session);assert(ng_app_cpu(&app));assert(!app.session.game.cpu_pending);}
  if(app.session.game.status){assert(ng_valid(&app.session.game));codec_roundtrip(&app.session);count++;continue;}
  NgGame before=app.session.game;
  if(id<=25){NgGame decision=before;SqMove move=sq_strategy_pick(&decision,true);if(id==24)press(&app,'1'+move.choice);else{if(id==21||id==22)while(app.session.game.cursor!=(unsigned)move.choice)press(&app,NGK_RIGHT);type_int(&app,move.amount);press(&app,NGK_EXE);}assert(app.session.game.moves==before.moves+1);codec_roundtrip(&app.session);if(app.session.game.cpu_pending){assert(ng_app_cpu(&app));assert(!app.session.game.cpu_pending);}}
  else if(id==26||id==27){for(int k=NGK_UP;k<=NGK_LEFT;k++){NgGame trial=before;if(sq_quick_action(&trial,k)){press(&app,k);break;}}assert(app.session.game.moves==before.moves+1);}
  else if(id==28){press(&app,NGK_F3);assert(app.session.game.assisted);press(&app,NGK_EXE);assert(app.session.game.moves==1);}
  assert(ng_valid(&app.session.game));codec_roundtrip(&app.session);
  if((ng_module(id)->flags&NGF_UNDO)&&!app.modal){uint32_t rng=before.rng;press(&app,NGK_F2);assert(app.session.game.assisted);assert(app.session.game.moves==before.moves);assert(app.session.game.rng==rng);assert(!memcmp(app.session.game.board,before.board,sizeof before.board));assert(!app.session.game.cpu_pending);assert(ng_valid(&app.session.game));codec_roundtrip(&app.session);}
  if(!app.modal){NgGame checkpoint=app.session.game;press(&app,NGK_EXIT);assert(app.screen==NG_ENTRY);unsigned resume_row=ng_entry_row(&app,NG_ENTRY_RESUME);assert(ng_entry_action(&app,resume_row)==NG_ENTRY_RESUME);while(app.entry_selection!=resume_row)press(&app,NGK_DOWN);press(&app,NGK_EXE);assert(app.screen==NG_PLAY);assert(!memcmp(&checkpoint,&app.session.game,sizeof checkpoint));press(&app,NGK_MENU);assert(ng_valid(&app.session.game));}
  count++;
 }
 printf("Retained public IDs21-28: %u game/mode/difficulty cases through real input, INIT, codec, CPU, undo, explicit RESUME, EXIT/MENU PASS\n",count);
}
static void legacy_codec_matrix(void){
 unsigned cases=0;
 for(unsigned id=29;id<=30;id++)for(unsigned d=0;d<3;d++)for(unsigned mode=0;mode<ng_module(id)->modes;mode++){
  NgSession session={0};session.game=fresh(id,d,mode,9123);session.stats.started=1;NgGame *g=&session.game;codec_roundtrip(&session);
  if(id==29){
   snprintf(g->input,sizeof g->input,"%ld",(long)g->data[4]);assert(sq_quick_action(g,NGK_EXE));assert(g->phase==1);codec_roundtrip(&session);sq_quick_tick(g,300);codec_roundtrip(&session);assert(sq_quick_action(g,NGK_EXE));assert(g->phase==0);codec_roundtrip(&session);
   if(mode)assert(sq_quick_action(g,'='));else sq_quick_tick(g,60000);assert(g->status==NG_WON);
  }else{
   sq_quick_tick(g,(uint32_t)g->data[2]-1);assert(g->phase==0&&g->data[2]==1);codec_roundtrip(&session);sq_quick_tick(g,1);assert(g->phase==1);codec_roundtrip(&session);sq_quick_tick(g,350);assert(g->phase==2);codec_roundtrip(&session);
   for(int i=0;i<g->data[0];i++)assert(sq_quick_action(g,'0'+g->board[i]));assert(sq_quick_action(g,NGK_EXE));assert(g->phase==3);codec_roundtrip(&session);sq_quick_tick(g,300);assert(sq_quick_action(g,NGK_EXE));codec_roundtrip(&session);
   sq_quick_tick(g,(uint32_t)g->data[2]);sq_quick_tick(g,350);for(int i=0;i<g->data[0];i++)assert(sq_quick_action(g,'0'+(g->board[i]+1)%10));assert(sq_quick_action(g,NGK_EXE));assert(g->status==NG_LOST&&g->phase==4);
  }
  ng_record_result(&session);codec_roundtrip(&session);cases++;
 }
 printf("Legacy IDs29/30: %u engine/codec mode-difficulty cases including exposure/input/results/terminal records, no public menu entry PASS\n",cases);
}
static void imported_state_rejections(void){
 for(unsigned id=21;id<=30;id++){
  NgGame g=fresh(id,2,0,19);g.phase=255;assert(!ng_valid(&g));
  g=fresh(id,2,0,19);g.mode=255;assert(!ng_valid(&g));
  g=fresh(id,2,0,19);g.cpu_pending=1;if(id<=25)g.turn=0;assert(!ng_valid(&g));
 }
 NgGame g=fresh(29,2,0,19);g.data[9]=-1;assert(!ng_valid(&g));g.data[9]=60001;assert(!ng_valid(&g));
 g=fresh(29,2,0,19);g.data[4]++;assert(!ng_valid(&g));
 g=fresh(30,2,0,19);g.data[2]=-1;assert(!ng_valid(&g));g.data[2]=INT_MAX;assert(!ng_valid(&g));
 g=fresh(30,2,0,19);g.phase=2;assert(!ng_valid(&g));
 g=fresh(30,2,0,19);g.data[0]=16;assert(!ng_valid(&g));
 g=fresh(21,2,0,19);memset(g.board,0,sizeof g.board);assert(!ng_valid(&g));
 puts("Imported states: phase/mode/CPU/time/answer/length/terminal inconsistencies rejected PASS");
}

static void removed_local_mode(void){
 unsigned cases=0,states=0,p2wins=0;
 for(unsigned id=21;id<=25;id++)for(unsigned d=0;d<4;d++){
  const NgModule *m=ng_module(id);assert(m->modes==2);
  assert(!strcmp(m->mode_name(0),"YOU FIRST")&&!strcmp(m->mode_name(1),"CPU FIRST"));
  NgGame new_run=fresh(id,d,2,772);assert(new_run.mode==0);
  NgSession legacy={0};legacy.stats.started=1;legacy.game=fresh(id,d,0,772);legacy.game.mode=2;
  /* Old local states remain decodable for archive inspection, never silently
   * treated as a CPU game. New selection and migration exclude this mode. */
  assert(ng_valid(&legacy.game));codec_roundtrip(&legacy);
  NgGame before=legacy.game;
  const int keys[]={NGK_CPU,NGK_EXE,NGK_HINT,NGK_AUX,NGK_DEL,NGK_RIGHT,'1','5'};
  for(unsigned i=0;i<sizeof keys/sizeof keys[0];i++){assert(!m->action(&legacy.game,keys[i]));assert(!memcmp(&before,&legacy.game,sizeof before));}
  legacy.game.pack_revision=3;assert(!ng_valid(&legacy.game));
  NgGame live=fresh(id,d,0,772);
  for(unsigned step=0;live.status==NG_PLAYING&&step<180;step++){
   assert(sq_strategy_apply(&live,sq_strategy_pick(&live,live.turn==1)));assert(ng_valid(&live));
   legacy.game=live;legacy.game.mode=2;legacy.game.cpu_pending=0;
   if(legacy.game.status==NG_LOST){legacy.game.status=NG_WON;p2wins++;}
   assert(ng_valid(&legacy.game));codec_roundtrip(&legacy);states++;
  }
  assert(live.status!=NG_PLAYING);cases++;
 }
 assert(p2wins);printf("Strategy removed LOCAL: %u legacy read-only codec cases + %u played states (%u P2 wins); modes2/new-run normalization PASS\n",cases,states,p2wins);
}

static void master_banks(void){
 unsigned starts=0,rounds=0;
 for(unsigned id=21;id<=28;id++)for(unsigned mode=0;mode<ng_module(id)->modes;mode++){
  unsigned count=sq_bank_count(id,3,mode);if(!count)continue;bool seen[30]={0};
  for(unsigned ordinal=0;ordinal<count;ordinal++){
   NgSession s={0};s.stats.started=1;NgGame *g=&s.game;ng_new_supply(g,id,3,mode,7101,1,0x98ab4321u,ordinal);
   assert(g->difficulty==3&&g->mode==mode&&ng_valid(g));assert(g->puzzle_id<count&&!seen[g->puzzle_id]);seen[g->puzzle_id]=true;codec_roundtrip(&s);starts++;
   NgGame initial=*g;ng_new_supply(g,id,3,mode,7101,1,0x98ab4321u,ordinal);assert(!memcmp(g,&initial,sizeof initial));
   if(id<=25){
    int expected=mode==1?-1:1;assert(sq_strategy_value(id,g)==expected);
    if(id==24){unsigned mine=0,other=0;for(unsigned i=0;i<9;i++){if(g->board[i]==g->turn+1)mine|=1u<<i;else if(g->board[i])other|=1u<<i;}assert(ref_fifteen(mine,other)==expected);}
    if(id==25){unsigned remain=(unsigned)(g->data[1]-g->board[0]);bool dp[64]={0};for(unsigned n=1;n<=remain;n++)for(unsigned k=1;k<=n&&k<=(unsigned)g->data[2];k++)if(!dp[n-k])dp[n]=true;assert((dp[remain]?1:-1)==expected);}
    for(unsigned ply=0;g->status==NG_PLAYING&&ply<180;ply++){NgGame before=*g;assert(sq_strategy_apply(g,sq_strategy_pick(g,true)));assert(ng_valid(g));if(!before.cpu_pending){s.undo_count=1;s.undo[0]=before;}codec_roundtrip(&s);}
    assert(g->status==NG_WON&&g->data[0]==1);rounds++;
   }else if(id==27){
    assert(sq_sliding_solvable(g));if(mode)assert(g->data[1]>=48);else assert(g->data[1]==31);
    for(int key=NGK_UP;key<=NGK_LEFT;key++){*g=initial;if(sq_quick_action(g,key)){assert(ng_valid(g));codec_roundtrip(&s);}}
   }else{
    uint32_t solution;assert(sq_lights_solution(g,&solution));assert(g->data[1]>=(mode?12:6));
    for(unsigned i=0;i<g->rows*g->cols;i++)if(solution&(1u<<i)){g->cursor=(uint8_t)i;assert(sq_quick_action(g,NGK_EXE));assert(ng_valid(g));}
    assert(g->status==NG_WON);codec_roundtrip(&s);
   }
   *g=initial;g->puzzle_id=count;assert(!ng_valid(g));*g=initial;g->difficulty=4;assert(!ng_valid(g));
  }
 }
 printf("MASTER banks: %u no-repeat starts, %u exact-vs-exact forced HUMAN wins, INIT/codec/invalid IDs PASS\n",starts,rounds);
}

static void target_2048(void){
 static const unsigned goal[4]={9,10,11,13};
 for(unsigned d=0;d<4;d++){
  NgSession session={0};session.stats.started=2;NgGame *g=&session.game;*g=fresh(26,d,1,77);assert(g->mode==1&&g->data[3]==(int)goal[d]);
  memset(g->board,0,sizeof g->board);g->board[0]=g->board[1]=(int16_t)(goal[d]-1);g->data[0]=(int)goal[d]-1;g->data[1]=g->data[0]>=11;assert(ng_valid(g));
  session.undo_count=1;session.undo[0]=*g;assert(sq_quick_action(g,NGK_LEFT));assert(g->status==NG_WON&&g->data[0]==(int)goal[d]&&ng_valid(g));ng_record_result(&session);codec_roundtrip(&session);
  assert(session.stats.best[1][d][0].completed==1&&session.stats.best[0][d][0].completed==0);
  NgGame won=*g;assert(!sq_quick_action(g,NGK_LEFT));assert(!memcmp(g,&won,sizeof won));
  *g=session.undo[0];assert(sq_quick_action(g,NGK_LEFT));g->recorded=1;assert(!memcmp(g,&won,sizeof won));
  *g=fresh(26,d,0,77);session.undo_count=0;assert(!g->data[3]);memset(g->board,0,sizeof g->board);g->board[0]=g->board[1]=12;g->data[0]=12;g->data[1]=1;assert(ng_valid(g));assert(sq_quick_action(g,NGK_LEFT));assert(g->status==NG_PLAYING&&g->data[0]==13&&ng_valid(g));codec_roundtrip(&session);
  g->data[3]=13;assert(!ng_valid(g));
  *g=fresh(26,d,1,77);for(unsigned i=0;i<16;i++)g->board[i]=(int16_t)(1+(i+i/4)%2);g->data[0]=2;g->status=NG_LOST;assert(ng_valid(g));g->status=NG_WON;assert(!ng_valid(g));
 }
 puts("2048: CLASSIC survives8192; TARGET512/1024/2048/8192 terminal+undo+mode-separated stats/codec PASS");
}

int test_strategyquick(void){
 setvbuf(stdout,NULL,_IONBF,0);
 audit_nim();audit_wythoff();audit_euclid();audit_fifteen();audit_race();strategy_lifecycle();quick_2048();quick_sliding();quick_lights();quick_time_memory();reject_corrupt();integrated_matrix();legacy_codec_matrix();imported_state_rejections();removed_local_mode();master_banks();target_2048();return 0;
}
#ifdef STRATEGYQUICK_TEST_MAIN
int main(void){return test_strategyquick();}
#endif
