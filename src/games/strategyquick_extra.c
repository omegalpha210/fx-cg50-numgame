#include "strategyquick.h"
#include <stdio.h>
#include <string.h>

/* REVERSI: board0..63: 0 empty,1 human,2 CPU. turn0 human/1 CPU.
 * data0 last placement(-1 initially),data1 automatically passed owner(0/1/2),
 * data2 last search nodes,data3 last fully completed search depth. */
static const int rv_dr[8]={-1,-1,-1,0,0,1,1,1};
static const int rv_dc[8]={-1,0,1,-1,1,-1,0,1};
static unsigned rv_flips(const int16_t b[64],unsigned player,unsigned p,uint8_t out[64])
{
 if(player<1 || player>2 || p>=64 || b[p])return 0;
 unsigned count=0;
 for(unsigned d=0;d<8;d++){
  int row=(int)(p/8)+rv_dr[d],col=(int)(p%8)+rv_dc[d];unsigned begin=count;
  while(row>=0 && row<8 && col>=0 && col<8 && b[row*8+col]==(int)(3-player)){
   if(out)out[count]=(uint8_t)(row*8+col);
   count++;row+=rv_dr[d];col+=rv_dc[d];
  }
  if(row<0 || row>=8 || col<0 || col>=8 || b[row*8+col]!=(int)player)count=begin;
 }
 return count;
}
unsigned sq_reversi_moves(const int16_t b[64],unsigned player,uint8_t moves[64])
{
 unsigned count=0;
 for(unsigned p=0;p<64;p++)if(rv_flips(b,player,p,NULL)){
  if(moves)moves[count]=(uint8_t)p;
  count++;
 }
 return count;
}
bool sq_reversi_place(int16_t b[64],unsigned player,unsigned p)
{
 uint8_t flipped[64];unsigned count=rv_flips(b,player,p,flipped);
 if(!count)return false;
 b[p]=(int16_t)player;
 for(unsigned i=0;i<count;i++)b[flipped[i]]=(int16_t)player;
 return true;
}
static int rv_count(const int16_t b[64],unsigned player)
{int count=0;for(unsigned p=0;p<64;p++)count+=b[p]==(int)player;return count;}
static int rv_terminal_value(const int16_t b[64],unsigned player)
{int difference=rv_count(b,player)-rv_count(b,3-player);return difference+(difference>0?10000:difference<0?-10000:0);}
static int rv_evaluate(const int16_t b[64],unsigned player)
{
 static const unsigned corners[4]={0,7,56,63};
 int value=0,occupied=0;
 for(unsigned p=0;p<64;p++)if(b[p]){
  occupied++;int sign=b[p]==(int)player?1:-1;
  if(p/8==0 || p/8==7 || p%8==0 || p%8==7)value+=sign*8;
 }
 for(unsigned i=0;i<4;i++){
  unsigned p=corners[i];
  if(b[p])value+=b[p]==(int)player?500:-500;
  else{
   unsigned r=p/8?6:1,c=p%8?6:1,q=r*8+c;
   if(b[q])value+=b[q]==(int)player?-120:120;
  }
 }
 value+=12*((int)sq_reversi_moves(b,player,NULL)-(int)sq_reversi_moves(b,3-player,NULL));
 value+=(occupied>48?8:1)*(rv_count(b,player)-rv_count(b,3-player));
 return value;
}
typedef struct {unsigned nodes,budget;bool exhausted;} RvSearch;
/* At most depth5 moves and one forced pass before each move: <=11 search
 * activations. No game-sized recursive frame, allocation, or unbounded search. */
