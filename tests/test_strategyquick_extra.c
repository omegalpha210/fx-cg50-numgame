#include "../src/games/strategyquick.h"
#include "storage.h"
#ifdef SQ_EXTRA_APP_TEST
#include "support.h"
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

static uint32_t random_state=UINT32_C(0x91826ab3);
static unsigned random_below(unsigned n)
{random_state^=random_state<<13;random_state^=random_state>>17;random_state^=random_state<<5;return random_state%n;}
static NgGame fresh_extra(unsigned id,unsigned d,unsigned mode,uint32_t seed)
{NgGame g;ng_new(&g,id,d,mode,seed,1);assert(ng_valid(&g));return g;}
static const NgModule *module_extra(unsigned id){return &ng_strategyquick_extra[id-37];}
static void roundtrip_extra(const NgGame *g)
{
 NgSession before={0},after;uint8_t bytes[NG_RECORD_MAX];before.game=*g;before.stats.started=1;
 size_t size=ng_encode(&before,bytes,sizeof bytes);assert(size && ng_decode(&after,bytes,size,g->id));
 assert(!memcmp(&after.game,g,sizeof *g));assert(ng_valid(&after.game));
}
/* Independent bitboard rule oracle; engine uses row/column ray scans. */
static uint64_t shifted(uint64_t b,unsigned d)
{
 const uint64_t a=UINT64_C(0x0101010101010101),h=UINT64_C(0x8080808080808080);
 switch(d){case 0:return b>>8;case 1:return (b&~h)>>7;case 2:return (b&~h)<<1;case 3:return (b&~h)<<9;
 case 4:return b<<8;case 5:return (b&~a)<<7;case 6:return (b&~a)>>1;default:return (b&~a)>>9;}
}
static uint64_t owners(const int16_t b[64],unsigned owner)
{uint64_t bits=0;for(unsigned p=0;p<64;p++)if(b[p]==(int)owner)bits|=UINT64_C(1)<<p;return bits;}
static uint64_t reference_flips(uint64_t mine,uint64_t other,unsigned p)
{
 uint64_t bit=UINT64_C(1)<<p,all=0;if((mine|other)&bit)return 0;
 for(unsigned d=0;d<8;d++){uint64_t ray=shifted(bit,d),found=0;
  while(ray&other){found|=ray;ray=shifted(ray,d);}
  if(ray&mine)all|=found;
 }
 return all;
}
static uint64_t reference_moves(const int16_t b[64],unsigned player)
{
 uint64_t mine=owners(b,player),other=owners(b,3-player),legal=0;
 for(unsigned p=0;p<64;p++)if(reference_flips(mine,other,p))legal|=UINT64_C(1)<<p;
 return legal;
}
static void reference_place(int16_t b[64],unsigned player,unsigned p)
{
 uint64_t bits=reference_flips(owners(b,player),owners(b,3-player),p);assert(bits);b[p]=(int16_t)player;
 for(unsigned q=0;q<64;q++)if(bits&(UINT64_C(1)<<q))b[q]=(int16_t)player;
}
static unsigned bit_count(uint64_t b){unsigned count=0;while(b){b&=b-1;count++;}return count;}
static unsigned nth_bit(uint64_t b,unsigned k)
{for(unsigned p=0;p<64;p++)if(b&(UINT64_C(1)<<p)){if(!k)return p;k--;}assert(false);return 0;}
static void reversi_rules(void)
{
 unsigned rays=0,boards=0,placements=0;
 for(unsigned d=0;d<8;d++){
  unsigned start=(d==0 || d==1)?56:(d==2 || d==3 || d==4)?0:(d==5 || d==6)?7:63;
  for(unsigned code=0;code<2187;code++){
   int16_t b[64]={0},actual[64],expected[64];uint64_t ray=UINT64_C(1)<<start;unsigned value=code;
   for(unsigned k=0;k<7;k++){ray=shifted(ray,d);assert(ray);b[nth_bit(ray,0)]=(int16_t)(value%3);value/=3;}
   for(unsigned player=1;player<=2;player++){
    uint64_t flips=reference_flips(owners(b,player),owners(b,3-player),start);memcpy(actual,b,sizeof b);memcpy(expected,b,sizeof b);
    if(flips)reference_place(expected,player,start);
    assert(sq_reversi_place(actual,player,start)==(flips!=0));assert(!memcmp(actual,expected,sizeof b));placements++;
   }
   rays++;
  }
 }
 for(unsigned trial=0;trial<400;trial++){
  int16_t b[64];for(unsigned p=0;p<64;p++)b[p]=(int16_t)random_below(3);
  for(unsigned player=1;player<=2;player++){
   uint64_t legal=reference_moves(b,player),actual=0;uint8_t list[64];unsigned count=sq_reversi_moves(b,player,list);
   assert(count==bit_count(legal));for(unsigned i=0;i<count;i++)actual|=UINT64_C(1)<<list[i];assert(actual==legal);
   for(unsigned p=0;p<64;p++){
    int16_t got[64],want[64];memcpy(got,b,sizeof b);memcpy(want,b,sizeof b);
    if(legal&(UINT64_C(1)<<p))reference_place(want,player,p);
    assert(sq_reversi_place(got,player,p)==!!(legal&(UINT64_C(1)<<p)));assert(!memcmp(got,want,sizeof got));placements++;
   }
  }
  boards++;
 }
 int16_t b[64]={0};assert(!sq_reversi_place(b,0,0));assert(!sq_reversi_place(b,3,0));assert(!sq_reversi_place(b,1,64));
 printf("REVERSI independent bitboards: %u ray states, %u arbitrary boards, %u placements PASS\n",rays,boards,placements);
}
static int reference_endgame(const int16_t b[64],unsigned player)
{
 uint64_t legal=reference_moves(b,player);
 if(!legal){if(reference_moves(b,3-player))return -reference_endgame(b,3-player);return (int)bit_count(owners(b,player))-(int)bit_count(owners(b,3-player));}
 int best=-65;
 for(unsigned p=0;p<64;p++)if(legal&(UINT64_C(1)<<p)){
  int16_t child[64];memcpy(child,b,sizeof child);reference_place(child,player,p);int value=-reference_endgame(child,3-player);if(value>best)best=value;
 }
 return best;
}
static void reversi_endgames(void)
{
 unsigned cases=0,passes=0,early=0,draws=0;
 for(unsigned trial=0;trial<96;trial++){
  NgGame g=fresh_extra(37,3,trial%2,trial+1);unsigned player=g.turn+1;
  while(64-bit_count(owners(g.board,1)|owners(g.board,2))>5){
   uint64_t legal=reference_moves(g.board,player);
   if(!legal){player=3-player;legal=reference_moves(g.board,player);passes++;if(!legal)break;}
   reference_place(g.board,player,nth_bit(legal,random_below(bit_count(legal))));player=3-player;
  }
  uint64_t legal=reference_moves(g.board,player);if(!legal){player=3-player;legal=reference_moves(g.board,player);passes++;}
  if(!legal)continue;
  g.turn=(uint8_t)(player-1);int expect=reference_endgame(g.board,player),p=sq_reversi_pick(&g,3);assert(p>=0 && (legal&(UINT64_C(1)<<(unsigned)p)));
  assert(g.data[2]<=6000 && g.data[3]==5);reference_place(g.board,player,(unsigned)p);assert(-reference_endgame(g.board,3-player)==expect);cases++;
  while(true){player=3-player;legal=reference_moves(g.board,player);if(!legal){player=3-player;legal=reference_moves(g.board,player);passes++;if(!legal)break;}reference_place(g.board,player,nth_bit(legal,random_below(bit_count(legal))));}
  early+=bit_count(owners(g.board,1)|owners(g.board,2))<64;draws+=bit_count(owners(g.board,1))==bit_count(owners(g.board,2));
 }
 assert(cases>=80);printf("REVERSI exact <=5-empty reference: %u reachable endgames, passes %u, early ends %u, draws %u PASS\n",cases,passes,early,draws);
}
static void reversi_replay(void)
{
 unsigned choices[64]={0};
 for(unsigned d=0;d<4;d++)for(unsigned mode=0;mode<2;mode++)for(unsigned seed=1;seed<=16;seed++){
  NgGame a=fresh_extra(37,d,mode,seed),b=a;int pick=sq_reversi_pick(&a,d),again=sq_reversi_pick(&b,d);
  assert(pick==again && !memcmp(&a,&b,sizeof a));assert(pick>=0 && (reference_moves(a.board,a.turn+1)&(UINT64_C(1)<<(unsigned)pick)));
  if(!d&&!mode)choices[pick]++;
 }
 unsigned distinct=0;for(unsigned p=0;p<64;p++)distinct+=choices[p]!=0;assert(distinct==4);
 puts("REVERSI search:128 deterministic level/mode/seed replays, Easy reaches all four opening moves PASS");
}
static void reversi_lifecycle(void)
{
 unsigned rounds=0,plies=0,passes=0;
 for(unsigned d=0;d<4;d++)for(unsigned mode=0;mode<2;mode++)for(unsigned seed=1;seed<=2;seed++){
  NgGame g=fresh_extra(37,d,mode,seed);const NgModule *m=module_extra(37);assert(sq_reversi_moves(g.board,g.turn+1,NULL)==4);
  roundtrip_extra(&g);
  for(unsigned step=0;g.status==NG_PLAYING && step<60;step++){
   uint64_t legal=reference_moves(g.board,g.turn+1);assert(legal);
   if(g.cpu_pending){NgGame before=g;const int blocked[]={NGK_EXE,NGK_HINT,NGK_AUX,NGK_RIGHT,NGK_DEL,'5'};for(unsigned k=0;k<sizeof blocked/sizeof blocked[0];k++){assert(!m->action(&g,blocked[k]));assert(!memcmp(&before,&g,sizeof g));}assert(m->action(&g,NGK_CPU));}
   else{g.cursor=(uint8_t)nth_bit(legal,random_below(bit_count(legal)));NgGame before=g;assert(m->action(&g,NGK_EXE));int16_t want[64];memcpy(want,before.board,sizeof want);reference_place(want,1,before.cursor);assert(!memcmp(g.board,want,sizeof want));}
   static const int budgets[4]={0,256,1500,6000},depths[4]={0,1,3,5};
   assert(ng_valid(&g));assert(g.data[2]<=budgets[d] && g.data[3]<=depths[d]);passes+=g.data[1]!=0;roundtrip_extra(&g);plies++;
  }
  assert(g.status!=NG_PLAYING);unsigned human=bit_count(owners(g.board,1)),cpu=bit_count(owners(g.board,2));assert(g.status==(human>cpu?NG_WON:human<cpu?NG_LOST:NG_DRAW));
  NgGame ended=g;assert(!m->action(&g,NGK_EXE));assert(!memcmp(&g,&ended,sizeof g));rounds++;
 }
 NgGame g=fresh_extra(37,2,0,999),before=g;const NgModule *m=module_extra(37);g.cursor=27;before=g;assert(m->action(&g,NGK_EXE));assert(!memcmp(g.board,before.board,sizeof g.board)&&g.rng==before.rng&&g.moves==before.moves);
 assert(m->action(&g,NGK_HINT)&&g.assisted&&reference_moves(g.board,1)&(UINT64_C(1)<<g.cursor));assert(ng_valid(&g));roundtrip_extra(&g);
 before=g;assert(m->action(&g,NGK_AUX)&&g.notes_mode!=before.notes_mode);assert(ng_valid(&g));
 printf("REVERSI lifecycle: %u full CPU rounds, %u placements, %u forced passes; hints/illegal/terminal/codec PASS\n",rounds,plies,passes);
}
/* Independent NET reference: build each undirected pair once and use DSU to
 * reject cycles. Production uses reciprocal-port checks plus BFS and count. */
