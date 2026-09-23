#include "ng.h"
#include "ui.h"
#include "grids_internal.h"
#include <stdio.h>
#include <string.h>

const GridsPuzzle *grids_puzzle(const NgGame *g)
{
 if(g->id<11||g->id>20||g->difficulty>4)return NULL;
 const GridsPuzzle *p=grids_record(g->puzzle_id);
 return p&&p->id==g->id&&p->difficulty==g->difficulty?p:NULL;
}
static unsigned size(const NgGame *g){return (unsigned)g->rows*g->cols;}
static bool toggle_game(const NgGame *g){return g->id==16||g->id==20;}
static bool two_digits(const NgGame *g){return g->id==18||g->id==19;}
static int blank(const NgGame *g){return g->id==17?-1:0;}
static int max_number(const NgGame *g)
{
 if(g->id==13)return 9;
 if(g->id==17||toggle_game(g))return 1;
 if(two_digits(g))return (int)size(g);
 return g->cols;
}
static bool editable(const NgGame *g,unsigned i)
{
 const GridsPuzzle *p=grids_puzzle(g);
 return p&&i<size(g)&&!g->fixed[i]&&(g->id!=13||p->cells[i]==255);
}
static void init(NgGame *g)
{
 unsigned count=grids_bank_count(g->id,g->difficulty,g->mode);if(!count)return;
 g->puzzle_id=grids_bank_id(g->id,g->difficulty,ng_bank_pick(g,count));
 const GridsPuzzle *p=grids_puzzle(g);
 if(!p)return;
 g->rows=p->n;g->cols=p->n;
 for(unsigned i=0;i<size(g);i++){
  if(toggle_game(g)){g->board[i]=g->id==20?1:0;continue;}
  if(g->id==13){g->board[i]=0;g->fixed[i]=p->cells[i]!=255;continue;}
  int v=p->cells[i];
  if(g->id==17&&v==255)v=-1;
  if(g->id==19&&g->mode==1)v=0;
  g->board[i]=(int16_t)v;g->fixed[i]=v!=blank(g);
 }
 while((unsigned)g->cursor+1u<size(g)&&!editable(g,g->cursor))g->cursor++;
 if(g->id==19&&g->difficulty==3)ng_message(g,"PAN: every wrapped diagonal sums to 34.");
 else if(two_digits(g))ng_message(g,"Digits + EXE: enter. F4: CHECK.");
 else if(toggle_game(g))ng_message(g,"EXE: toggle. F4: CHECK.");
 else if(g->id==11)ng_message(g,"Digits: enter. F4: NOTES. EXE: CHECK.");
 else ng_message(g,"Digits: enter. EXE: CHECK.");
}
static bool latin_complete(const NgGame *g)
{
 unsigned n=g->cols;
 for(unsigned r=0;r<n;r++){
  unsigned rm=0,cm=0;
  for(unsigned c=0;c<n;c++){
   int a=g->board[r*n+c],b=g->board[c*n+r];
   if(a<1||a>(int)n||b<1||b>(int)n)return false;
   if((rm&(1u<<a))||(cm&(1u<<b)))return false;
   rm|=1u<<a;cm|=1u<<b;
  }
 }
 return true;
}
static int visible(const NgGame *g,int start,int step,unsigned n)
{
 int high=0,count=0;
 for(unsigned j=0;j<n;j++,start+=step)if(g->board[start]>high){high=g->board[start];count++;}
 return count;
}
static bool white_connected(const NgGame *g)
{
 unsigned n=g->cols,total=0,head=0,tail=0;uint8_t queue[81],seen[81]={0};
 for(unsigned i=0;i<size(g);i++)if(!g->board[i]){total++;if(!tail){queue[tail++]=(uint8_t)i;seen[i]=1;}}
 if(!total)return false;
 while(head<tail){
  unsigned i=queue[head++];
  unsigned nn[4]={i>=n?i-n:i,i+n<size(g)?i+n:i,i%n?i-1:i,i%n+1<n?i+1:i};
  for(unsigned k=0;k<4;k++){unsigned j=nn[k];if(!seen[j]&&!g->board[j]){seen[j]=1;queue[tail++]=(uint8_t)j;}}
 }
 return tail==total;
}
/* Completion reads public clues and the player's board, never solution[]. */
bool grids_rules_complete(const NgGame *g,const GridsPuzzle *p)
{
 if(!p||p->n<3||p->n>9||g->rows!=p->n||g->cols!=p->n)return false;
 unsigned n=p->n,nn=n*n;
 for(unsigned i=0;i<nn;i++){
  if(g->id!=13&&!toggle_game(g)&&!(g->id==19&&g->mode==1)){
   int clue=p->cells[i];if(g->id==17&&clue==255)clue=-1;
   if(clue!=blank(g)&&g->board[i]!=clue)return false;
  }
  if(g->id==13&&p->cells[i]==255&&(g->board[i]<1||g->board[i]>9))return false;
 }
 if(g->id==11||g->id==12||g->id==14||g->id==15){
  if(!latin_complete(g))return false;
  if(g->id==11){
   for(unsigned br=0;br<9;br+=3)for(unsigned bc=0;bc<9;bc+=3){unsigned bits=0;for(unsigned y=0;y<3;y++)for(unsigned x=0;x<3;x++){unsigned bit=1u<<g->board[(br+y)*9+bc+x];if(bits&bit)return false;bits|=bit;}}
  }else if(g->id==12){
   for(unsigned cage=0;cage<nn;cage++){
    int sum=0,prod=1,lo=99,hi=0;unsigned count=0;
    for(unsigned i=0;i<nn;i++)if(p->a[i]==(int)cage){int v=g->board[i];sum+=v;prod*=v;if(v<lo)lo=v;if(v>hi)hi=v;count++;}
    if(!count)continue;
    int op=p->b[cage*2],target=p->b[cage*2+1];
    if((op==0&&(count!=1||sum!=target))||(op==1&&sum!=target)||(op==2&&prod!=target)||(op==3&&(count!=2||hi-lo!=target))||(op==4&&(count!=2||hi!=lo*target)))return false;
   }
  }else if(g->id==14){
   for(unsigned i=0;i<nn;i++){
    if(i%n+1<n&&p->a[i]&&(g->board[i]-g->board[i+1])*p->a[i]>=0)return false;
    if(i+n<nn&&p->b[i]&&(g->board[i]-g->board[i+n])*p->b[i]>=0)return false;
   }
  }else{
   for(unsigned j=0;j<n;j++){
    if(p->a[j]&&visible(g,(int)j,(int)n,n)!=p->a[j])return false;
    if(p->a[n+j]&&visible(g,(int)((n-1)*n+j),-(int)n,n)!=p->a[n+j])return false;
    if(p->a[2*n+j]&&visible(g,(int)(j*n),1,n)!=p->a[2*n+j])return false;
    if(p->a[3*n+j]&&visible(g,(int)(j*n+n-1),-1,n)!=p->a[3*n+j])return false;
   }
  }
  return true;
 }
 if(g->id==13){
  unsigned covered[81]={0};
  for(unsigned i=0;i<nn;i++)for(unsigned direction=0;direction<2;direction++){
   int target=direction?p->b[i]:p->a[i];if(!target)continue;
   unsigned step=direction?n:1,j=i+step,bits=0,count=0;int sum=0;
   while(j<nn&&p->cells[j]==255&&(direction||j/n==i/n)){
    int v=g->board[j];if(v<1||v>9||(bits&(1u<<v)))return false;
    bits|=1u<<v;sum+=v;count++;covered[j]++;j+=step;
   }
   if(count<2||count>9||sum!=target)return false;
  }
  for(unsigned i=0;i<nn;i++)if(p->cells[i]==255&&covered[i]!=2)return false;
  return true;
 }
 if(g->id==16){
  for(unsigned i=0;i<nn;i++){
   if(g->board[i]<0||g->board[i]>1)return false;
   if(g->board[i]){if((i%n+1<n&&g->board[i+1])||(i+n<nn&&g->board[i+n]))return false;}
   else for(unsigned j=i+1;j<nn;j++)if(!g->board[j]&&(i/n==j/n||i%n==j%n)&&p->cells[i]==p->cells[j])return false;
  }
  return white_connected(g);
 }
 if(g->id==17){
  for(unsigned r=0;r<n;r++){
   unsigned ones_row=0,ones_col=0;
   for(unsigned c=0;c<n;c++){
    int a=g->board[r*n+c],b=g->board[c*n+r];if(a<0||a>1||b<0||b>1)return false;
    ones_row+=(unsigned)a;ones_col+=(unsigned)b;
    if(c+2<n&&(a==g->board[r*n+c+1]&&a==g->board[r*n+c+2]))return false;
    if(c+2<n&&(b==g->board[(c+1)*n+r]&&b==g->board[(c+2)*n+r]))return false;
   }
   if(ones_row!=n/2||ones_col!=n/2)return false;
   for(unsigned q=r+1;q<n;q++){
    bool rows_same=true,cols_same=true;
    for(unsigned c=0;c<n;c++){if(g->board[r*n+c]!=g->board[q*n+c])rows_same=false;if(g->board[c*n+r]!=g->board[c*n+q])cols_same=false;}
    if(rows_same||cols_same)return false;
   }
  }
  return true;
 }
 if(g->id==18||g->id==19){
  uint8_t positions[82]={0};
  for(unsigned i=0;i<nn;i++){int v=g->board[i];if(v<1||v>(int)nn||positions[v])return false;positions[v]=(uint8_t)(i+1);}
  if(g->id==18){
   for(unsigned v=1;v<nn;v++){int a=positions[v]-1,b=positions[v+1]-1,dy=a/(int)n-b/(int)n,dx=a%(int)n-b%(int)n;if(dy<0)dy=-dy;if(dx<0)dx=-dx;if(dx+dy!=1)return false;}
  }else{
   int target=(int)(n*(nn+1)/2),d1=0,d2=0;
   for(unsigned r=0;r<n;r++){int rs=0,cs=0;for(unsigned c=0;c<n;c++){rs+=g->board[r*n+c];cs+=g->board[c*n+r];}if(rs!=target||cs!=target)return false;d1+=g->board[r*n+r];d2+=g->board[r*n+n-1-r];}
   if(d1!=target||d2!=target)return false;
   if(g->difficulty==3){
    for(unsigned start=0;start<n;start++){
     int rising=0,falling=0;
     for(unsigned r=0;r<n;r++){rising+=g->board[r*n+(start+r)%n];falling+=g->board[r*n+(start+n-r)%n];}
     if(rising!=target||falling!=target)return false;
    }
   }
  }
  return true;
 }
 if(g->id==20){
  for(unsigned i=0;i<nn;i++)if(g->board[i]<0||g->board[i]>1)return false;
  for(unsigned r=0;r<n;r++){int rs=0,cs=0;for(unsigned c=0;c<n;c++){rs+=p->cells[r*n+c]*g->board[r*n+c];cs+=p->cells[c*n+r]*g->board[c*n+r];}if(rs!=p->a[r]||cs!=p->a[n+r])return false;}
  return true;
 }
 return false;
}
bool grids_complete(const NgGame *g)
{
 return grids_rules_complete(g,grids_puzzle(g));
}
static bool check(NgGame *g)
{
 if(grids_complete(g)){g->status=NG_WON;g->score=g->moves;ng_message(g,"Complete: every rule is satisfied.");}
 else ng_message(g,"Not complete. Check the visible rules.");
 return true;
}
static bool set_cell(NgGame *g,int value)
{
 if(!editable(g,g->cursor)){ng_message(g,"Fixed clue: choose an editable cell.");return true;}
 if(g->board[g->cursor]==value)return false;
 g->board[g->cursor]=(int16_t)value;g->notes[g->cursor]=0;g->moves++;g->message[0]='\0';return true;
}
static bool action(NgGame *g,int key)
{
 const GridsPuzzle *p=grids_puzzle(g);if(!p||g->status!=NG_PLAYING)return false;
 if(ng_grid_nav(g,key)){
  g->input[0]='\0';
  if(g->id==13){for(unsigned tries=0;tries<size(g)&&!editable(g,g->cursor);tries++)ng_grid_nav(g,key);}
  return true;
 }
 if(key==NGK_HINT&&g->id!=19){
  unsigned i=g->cursor;
  if(!editable(g,i)){ng_message(g,"Choose an editable cell for REVEAL.");return true;}
  int v=p->solution[i];g->assisted=1;
  if(g->board[i]!=v){g->board[i]=(int16_t)v;g->notes[i]=0;g->moves++;}
  g->input[0]='\0';ng_message(g,"REVEAL: one cell shown. ASSISTED.");return true;
 }
 if(key==NGK_AUX){
  if(g->id==11){g->notes_mode^=1;ng_message(g,g->notes_mode?"NOTES ON: digits toggle pencil marks.":"NOTES OFF: digits enter values.");return true;}
  if(toggle_game(g)||two_digits(g))return check(g);
  return false;
 }
 if(key==NGK_DEL){
  if(two_digits(g)&&g->input[0]){size_t len=strlen(g->input);g->input[len-1]='\0';return true;}
  if(toggle_game(g))return set_cell(g,g->id==20?1:0);
  if(g->notes[g->cursor]&&editable(g,g->cursor)){g->notes[g->cursor]=0;g->moves++;return true;}
  return set_cell(g,blank(g));
 }
 if(key==NGK_EXE){
  if(toggle_game(g))return set_cell(g,!g->board[g->cursor]);
  if(two_digits(g)&&g->input[0]){
   int v=0;for(unsigned i=0;g->input[i];i++)v=v*10+g->input[i]-'0';
   if(v<1||v>max_number(g)){ng_message(g,"Value is outside the board's range.");return true;}
   (void)set_cell(g,v);g->input[0]='\0';return true;
  }
  return check(g);
 }
 if(key>='0'&&key<='9'&&!toggle_game(g)){
  if(!editable(g,g->cursor)){ng_message(g,"Fixed clue: choose an editable cell.");return true;}
  if(two_digits(g)){
   size_t len=strlen(g->input);
   if(len>=2){ng_message(g,"Two digits maximum. DEL edits input.");return true;}
   g->input[len]=(char)key;g->input[len+1]='\0';return true;
  }
  int v=key-'0';
  if(v<(g->id==17?0:1)||v>max_number(g)){ng_message(g,"Number outside this puzzle's range.");return true;}
  if(g->id==11&&g->notes_mode){if(g->board[g->cursor]){ng_message(g,"Clear this value before adding notes.");return true;}g->notes[g->cursor]^=(uint16_t)(1u<<(v-1));g->moves++;return true;}
  return set_cell(g,v);
 }
 return false;
}
static bool valid(const NgGame *g)
{
 const GridsPuzzle *p=grids_puzzle(g);
 if(!p||g->rows!=p->n||g->cols!=p->n||g->cursor>=size(g)||g->mode>(g->id==19?1:0)||g->notes_mode>1)return false;
 /* Pre-expansion saves can name only the retained 900 + 80 records. */
 if(g->pack_revision==1&&g->puzzle_id>=980)return false;
 if(g->phase||g->turn||g->cpu_pending||g->history_count||g->scroll||g->reserved)return false;
 if(g->id!=11&&g->notes_mode)return false;
 for(unsigned i=0;i<NG_DATA;i++)if(g->data[i])return false;
 size_t len=0;while(len<NG_INPUT&&g->input[len])len++;
 if((!two_digits(g)&&len)||len>2)return false;
 for(size_t i=0;i<len;i++)if(g->input[i]<'0'||g->input[i]>'9')return false;
 for(unsigned i=0;i<NG_CELLS;i++){
  if(i>=size(g)){if(g->board[i]||g->fixed[i]||g->notes[i])return false;continue;}
  int low=g->id==17?-1:0,high=max_number(g);
  if(g->board[i]<low||g->board[i]>high||g->fixed[i]>1)return false;
  if(g->notes[i]&&(g->id!=11||g->board[i]||g->fixed[i]||g->notes[i]>511))return false;
  bool expected=false;int clue=p->cells[i];
  if(g->id==13){expected=clue!=255;if(expected&&g->board[i])return false;}
  else if(!toggle_game(g)){if(g->id==17&&clue==255)clue=-1;if(g->id==19&&g->mode==1)clue=0;expected=clue!=blank(g);if(expected&&g->board[i]!=clue)return false;}
  if((bool)g->fixed[i]!=expected)return false;
 }
 if(g->status==NG_WON&&!grids_complete(g))return false;
 return g->status==NG_PLAYING||g->status==NG_WON;
}
/* Readable 3x5 pencil numerals keep all nine notes inside a 17px cell. */
static void tiny_digit(NgCanvas *c,int x,int y,unsigned d)
{
 static const uint16_t glyph[10]={31599,11415,29671,29647,23497,31183,31215,29257,31727,31695};
 if(d>9)return;
 for(unsigned r=0;r<5;r++)for(unsigned col=0;col<3;col++)if(glyph[d]&(1u<<(14-r*3-col)))ng_rect(c,x+(int)col,y+(int)r,1,1,NG_BLUE);
}
static void label_number(NgCanvas *c,int x,int y,int w,int v,int color)
{
 char s[12];snprintf(s,sizeof s,"%d",v);ng_center(c,x,y,w,s,color,1);
}
static NgGridLayout layout(const NgGame *g)
{
 bool outside=g->id==15||g->id==20;
 NgGridLayout l=ng_grid_layout(g->rows,g->cols,outside);
 if(outside){l.size=128/(int)g->cols;l.x=15+(210-(int)g->cols*l.size)/2;l.y=40;}
 if(g->id==14){l.size=(154-((int)g->cols-1)*6)/(int)g->cols;l.x=15+(210-((int)g->cols*l.size+((int)g->cols-1)*6))/2;l.y=29;}
 return l;
}
static void render(const NgGame *g,NgCanvas *c)
{
 const GridsPuzzle *p=grids_puzzle(g);if(!p)return;
 unsigned n=p->n,nn=n*n;NgGridLayout l=layout(g);char text[64];
 int pitch=l.size+(g->id==14?6:0);
 for(unsigned i=0;i<nn;i++){
  int x=l.x+(int)(i%n)*pitch,y=l.y+(int)(i/n)*pitch;
  NgGridLayout cell={x,y,l.size};int value=g->board[i],fill=NG_WHITE;char s[12]="";
  if(toggle_game(g))value=p->cells[i];
  if(g->id==13&&p->cells[i]!=255){
   ng_rect(c,x,y,l.size,l.size,NG_INK);ng_border(c,x,y,l.size,l.size,NG_LINE,1);
   if(p->a[i]||p->b[i]){
    ng_line(c,x,y,x+l.size-1,y+l.size-1,NG_WHITE);
    if(p->b[i]){snprintf(s,sizeof s,"%d",p->b[i]);ng_small(c,x+1,y+l.size-8,s,NG_WHITE);}
    if(p->a[i]){snprintf(s,sizeof s,"%d",p->a[i]);ng_small(c,x+l.size-(int)strlen(s)*6,y+1,s,NG_WHITE);}
   }
   continue;
  }
  if(g->id==16&&g->board[i])fill=NG_INK;
  if(g->id==20&&!g->board[i])fill=NG_PALE;
  if(value!=blank(g)||toggle_game(g))snprintf(s,sizeof s,"%d",value);
  if(g->id==12){
   ng_grid_cell(c,cell,1,0,"",g->fixed[i],i==g->cursor,fill);
   if(s[0])ng_center(c,x,y+l.size-12,l.size,s,g->fixed[i]?NG_INK:NG_BLUE,1);
  }else if(toggle_game(g)){
   ng_grid_cell(c,cell,1,0,"",false,i==g->cursor,fill);
   int color=g->id==16&&g->board[i]?NG_WHITE:(g->id==20&&!g->board[i]?NG_MUTED:NG_INK);
   int scale=l.size>=30&&ng_text_width(s,2)<l.size-5?2:1;
   ng_center(c,x,y+(l.size-10*scale)/2,l.size,s,color,scale);
  }else ng_grid_cell(c,cell,1,0,s,g->fixed[i],i==g->cursor,fill);
  if(g->id==20&&!g->board[i])ng_line(c,x+3,y+l.size/2,x+l.size-4,y+l.size/2,NG_MUTED);
  if(g->id==11&&g->notes[i])for(unsigned d=1;d<=9;d++)if(g->notes[i]&(1u<<(d-1)))tiny_digit(c,x+2+(int)((d-1)%3)*4,y+1+(int)((d-1)/3)*5,d);
 }
 if(g->id==11){
  for(unsigned i=0;i<=9;i+=3){int x=l.x+(int)i*l.size,y=l.y+(int)i*l.size;ng_rect(c,x-1,l.y,2,9*l.size,NG_INK);ng_rect(c,l.x,y-1,9*l.size,2,NG_INK);}
 }
 if(g->id==12){
  for(unsigned i=0;i<nn;i++){
   int x=l.x+(int)(i%n)*pitch,y=l.y+(int)(i/n)*pitch,cc=p->a[i];
   if(i%n==0||p->a[i-1]!=cc)ng_rect(c,x,y,2,l.size,NG_INK);
   if(i%n==n-1||p->a[i+1]!=cc)ng_rect(c,x+l.size-2,y,2,l.size,NG_INK);
   if(i<n||p->a[i-n]!=cc)ng_rect(c,x,y,l.size,2,NG_INK);
   if(i+n>=nn||p->a[i+n]!=cc)ng_rect(c,x,y+l.size-2,l.size,2,NG_INK);
   bool first=true;for(unsigned j=0;j<i;j++)if(p->a[j]==cc)first=false;
   if(first){
    static const char ops[]=" +*- /";int op=p->b[cc*2];char oper=op==4?'/':ops[op];
    snprintf(text,sizeof text,"%d%c",p->b[cc*2+1],oper);
    if(op==2 && ng_small_width(text)>l.size-5){
     /* Keep wide products such as 360* inside a 26px cell: compact x sign. */
     snprintf(text,sizeof text,"%d",p->b[cc*2+1]);ng_small(c,x+3,y+3,text,NG_INK);
     int ox=x+4+ng_small_width(text),color=ng_operator_color('*',NG_INK);ng_line(c,ox,y+5,ox+2,y+7,color);ng_line(c,ox,y+7,ox+2,y+5,color);
    }else ng_small_expression(c,x+3,y+3,text,NG_INK);
   }
  }
 }
 if(g->id==14){
  for(unsigned i=0;i<nn;i++){
   int x=l.x+(int)(i%n)*pitch,y=l.y+(int)(i/n)*pitch;
   if(p->a[i]&&i%n+1<n)ng_small(c,x+l.size+1,y+l.size/2-3,p->a[i]>0?"<":">",NG_RED);
   if(p->b[i]&&i+n<nn){int xx=x+l.size/2,yy=y+l.size+1;int top=p->b[i]>0?yy:yy+3,bot=p->b[i]>0?yy+3:yy;ng_line(c,xx,top,xx-3,bot,NG_RED);ng_line(c,xx,top,xx+3,bot,NG_RED);}
  }
 }
 if(g->id==15){
  for(unsigned j=0;j<n;j++){
   if(p->a[j])label_number(c,l.x+(int)j*pitch,l.y-12,l.size,p->a[j],NG_GREEN);
   if(p->a[n+j])label_number(c,l.x+(int)j*pitch,l.y+(int)n*pitch+2,l.size,p->a[n+j],NG_GREEN);
   if(p->a[2*n+j])label_number(c,l.x-17,l.y+(int)j*pitch+(l.size-10)/2,15,p->a[2*n+j],NG_GREEN);
   if(p->a[3*n+j])label_number(c,l.x+(int)n*pitch+2,l.y+(int)j*pitch+(l.size-10)/2,15,p->a[3*n+j],NG_GREEN);
  }
 }
 if(g->id==20){
  for(unsigned j=0;j<n;j++){
   label_number(c,l.x+(int)n*pitch+3,l.y+(int)j*pitch+(l.size-10)/2,25,p->a[j],NG_GREEN);
   label_number(c,l.x+(int)j*pitch,l.y+(int)n*pitch+3,l.size,p->a[n+j],NG_GREEN);
  }
 }
 /* Cage borders must not hide the active cell highlight. */
 int sx=l.x+(int)(g->cursor%n)*pitch,sy=l.y+(int)(g->cursor/n)*pitch;
 ng_border(c,sx,sy,l.size,l.size,NG_BLUE,g->id==11&&g->notes[g->cursor]?1:2);
 int info=235;
 if(g->id==19&&g->mode==1)snprintf(text,sizeof text,"%s %ux%u",g->difficulty==3?"FREE PAN":"FREE",n,n);
 else if(g->difficulty>=3){unsigned count=grids_bank_count(g->id,g->difficulty,g->mode),ordinal=0;while(ordinal<count&&grids_bank_id(g->id,g->difficulty,ordinal)!=g->puzzle_id)ordinal++;snprintf(text,sizeof text,"%s %02u / %u",g->difficulty==4?"HELL":"MASTER",ordinal+1,count);}
 else {unsigned count=grids_bank_count(g->id,g->difficulty,g->mode),ordinal=0;while(ordinal<count&&grids_bank_id(g->id,g->difficulty,ordinal)!=g->puzzle_id)ordinal++;snprintf(text,sizeof text,"PUZZLE %u / %u",ordinal+1,count);}
 ng_text(c,info,34,text,NG_MUTED,1);
 snprintf(text,sizeof text,"CELL %u,%u",g->cursor/n+1,g->cursor%n+1);ng_text(c,info,50,text,NG_INK,1);
 if(two_digits(g)){
  snprintf(text,sizeof text,"RANGE 1-%u",nn);ng_text(c,info,70,text,NG_INK,1);ng_input(c,info,89,140,g->input);
  ng_text(c,info,119,"EXE: ENTER",NG_BLUE,1);ng_text(c,info,137,"F4: CHECK",NG_MUTED,1);
  if(g->id==19){snprintf(text,sizeof text,"%sSUM = %u",g->difficulty==3?"PAN ":"",n*(nn+1)/2);ng_text(c,info,156,text,NG_GREEN,1);if(g->difficulty==3)ng_small(c,info,173,"ALL WRAP DIAGONALS",NG_GREEN);}
 }else if(g->id==11){
  ng_text(c,info,73,g->notes_mode?"NOTES ON":"NOTES OFF",NG_BLUE,1);
  ng_text(c,info,93,"F4: NOTES",NG_MUTED,1);ng_text(c,info,111,"DEL: CLEAR",NG_MUTED,1);
  if(g->notes[g->cursor]){unsigned k=0;for(unsigned d=1;d<=9;d++)if(g->notes[g->cursor]&(1u<<(d-1))){text[k++]=(char)('0'+d);text[k++]=' ';}text[k]=0;ng_text(c,info,140,"CELL NOTES",NG_INK,1);ng_text(c,info,155,text,NG_BLUE,1);}
  else{ng_text(c,info,140,"1-9 EACH",NG_INK,1);ng_text(c,info,155,"ROW COL BOX",NG_INK,1);}
 }else if(g->id==12){
  unsigned cc=(unsigned)p->a[g->cursor];static const char *ops[]={"=","+","*","-","/"};
  snprintf(text,sizeof text,"CAGE %d%s",p->b[2*cc+1],ops[p->b[2*cc]]);ng_expression(c,info,76,text,NG_BLUE,1);ng_text(c,info,98,"1-N EACH",NG_INK,1);ng_text(c,info,113,"ROW + COL",NG_INK,1);ng_text(c,info,141,"EXE: CHECK",NG_MUTED,1);
 }else if(g->id==13){
  ng_text(c,info,74,"CLUES",NG_INK,1);ng_text(c,info,91,"UPPER: RIGHT",NG_MUTED,1);ng_text(c,info,108,"LOWER: DOWN",NG_MUTED,1);ng_text(c,info,137,"RUNS: 1-9",NG_INK,1);ng_text(c,info,153,"NO REPEATS",NG_INK,1);
 }else if(g->id==14){ng_text(c,info,77,"1-N EACH",NG_INK,1);ng_text(c,info,94,"ROW + COL",NG_INK,1);ng_text(c,info,120,"TIP POINTS",NG_MUTED,1);ng_text(c,info,136,"TO SMALLER",NG_MUTED,1);
 }else if(g->id==15){ng_text(c,info,77,"OUTSIDE =",NG_GREEN,1);ng_text(c,info,94,"VISIBLE",NG_GREEN,1);ng_text(c,info,122,"1-N EACH",NG_INK,1);ng_text(c,info,139,"ROW + COL",NG_INK,1);
 }else if(g->id==16){ng_text(c,info,77,"EXE: SHADE",NG_BLUE,1);ng_text(c,info,98,"WHITE CELLS",NG_INK,1);ng_text(c,info,115,"STAY JOINED",NG_INK,1);ng_text(c,info,141,"F4: CHECK",NG_MUTED,1);
 }else if(g->id==17){ng_text(c,info,77,"0 / 1",NG_BLUE,1);ng_text(c,info,97,"HALF OF EACH",NG_INK,1);ng_text(c,info,115,"NO TRIPLES",NG_INK,1);ng_text(c,info,133,"UNIQUE LINES",NG_INK,1);
 }else if(g->id==20){ng_text(c,info,76,"EXE: TOGGLE",NG_BLUE,1);ng_text(c,info,100,"KEEP SUMS =",NG_INK,1);ng_text(c,info,117,"GREEN TARGETS",NG_GREEN,1);ng_text(c,info,143,"F4: CHECK",NG_MUTED,1);}
}
static const char *normal_mode(unsigned mode){(void)mode;return "CLASSIC";}
static const char *magic_mode(unsigned mode){return mode?"FREE":"PARTIAL";}
#define GRID(ID,NAME,SHORT,RULES,FLAGS,AUX,PRIMARY,MODES,MODE) \
 {ID,NAME,SHORT,RULES,FLAGS,AUX,PRIMARY,MODES,MODE,init,action,NULL,valid,render}