static int rv_search(const int16_t b[64],unsigned player,unsigned depth,int alpha,int beta,RvSearch *s)
{
 if(s->nodes>=s->budget){s->exhausted=true;return 0;}
 s->nodes++;uint8_t moves[64];unsigned count=sq_reversi_moves(b,player,moves);
 if(!count && !sq_reversi_moves(b,3-player,NULL))return rv_terminal_value(b,player);
 if(!depth)return rv_evaluate(b,player);
 if(!count)return -rv_search(b,3-player,depth,-beta,-alpha,s);
 int best=-20000;
 for(unsigned i=0;i<count;i++){
  int16_t child[64];memcpy(child,b,sizeof child);(void)sq_reversi_place(child,player,moves[i]);
  int value=-rv_search(child,3-player,depth-1,-beta,-alpha,s);
  if(s->exhausted)return 0;
  if(value>best)best=value;
  if(value>alpha)alpha=value;
  if(alpha>=beta)break;
 }
 return best;
}
int sq_reversi_pick(NgGame *g,unsigned level)
{
 static const unsigned depths[4]={0,1,3,5},budgets[4]={0,256,1500,6000};
 uint8_t moves[64];unsigned player=g->turn+1u,count=sq_reversi_moves(g->board,player,moves);
 g->data[2]=g->data[3]=0;
 if(!count)return -1;
 if(level>3)level=3;
 if(!level)return moves[ng_rand(g,count)];
 int chosen=moves[0];RvSearch search={0,budgets[level],false};
 for(unsigned depth=1;depth<=depths[level];depth++){
  int best=-20000,iteration=chosen;unsigned ties=0;
  for(unsigned i=0;i<count;i++){
   int16_t child[64];memcpy(child,g->board,sizeof child);(void)sq_reversi_place(child,player,moves[i]);
   int value=-rv_search(child,3-player,depth-1,-20000,20000,&search);
   if(search.exhausted)break;
   if(value>best){best=value;iteration=moves[i];ties=1;}
   else if(value==best && !ng_rand(g,++ties))iteration=moves[i];
  }
  if(search.exhausted)break;
  chosen=iteration;g->data[3]=(int32_t)depth;
 }
 g->data[2]=(int32_t)search.nodes;return chosen;
}
static void rv_initial(int16_t b[64],unsigned mode)
{
 memset(b,0,64*sizeof *b);int16_t black=(int16_t)(mode+1),white=(int16_t)(3-black);
 b[27]=b[36]=white;b[28]=b[35]=black;
}
static bool rv_apply(NgGame *g,unsigned position)
{
 unsigned player=g->turn+1u;
 if(!sq_reversi_place(g->board,player,position))return false;
 g->moves++;g->data[0]=(int32_t)position;g->data[1]=0;g->message[0]=0;
 unsigned other=3-player;
 if(sq_reversi_moves(g->board,other,NULL))g->turn=(uint8_t)(other-1);
 else if(sq_reversi_moves(g->board,player,NULL)){
  g->data[1]=(int32_t)other;ng_message(g,other==1?"You have no legal move: automatic pass":"CPU has no legal move: your turn again");
 }else{
  int human=rv_count(g->board,1),cpu=rv_count(g->board,2);
  g->status=human>cpu?NG_WON:human<cpu?NG_LOST:NG_DRAW;
  snprintf(g->message,sizeof g->message,"No moves remain. You %d - CPU %d",human,cpu);
 }
 g->score=(uint32_t)rv_count(g->board,1);g->cpu_pending=g->status==NG_PLAYING && g->turn==1;
 return true;
}
static void rv_init(NgGame *g)
{
 g->rows=g->cols=8;g->turn=g->mode;g->cpu_pending=g->turn==1;g->notes_mode=1;g->data[0]=-1;g->score=2;
 rv_initial(g->board,g->mode);uint8_t moves[64];(void)sq_reversi_moves(g->board,g->turn+1u,moves);g->cursor=moves[0];
 ng_message(g,g->mode?"CPU is black and moves first":"You are black and move first");
}
static bool rv_action(NgGame *g,int key)
{
 if(g->status!=NG_PLAYING)return false;
 if(g->cpu_pending){if(key!=NGK_CPU)return false;int p=sq_reversi_pick(g,g->difficulty);return p>=0 && rv_apply(g,(unsigned)p);}
 if(key==NGK_CPU)return false;
 if(ng_grid_nav(g,key))return true;
 if(key==NGK_AUX){g->notes_mode^=1;return true;}
 if(key==NGK_HINT){int p=sq_reversi_pick(g,g->difficulty<2?2:g->difficulty);if(p<0)return false;g->cursor=(uint8_t)p;g->assisted=1;ng_message(g,"Suggested move: bounded search, not perfect play");return true;}
 if(key==NGK_EXE || key=='5'){
  if(!rv_apply(g,g->cursor))ng_message(g,"Place on an empty square that flips a line");
  return true;
 }
 return false;
}
static bool rv_valid(const NgGame *g)
{
 if(g->id!=37 || g->difficulty>3 || g->mode>1 || g->rows!=8 || g->cols!=8 || g->cursor>=64 || g->turn>1 || g->phase || g->input[0] || g->history_count || g->scroll || g->puzzle_id)return false;
 if(g->data[0]<-1 || g->data[0]>=64 || g->data[1]<0 || g->data[1]>2 || g->data[2]<0 || g->data[2]>6000 || g->data[3]<0 || g->data[3]>5)return false;
 unsigned occupied=0;
 for(unsigned p=0;p<NG_CELLS;p++){
  if(g->fixed[p] || g->notes[p] || (p>=64 && g->board[p]))return false;
  if(p<64){if(g->board[p]<0 || g->board[p]>2)return false;occupied+=g->board[p]!=0;}
 }
 for(unsigned i=4;i<NG_DATA;i++)if(g->data[i])return false;
 if(occupied<4 || g->moves!=occupied-4 || !g->board[27] || !g->board[28] || !g->board[35] || !g->board[36] || g->score!=(uint32_t)rv_count(g->board,1))return false;
 if(!g->moves){int16_t initial[64];rv_initial(initial,g->mode);if(memcmp(initial,g->board,sizeof initial) || g->turn!=g->mode || g->data[0]!=-1 || g->data[1])return false;}
 else if(g->data[0]<0 || !g->board[g->data[0]])return false;
 bool human=sq_reversi_moves(g->board,1,NULL)!=0,cpu=sq_reversi_moves(g->board,2,NULL)!=0;
 unsigned expected=human || cpu?NG_PLAYING:g->score>(occupied-g->score)?NG_WON:g->score<(occupied-g->score)?NG_LOST:NG_DRAW;
 if(g->status!=expected || g->cpu_pending!=(expected==NG_PLAYING && g->turn==1))return false;
 if(expected==NG_PLAYING && !(g->turn?cpu:human))return false;
 if(g->moves){unsigned last=(unsigned)g->board[g->data[0]];
  if(expected!=NG_PLAYING){if(g->turn!=last-1 || g->data[1])return false;}
  else if(g->data[1]){if(g->data[1]!=(int)(3-last) || g->turn!=last-1 || (g->turn?human:cpu))return false;}
  else if(g->turn!=2-last)return false;
 }
 return true;
}
static void rv_disc(NgCanvas *c,int x,int y,int color)
{
 for(int dy=-5;dy<=5;dy++){int width=dy==-5 || dy==5?5:dy==-4 || dy==4?9:11;ng_rect(c,x-width/2,y+dy,width,1,NG_INK);}
 for(int dy=-4;dy<=4;dy++){int width=dy==-4 || dy==4?5:dy==-3 || dy==3?7:9;ng_rect(c,x-width/2,y+dy,width,1,color);}
}
static void rv_render(const NgGame *g,NgCanvas *c)
{
 char text[64];
 for(unsigned p=0;p<64;p++){
  int x=20+(int)(p%8)*17,y=43+(int)(p/8)*17;ng_rect(c,x,y,17,17,NG_PALE);ng_border(c,x,y,17,17,NG_LINE,1);
  if(g->board[p])rv_disc(c,x+8,y+8,g->board[p]==(int)g->mode+1?NG_BLACK:NG_WHITE);
  else if(g->notes_mode && rv_flips(g->board,g->turn+1u,p,NULL))ng_rect(c,x+7,y+7,3,3,NG_GREEN);
  if(g->cursor==p)ng_border(c,x,y,17,17,NG_BLUE,2);
 }
 ng_text(c,180,35,g->status==NG_WON?"YOU WIN":g->status==NG_LOST?"CPU WINS":g->status==NG_DRAW?"DRAW":g->cpu_pending?"CPU THINKING...":"YOUR MOVE",NG_BLUE,1);
 snprintf(text,sizeof text,"YOU (%s): %d",g->mode?"WHITE":"BLACK",rv_count(g->board,1));ng_text(c,180,62,text,NG_INK,1);
 snprintf(text,sizeof text,"CPU (%s): %d",g->mode?"BLACK":"WHITE",rv_count(g->board,2));ng_text(c,180,85,text,NG_INK,1);
 snprintf(text,sizeof text,"Legal moves: %u",sq_reversi_moves(g->board,g->turn+1u,NULL));ng_text(c,180,112,text,NG_MUTED,1);
 ng_text(c,180,139,"EXE: PLACE DISC",NG_INK,1);ng_small(c,180,159,"F4 toggles legal-move dots",NG_MUTED);
}

