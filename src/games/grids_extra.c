#include "grids_extra.h"
#include "ui.h"
#include <stdio.h>
#include <string.h>

unsigned grids_extra_bank_count(unsigned id,unsigned difficulty,unsigned mode)
{return !mode&&((id==35&&difficulty<5)||(id==36&&difficulty<4))?GRIDS_EXTRA_BANK:0;}
unsigned grids_extra_bank_id(unsigned id,unsigned difficulty,unsigned ordinal)
{return ordinal<grids_extra_bank_count(id,difficulty,0)?difficulty*GRIDS_EXTRA_BANK+ordinal:UINT32_MAX;}
static bool record_valid(const NgGame *g)
{return grids_extra_bank_count(g->id,g->difficulty,g->mode)&&g->puzzle_id/GRIDS_EXTRA_BANK==g->difficulty;}
static const GridsHashiPuzzle *hashi(const NgGame *g)
{return g->id==35&&record_valid(g)?&grids_hashi_pack[g->puzzle_id]:NULL;}
static const GridsNonoPuzzle *nono(const NgGame *g)
{return g->id==36&&record_valid(g)?&grids_nono_pack[g->puzzle_id]:NULL;}
static int island(const GridsHashiPuzzle *p,unsigned cell)
{for(unsigned i=0;i<p->count;i++)if(p->pos[i]==cell)return (int)i;return -1;}
/* Directions are UP, RIGHT, DOWN, LEFT. Only nearest visible islands join. */
static int neighbor(const GridsHashiPuzzle *p,unsigned cell,unsigned direction)
{
 int best=-1;unsigned distance=100,n=p->n;
 for(unsigned i=0;i<p->count;i++){
  unsigned other=p->pos[i],gap=100;
  if(direction==0&&other%n==cell%n&&other<cell)gap=cell-other;
  if(direction==1&&other/n==cell/n&&other>cell)gap=other-cell;
  if(direction==2&&other%n==cell%n&&other>cell)gap=other-cell;
  if(direction==3&&other/n==cell/n&&other<cell)gap=cell-other;
  if(gap<distance){distance=gap;best=(int)other;}
 }
 return best;
}
static int bridges(const NgGame *g,unsigned a,unsigned b)
{
 if(a>b){unsigned t=a;a=b;b=t;}
 return a/g->cols==b/g->cols?g->board[a]%3:g->board[a]/3;
}
static unsigned incident(const NgGame *g,const GridsHashiPuzzle *p,unsigned cell)
{
 unsigned total=0;
 for(unsigned d=0;d<4;d++){int other=neighbor(p,cell,d);if(other>=0)total+=(unsigned)bridges(g,cell,(unsigned)other);}
 return total;
}
static bool hashi_schema(const GridsHashiPuzzle *p)
{
 if(!p||p->n<3||p->n>9||p->count<2||p->count>25)return false;
 for(unsigned i=0;i<25;i++){
  if(i>=p->count){if(p->pos[i]||p->clue[i])return false;continue;}
  if(p->pos[i]>=p->n*p->n||!p->clue[i]||p->clue[i]>8||(i&&p->pos[i]<=p->pos[i-1]))return false;
 }
 return true;
}
bool grids_hashi_rules(const NgGame *g,const GridsHashiPuzzle *p)
{
 if(!hashi_schema(p)||g->rows!=p->n||g->cols!=p->n)return false;
 unsigned n=p->n;
 for(unsigned cell=0;cell<NG_CELLS;cell++){
  int i=island(p,cell),v=g->board[cell];
  if(i<0){if(v)return false;continue;}
  if(v<0||v>8||(v%3&&neighbor(p,cell,1)<0)||(v/3&&neighbor(p,cell,2)<0))return false;
  if(incident(g,p,cell)!=p->clue[i])return false;
 }
 /* A horizontal and vertical segment may meet only at a shared island. */
 for(unsigned i=0;i<p->count;i++){
  unsigned a=p->pos[i];int b=neighbor(p,a,1);if(b<0||!bridges(g,a,(unsigned)b))continue;
  for(unsigned j=0;j<p->count;j++){
   unsigned c=p->pos[j];int d=neighbor(p,c,2);if(d<0||!bridges(g,c,(unsigned)d))continue;
   if(a%n<c%n&&c%n<(unsigned)b%n&&c/n<a/n&&a/n<(unsigned)d/n)return false;
  }
 }
 uint8_t queue[25],seen[25]={0};unsigned head=0,tail=1;queue[0]=0;seen[0]=1;
 while(head<tail){
  unsigned a=p->pos[queue[head++]];
  for(unsigned d=0;d<4;d++){
   int b=neighbor(p,a,d);if(b<0||!bridges(g,a,(unsigned)b))continue;
   int i=island(p,(unsigned)b);if(i>=0&&!seen[i]){seen[i]=1;queue[tail++]=(uint8_t)i;}
  }
 }
 return tail==p->count;
}
static bool run_schema(uint32_t clue,unsigned n)
{
 unsigned total=0,count=0;
 while(clue){unsigned v=clue&15u;if(!v||v>n||count>=5)return false;total+=v;count++;clue>>=4;}
 return total+(count?count-1:0)<=n;
}
static uint32_t board_run(const NgGame *g,unsigned start,unsigned step)
{
 uint32_t result=0;unsigned length=0,shift=0;
 for(unsigned k=0;k<=g->cols;k++){
  if(k<g->cols&&g->board[start+k*step]==1)length++;
  else if(length){result|=length<<shift;shift+=4;length=0;}
 }
 return result;
}
bool grids_nono_rules(const NgGame *g,const GridsNonoPuzzle *p)
{
 if(!p||p->n<3||p->n>9||g->rows!=p->n||g->cols!=p->n)return false;
 unsigned n=p->n;
 for(unsigned i=0;i<NG_CELLS;i++)if(i<n*n?(g->board[i]<0||g->board[i]>1):g->board[i]!=0)return false;
 for(unsigned i=0;i<18;i++)if(i<2*n?!run_schema(p->clue[i],n):p->clue[i]!=0)return false;
 for(unsigned i=0;i<n;i++)if(board_run(g,i*n,1)!=p->clue[i]||board_run(g,i,n)!=p->clue[n+i])return false;
 return true;
}
bool grids_extra_complete(const NgGame *g)
{return g->id==35?grids_hashi_rules(g,hashi(g)):g->id==36?grids_nono_rules(g,nono(g)):false;}
bool grids_extra_witness(unsigned id,unsigned puzzle_id,int16_t out[NG_CELLS])
{
 memset(out,0,NG_CELLS*sizeof(*out));
 if(id==35&&puzzle_id<150){const GridsHashiPuzzle *p=&grids_hashi_pack[puzzle_id];for(unsigned i=0;i<p->count;i++)out[p->pos[i]]=p->solution[i];return true;}
 if(id==36&&puzzle_id<120){const GridsNonoPuzzle *p=&grids_nono_pack[puzzle_id];for(unsigned r=0;r<p->n;r++)for(unsigned c=0;c<p->n;c++)out[r*p->n+c]=(int16_t)((p->solution[r]>>c)&1);return true;}
 return false;
}
static void init(NgGame *g)
{
 unsigned count=grids_extra_bank_count(g->id,g->difficulty,g->mode);if(!count)return;
 g->puzzle_id=grids_extra_bank_id(g->id,g->difficulty,ng_bank_pick(g,count));
 if(g->id==35){const GridsHashiPuzzle *p=hashi(g);g->rows=g->cols=p->n;g->cursor=p->pos[0];g->data[0]=1;ng_message(g,"2/4/6/8: cycle bridge. F4 CHECK.");}
 else {const GridsNonoPuzzle *p=nono(g);g->rows=g->cols=p->n;for(unsigned i=0;i<(unsigned)p->n*p->n;i++)g->board[i]=-1;ng_message(g,"1 fill, 0 cross. EXE cycles. F4 CHECK.");}
}
static bool check(NgGame *g)
{
 if(grids_extra_complete(g)){g->status=NG_WON;g->score=g->moves;ng_message(g,"Complete: every visible rule is satisfied.");}
 else ng_message(g,g->id==35?"Check island totals, crossings and connection.":"Mark every cell; match all row/column clues.");
 return true;
}
static bool set_bridge(NgGame *g,const GridsHashiPuzzle *p,int value)
{
 unsigned a=g->cursor;int end=neighbor(p,a,(unsigned)g->data[0]);
 if(end<0){ng_message(g,"No visible island in this direction.");return true;}
 unsigned b=(unsigned)end;if(a>b){unsigned t=a;a=b;b=t;}
 int factor=a/g->cols==b/g->cols?1:3,old=(g->board[a]/factor)%3;
 if(value<0)value=(old+1)%3;
 if(value==old)return false;
 g->board[a]=(int16_t)(g->board[a]+(value-old)*factor);g->moves++;g->message[0]=0;return true;
}
static bool action(NgGame *g,int key)
{
 if(!record_valid(g)||g->status!=NG_PLAYING)return false;
 if(key==NGK_AUX)return check(g);
 if(g->id==35){
  const GridsHashiPuzzle *p=hashi(g);int direction=-1;
  if(key==NGK_UP)direction=0;
  if(key==NGK_RIGHT)direction=1;
  if(key==NGK_DOWN)direction=2;
  if(key==NGK_LEFT)direction=3;
  if(direction>=0){int to=neighbor(p,g->cursor,(unsigned)direction);if(to>=0)g->cursor=(uint8_t)to;return true;}
  if(key=='8')direction=0;
  if(key=='6')direction=1;
  if(key=='2')direction=2;
  if(key=='4')direction=3;
  if(direction>=0){g->data[0]=direction;return set_bridge(g,p,-1);}
  if(key==NGK_EXE)return set_bridge(g,p,-1);
  if(key==NGK_DEL||key=='0')return set_bridge(g,p,0);
  if(key==NGK_HINT){
   int other=neighbor(p,g->cursor,(unsigned)g->data[0]);if(other<0){ng_message(g,"Select a direction with a visible island.");return true;}
   unsigned a=g->cursor,b=(unsigned)other;if(a>b){unsigned t=a;a=b;b=t;}
   int i=island(p,a),v=p->solution[i];v=a/g->cols==b/g->cols?v%3:v/3;
   (void)set_bridge(g,p,v);g->assisted=1;ng_message(g,"REVEAL: one bridge shown. ASSISTED.");return true;
  }
  return false;
 }
 if(ng_grid_nav(g,key))return true;
 int value=-2;
 if(key=='0'||key=='1')value=key-'0';
 if(key==NGK_DEL)value=-1;
 if(key==NGK_EXE)value=g->board[g->cursor]==-1?1:g->board[g->cursor]==1?0:-1;
 if(key==NGK_HINT){const GridsNonoPuzzle *p=nono(g);value=(p->solution[g->cursor/g->cols]>>(g->cursor%g->cols))&1;g->assisted=1;}
 if(value==-2)return false;
 bool changed=g->board[g->cursor]!=value;
 if(changed){g->board[g->cursor]=(int16_t)value;g->moves++;g->message[0]=0;}
 if(key==NGK_HINT){ng_message(g,"REVEAL: one cell shown. ASSISTED.");return true;}
 return changed;
}
static bool valid(const NgGame *g)
{
 if(!record_valid(g)||g->mode||g->pack_revision==1||g->rows!=g->cols||g->cursor>=(unsigned)g->rows*g->cols)return false;
 if(g->phase||g->turn||g->cpu_pending||g->history_count||g->scroll||g->reserved||g->notes_mode)return false;
 if(g->status!=NG_PLAYING&&g->status!=NG_WON)return false;
 if(g->score!=(g->status==NG_WON?g->moves:0))return false;
 for(unsigned i=0;i<NG_INPUT;i++)if(g->input[i])return false;
 for(unsigned i=0;i<NG_HISTORY;i++)for(unsigned j=0;j<sizeof(g->history[0]);j++)if(g->history[i][j])return false;
 for(unsigned i=0;i<NG_DATA;i++)if((g->id==35&&i==0)?(g->data[i]<0||g->data[i]>3):g->data[i]!=0)return false;
 const GridsHashiPuzzle *hp=hashi(g);const GridsNonoPuzzle *np=nono(g);
 unsigned n=hp?hp->n:np->n;
 if(g->rows!=n)return false;
 if(hp&&(!hashi_schema(hp)||island(hp,g->cursor)<0))return false;
 for(unsigned i=0;i<NG_CELLS;i++){
  if(g->fixed[i]||g->notes[i])return false;
  if(i>=n*n){if(g->board[i])return false;continue;}
  int v=g->board[i];
  if(hp){if(island(hp,i)<0){if(v)return false;}else if(v<0||v>8||(v%3&&neighbor(hp,i,1)<0)||(v/3&&neighbor(hp,i,2)<0))return false;}
  else if(v< -1||v>1)return false;
 }
 return g->status!=NG_WON||grids_extra_complete(g);
}
static void render_hashi(const NgGame *g,NgCanvas *c,const GridsHashiPuzzle *p)
{
 int pitch=128/(p->n-1),ox=48,oy=40;char text[48];
 for(unsigned i=0;i<p->count;i++)for(unsigned d=1;d<=2;d++){
  unsigned a=p->pos[i];int b=neighbor(p,a,d);if(b<0)continue;int count=bridges(g,a,(unsigned)b);if(!count)continue;
  int x=ox+(int)(a%p->n)*pitch,y=oy+(int)(a/p->n)*pitch,xx=ox+(b%p->n)*pitch,yy=oy+(b/p->n)*pitch;
  if(count==1)ng_line(c,x,y,xx,yy,NG_INK);
  else if(d==1){ng_line(c,x,y-2,xx,y-2,NG_INK);ng_line(c,x,y+2,xx,y+2,NG_INK);}
  else {ng_line(c,x-2,y,x-2,yy,NG_INK);ng_line(c,x+2,y,x+2,yy,NG_INK);}
 }
 for(unsigned i=0;i<p->count;i++){
  unsigned cell=p->pos[i];int x=ox+(int)(cell%p->n)*pitch,y=oy+(int)(cell/p->n)*pitch;
  unsigned total=incident(g,p,cell);int color=total>p->clue[i]?NG_RED:total==p->clue[i]?NG_GREEN:NG_INK;
  ng_rect(c,x-7,y-7,15,15,NG_WHITE);ng_border(c,x-7,y-7,15,15,cell==g->cursor?NG_BLUE:NG_LINE,cell==g->cursor?2:1);
  snprintf(text,sizeof text,"%u",p->clue[i]);ng_center(c,x-6,y-5,13,text,color,1);
 }
 static const char *directions[]={"UP (8)","RIGHT (6)","DOWN (2)","LEFT (4)"};
 ng_text(c,235,66,"BRIDGE DIRECTION",NG_INK,1);ng_text(c,235,83,directions[g->data[0]],NG_BLUE,1);
 snprintf(text,sizeof text,"TOTAL %u / %u",incident(g,p,g->cursor),p->clue[island(p,g->cursor)]);ng_text(c,235,104,text,NG_INK,1);
 ng_small(c,235,128,"2/4/6/8 CYCLE 0-1-2",NG_MUTED);ng_text(c,235,143,"EXE: CYCLE",NG_BLUE,1);ng_text(c,235,160,"F4: CHECK",NG_MUTED,1);ng_small(c,235,178,"DEL: CLEAR BRIDGE",NG_MUTED);
}
static unsigned unpack(uint32_t clue,unsigned values[5])
{unsigned count=0;while(clue&&count<5){values[count++]=clue&15u;clue>>=4;}return count;}
static void render_nono(const NgGame *g,NgCanvas *c,const GridsNonoPuzzle *p)
{
 unsigned n=p->n;int pitch=n<=5?20:n==6?18:n==7?16:13,ox=77,oy=67;char text[24];
 for(unsigned i=0;i<n;i++){
  unsigned v[5],count=unpack(p->clue[i],v);if(!count){v[0]=0;count=1;}
  for(unsigned k=0;k<count;k++){snprintf(text,sizeof text,"%u",v[k]);ng_small(c,ox-8-(int)(count-k-1)*8,oy+(int)i*pitch+(pitch-7)/2,text,NG_GREEN);}
  count=unpack(p->clue[n+i],v);if(!count){v[0]=0;count=1;}
  for(unsigned k=0;k<count;k++){snprintf(text,sizeof text,"%u",v[k]);ng_small(c,ox+(int)i*pitch+(pitch-5)/2,oy-8-(int)(count-k-1)*8,text,NG_GREEN);}
 }
 for(unsigned i=0;i<n*n;i++){
  int x=ox+(int)(i%n)*pitch,y=oy+(int)(i/n)*pitch,v=g->board[i];
  ng_rect(c,x,y,pitch,pitch,v==1?NG_INK:NG_WHITE);ng_border(c,x,y,pitch,pitch,NG_LINE,1);
  if(v==0){ng_line(c,x+3,y+3,x+pitch-4,y+pitch-4,NG_MUTED);ng_line(c,x+3,y+pitch-4,x+pitch-4,y+3,NG_MUTED);}
  if(i==g->cursor)ng_border(c,x,y,pitch,pitch,NG_BLUE,2);
 }
 ng_text(c,235,68,"1: FILL",NG_INK,1);ng_text(c,235,85,"0: CROSS",NG_MUTED,1);ng_text(c,235,107,"EXE: CYCLE",NG_BLUE,1);
 ng_text(c,235,124,"DEL: UNKNOWN",NG_MUTED,1);ng_text(c,235,147,"F4: CHECK",NG_MUTED,1);ng_small(c,235,169,"RUNS HAVE AN EMPTY GAP",NG_GREEN);
}
static void render(const NgGame *g,NgCanvas *c)
{
 if(!valid(g))return;
 char text[40];
 snprintf(text,sizeof text,"PUZZLE %u / %u",(unsigned)(g->puzzle_id%GRIDS_EXTRA_BANK+1),GRIDS_EXTRA_BANK);ng_text(c,235,34,text,NG_MUTED,1);
 snprintf(text,sizeof text,"CELL %u,%u",g->cursor/g->cols+1,g->cursor%g->cols+1);ng_text(c,235,50,text,NG_INK,1);
 if(g->id==35)render_hashi(g,c,hashi(g));else render_nono(g,c,nono(g));
}
static const char *mode_name(unsigned mode){(void)mode;return "CLASSIC";}
const NgModule ng_grids_extra[2]={
 {35,"HASHI","HASHI","Join visible islands with straight bridges.\nOnly horizontal/vertical bridges are allowed.\nEach pair has zero, one or two bridges.\nIsland numbers give their total bridge count.\nBridges cannot cross or pass through islands.\nAll islands must form one connected network.\nArrows select a visible neighboring island.\n2/4/6/8 cycle down/left/right/up bridges.\nEXE cycles last direction; DEL clears it.\nF4 checks. F3 REVEAL marks ASSISTED.",NGF_UNDO|NGF_HINT,"CHECK","CYCLE",1,mode_name,init,action,NULL,valid,render},
 {36,"NONOGRAM","NONOGRAM","Fill cells to match all row/column run clues.\nRuns appear in order, with an empty gap.\nA zero clue means the whole line is empty.\nMark every cell filled or empty to finish.\nArrows move. 1 fills; 0 marks an empty cross.\nEXE cycles unknown, filled, empty. DEL clears.\nF4 checks every visible run clue.\nF3 REVEAL shows one cell, marking ASSISTED.",NGF_UNDO|NGF_HINT,"CHECK","MARK",1,mode_name,init,action,NULL,valid,render}
};