static unsigned find_set(unsigned parent[36],unsigned p)
{while(parent[p]!=p)p=parent[p];return p;}
static bool reference_net(const int16_t *b,unsigned n)
{
 unsigned parent[36];for(unsigned i=0;i<n*n;i++)parent[i]=i;
 for(unsigned p=0;p<n*n;p++){
  unsigned mask=(unsigned)b[p],row=p/n,col=p%n;if(!mask || mask>15)return false;
  if((!row&&(mask&1)) || (!col&&(mask&8)) || (row+1==n&&(mask&4)) || (col+1==n&&(mask&2)))return false;
  if(col+1<n){bool edge=!!(mask&2);if(edge!=!!(b[p+1]&8))return false;if(edge){unsigned a=find_set(parent,p),z=find_set(parent,p+1);if(a==z)return false;parent[a]=z;}}
  if(row+1<n){bool edge=!!(mask&4);if(edge!=!!(b[p+n]&1))return false;if(edge){unsigned a=find_set(parent,p),z=find_set(parent,p+n);if(a==z)return false;parent[a]=z;}}
 }
 for(unsigned p=1;p<n*n;p++)if(find_set(parent,p)!=find_set(parent,0))return false;
 return true;
}
static unsigned reference_connected(const int16_t *b,unsigned n)
{
 unsigned parent[36];for(unsigned i=0;i<n*n;i++)parent[i]=i;
 for(unsigned p=0;p<n*n;p++){
  if(p%n+1<n && (b[p]&2) && (b[p+1]&8))parent[find_set(parent,p)]=find_set(parent,p+1);
  if(p+n<n*n && (b[p]&4) && (b[p+n]&1))parent[find_set(parent,p)]=find_set(parent,p+n);
 }
 unsigned root=find_set(parent,(n/2)*n+n/2),count=0;
 for(unsigned p=0;p<n*n;p++)count+=find_set(parent,p)==root;
 return count;
}
static unsigned rotate_mask(unsigned mask){return ((mask&7)<<1)|(mask>>3);}
static void net_rules(void)
{
 NgGame g={0};g.rows=g.cols=2;unsigned valid=0;
 for(unsigned code=0;code<50625;code++){unsigned value=code;for(unsigned p=0;p<4;p++){g.board[p]=(int16_t)(1+value%15);value/=15;}bool expected=reference_net(g.board,2);assert(sq_net_complete(&g)==expected);valid+=expected;}
 assert(valid==4);g.board[0]=6;g.board[1]=12;g.board[2]=3;g.board[3]=9;assert(!sq_net_complete(&g));/* closed loop */
 g.board[0]=2;g.board[1]=8;g.board[2]=2;g.board[3]=8;assert(!sq_net_complete(&g));/* two components */
 g.rows=g.cols=7;assert(!sq_net_complete(&g));g.rows=g.cols=2;g.board[0]=16;assert(!sq_net_complete(&g));
 printf("NET independent DSU: all 50,625 2x2 nonempty-mask boards, %u valid trees PASS\n",valid);
}
static unsigned alternate_solutions(const NgGame *g,int16_t board[81],unsigned p,int16_t alternate[81])
{
 unsigned n=g->rows;if(p==n*n){if(!reference_net(board,n))return 0;for(unsigned i=0;i<n*n;i++)if(board[i]!=g->data[i]){memcpy(alternate,board,sizeof g->board);return 1;}return 0;}
 unsigned rotations[4],count=0,mask=(unsigned)g->data[p];
 for(unsigned i=0;i<4;i++){bool duplicate=false;for(unsigned j=0;j<count;j++)duplicate|=rotations[j]==mask;if(!duplicate)rotations[count++]=mask;mask=rotate_mask(mask);}
 for(unsigned i=0;i<count;i++){
  mask=rotations[i];unsigned row=p/n,col=p%n;
  if((!row&&(mask&1)) || (!col&&(mask&8)) || (row+1==n&&(mask&4)) || (col+1==n&&(mask&2)))continue;
  if(row && !!(mask&1)!=!!(board[p-n]&4))continue;
  if(col && !!(mask&8)!=!!(board[p-1]&2))continue;
  board[p]=(int16_t)mask;if(alternate_solutions(g,board,p+1,alternate))return 1;
 }
 return 0;
}
static void net_lifecycle(void)
{
 unsigned cases=0,alternative=0;
 for(unsigned d=0;d<4;d++)for(unsigned seed=1;seed<=120;seed++){
  NgGame g=fresh_extra(38,d,0,seed);const NgModule *m=module_extra(38);NgGame replay=fresh_extra(38,d,0,seed);assert(!memcmp(&g,&replay,sizeof g));assert(!sq_net_complete(&g));assert(g.score==reference_connected(g.board,g.rows));
  int16_t tree[36];for(unsigned p=0;p<g.rows*g.cols;p++)tree[p]=(int16_t)g.data[p];assert(reference_net(tree,g.rows));uint32_t rng=g.rng;roundtrip_extra(&g);
  for(unsigned p=0;p<g.rows*g.cols && g.status==NG_PLAYING;p++){
   g.cursor=(uint8_t)p;unsigned spins=0;
   while(g.board[p]!=g.data[p] && g.status==NG_PLAYING){assert(++spins<=3);assert(m->action(&g,NGK_EXE));assert(ng_valid(&g)&&g.rng==rng&&g.score==reference_connected(g.board,g.rows));}
  }
  assert(g.status==NG_WON&&reference_net(g.board,g.rows));roundtrip_extra(&g);NgGame done=g;assert(!m->action(&g,NGK_DEL));assert(!memcmp(&g,&done,sizeof g));cases++;
 }
 for(unsigned seed=1;seed<=500 && !alternative;seed++){
  NgGame g=fresh_extra(38,0,0,seed);int16_t working[81]={0},alternate[81]={0};if(!alternate_solutions(&g,working,0,alternate))continue;
  memcpy(g.board,alternate,sizeof alternate);g.score=9;g.status=NG_WON;g.moves=10;assert(ng_valid(&g));roundtrip_extra(&g);alternative=seed;
  unsigned p=0;while(rotate_mask((unsigned)g.board[p])==(unsigned)g.board[p])p++;
  g.board[p]=(int16_t)rotate_mask((unsigned)g.board[p]);g.status=NG_PLAYING;g.score=sq_net_connected(&g);g.cursor=(uint8_t)p;assert(ng_valid(&g));assert(module_extra(38)->action(&g,NGK_DEL));assert(g.status==NG_WON&&ng_valid(&g));
 }
 assert(alternative);NgGame g=fresh_extra(38,3,0,991);const NgModule *m=module_extra(38);unsigned p=0;while(rotate_mask((unsigned)g.board[p])==(unsigned)g.board[p])p++;g.cursor=(uint8_t)p;
 int16_t tile=g.board[p];assert(m->action(&g,NGK_AUX)&&g.fixed[p]);unsigned moves=g.moves;assert(m->action(&g,NGK_EXE)&&g.board[p]==tile&&g.moves==moves);assert(m->action(&g,NGK_AUX)&&!g.fixed[p]);
 assert(m->action(&g,NGK_EXE)&&m->action(&g,NGK_DEL)&&g.board[p]==tile);assert(ng_valid(&g));
 for(unsigned reveals=0;g.status==NG_PLAYING && reveals<36;reveals++){assert(m->action(&g,NGK_HINT)&&g.assisted);assert(ng_valid(&g));roundtrip_extra(&g);}
 assert(g.status==NG_WON);
 printf("NET lifecycle: %u generated/replayed/completed seeds, alternate solution seed %u, rotations/locks/reveals/codec PASS\n",cases,alternative);
}
/* Named edge fixtures complement the broad randomized/ray audits. These
 * synthetic positions isolate rules; they are not claimed as reachable games. */
