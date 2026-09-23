#include "boards.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

static bool side_valid(const NgGame *g){return g->rows>=1&&g->rows<=NB_SIDE&&g->rows==g->cols;}
/* Labels describe user rectangles. No label/witness identity is required. */
bool nb_shikaku_partial(const NgGame *g){
 if(!side_valid(g))return false;
 unsigned n=g->rows,cells=n*n;
 for(unsigned p=0;p<cells;p++)if(g->board[p]<0||g->board[p]>NB_CELLS||g->data[p]<0||g->data[p]>(int)cells)return false;
 for(int tag=1;tag<=NB_CELLS;tag++){
  unsigned minr=n,minc=n,maxr=0,maxc=0,count=0,clues=0,area=0;
  for(unsigned p=0;p<cells;p++)if(g->board[p]==tag){unsigned r=p/n,c=p%n;if(r<minr)minr=r;if(r>maxr)maxr=r;if(c<minc)minc=c;if(c>maxc)maxc=c;count++;if(g->data[p]){clues++;area=(unsigned)g->data[p];}}
  if(!count)continue;
  if(count!=(maxr-minr+1)*(maxc-minc+1)||clues!=1||area!=count)return false;
  for(unsigned r=minr;r<=maxr;r++)for(unsigned c=minc;c<=maxc;c++)if(g->board[r*n+c]!=tag)return false;
 }
 return true;
}
bool nb_shikaku_complete(const NgGame *g){
 if(!nb_shikaku_partial(g))return false;
 for(unsigned p=0;p<g->rows*g->cols;p++)if(!g->board[p])return false;
 return true;
}
unsigned nb_edge_count(unsigned n){return n<=NB_SIDE?2*n*(n+1):0;}
unsigned nb_edge_get(const NgGame *g,unsigned edge){
 if(edge>=NB_EDGES)return 3;
 return ((uint32_t)g->data[edge/16]>>((edge%16)*2))&3u;
}
bool nb_edge_set(NgGame *g,unsigned edge,unsigned value){
 if(edge>=nb_edge_count(g->rows)||value>2)return false;
 unsigned shift=(edge%16)*2;uint32_t word=((uint32_t)g->data[edge/16]&~(UINT32_C(3)<<shift))|((uint32_t)value<<shift);
 g->data[edge/16]=word<=INT32_MAX?(int32_t)word:-1-(int32_t)(UINT32_MAX-word);return true;
}
void nb_edge_vertices(unsigned n,unsigned edge,unsigned *a,unsigned *b){
 unsigned split=n*(n+1);
 if(edge<split){unsigned r=edge/n,c=edge%n;*a=r*(n+1)+c;*b=*a+1;}
 else{unsigned p=edge-split,r=p/(n+1),c=p%(n+1);*a=r*(n+1)+c;*b=*a+n+1;}
}
bool nb_slither_complete(const NgGame *g){
 if(!side_valid(g))return false;
 unsigned n=g->rows,edges=nb_edge_count(n),split=n*(n+1);uint8_t degree[81]={0},visited[81]={0},queue[81];unsigned first=81,used=0;
 for(unsigned p=0;p<n*n;p++){
  int clue=g->board[p];if(clue<-1||clue>3)return false;
  unsigned r=p/n,c=p%n;unsigned count=(nb_edge_get(g,r*n+c)==1)+(nb_edge_get(g,(r+1)*n+c)==1)+(nb_edge_get(g,split+r*(n+1)+c)==1)+(nb_edge_get(g,split+r*(n+1)+c+1)==1);
  if(clue>=0&&count!=(unsigned)clue)return false;
 }
 for(unsigned e=0;e<edges;e++){
  unsigned state=nb_edge_get(g,e);if(state>2)return false;
  if(state==1){unsigned a,b;nb_edge_vertices(n,e,&a,&b);degree[a]++;degree[b]++;first=a;}
 }
 for(unsigned v=0;v<(n+1)*(n+1);v++)if(degree[v]){if(degree[v]!=2)return false;used++;}
 if(first==81)return false;
 unsigned head=0,tail=0;queue[tail++]=(uint8_t)first;visited[first]=1;
 while(head<tail){unsigned v=queue[head++];for(unsigned e=0;e<edges;e++)if(nb_edge_get(g,e)==1){unsigned a,b;nb_edge_vertices(n,e,&a,&b);if(a==v||b==v){unsigned next=a==v?b:a;if(!visited[next]){visited[next]=1;queue[tail++]=(uint8_t)next;}}}}
 return tail==used;
}
unsigned nb_bank_count(unsigned id,unsigned difficulty,unsigned mode){return (id==31||id==32)&&difficulty<4&&!mode?30:0;}
static void boards_init(NgGame *g){
 unsigned index=g->difficulty*30u+ng_bank_pick(g,30);const NbPuzzle *p=g->id==31?&nb_shikaku_pack[index]:&nb_slitherlink_pack[index];
 g->puzzle_id=index;g->rows=g->cols=p->size;
 for(unsigned i=0;i<p->size*p->size;i++){
  if(g->id==31){g->data[i]=p->clues[i];g->fixed[i]=p->clues[i]!=0;}
  else{g->board[i]=p->clues[i]==255?-1:p->clues[i];g->fixed[i]=p->clues[i]!=255;}
 }
 if(g->id==31)g->data[64]=-1;
}
static bool rectangle(NgGame *g){
 unsigned n=g->rows,a=(unsigned)g->data[64],b=g->cursor,r0=a/n,r1=b/n,c0=a%n,c1=b%n;
 if(r0>r1){unsigned t=r0;r0=r1;r1=t;}if(c0>c1){unsigned t=c0;c0=c1;c1=t;}
 unsigned area=(r1-r0+1)*(c1-c0+1),count=0,clue=0,label=0;
 for(unsigned r=r0;r<=r1;r++)for(unsigned c=c0;c<=c1;c++){
  unsigned p=r*n+c;if(g->board[p]){ng_message(g,"Rectangle overlaps an existing region");return true;}
  if(g->data[p]){count++;clue=(unsigned)g->data[p];label=p+1;}
 }
 if(count!=1){ng_message(g,count?"Rectangle contains more than one clue":"Rectangle needs exactly one clue");return true;}
 if(area!=clue){ng_message(g,"Rectangle area must equal its clue");return true;}
 /* If an imported valid layout uses arbitrary labels, choose an unused label. */
 for(unsigned p=0;p<n*n;p++)if(g->board[p]==(int)label){label=0;break;}
 if(!label){for(unsigned candidate=1;candidate<=NB_CELLS;candidate++){bool used=false;for(unsigned p=0;p<n*n;p++)used|=g->board[p]==(int)candidate;if(!used){label=candidate;break;}}}
 if(!label){ng_message(g,"No rectangle label available");return true;}
 for(unsigned r=r0;r<=r1;r++)for(unsigned c=c0;c<=c1;c++)g->board[r*n+c]=(int16_t)label;
 g->phase=0;g->data[64]=-1;g->moves++;g->message[0]=0;
 if(nb_shikaku_complete(g)){g->status=NG_WON;ng_message(g,"Every rectangle satisfies its clue");}return true;
}
static bool shikaku_action(NgGame *g,int key){
 if(key==NGK_EXIT&&g->phase==1){g->phase=0;g->data[64]=-1;g->message[0]=0;return true;}
 if(g->status!=NG_PLAYING)return false;
 if(ng_grid_nav(g,key))return true;
 if(key==NGK_EXE){if(g->phase==0){g->phase=1;g->data[64]=g->cursor;g->message[0]=0;return true;}return rectangle(g);}
 if(key==NGK_DEL){int tag=g->board[g->cursor];if(!tag){ng_message(g,"No rectangle at this cell");return true;}for(unsigned p=0;p<g->rows*g->cols;p++)if(g->board[p]==tag)g->board[p]=0;g->phase=0;g->data[64]=-1;g->moves++;g->message[0]=0;return true;}
 return false;
}
static bool edge_nav(NgGame *g,int key){
 unsigned n=g->rows,split=n*(n+1),index=g->cursor;bool vertical=index>=split;unsigned p=vertical?index-split:index,rows=vertical?n:n+1,cols=vertical?n+1:n,r=p/cols,c=p%cols;
 if(key==NGK_AUX){if(vertical){if(c==n)c=n-1;g->cursor=(uint8_t)(r*n+c);}else{if(r==n)r=n-1;g->cursor=(uint8_t)(split+r*(n+1)+c);}return true;}
 if(key==NGK_UP)r=(r+rows-1)%rows;else if(key==NGK_DOWN)r=(r+1)%rows;else if(key==NGK_LEFT)c=(c+cols-1)%cols;else if(key==NGK_RIGHT)c=(c+1)%cols;else return false;
 g->cursor=(uint8_t)((vertical?split:0)+r*cols+c);return true;
}
static bool slither_action(NgGame *g,int key){
 if(g->status!=NG_PLAYING)return false;
 if(edge_nav(g,key))return true;
 if(key!=NGK_EXE&&key!=NGK_DEL)return false;
 unsigned old=nb_edge_get(g,g->cursor),value=key==NGK_EXE?(old==1?0:1):(old==2?0:2);nb_edge_set(g,g->cursor,value);g->moves++;g->message[0]=0;
 if(nb_slither_complete(g)){g->status=NG_WON;ng_message(g,"Clues matched: one complete loop");}return true;
}
static bool boards_valid(const NgGame *g){
 if((g->id!=31&&g->id!=32)||g->difficulty>3||g->mode||g->turn||g->cpu_pending||g->history_count||g->scroll||g->notes_mode||g->input[0]||g->score)return false;
 if(g->puzzle_id>=NB_PACK_COUNT||g->puzzle_id/30!=g->difficulty)return false;
 const NbPuzzle *p=g->id==31?&nb_shikaku_pack[g->puzzle_id]:&nb_slitherlink_pack[g->puzzle_id];unsigned n=p->size,cells=n*n;
 if(n!=(g->difficulty==0?5:g->difficulty==1?6:8)||g->rows!=n||g->cols!=n||g->cursor>=(g->id==31?cells:nb_edge_count(n)))return false;
 for(unsigned i=0;i<NG_CELLS;i++){
  if(g->notes[i])return false;
  if(i>=cells){if(g->board[i]||g->fixed[i])return false;continue;}
  if(g->id==31){if(g->data[i]!=p->clues[i]||g->fixed[i]!=(p->clues[i]!=0))return false;}
  else if(g->board[i]!=(p->clues[i]==255?-1:p->clues[i])||g->fixed[i]!=(p->clues[i]!=255))return false;
 }
 bool complete;
 if(g->id==31){
  if(g->phase>1||g->data[64]<(g->phase?0:-1)||g->data[64]>=(int)cells||(!g->phase&&g->data[64]!=-1))return false;
  for(unsigned i=cells;i<64;i++)if(g->data[i])return false;
  for(unsigned i=65;i<NG_DATA;i++)if(g->data[i])return false;
  if(!nb_shikaku_partial(g))return false;
  complete=nb_shikaku_complete(g);
  if(complete&&g->phase)return false;
 }else{
  if(g->phase)return false;
  unsigned edges=nb_edge_count(n),words=(edges+15)/16;
  for(unsigned e=0;e<words*16;e++)if(nb_edge_get(g,e)>(e<edges?2u:0u))return false;
  for(unsigned i=words;i<NG_DATA;i++)if(g->data[i])return false;
  complete=nb_slither_complete(g);
 }
 return g->status==(complete?NG_WON:NG_PLAYING);
}
static NgGridLayout board_layout(unsigned n){int size=150/(int)n;return (NgGridLayout){14+(192-(int)n*size)/2,30+(150-(int)n*size)/2,size};}
static void shikaku_render(const NgGame *g,NgCanvas *c){
 unsigned n=g->rows;NgGridLayout l=board_layout(n);unsigned a=g->phase?(unsigned)g->data[64]:g->cursor,b=g->cursor;unsigned r0=a/n,r1=b/n,c0=a%n,c1=b%n;
 if(r0>r1){unsigned t=r0;r0=r1;r1=t;}if(c0>c1){unsigned t=c0;c0=c1;c1=t;}
 for(unsigned p=0;p<n*n;p++){
  int x=l.x+(int)(p%n)*l.size,y=l.y+(int)(p/n)*l.size,tag=g->board[p];bool pending=g->phase&&p/n>=r0&&p/n<=r1&&p%n>=c0&&p%n<=c1;
  ng_rect(c,x,y,l.size,l.size,pending?NG_PALE:tag?(tag%2?NG_WHITE:NG_PAPER):NG_WHITE);ng_border(c,x,y,l.size,l.size,NG_LINE,1);
  if(tag){if(p<n||g->board[p-n]!=tag)ng_rect(c,x,y,l.size,2,NG_INK);if(p+n>=n*n||g->board[p+n]!=tag)ng_rect(c,x,y+l.size-2,l.size,2,NG_INK);if(!((int)p%(int)n)||g->board[p-1]!=tag)ng_rect(c,x,y,2,l.size,NG_INK);if(p%n==n-1||g->board[p+1]!=tag)ng_rect(c,x+l.size-2,y,2,l.size,NG_INK);}
  if(g->data[p]){char text[16];snprintf(text,sizeof text,"%ld",(long)g->data[p]);if(ng_text_width(text,1)<l.size-3)ng_center(c,x,y+(l.size-11)/2,l.size,text,NG_INK,1);else ng_small(c,x+(l.size-ng_small_width(text))/2,y+(l.size-7)/2,text,NG_INK);}
 }
 if(g->phase)ng_border(c,l.x+(int)c0*l.size,l.y+(int)r0*l.size,(int)(c1-c0+1)*l.size,(int)(r1-r0+1)*l.size,NG_BLUE,2);
 ng_border(c,l.x+(int)(g->cursor%n)*l.size+1,l.y+(int)(g->cursor/n)*l.size+1,l.size-2,l.size-2,NG_BLUE,2);
 int x=220;char text[40];ng_text(c,x,36,g->phase?"SECOND CORNER":"FIRST CORNER",NG_BLUE,1);
 snprintf(text,sizeof text,"AREA %u",(r1-r0+1)*(c1-c0+1));ng_text(c,x,61,text,NG_INK,1);snprintf(text,sizeof text,"MOVES %lu",(unsigned long)g->moves);ng_text(c,x,82,text,NG_MUTED,1);
 ng_text(c,x,115,"EXE: SELECT",NG_INK,1);ng_text(c,x,134,"DEL: REMOVE",NG_MUTED,1);if(g->phase)ng_text(c,x,153,"EXIT: CANCEL",NG_BLUE,1);
}
static void slither_render(const NgGame *g,NgCanvas *c){
 unsigned n=g->rows,split=n*(n+1);NgGridLayout l=board_layout(n);
 for(unsigned p=0;p<n*n;p++)if(g->board[p]>=0){char text[16];snprintf(text,sizeof text,"%d",g->board[p]);int x=l.x+(int)(p%n)*l.size,y=l.y+(int)(p/n)*l.size;ng_center(c,x,y+(l.size-11)/2,l.size,text,NG_INK,1);}
 for(unsigned e=0;e<nb_edge_count(n);e++){
  unsigned a,b;nb_edge_vertices(n,e,&a,&b);int x=l.x+(int)(a%(n+1))*l.size,y=l.y+(int)(a/(n+1))*l.size;bool horizontal=e<split;unsigned state=nb_edge_get(g,e);
  if(state==1)ng_rect(c,x-(horizontal?0:1),y-(horizontal?1:0),horizontal?l.size:3,horizontal?3:l.size,NG_INK);
  int mx=x+(horizontal?l.size/2:0),my=y+(horizontal?0:l.size/2);
  if(state==2){ng_line(c,mx-2,my-2,mx+2,my+2,NG_RED);ng_line(c,mx+2,my-2,mx-2,my+2,NG_RED);}
  if(g->cursor==e)ng_border(c,mx-(horizontal?6:3),my-(horizontal?3:6),horizontal?13:7,horizontal?7:13,NG_BLUE,1);
 }
 for(unsigned r=0;r<=n;r++)for(unsigned col=0;col<=n;col++)ng_rect(c,l.x+(int)col*l.size-1,l.y+(int)r*l.size-1,3,3,NG_INK);
 int x=220;char text[40];ng_text(c,x,36,g->cursor<split?"HORIZONTAL":"VERTICAL",NG_BLUE,1);snprintf(text,sizeof text,"EDGE %u / %u",g->cursor+1,nb_edge_count(n));ng_text(c,x,60,text,NG_MUTED,1);
 ng_text(c,x,96,"EXE: LINE",NG_INK,1);ng_text(c,x,116,"DEL: EXCLUDE",NG_MUTED,1);ng_text(c,x,136,"F4: H / V",NG_BLUE,1);snprintf(text,sizeof text,"MOVES %lu",(unsigned long)g->moves);ng_text(c,x,162,text,NG_MUTED,1);
}
const NgModule ng_boards[2]={
 {31,"SHIKAKU","SHIKAKU","Divide all cells into rectangles.\nEach rectangle contains exactly one clue.\nIts area must equal that clue number.\nRectangles cannot overlap or leave gaps.\nArrows move the selected cell.\nEXE: first corner, then opposite corner.\nEXE again places the rectangle.\nEXIT cancels an unfinished selection.\nDEL removes the selected rectangle.\nUNDO restores a created/deleted rectangle.\nEASY 5x5, NORMAL 6x6, HARD 8x8.\nMASTER 8x8: interacting choices.\n30 original unique puzzles per level.",NGF_UNDO,NULL,"SELECT",1,NULL,boards_init,shikaku_action,NULL,boards_valid,shikaku_render},
 {32,"SLITHERLINK","SLITHERLINK","Connect neighbouring dots in ONE loop.\nA clue counts the lines around its cell.\nBlank cells have no number constraint.\n0 and blank are different.\nEvery used dot must have two lines.\nNo branches, crossings, or separate loops.\nArrows move the selected edge.\nF4 switches horizontal/vertical edges.\nEXE toggles a line; DEL toggles an X.\nX means excluded and is never a line.\nUNDO restores the last edge edit.\nEASY 5x5, NORMAL 6x6, HARD 8x8.\nMASTER 8x8: interacting choices.\n30 original unique puzzles per level.",NGF_UNDO,"H / V","LINE",1,NULL,boards_init,slither_action,NULL,boards_valid,slither_render}
};
