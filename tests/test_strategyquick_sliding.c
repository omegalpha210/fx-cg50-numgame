/* Independent beta.4 Sliding audit: permutation-rank BFS and fixed-order
 * Manhattan IDA*. The generator uses byte-map BFS and ordered incremental IDA*.
 * No host distance table or solver is linked into the calculator. */
#include "strategyquick.h"
#include "support.h"
#include <stdlib.h>

static uint8_t distance3[362880];
static uint64_t queue3[181440];
static const unsigned factorial[9]={1,1,2,6,24,120,720,5040,40320};
static bool emit;
static unsigned rank3(const uint8_t *board)
{
 unsigned rank=0;for(unsigned p=0;p<9;p++)for(unsigned q=p+1;q<9;q++)if(board[q]<board[p])rank+=factorial[8-p];return rank;
}
static uint64_t encode3(const uint8_t *board)
{uint64_t value=0;for(unsigned p=0;p<9;p++)value|=(uint64_t)board[p]<<(4*p);return value;}
static int neighbour(unsigned p,unsigned n,unsigned direction)
{
 if(direction==0)return p>=n?(int)(p-n):-1;
 if(direction==1)return p%n+1<n?(int)(p+1):-1;
 if(direction==2)return p<n*(n-1)?(int)(p+n):-1;
 return p%n?(int)p-1:-1;
}
static void build_bfs(void)
{
 uint8_t board[9]={1,2,3,4,5,6,7,8,0};memset(distance3,255,sizeof distance3);
 distance3[rank3(board)]=0;queue3[0]=encode3(board);unsigned head=0,tail=1,maximum=0;
 while(head<tail){
  uint64_t code=queue3[head++];unsigned blank=0;
  for(unsigned p=0;p<9;p++){board[p]=(uint8_t)((code>>(4*p))&15u);if(!board[p])blank=p;}
  unsigned d=distance3[rank3(board)];if(d>maximum)maximum=d;
  for(unsigned direction=0;direction<4;direction++){
   int q=neighbour(blank,3,direction);if(q<0)continue;
   board[blank]=board[q];board[q]=0;unsigned rank=rank3(board);
   if(distance3[rank]==255){distance3[rank]=(uint8_t)(d+1);assert(tail<181440);queue3[tail++]=encode3(board);}
   board[q]=board[blank];board[blank]=0;
  }
 }
 assert(tail==181440&&maximum==31);
}
static unsigned manhattan_ref(const uint8_t *board,unsigned n)
{
 unsigned result=0;
 for(unsigned p=0;p<n*n;p++)if(board[p]){
  unsigned goal=board[p]-1,r=p/n,c=p%n,gr=goal/n,gc=goal%n;
  result+=(r>gr?r-gr:gr-r)+(c>gc?c-gc:gc-c);
 }
 return result;
}
static uint64_t reference_nodes;
static uint8_t ida_board[16];
static uint8_t reference_path[64];
static unsigned ida_search(unsigned blank,int previous,unsigned depth,unsigned bound)
{
 reference_nodes++;assert(reference_nodes<UINT64_C(2000000));unsigned h=manhattan_ref(ida_board,4);
 if(depth+h>bound)return 0;
 if(!h)return 1;
 for(unsigned direction=0;direction<4;direction++){
  int q=neighbour(blank,4,direction);if(q<0 || q==previous)continue;
  reference_path[depth]=(uint8_t)direction;
  ida_board[blank]=ida_board[q];ida_board[q]=0;
  unsigned solved=ida_search((unsigned)q,(int)blank,depth+1,bound);
  ida_board[q]=ida_board[blank];ida_board[blank]=0;
  if(solved)return 1;
 }
 return 0;
}
static unsigned exact4(const uint8_t *board,unsigned upper)
{
 memcpy(ida_board,board,16);unsigned blank=0;while(ida_board[blank])blank++;
 reference_nodes=0;
 for(unsigned bound=manhattan_ref(board,4);bound<=upper;bound++)if(ida_search(blank,-1,0,bound))return bound;
 assert(!"Certified start has no solution within its claimed distance");return 0;
}
static void finish_production(NgGame g,unsigned distance)
{
 for(unsigned step=0;step<distance;step++){
  unsigned direction=reference_path[step];
  if(!g.mode){
   uint8_t board[9];for(unsigned p=0;p<9;p++)board[p]=(uint8_t)g.board[p];
   for(direction=0;direction<4;direction++){
    int q=neighbour(g.cursor,3,direction);if(q<0)continue;
    board[g.cursor]=board[q];board[q]=0;bool lower=distance3[rank3(board)]==distance-step-1;
    board[q]=board[g.cursor];board[g.cursor]=0;if(lower)break;
   }
   assert(direction<4);
  }
  assert(sq_quick_action(&g,NGK_UP+(int)direction)&&ng_valid(&g));
 }
 assert(g.status==NG_WON&&g.moves==distance);
}
static void emit_state(const NgGame *g,unsigned sample,int exact,uint64_t work)
{
 if(!emit)return;
 uint8_t board[16];for(unsigned p=0;p<g->rows*g->cols;p++)board[p]=(uint8_t)g->board[p];
 printf("{\"revision\":%lu,\"mode\":%u,\"difficulty\":%u,\"sample\":%u,\"puzzle_id\":%lu,\"exact\":",(unsigned long)g->pack_revision,g->mode,g->difficulty,sample,(unsigned long)g->puzzle_id);
 if(exact<0)printf("null");else printf("%d",exact);
 printf(",\"lower_bound\":%u,\"stored_metric\":%ld,\"reference_nodes\":%llu,\"board\":[",manhattan_ref(board,g->rows),(long)g->data[1],(unsigned long long)work);
 for(unsigned p=0;p<g->rows*g->cols;p++)printf("%s%u",p?",":"",board[p]);
 puts("]}");
}
static void all_samples(void)
{
 for(unsigned revision=3;revision<=4;revision++)for(unsigned mode=0;mode<2;mode++)for(unsigned level=0;level<4;level++){
  unsigned count=ng_bank_count_version(27,level,mode,revision),samples=count?count:128;bool seen[128]={0};
  assert(count==(level==3?(mode?30u:2u):revision==4?128u:0u));
  for(unsigned sample=0;sample<samples;sample++){
   NgGame g,replay;ng_new_supply_version(&g,27,level,mode,sample+1,1,count?1:0,sample,revision);
   ng_new_supply_version(&replay,27,level,mode,sample+1,1,count?1:0,sample,revision);
   assert(ng_valid(&g)&&!memcmp(&g,&replay,sizeof g));uint8_t board[16];
   for(unsigned p=0;p<g.rows*g.cols;p++)board[p]=(uint8_t)g.board[p];
   int exact=-1;uint64_t work=0;
   if(!mode){exact=distance3[rank3(board)];assert(exact>=1&&exact<=31);}
   else if(revision==4&&level<3){exact=(int)exact4(board,(unsigned)g.data[1]);work=reference_nodes;}
   if(revision==4&&level<3){
    static const int low[3]={8,15,21},high[3]={14,20,26};
    assert(exact>=low[level]&&exact<=high[level]&&g.data[0]==exact&&g.data[1]==exact);
    NgGame bad=g;bad.puzzle_id=128;assert(!ng_valid(&bad));bad=g;bad.data[1]++;assert(!ng_valid(&bad));bad=g;bad.data[0]++;assert(!ng_valid(&bad));
   }
   if(level==3){if(!mode)assert(exact==31);else assert(manhattan_ref(board,4)>=48);}
   if(count){assert(g.puzzle_id<count&&!seen[g.puzzle_id]);seen[g.puzzle_id]=true;}
   emit_state(&g,sample,exact,work);
   if(exact>=0)finish_production(g,(unsigned)exact);
  }
 }
}
static uint32_t digest_word(uint32_t hash,uint32_t value,unsigned bytes)
{for(unsigned i=0;i<bytes;i++)hash=(hash^((value>>(8*i))&255u))*UINT32_C(16777619);return hash;}
static void legacy_identity(void)
{
 /* Captured by compiling beta.3 commit 14aa932's unmodified quick engine,
  * seeds1..128. Covers every tile, RNG, both distance/shuffle fields, cursor/ID. */
 static const uint32_t expected[2][4]={{0x546f84e7,0xe725b99a,0x4b9d2e63,0x2f8e93ee},{0xf25c70d9,0xbf90c117,0x6aab06ba,0x383ba319}};
 for(unsigned revision=1;revision<=3;revision++)for(unsigned mode=0;mode<2;mode++)for(unsigned level=0;level<4;level++){
  uint32_t hash=UINT32_C(2166136261);
  for(unsigned seed=1;seed<=128;seed++){
   NgGame g;ng_new_supply_version(&g,27,level,mode,seed,1,0,0,revision);assert(ng_valid(&g));
   for(unsigned p=0;p<g.rows*g.cols;p++)hash=digest_word(hash,(uint32_t)g.board[p],2);
   hash=digest_word(hash,g.rng,4);hash=digest_word(hash,(uint32_t)g.data[0],4);hash=digest_word(hash,(uint32_t)g.data[1],4);hash=digest_word(hash,g.cursor,1);hash=digest_word(hash,g.puzzle_id,4);
  }
  assert(hash==expected[mode][level]);
 }
}
static void compatibility(void)
{
 static TestDisk disk;static NgApp app,cold;static uint8_t bytes[NG_RECORD_MAX];static NgSession decoded;
 for(unsigned revision=1;revision<=4;revision++)for(unsigned mode=0;mode<2;mode++)for(unsigned level=0;level<4;level++){
  memset(&disk,0,sizeof disk);disk.write_budget=-1;ng_app_init(&app,test_hooks(&disk),881);
  app.screen=NG_PLAY;app.selected_id=27;app.settings.last_game=27;app.settings.mode[26]=(uint8_t)mode;app.settings.difficulty[26]=(uint8_t)level;
  app.active=app.resumable=true;app.session.stats.started=1;
  ng_new_supply_version(&app.session.game,27,level,mode,931,1,0,0,revision);NgGame initial=app.session.game;
  assert(ng_valid(&initial));
  for(int key=NGK_UP;key<=NGK_LEFT;key++){
   NgGame trial=initial;if(sq_quick_action(&trial,key)){tap(&app,key);break;}
  }
  assert(app.session.game.moves==1&&app.session.undo_count==1&&ng_checkpoint(&app));NgGame progress=app.session.game;
  ng_app_init(&cold,test_hooks(&disk),998);assert(cold.resumable&&!memcmp(&cold.session.game,&progress,sizeof progress));tap(&cold,NGK_F1);assert(cold.screen==NG_PLAY);
  size_t n=ng_single_encode(&cold.session,bytes,sizeof bytes);assert(n&&ng_single_decode(&decoded,bytes,n,27)&&!memcmp(&decoded.game,&progress,sizeof progress));
  tap(&cold,NGK_F1);assert(cold.modal==NG_MODAL_INIT);tap(&cold,NGK_EXE);
  assert(!cold.modal&&cold.session.game.assisted&&cold.session.game.pack_revision==revision&&cold.session.game.rng==initial.rng&&cold.session.game.puzzle_id==initial.puzzle_id);
  assert(!memcmp(cold.session.game.board,initial.board,sizeof initial.board)&&!memcmp(cold.session.game.data,initial.data,sizeof initial.data));
  if(revision<4&&level<3)assert(cold.session.game.generation_policy==NG_SUPPLY_RUNTIME);
 }
}
static void cycles(void)
{
 static TestDisk disk;static NgApp app,cold;
 for(unsigned mode=0;mode<2;mode++)for(unsigned level=0;level<4;level++){
  memset(&disk,0,sizeof disk);disk.write_budget=-1;ng_app_init(&app,test_hooks(&disk),417);
  app.settings.mode[26]=(uint8_t)mode;app.settings.difficulty[26]=(uint8_t)level;open_game(&app,27);
  unsigned count=ng_bank_count(27,level,mode),previous=UINT32_MAX;bool seen[128]={0};
  for(unsigned step=0;step<count*2+1;step++){
   if(step){tap(&app,NGK_EXIT);app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_NEW);tap(&app,NGK_EXE);}
   NgGame *g=&app.session.game;assert(app.screen==NG_PLAY&&g->pack_revision==4&&g->puzzle_id<count&&g->puzzle_id!=previous);
   if(step%count==0)memset(seen,0,sizeof seen);
   assert(!seen[g->puzzle_id]);seen[g->puzzle_id]=true;previous=g->puzzle_id;
   if(step==count/2){assert(ng_checkpoint(&app));ng_app_init(&cold,test_hooks(&disk),971);assert(cold.resumable);tap(&cold,NGK_F1);app=cold;}
  }
 }
}
int main(int argc,char **argv)
{
 assert(argc==1 || (argc==2&&!strcmp(argv[1],"--emit")));emit=argc==2;
 build_bfs();all_samples();legacy_identity();compatibility();cycles();
 if(!emit)puts("PASS: independent BFS181440, 384 exact 4x4 IDA proofs, 1600 before/after starts, 1156 production completions, 3072 legacy identity starts, 32 revision resume/INIT cases, all 8 banks two cycles plus boundary and midcycle cold resume");
 return 0;
}