static NgGame reversi_fixture(const int16_t board[64],unsigned turn,unsigned last)
{
 NgGame g=fresh_extra(37,0,0,73);memcpy(g.board,board,64*sizeof *board);
 g.turn=(uint8_t)turn;g.cpu_pending=(uint8_t)turn;g.data[0]=(int32_t)last;
 g.moves=bit_count(owners(board,1)|owners(board,2))-4;g.score=bit_count(owners(board,1));g.message[0]=0;
 assert(ng_valid(&g));return g;
}
static void extra_pixel(void *context,int x,int y,int w,int h,uint16_t color);
static void frozen_extra(const NgGame *g)
{
 const int keys[]={NGK_UP,NGK_DOWN,NGK_LEFT,NGK_RIGHT,NGK_EXE,NGK_DEL,NGK_HINT,NGK_AUX,NGK_CPU,'0','5','9','+','='};
 for(unsigned i=0;i<sizeof keys/sizeof keys[0];i++){NgGame copy=*g;assert(!module_extra(g->id)->action(&copy,keys[i]));assert(!memcmp(&copy,g,sizeof copy));}
 unsigned count=0;NgCanvas canvas={&count,extra_pixel};module_extra(g->id)->render(g,&canvas);assert(count);
}
static void reversi_edges(void)
{
 const NgModule *m=module_extra(37);unsigned checks=0;
 for(unsigned player=1;player<=2;player++){
  int16_t b[64]={0};unsigned center=27;
  for(unsigned d=0;d<8;d++){uint64_t near=shifted(UINT64_C(1)<<center,d),far=shifted(near,d);assert(near&&far);b[nth_bit(near,0)]=(int16_t)(3-player);b[nth_bit(far,0)]=(int16_t)player;}
  int16_t expected[64];memcpy(expected,b,sizeof b);assert(bit_count(reference_flips(owners(b,player),owners(b,3-player),center))==8);
  reference_place(expected,player,center);assert(sq_reversi_place(b,player,center)&&!memcmp(b,expected,sizeof b));assert(bit_count(owners(b,player))==17);checks++;
 }
 /* Flipping the ray at 27->28 must not cascade from 28 into a second line. */
 int16_t b[64]={0};b[28]=2;b[29]=1;b[20]=2;b[12]=1;assert(sq_reversi_place(b,1,27));assert(b[28]==1&&b[20]==2);checks++;
 /* Neither an adjacent friendly disc nor the far board edge brackets an
  * opponent line. Occupied destinations also leave the board unchanged. */
 memset(b,0,sizeof b);b[1]=1;b[2]=2;b[3]=1;int16_t before[64];memcpy(before,b,sizeof b);assert(!sq_reversi_place(b,1,0)&&!memcmp(b,before,sizeof b));
 memset(b,0,sizeof b);for(unsigned p=1;p<8;p++)b[p]=2;memcpy(before,b,sizeof b);assert(!sq_reversi_place(b,1,0)&&!memcmp(b,before,sizeof b));assert(!sq_reversi_place(b,1,1)&&!memcmp(b,before,sizeof b));checks+=3;
 /* Human and CPU both retain their turn when the opponent must pass. */
 for(unsigned player=1;player<=2;player++){
  for(unsigned p=0;p<64;p++)b[p]=(int16_t)player;
  b[0]=b[63]=0;b[1]=b[62]=(int16_t)(3-player);NgGame g=reversi_fixture(b,player-1,1);
  assert(reference_moves(b,player)==((UINT64_C(1)<<63)|1)&&!reference_moves(b,3-player));g.cursor=0;
  assert(m->action(&g,player==1?NGK_EXE:NGK_CPU));assert(ng_valid(&g)&&g.status==NG_PLAYING&&g.turn==player-1&&g.data[1]==(int)(3-player));
  assert(g.cpu_pending==(player==2)&&g.score==(player==1?62u:1u));roundtrip_extra(&g);
  NgGame bad=g;bad.data[1]=(int32_t)player;assert(!ng_valid(&bad));bad=g;bad.turn^=1;bad.cpu_pending=bad.turn;assert(!ng_valid(&bad));
  uint64_t legal=reference_moves(g.board,player);assert(bit_count(legal)==1);g.cursor=(uint8_t)nth_bit(legal,0);
  assert(m->action(&g,player==1?NGK_EXE:NGK_CPU));assert(ng_valid(&g)&&g.status==(player==1?NG_WON:NG_LOST)&&g.moves==60&&!g.cpu_pending&&!g.data[1]);frozen_extra(&g);roundtrip_extra(&g);checks++;
 }
 for(int outcome=-1;outcome<=1;outcome++){
  for(unsigned p=0;p<64;p++)b[p]=p>=2&&p<=31?1:2;
  b[0]=0;if(outcome<0)b[31]=2;else if(outcome>0)b[63]=1;
  NgGame g=reversi_fixture(b,0,62);g.cursor=0;assert(m->action(&g,NGK_EXE));
  assert(ng_valid(&g)&&g.status==(outcome<0?NG_LOST:outcome>0?NG_WON:NG_DRAW)&&g.score==(uint32_t)(32+outcome)&&g.moves==60);
  NgGame bad=g;bad.score++;assert(!ng_valid(&bad));bad=g;bad.status=NG_PLAYING;assert(!ng_valid(&bad));
  frozen_extra(&g);roundtrip_extra(&g);checks++;
 }
 for(unsigned p=0;p<64;p++)b[p]=1;
 b[0]=b[63]=0;b[1]=2;NgGame g=reversi_fixture(b,0,1);g.cursor=0;assert(m->action(&g,NGK_EXE));
 assert(ng_valid(&g)&&g.status==NG_WON&&g.score==63&&g.moves==59&&!g.board[63]);assert(!reference_moves(g.board,1)&&!reference_moves(g.board,2));frozen_extra(&g);roundtrip_extra(&g);checks++;
 printf("REVERSI named edges: %u simultaneous/cascade/illegal/pass/full/draw/early-end cases; frozen input and corruption checks PASS\n",checks);
}
static void net_edges(void)
{
 NgGame g={0};g.rows=g.cols=3;
 const int16_t snake[9]={2,10,12,6,10,9,3,10,8};memcpy(g.board,snake,sizeof snake);assert(reference_net(g.board,3)&&sq_net_complete(&g)&&sq_net_connected(&g)==9);
 /* A connected tree with a dangling north port remains connected, but loses. */
 g.board[0]|=1;assert(sq_net_connected(&g)==9&&!sq_net_complete(&g)&&!reference_net(g.board,3));
 memcpy(g.board,snake,sizeof snake);g.board[0]|=4;g.board[3]|=1;assert(sq_net_connected(&g)==9&&!sq_net_complete(&g)&&!reference_net(g.board,3));/* connected cycle */
 memcpy(g.board,snake,sizeof snake);g.board[0]=4;assert(!sq_net_complete(&g)&&!reference_net(g.board,3));/* unmatched neighbours */
 const int16_t deceptive[9]={6,12,4,3,9,5,2,10,9};memcpy(g.board,deceptive,sizeof deceptive);
 unsigned ports=0;for(unsigned p=0;p<9;p++)ports+=bit_count((unsigned)g.board[p]);assert(ports==16&&sq_net_connected(&g)==4&&!sq_net_complete(&g));/* N-1 edges, but cycle + separate tree */
 const NgModule *m=module_extra(38);unsigned seen=0,cases=0;
 for(unsigned seed=1;seed<=100 && seen!=0xfffeu;seed++){
  NgGame start=fresh_extra(38,3,0,seed);
  for(unsigned p=0;p<36;p++){
   unsigned mask=(unsigned)start.board[p];if(seen&(1u<<mask))continue;
   for(unsigned direction=0;direction<2;direction++){
    g=start;g.cursor=(uint8_t)p;NgGame before=g;unsigned expected=mask;
    for(unsigned k=0;k<(direction?3u:1u);k++)expected=rotate_mask(expected);
    assert(m->action(&g,direction?NGK_DEL:NGK_EXE));assert((unsigned)g.board[p]==expected&&g.rng==before.rng&&g.moves==before.moves+(expected!=mask)&&ng_valid(&g));
    if(expected==mask)assert(strstr(g.message,"same after rotation"));
    assert(!memcmp(g.fixed,before.fixed,sizeof g.fixed)&&g.assisted==before.assisted);cases++;
   }
   g=start;g.cursor=(uint8_t)p;assert(m->action(&g,NGK_AUX)&&g.fixed[p]);unsigned moves=g.moves;uint32_t rng=g.rng;
   const int locked_keys[]={NGK_EXE,'5',NGK_DEL};for(unsigned i=0;i<3;i++){assert(m->action(&g,locked_keys[i]));assert(g.board[p]==(int)mask&&g.moves==moves&&g.rng==rng&&g.fixed[p]&&strstr(g.message,"Locked"));}
   assert(ng_valid(&g));assert(m->action(&g,NGK_AUX)&&!g.fixed[p]&&g.moves==moves+1);roundtrip_extra(&g);seen|=1u<<mask;
  }
 }
 assert(seen==0xfffeu);
 g=fresh_extra(38,2,0,971);unsigned p=0;while(g.board[p]==g.data[p])p++;g.cursor=(uint8_t)p;
 assert(m->action(&g,NGK_AUX)&&g.fixed[p]);assert(m->action(&g,NGK_HINT)&&g.fixed[p]&&g.board[p]==g.data[p]&&g.assisted&&g.data[81]==(int)p&&ng_valid(&g));roundtrip_extra(&g);
 for(unsigned i=0;g.status==NG_PLAYING&&i<25;i++)assert(m->action(&g,NGK_HINT));
 assert(g.status==NG_WON&&ng_valid(&g));frozen_extra(&g);roundtrip_extra(&g);
 printf("NET named edges: connected dangling port/cycle, reciprocal mismatch, N-1-edge disconnected graph; %u directional rotations/all 15 masks and locks/reveal/frozen input PASS\n",cases);
}