/* NET: N/E/S/W ports1/2/4/8. data0..35 retains the original tree solely
 * for immutable-shape validation and explicitly assisted one-tile reveal.
 * Completion never compares player orientation with this original. */
static const unsigned net_bit[4]={1,2,4,8},net_other[4]={4,8,1,2};
static int net_neighbour(unsigned p,unsigned n,unsigned d)
{
 if(d==0)return p>=n?(int)(p-n):-1;
 if(d==1)return p%n+1<n?(int)p+1:-1;
 if(d==2)return p+n<n*n?(int)(p+n):-1;
 return p%n?(int)p-1:-1;
}
static uint8_t net_rotate(unsigned mask){return (uint8_t)(((mask<<1)&15u)|(mask>>3));}
static unsigned net_reached(const uint8_t *b,unsigned n,uint8_t *seen)
{
 uint8_t local[36]={0},queue[36];unsigned used=1,head=0,start=(n/2)*n+n/2;local[start]=1;queue[0]=(uint8_t)start;
 while(head<used){unsigned p=queue[head++];for(unsigned d=0;d<4;d++)if(b[p]&net_bit[d]){
   int q=net_neighbour(p,n,d);if(q>=0 && !local[q] && (b[q]&net_other[d])){local[q]=1;queue[used++]=(uint8_t)q;}
  }}
 if(seen)memcpy(seen,local,n*n);
 return used;
}
static bool net_network(const uint8_t *b,unsigned n)
{
 unsigned ports=0;
 for(unsigned p=0;p<n*n;p++){
  if(!b[p] || b[p]>15)return false;
  for(unsigned d=0;d<4;d++)if(b[p]&net_bit[d]){int q=net_neighbour(p,n,d);if(q<0 || !(b[q]&net_other[d]))return false;ports++;}
 }
 return ports==2*(n*n-1) && net_reached(b,n,NULL)==n*n;
}
static bool net_copy(const NgGame *g,uint8_t b[36])
{
 if(g->rows<2 || g->rows>6 || g->rows!=g->cols)return false;
 for(unsigned p=0;p<g->rows*g->cols;p++){if(g->board[p]<1 || g->board[p]>15)return false;b[p]=(uint8_t)g->board[p];}
 return true;
}
bool sq_net_complete(const NgGame *g)
{uint8_t b[36];return net_copy(g,b) && net_network(b,g->rows);}
unsigned sq_net_connected(const NgGame *g)
{uint8_t b[36];return net_copy(g,b)?net_reached(b,g->rows,NULL):0;}
static uint32_t net_random(uint32_t *state)
{uint32_t x=*state;x^=x<<13;x^=x>>17;x^=x<<5;*state=x?x:UINT32_C(0x9e3779b9);return *state;}
static void net_make(uint32_t seed,unsigned n,uint8_t tree[36],uint8_t start[36],uint32_t *final_rng)
{
 uint8_t visited[36]={0};uint32_t rng=seed;memset(tree,0,36);visited[(n/2)*n+n/2]=1;
 for(unsigned edge=0;edge<n*n-1;edge++){
  unsigned choices=0,chosen=0,direction=0;
  for(unsigned p=0;p<n*n;p++)if(visited[p])for(unsigned d=0;d<4;d++){
   int q=net_neighbour(p,n,d);if(q>=0 && !visited[q] && net_random(&rng)%++choices==0){chosen=p;direction=d;}
  }
  unsigned q=(unsigned)net_neighbour(chosen,n,direction);tree[chosen]|=(uint8_t)net_bit[direction];tree[q]|=(uint8_t)net_other[direction];visited[q]=1;
 }
 for(unsigned p=0;p<n*n;p++){start[p]=tree[p];unsigned rotations=net_random(&rng)%4;while(rotations--)start[p]=net_rotate(start[p]);}
 if(net_network(start,n))for(unsigned p=0;p<n*n;p++)if(net_rotate(start[p])!=start[p]){start[p]=net_rotate(start[p]);break;}
 *final_rng=rng;
}
static void net_update(NgGame *g)
{
 g->score=sq_net_connected(g);
 if(sq_net_complete(g)){g->status=NG_WON;ng_message(g,"Every tile connected: one loop-free network!");}
}
static void net_init(NgGame *g)
{
 uint8_t tree[36],start[36];g->rows=g->cols=(uint8_t)(3+g->difficulty);g->cursor=(uint8_t)((g->rows/2)*g->rows+g->rows/2);g->puzzle_id=g->seed;g->data[81]=-1;
 net_make(g->seed,g->rows,tree,start,&g->rng);
 for(unsigned p=0;p<g->rows*g->cols;p++){g->data[p]=tree[p];g->board[p]=start[p];}
 net_update(g);ng_message(g,"Rotate tiles into one connected tree; no loops");
}
static bool net_action(NgGame *g,int key)
{
 if(g->status!=NG_PLAYING)return false;
 if(ng_grid_nav(g,key))return true;
 unsigned p=g->cursor;
 if(key==NGK_AUX){g->fixed[p]^=1;g->data[81]=-1;g->moves++;ng_message(g,g->fixed[p]?"Tile locked; F4 unlocks":"Tile unlocked");return true;}
 if(key==NGK_HINT){
  if(g->board[p]==g->data[p]){for(p=0;p<g->rows*g->cols;p++)if(g->board[p]!=g->data[p])break;}
  if(p>=g->rows*g->cols)return false;
  g->cursor=(uint8_t)p;g->board[p]=(int16_t)g->data[p];g->fixed[p]=1;g->data[81]=(int32_t)p;g->assisted=1;g->moves++;
  ng_message(g,"One original orientation revealed and locked");net_update(g);return true;
 }
 if(key==NGK_EXE || key=='5' || key==NGK_DEL){
  if(g->fixed[p]){ng_message(g,"Locked tile: F4 unlocks");return true;}
  unsigned rotations=key==NGK_DEL?3:1;int16_t before=g->board[p];while(rotations--)g->board[p]=(int16_t)net_rotate((unsigned)g->board[p]);
  if(g->board[p]==before)return false;
  g->moves++;g->data[81]=-1;g->message[0]=0;net_update(g);return true;
 }
 return false;
}
static bool net_valid(const NgGame *g)
{
 if(g->id!=38 || g->difficulty>3 || g->mode || g->rows!=3+g->difficulty || g->cols!=g->rows || g->cursor>=g->rows*g->cols || g->turn || g->cpu_pending || g->phase || g->notes_mode || g->input[0] || g->history_count || g->scroll || g->puzzle_id!=g->seed)return false;
 uint8_t tree[36],start[36];uint32_t rng;unsigned cells=g->rows*g->cols;net_make(g->seed,g->rows,tree,start,&rng);
 if(g->rng!=rng || g->data[81]<-1 || g->data[81]>=(int)cells)return false;
 if(g->data[81]>=0 && (!g->assisted || !g->fixed[g->data[81]] || g->board[g->data[81]]!=g->data[g->data[81]]))return false;
 for(unsigned p=0;p<NG_CELLS;p++){
  if(g->notes[p] || (p>=cells && (g->board[p] || g->fixed[p] || g->data[p])))return false;
  if(p<cells){
   if(g->data[p]!=tree[p] || g->board[p]<1 || g->board[p]>15)return false;
   unsigned rotation=tree[p];bool match=false;
   for(unsigned i=0;i<4;i++){match|=rotation==(unsigned)g->board[p];rotation=net_rotate(rotation);}
   if(!match)return false;
  }
 }
 for(unsigned i=82;i<NG_DATA;i++)if(g->data[i])return false;
 if(g->score!=sq_net_connected(g) || g->status!=(sq_net_complete(g)?NG_WON:NG_PLAYING))return false;
 return true;
}
static void net_render(const NgGame *g,NgCanvas *c)
{
 uint8_t b[36],seen[36];(void)net_copy(g,b);unsigned connected=net_reached(b,g->rows,seen),n=g->rows;int size=132/(int)n;
 for(unsigned p=0;p<n*n;p++){
  int x=24+(int)(p%n)*size,y=45+(int)(p/n)*size,cx=x+size/2,cy=y+size/2,color=seen[p]?NG_BLUE:NG_MUTED;
  ng_rect(c,x,y,size,size,seen[p]?NG_PALE:NG_WHITE);ng_border(c,x,y,size,size,NG_LINE,1);
  if(b[p]&1)ng_rect(c,cx-1,y+1,3,size/2,color);
  if(b[p]&2)ng_rect(c,cx,cy-1,size-size/2-1,3,color);
  if(b[p]&4)ng_rect(c,cx-1,cy,3,size-size/2-1,color);
  if(b[p]&8)ng_rect(c,x+1,cy-1,size/2,3,color);
  ng_rect(c,cx-2,cy-2,5,5,p==(n/2)*n+n/2?NG_RED:color);
  if(g->fixed[p])ng_rect(c,x+2,y+2,3,3,NG_INK);
  if(g->cursor==p)ng_border(c,x,y,size,size,NG_BLUE,2);
 }
 char text[48];snprintf(text,sizeof text,"CONNECTED %u / %u",connected,n*n);ng_text(c,181,39,text,NG_BLUE,1);
 ng_text(c,181,65,"ONE TREE, NO LOOPS",NG_INK,1);ng_small(c,181,87,"Red node is the source",NG_MUTED);
 ng_text(c,181,109,"EXE: TURN RIGHT",NG_INK,1);ng_text(c,181,132,"DEL: TURN LEFT",NG_INK,1);
 ng_small(c,181,155,"F4 LOCK / F3 REVEAL",NG_MUTED);
}
const NgModule ng_strategyquick_extra[2]={
 {37,"REVERSI","REVERSI",
 "Place a disc to bracket an enemy line.\nFlip every bracketed line in 8 directions.\nBlack moves first; choose YOU or CPU first.\nArrows select; EXE or 5 places a disc.\nIf you cannot move, you pass automatically.\nNeither player can move: most discs wins.\nEqual counts draw; empty cells stay empty.\nF4 toggles dots marking legal placements.\nEasy CPU chooses a random legal move.\nNormal searches 1 ply, Hard up to 3,\nMASTER up to 5, with fixed node budgets.\nThe bounded CPU is not perfect play.\nHINT suggests a move and marks assisted.\nUNDO restores your complete CPU round.",
 NGF_CPU|NGF_UNDO|NGF_HINT,"MOVES","PLACE",2,sq_strategy_mode,rv_init,rv_action,NULL,rv_valid,rv_render},
 {38,"NET","NET",
 "Rotate tiles to connect every square.\nEvery wire must meet its neighbour's wire.\nNo wires may leave the board; no wrapping.\nThe whole network must form one tree:\nall tiles connected, with no closed loops.\nArrows select. EXE/5 turns clockwise;\nDEL turns anticlockwise. F4 locks a tile.\nLocked tiles cannot be rotated until freed.\nF3 REVEAL sets one original orientation\nand locks it, marking assisted practice.\nEvery valid network wins, not just ours.\nE/N/H/M use 3/4/5/6 by 3/4/5/6 grids.\nSeeds construct a tree then rotate tiles.\nSolutions need not be unique; no rating claim.\nMoves count rotations, locks and reveals.",
 NGF_UNDO|NGF_HINT,"LOCK","ROTATE",1,NULL,net_init,net_action,NULL,net_valid,net_render}
};