const NgModule ng_grids[10]={
 GRID(11,"SUDOKU","SUDOKU","Fill 1-9 once in every row, column, 3x3 box.\nGiven clues cannot change. Digits enter values.\nF4 NOTES toggles pencil marks; DEL clears.\nEXE CHECK validates visible rules only.\nF3 REVEAL shows one cell and marks ASSISTED.\nArrows select. UNDO restores edits.",NGF_UNDO|NGF_HINT,"NOTES","CHECK",1,normal_mode),
 GRID(12,"CALCUDOKU","CALCUDOKU","Use 1-N once in each row and column.\nCages satisfy their target and operator.\n+ and * combine all cells. - is difference.\n/ is larger divided by smaller, exactly.\n- and / cages have two cells. = is fixed.\nA cage may repeat values in different lines.\nF3 REVEAL is assisted. EXE checks all rules.",NGF_UNDO|NGF_HINT,"","CHECK",1,normal_mode),
 GRID(13,"KAKURO","KAKURO","Enter 1-9 in every white cell; zero is illegal.\nEvery run sums to its clue without repeats.\nUpper-right clue: across. Lower-left: down.\nArrows select cells. DEL clears. EXE checks.\nF3 REVEAL shows one cell, marking ASSISTED.",NGF_UNDO|NGF_HINT,"","CHECK",1,normal_mode),
 GRID(14,"FUTOSHIKI","FUTOSHIKI","Use 1-N once in each row and column.\nAll printed inequalities must hold.\nThe pointed tip faces the smaller value.\nGiven clues stay fixed. DEL clears.\nF3 REVEAL is assisted. EXE checks all rules.",NGF_UNDO|NGF_HINT,"","CHECK",1,normal_mode),
 GRID(15,"SKYSCRAPERS","SKYSCRAPERS","Use heights 1-N once in each row and column.\nOuter clues count buildings seen from there.\nA taller building hides lower ones behind it.\nMissing outer clues impose no condition.\nDEL clears. EXE checks. F3 REVEAL is assisted.",NGF_UNDO|NGF_HINT,"","CHECK",1,normal_mode),
 GRID(16,"HITORI","HITORI","EXE shades or unshades the selected cell.\nUnshaded values cannot repeat in a row/column.\nShaded cells cannot share an edge.\nAll unshaded cells must connect by edges.\nDiagonal shaded contacts are allowed.\nF4 CHECK validates all three rules.\nDEL unshades. F3 REVEAL is assisted.",NGF_UNDO|NGF_HINT,"CHECK","SHADE",1,normal_mode),
 GRID(17,"BINARY PUZZLE","BINARY","Enter 0 or 1. A blank differs from zero.\nEach row/column contains half zeros/half ones.\nNever three identical neighbors in a line.\nNo two completed rows or columns may match.\nGiven clues stay fixed. DEL returns to blank.\nEXE checks. F3 REVEAL is assisted.",NGF_UNDO|NGF_HINT,"","CHECK",1,normal_mode),
 GRID(18,"NUMBRIX","NUMBRIX","Place every integer from 1 to N*N once.\nConsecutive numbers must share an edge.\nDiagonal steps are forbidden. Clues are fixed.\nType one or two digits, then EXE to enter.\nMoving the cursor cancels a draft. DEL erases.\nF4 CHECK validates the path.\nF3 REVEAL shows one cell and marks ASSISTED.",NGF_UNDO|NGF_HINT,"CHECK","ENTER",1,normal_mode),
 GRID(19,"MAGIC SQUARE","MAGIC SQUARE","Use every integer 1 to N*N exactly once.\nRows, columns and both main diagonals must\nhave the displayed common sum.\nMASTER PAN: all wrapped diagonals also sum 34.\nPARTIAL has fixed clues. FREE has none.\nAny rule-valid solution is accepted.\nType 1-2 digits, EXE enters; F4 checks.\nDEL edits draft or clears. Arrows select.",NGF_UNDO,"CHECK","ENTER",2,magic_mode),
 GRID(20,"SUM GRID","SUM GRID","EXE toggles KEEP or REMOVE for a number.\nKept numbers must sum to each row/column target.\nRemoved numbers remain visible with a line.\nA zero target means remove the whole line.\nF4 CHECK validates every sum. DEL keeps.\nF3 REVEAL shows one choice, marking ASSISTED.",NGF_UNDO|NGF_HINT,"CHECK","TOGGLE",1,normal_mode)
};