#define REJECT(field,value) do{NgGame bad=original;bad.field=(value);assert(!ng_valid(&bad));}while(0)
static void invalid_extra(void)
{
 for(unsigned id=37;id<=38;id++)for(unsigned d=0;d<4;d++){
  NgGame original=fresh_extra(id,d,0,871);REJECT(difficulty,4);REJECT(mode,2);REJECT(phase,1);REJECT(board[0],16);REJECT(data[127],1);REJECT(notes[0],1);REJECT(score,99);REJECT(status,NG_WON);REJECT(cursor,80);REJECT(history_count,1);REJECT(scroll,1);REJECT(input[0],'1');
  if(id==37){REJECT(board[27],0);REJECT(fixed[0],1);REJECT(data[0],0);REJECT(data[1],1);REJECT(data[2],6001);REJECT(data[3],6);REJECT(cpu_pending,1);REJECT(turn,1);REJECT(moves,1);REJECT(puzzle_id,2);}
  else{REJECT(data[0],0);REJECT(data[81],0);REJECT(data[82],1);REJECT(rng,original.rng^1u);REJECT(seed,original.seed^1u);REJECT(notes_mode,1);REJECT(turn,1);REJECT(cpu_pending,1);REJECT(rows,2);REJECT(cols,7);}
 }
 puts("Extra validators: invalid modes/difficulty/shape/seed/metadata/phase/score/turn/reserved state PASS");
}
#undef REJECT
static void extra_pixel(void *context,int x,int y,int w,int h,uint16_t color)
{unsigned *count=context;assert(x>=0&&y>=27&&x+w<=396&&y+h<=185&&w>0&&h>0);(void)color;(*count)++;}
static void extra_render(void)
{
 unsigned calls=0;NgCanvas canvas={&calls,extra_pixel};
 for(unsigned id=37;id<=38;id++)for(unsigned d=0;d<4;d++)for(unsigned mode=0;mode<module_extra(id)->modes;mode++){
  NgGame g=fresh_extra(id,d,mode,99);module_extra(id)->render(&g,&canvas);g.cursor=(uint8_t)(g.rows*g.cols-1);module_extra(id)->render(&g,&canvas);
 }
 assert(calls);printf("Extra render bounds: %u pixel rectangles within content pane PASS\n",calls);
}
#ifdef SQ_EXTRA_APP_TEST
static void extra_press(NgApp *app,int key)
{ng_app_event(app,key,NG_DOWN);ng_app_event(app,key,NG_UP);}
static void extra_app(void)
{
 static TestDisk disk;unsigned cases=0;
 for(unsigned id=37;id<=38;id++)for(unsigned d=0;d<4;d++)for(unsigned mode=0;mode<module_extra(id)->modes;mode++){
  memset(&disk,0,sizeof disk);disk.write_budget=-1;NgApp app;ng_app_init(&app,test_hooks(&disk),812);app.settings.difficulty[id-1]=(uint8_t)d;app.settings.mode[id-1]=(uint8_t)mode;
  extra_press(&app,id==37?'5':'6');extra_press(&app,'6');assert(app.screen==NG_ENTRY&&app.selected_id==id);extra_press(&app,NGK_F6);
  NgGame *g=&app.session.game;assert(app.screen==NG_PLAY&&g->id==id&&g->difficulty==d&&g->mode==mode&&ng_valid(g));
  if(g->cpu_pending){assert(ng_app_cpu(&app));assert(!g->cpu_pending&&!app.session.undo_count);}
  if(id==37)g->cursor=(uint8_t)nth_bit(reference_moves(g->board,1),0);
  else {unsigned p=0;while(rotate_mask((unsigned)g->board[p])==(unsigned)g->board[p])p++;g->cursor=(uint8_t)p;}
  NgGame before=*g;extra_press(&app,NGK_EXE);assert(g->moves==before.moves+1&&app.session.undo_count==1);
  if(id==37){
   assert(g->cpu_pending);NgGame pending=*g;extra_press(&app,NGK_EXE);assert(!memcmp(g,&pending,sizeof pending));assert(ng_checkpoint(&app));
   NgApp cold_cpu;ng_app_init(&cold_cpu,test_hooks(&disk),992);assert(cold_cpu.resumable&&cold_cpu.session.game.cpu_pending);
   assert(cold_cpu.session.undo_count==app.session.undo_count&&!memcmp(cold_cpu.session.undo,app.session.undo,app.session.undo_count*sizeof(NgGame)));extra_press(&cold_cpu,NGK_F1);
   assert(ng_app_cpu(&cold_cpu)&&ng_valid(&cold_cpu.session.game));assert(ng_app_cpu(&app));assert(app.session.undo_count==1&&!memcmp(g,&cold_cpu.session.game,sizeof *g));
  }
  assert(ng_valid(g));roundtrip_extra(g);extra_press(&app,NGK_F2);assert(g->assisted&&g->moves==before.moves&&g->rng==before.rng&&!memcmp(g->board,before.board,sizeof g->board)&&ng_valid(g));
  extra_press(&app,NGK_F3);assert(g->assisted&&ng_valid(g));if(id==38)assert(g->data[81]>=0&&g->fixed[g->data[81]]);
  NgGame checkpoint=*g;extra_press(&app,NGK_EXIT);assert(app.screen==NG_ENTRY);
  NgApp cold;ng_app_init(&cold,test_hooks(&disk),912);assert(cold.resumable&&cold.screen==NG_MAIN);extra_press(&cold,NGK_F1);assert(cold.screen==NG_PLAY&&!memcmp(&cold.session.game,&checkpoint,sizeof checkpoint));
 app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_RESUME);extra_press(&app,NGK_EXE);assert(app.screen==NG_PLAY&&!memcmp(g,&checkpoint,sizeof checkpoint));
  extra_press(&app,NGK_F5);assert(app.modal==NG_MODAL_RULES);extra_press(&app,NGK_EXIT);assert(!app.modal&&!memcmp(g,&checkpoint,sizeof checkpoint));
  extra_press(&app,NGK_F1);assert(app.modal==NG_MODAL_INIT);extra_press(&app,NGK_EXE);assert(g->seed==checkpoint.seed&&g->moves==0&&g->assisted&&ng_valid(g));
  cases++;
 }
 printf("Extra real app: %u configurations; START/CPU barrier/round UNDO/REVEAL/v5 cold + local RESUME/RULES/INIT PASS\n",cases);
}
static void app_pixel_extra(void *context,int x,int y,int w,int h,uint16_t color)
{unsigned *draws=context;assert(x>=0&&y>=0&&w>0&&h>0&&x+w<=396&&y+h<=224);(void)color;(*draws)++;}
static void render_app_extra(const NgApp *app)
{unsigned draws=0;NgCanvas canvas={&draws,app_pixel_extra};ng_render(app,&canvas);assert(draws);}
static void completion_app_extra(void)
{
 static TestDisk disk;unsigned cases=0;
 for(unsigned id=37;id<=38;id++)for(unsigned path=0;path<3;path++){
  memset(&disk,0,sizeof disk);disk.write_budget=-1;NgApp app;ng_app_init(&app,test_hooks(&disk),881);app.settings.difficulty[id-1]=0;
  open_game(&app,id);
  assert(app.screen==NG_PLAY&&app.resumable);NgGame *g=&app.session.game;
  if(id==37){
   int16_t board[64];for(unsigned p=0;p<64;p++)board[p]=p>=2&&p<=31?1:2;
   board[0]=0;if(path==0)board[31]=2;else if(path==2)board[63]=1;
   *g=reversi_fixture(board,0,62);g->cursor=0;extra_press(&app,NGK_EXE);
   assert(g->status==(path==0?NG_LOST:path==1?NG_DRAW:NG_WON));
  }else{
   for(unsigned p=0;p<g->rows*g->cols;p++)g->board[p]=(int16_t)g->data[p];
   unsigned p=0;while(rotate_mask((unsigned)g->board[p])==(unsigned)g->board[p])p++;
   g->board[p]=(int16_t)rotate_mask((unsigned)g->board[p]);g->cursor=(uint8_t)p;g->moves=12;g->score=sq_net_connected(g);assert(ng_valid(g));
   extra_press(&app,NGK_DEL);assert(g->status==NG_WON);
  }
  assert(ng_valid(g)&&app.modal==NG_MODAL_RESULT&&!app.resumable);render_app_extra(&app);NgGame finished=*g;
  NgApp cold;ng_app_init(&cold,test_hooks(&disk),990);assert(!cold.resumable);extra_press(&cold,NGK_F1);assert(cold.screen==NG_MAIN);
  if(path!=1){
   ng_app_event(&app,NGK_EXIT,NG_DOWN);assert(app.screen==NG_PLAY&&app.result_view&&!app.modal);
   ng_app_event(&app,NGK_EXIT,NG_HOLD);ng_app_event(&app,NGK_EXIT,NG_DOWN);assert(app.screen==NG_PLAY&&app.result_view&&!memcmp(g,&finished,sizeof finished));
   ng_app_event(&app,NGK_EXIT,NG_UP);render_app_extra(&app);
   const int frozen_keys[]={NGK_F1,NGK_F2,NGK_F3,NGK_F4,NGK_EXE,NGK_DEL,NGK_UP,NGK_RIGHT,NGK_DOWN,NGK_LEFT,'0','5','9','+','='};
   for(unsigned i=0;i<sizeof frozen_keys/sizeof frozen_keys[0];i++){extra_press(&app,frozen_keys[i]);assert(!app.modal&&app.result_view&&!memcmp(g,&finished,sizeof finished));}
   assert(!ng_app_tick(&app,1000)&&!ng_app_cpu(&app)&&!memcmp(g,&finished,sizeof finished));
   extra_press(&app,NGK_F5);assert(app.modal==NG_MODAL_RULES);extra_press(&app,NGK_EXIT);assert(!app.modal&&app.result_view&&!memcmp(g,&finished,sizeof finished));
   if(path==0){
    extra_press(&app,NGK_EXIT);assert(app.screen==NG_ENTRY&&!app.resumable&&!app.result_view);
    assert(ng_entry_action(&app,app.entry_selection)==NG_ENTRY_NEW);
    for(unsigned row=0;row<ng_entry_count(&app);row++)assert(ng_entry_action(&app,row)!=NG_ENTRY_RESUME);
    render_app_extra(&app);
   }
  }
  int key=path==1?NGK_EXE:NGK_F6;ng_app_event(&app,key,NG_DOWN);
  assert(app.screen==NG_PLAY&&!app.modal&&!app.result_view&&app.resumable&&g->status==NG_PLAYING&&g->id==id&&g->difficulty==finished.difficulty&&g->mode==finished.mode&&g->seed!=finished.seed&&ng_valid(g));
  NgGame started=*g;ng_app_event(&app,key,NG_HOLD);ng_app_event(&app,key,NG_DOWN);assert(!memcmp(g,&started,sizeof started));ng_app_event(&app,key,NG_UP);render_app_extra(&app);
  ng_app_init(&cold,test_hooks(&disk),991);assert(cold.resumable&&!memcmp(&cold.session.game,&started,sizeof started));cases++;
 }
 printf("Extra actual result flow: %u WIN/LOSS/DRAW/SOLVED paths, modal NEW / VIEW RESULT / entry / F6 NEW, frozen keys/timer/held barriers and durable clear/cold NEW PASS\n",cases);
}
#endif
int test_strategyquick_extra(void)
{
 setvbuf(stdout,NULL,_IONBF,0);reversi_rules();reversi_endgames();reversi_replay();reversi_lifecycle();net_rules();net_lifecycle();reversi_edges();net_edges();invalid_extra();extra_render();
#ifdef SQ_EXTRA_APP_TEST
 extra_app();completion_app_extra();
#endif
 return 0;
}
#ifdef SQ_EXTRA_TEST_MAIN
int main(void){return test_strategyquick_extra();}
#endif
