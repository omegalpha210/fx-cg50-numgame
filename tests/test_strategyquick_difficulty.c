/* Deterministic native-engine samples for the host difficulty audit.
 * JSONL stdout is consumed by tools/generate/strategyquick_difficulty.py.
 * Counterfactual CPU comparisons reuse one reachable position at each level;
 * these copies are decision probes, not loadable MASTER-bank save states. */
#include "strategyquick.h"
#include "app.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void vector(const int16_t *values,unsigned count)
{
 putchar('[');for(unsigned i=0;i<count;i++)printf("%s%d",i?",":"",values[i]);putchar(']');
}
static unsigned lights_reference_minimum(const NgGame *g)
{
 unsigned n=g->rows,best=n*n+1;
 for(unsigned first=0;first<(1u<<n);first++){
  unsigned rows[5]={0},count=0;
  for(unsigned r=0;r<n;r++)for(unsigned c=0;c<n;c++)rows[r]|=(unsigned)g->board[r*n+c]<<c;
  for(unsigned r=0;r<n;r++){
   unsigned press=r?rows[r-1]:first;
   for(unsigned c=0;c<n;c++)count+=(press>>c)&1u;
   rows[r]^=press^((press<<1)&((1u<<n)-1))^(press>>1);
   if(r)rows[r-1]^=press;
   if(r+1<n)rows[r+1]^=press;
  }
  if(!rows[n-1]&&count<best)best=count;
 }
 return best;
}
static uint32_t hash_word(uint32_t hash,uint32_t word,unsigned bytes)
{for(unsigned i=0;i<bytes;i++)hash=(hash^((word>>(8*i))&255u))*UINT32_C(16777619);return hash;}
static void revision_init_codec(void)
{
 static NgApp app;static NgSession decoded;static uint8_t bytes[NG_RECORD_MAX];
 for(unsigned revision=2;revision<=3;revision++)for(unsigned mode=0;mode<2;mode++)for(unsigned level=0;level<4;level++){
  ng_app_init(&app,(NgHooks){0},991);app.screen=NG_PLAY;app.selected_id=28;app.active=app.resumable=true;app.session.stats.started=1;
  ng_new_supply_version(&app.session.game,28,level,mode,71,1,0,0,revision);
  NgGame initial=app.session.game;size_t length=ng_single_encode(&app.session,bytes,sizeof bytes);
  assert(length&&ng_single_decode(&decoded,bytes,length,28));assert(!memcmp(&decoded.game,&initial,sizeof initial));
  for(unsigned p=0;p<initial.rows*initial.cols;p++){
   app.session.game=initial;app.session.game.cursor=(uint8_t)p;assert(sq_quick_action(&app.session.game,NGK_EXE));
   if(app.session.game.status==NG_PLAYING)break;
  }
  assert(app.session.game.status==NG_PLAYING&&app.session.game.moves==1);
  ng_app_event(&app,NGK_F1,NG_DOWN);ng_app_event(&app,NGK_F1,NG_UP);assert(app.modal==NG_MODAL_INIT);
  ng_app_event(&app,NGK_EXE,NG_DOWN);ng_app_event(&app,NGK_EXE,NG_UP);
  assert(!app.modal&&app.session.game.assisted&&app.session.game.pack_revision==revision);
  assert(app.session.game.moves==0&&app.session.game.rng==initial.rng&&app.session.game.puzzle_id==initial.puzzle_id);
  assert(!memcmp(app.session.game.board,initial.board,sizeof initial.board)&&!memcmp(app.session.game.data,initial.data,sizeof initial.data)&&ng_valid(&app.session.game));
 }
}
static void lights_revision_checks(void)
{
 /* Pre-change revision-2 board/RNG/count digest, seeds 1..128, all E/N/H.
  * Tests the old generator identity rather than only legal/solvable output. */
 static const uint32_t legacy[2][3]={{0x963488a2,0xc97ab384,0x667129dd},{0x548ea8d7,0x6754b548,0x8bfaaf5e}};
 for(unsigned mode=0;mode<2;mode++)for(unsigned level=0;level<3;level++){
  uint32_t hash=UINT32_C(2166136261);
  for(unsigned seed=1;seed<=128;seed++){
   NgGame g;ng_new_supply_version(&g,28,level,mode,seed,1,0,0,2);assert(ng_valid(&g));
   for(unsigned p=0;p<g.rows*g.cols;p++)hash=hash_word(hash,(uint32_t)g.board[p],2);
   hash=hash_word(hash,g.rng,4);hash=hash_word(hash,(uint32_t)g.data[1],4);
  }
  assert(hash==legacy[mode][level]);
 }
 for(unsigned level=0;level<3;level++)for(unsigned seed=1;seed<=4096;seed++){
  static const unsigned target[]={2,4,5};NgGame g;
  ng_new_supply_version(&g,28,level,0,seed,1,0,0,3);assert(ng_valid(&g));
  assert(lights_reference_minimum(&g)==target[level]&&g.data[1]==(int)target[level]);
  NgGame bad=g;bad.data[1]++;assert(!ng_valid(&bad));
 }
 /* The bounded rejection fallback presses the first five cells. */
 NgGame fallback={0};fallback.rows=fallback.cols=4;
 for(unsigned p=0;p<5;p++)for(unsigned q=0;q<16;q++){
  unsigned dr=p/4>q/4?p/4-q/4:q/4-p/4,dc=p%4>q%4?p%4-q%4:q%4-p%4;
  if(dr+dc<=1)fallback.board[q]^=1;
 }
 assert(lights_reference_minimum(&fallback)==5);
}
static void initial_samples(void)
{
 static const unsigned ids[]={21,22,23,24,25,26,27,28,31,32,37,38};
 for(unsigned j=0;j<sizeof ids/sizeof ids[0];j++){
  unsigned id=ids[j];const NgModule *module=ng_module(id);
  for(unsigned mode=0;mode<module->modes;mode++)for(unsigned level=0;level<4;level++){
   unsigned bank=ng_bank_count(id,level,mode),count=bank?bank:128;
   for(unsigned sample=0;sample<count;sample++){
    NgGame g,replay;uint32_t seed=sample+1;
    if(bank){ng_new_supply(&g,id,level,mode,seed,1,1,sample);ng_new_supply(&replay,id,level,mode,seed,1,1,sample);}
    else{ng_new(&g,id,level,mode,seed,1);ng_new(&replay,id,level,mode,seed,1);}
    assert(ng_valid(&g)&&!memcmp(&g,&replay,sizeof g));
    if(id==26)assert(g.data[3]==(mode?(level==3?13:(int)level+9):0));
    printf("{\"kind\":\"start\",\"id\":%u,\"mode\":%u,\"level\":%u,\"sample\":%u,\"bank\":%u,\"size\":%u,\"revision\":%lu,\"puzzle\":%lu,\"rng\":%lu,\"knobs\":[%ld,%ld,%ld,%ld,%ld],\"board\":",id,mode,level,sample,bank,g.rows,(unsigned long)g.pack_revision,(unsigned long)g.puzzle_id,(unsigned long)g.rng,(long)g.data[0],(long)g.data[1],(long)g.data[2],(long)g.data[3],(long)g.data[4]);
    unsigned cells=id==23?2u:(unsigned)g.rows*g.cols;vector(g.board,cells);
    if(id==31 || id==38){int16_t clues[81];for(unsigned i=0;i<cells;i++)clues[i]=(int16_t)g.data[i];printf(",\"construction\":");vector(clues,cells);}
    puts("}");
   }
  }
 }
}
static int outcome(const NgGame *g)
{return g->status==NG_DRAW?0:g->status!=NG_PLAYING?1:-sq_strategy_value(g->id,g);}
static void strategy_decisions(void)
{
 for(unsigned id=21;id<=25;id++){
  unsigned samples=0;
  for(unsigned seed=1;samples<128;seed++){
   assert(seed<1024);NgGame g;ng_new(&g,id,NG_HARD,1,seed,1);
   while(!g.status && samples<128){
    if(g.cpu_pending){
     int best=sq_strategy_value(id,&g),worst=2;
     unsigned choices=id==21?g.cols:id==22?3:id==24?9:1;
     for(unsigned c=0;c<choices;c++)for(unsigned amount=1;amount<100;amount++){
      SqMove move={(int)c,(int)amount};if(!sq_strategy_legal(&g,move))break;
      NgGame child=g;assert(sq_strategy_apply(&child,move));int value=outcome(&child);
      assert(value<=best);if(value<worst)worst=value;
     }
     NgGame hard={0};
     for(unsigned level=0;level<4;level++){
      NgGame probe=g;probe.difficulty=(uint8_t)level;assert(sq_strategy_action(&probe,NGK_CPU));
      int value=outcome(&probe);if(level>=2)assert(value==best);
      if(level==2)hard=probe;
      if(level==3)assert(!memcmp(hard.board,probe.board,sizeof probe.board)&&hard.rng==probe.rng&&hard.status==probe.status);
      printf("{\"kind\":\"strategy_cpu\",\"id\":%u,\"sample\":%u,\"level\":%u,\"best\":%d,\"worst\":%d,\"actual\":%d,\"position_moves\":%lu,\"board\":",id,samples,level,best,worst,value,(unsigned long)g.moves);vector(probe.board,id==23?2u:(unsigned)g.rows*g.cols);puts("}");
     }
     samples++;
    }
    SqMove random=sq_strategy_pick(&g,false);assert(sq_strategy_apply(&g,random));assert(ng_valid(&g));
   }
  }
 }
}
static void reversi_decisions(void)
{
 unsigned samples=0;
 for(unsigned seed=1;samples<64;seed++){
  assert(seed<32);NgGame g;ng_new(&g,37,0,1,seed,1);const NgModule *module=ng_module(37);
  while(!g.status && samples<64){
   if(g.cpu_pending){
    if(g.moves>=8){
     for(unsigned level=0;level<4;level++){
      NgGame probe=g,replay=g;int move=sq_reversi_pick(&probe,level),again=sq_reversi_pick(&replay,level);
      static const int budgets[]={0,256,1500,6000};
      assert(move>=0&&move==again&&!memcmp(&probe,&replay,sizeof probe)&&probe.data[2]<=budgets[level]);
      int16_t check[64];memcpy(check,g.board,sizeof check);assert(sq_reversi_place(check,2,(unsigned)move));
      printf("{\"kind\":\"reversi_cpu\",\"id\":37,\"sample\":%u,\"level\":%u,\"position_moves\":%lu,\"choice\":%d,\"nodes\":%ld,\"depth\":%ld}\n",samples,level,(unsigned long)g.moves,move,(long)probe.data[2],(long)probe.data[3]);
     }
     samples++;
    }
    assert(module->action(&g,NGK_CPU));
   }else{
    uint8_t moves[64];unsigned count=sq_reversi_moves(g.board,1,moves);assert(count);
    g.cursor=moves[ng_rand(&g,count)];assert(module->action(&g,NGK_EXE));
   }
   assert(ng_valid(&g));
  }
 }
}
int main(void)
{lights_revision_checks();revision_init_codec();initial_samples();strategy_decisions();reversi_decisions();return 0;}
