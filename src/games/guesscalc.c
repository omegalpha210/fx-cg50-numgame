#include "ng.h"
#include "ui.h"
#include "guesscalc_math.h"
#include "guesscalc.h"
#include "prime_beta6.h"
#include "../../assets/guesscalc_packs.h"
#include "../../assets/guesscalc_master.h"
#include "../../assets/guesscalc_beta4.h"
#include "../../assets/guesscalc_target_beta4.h"
#include "../../assets/guesscalc_target_beta5.h"
#include "../../assets/guesscalc_target_beta6.h"
#include "../../assets/guesscalc_countdown_beta6.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char *default_mode(unsigned mode){(void)mode;return "STANDARD";}
static const char *equation_mode(unsigned mode){static const char *const m[]={"STANDARD","SHORT","LONG"};return mode<3?m[mode]:"?";}
unsigned gc_bank_count(unsigned id,unsigned difficulty,unsigned mode){
 if(id<1||id>10||difficulty>3)return 0;
 if(id==1||id==10||(id==8&&difficulty<3))return 0;
 if(mode>=(id==2?3u:1u))return 0;
 if(id==6)return 2;
 return 30;
}
static unsigned bank_index(NgGame *g,unsigned legacy_modes){
 if(g->pack_revision==4&&g->id==5&&g->difficulty<2)return 120+g->difficulty*30+ng_bank_pick(g,30);
 if(g->pack_revision==4&&g->id==7&&g->difficulty==2)return 120+ng_bank_pick(g,30);
 if(g->difficulty==3)return legacy_modes*90+g->mode*30+ng_bank_pick(g,30);
 return g->mode*90+g->difficulty*30+ng_bank_pick(g,30);
}
static bool bank_valid(const NgGame *g,unsigned modes){
 if(g->difficulty>3||g->mode>=modes||(g->difficulty==3&&g->pack_revision!=2&&!(g->pack_revision==4&&(g->id==5||g->id==7))))return false;
 if(g->pack_revision==4&&g->id==5&&g->difficulty<2)return g->puzzle_id>=120+g->difficulty*30u&&g->puzzle_id<150+g->difficulty*30u;
 if(g->pack_revision==4&&g->id==7&&g->difficulty==2)return g->puzzle_id>=120&&g->puzzle_id<150;
 unsigned start=g->difficulty==3?modes*90+g->mode*30:g->mode*90u+g->difficulty*30u;
 return g->puzzle_id>=start&&g->puzzle_id<start+30;
}
static unsigned baseball_length(const NgGame *g){static const unsigned n[]={4,3,5,4,3,5};return g->pack_revision>=3?4u+g->difficulty:n[g->mode]+(g->difficulty==3?2u:0u);}
static bool baseball_unique(const NgGame *g){return g->pack_revision<3&&g->mode<3;}
static unsigned baseball_limit(const NgGame *g){static const unsigned old[]={16,12,10,16},current[]={20,30,40,50};return (g->pack_revision>=4?current:old)[g->difficulty];}
static const char *equation_pack(const NgGame *g){return g->difficulty==3?gc_master_equation[g->puzzle_id-270]:gc_equation_pack[g->puzzle_id];}
typedef struct {unsigned n,alphabet,count,stride;const unsigned char *clues,*matches,*solution;} MindView;
static MindView mind_pack(const NgGame *g){
 if(g->difficulty==3){const GcMasterMindPack *p=&gc_master_mind[g->puzzle_id-90];return (MindView){6,8,p->count,6,&p->clues[0][0],p->matches,p->solution};}
 const GcMindPack *p=&gc_mind_pack[g->puzzle_id];return (MindView){p->n,p->alphabet,p->count,5,&p->clues[0][0],p->matches,p->solution};
}
static const GcSequencePack *sequence_pack(const NgGame *g){if(g->pack_revision==4&&g->difficulty<2)return &gc_sequence_beta4[g->puzzle_id-120];return g->difficulty==3?&gc_master_sequence[g->puzzle_id-90]:&gc_sequence_pack[g->puzzle_id];}
static const GcCrossPack *cross_pack(const NgGame *g){return g->difficulty==3?&gc_master_cross[g->puzzle_id-90]:&gc_cross_pack[g->puzzle_id];}
static bool auxiliary(int key){return key==NGK_AUX||key==NGK_ANSWER||key==NGK_F4;}
static bool submit(int key){return key==NGK_EXE||key==NGK_F6;}
static bool hint(int key){return key==NGK_HINT||key==NGK_F3;}
static bool input_ok(const NgGame *g,const char *allowed,unsigned limit){
 for(unsigned i=0;i<NG_INPUT;i++){
  if(!g->input[i])return i<=limit;
  if(!strchr(allowed,(unsigned char)g->input[i]))return false;
 }
 return false;
}
static bool op_ok(int value){return value=='+'||value=='-'||value=='*'||value=='/';}
static bool integer_input(const char *s,int *out){
 unsigned i=0;int sign=1,value=0;if(s[i]=='-'){sign=-1;++i;}if(!s[i])return false;
 for(;s[i];i++){if(s[i]<'0'||s[i]>'9'||i>7)return false;value=value*10+s[i]-'0';if(value>1000000)return false;}
 *out=sign*value;return true;
}
static void secret_text(const NgGame *g,char *out,unsigned n){for(unsigned i=0;i<n;i++)out[i]=(char)g->board[i];out[n]=0;}
static void number_code(const NgGame *g,char *out,unsigned offset,unsigned n){for(unsigned i=0;i<n;i++)out[i]=(char)('0'+g->board[offset+i]);out[n]=0;}
static bool scroll_history(NgGame *g,int key,unsigned visible){
 unsigned max=g->history_count>visible?g->history_count-visible:0;
 if(key==NGK_UP){if(g->scroll)g->scroll--;return true;}
 if(key==NGK_DOWN){if(g->scroll<max)g->scroll++;return true;}
 return false;
}
static void text_input(const NgGame *g,NgCanvas *c,int y){
 if(g->id==2||g->id==6||g->id==7||g->id==10)ng_input_expression(c,10,y,376,g->input);
 else ng_input(c,10,y,376,g->input);
}
static void center_expression(NgCanvas *c,int x,int y,int width,const char *text,int color,int scale){
 while(scale>1&&ng_text_width(text,scale)>width)--scale;
 if(ng_text_width(text,scale)<=width)ng_expression(c,x+(width-ng_text_width(text,scale))/2,y,text,color,scale);
 else {
  char line[128];const char *remaining=text;
  if(ng_text_line(line,sizeof(line),&remaining,width,true))ng_small_expression(c,x+(width-ng_small_width(line))/2,y,line,color);
 }
}
static void operator_card(NgCanvas *c,int x,int y,const char *text,bool selected){
 /* Draw the card without a label, then draw the operator exactly once. */
 ng_card(c,x,y,29,30,"",selected,false);
 int scale=ng_text_width(text,2)<21?2:1;
 center_expression(c,x,y+(30-10*scale)/2,29,text,NG_INK,scale);
}

static void init_baseball(NgGame *g){
 unsigned n=baseball_length(g);
 g->data[0]=(int32_t)n;g->data[1]=(int32_t)baseball_limit(g);
 unsigned used=0;
 for(unsigned i=0;i<n;i++){unsigned d=ng_rand(g,10);if(baseball_unique(g)){while(used&(1u<<d))d=(d+1)%10;used|=1u<<d;}g->board[i]=(int16_t)('0'+d);}
 g->puzzle_id=g->seed;ng_message(g,"Leading zero allowed. Enter a code.");
}
/* Revision 4 keeps all 50 attempts in existing module-owned data slots. The
 * fixed digit count restores leading zeros; the high byte preserves S/B for
 * validation against the secret after a save/load round trip. */
static uint32_t baseball_record(const char *guess,unsigned strikes,unsigned balls){
 uint32_t value=0;for(unsigned i=0;guess[i];i++)value=value*10u+(unsigned)(guess[i]-'0');
 return value|((uint32_t)strikes<<24)|((uint32_t)balls<<28);
}
static void baseball_guess(uint32_t record,unsigned n,char out[8])
{snprintf(out,8,"%0*u",(int)n,(unsigned)(record&UINT32_C(0x00ffffff)));}
static bool baseball_scroll(NgGame *g,int key){
 unsigned count=g->pack_revision>=4?g->moves:g->history_count;
 unsigned max=count>5?count-5:0;
 if(key==NGK_UP){if(g->scroll)g->scroll--;return true;}
 if(key==NGK_DOWN){if(g->scroll<max)g->scroll++;return true;}
 return false;
}
static bool action_baseball(NgGame *g,int key){
 if(baseball_scroll(g,key))return true;
 if(g->status!=NG_PLAYING)return false;
 if(g->moves>=(unsigned)g->data[1])return false;
 if(g->pack_revision>=4 && g->phase==1){
  if(submit(key)||key==NGK_EXIT){g->phase=2;ng_message(g,"ONE LAST TRY. B: correct digit, wrong position.");return true;}
  return false;
 }
 if(!submit(key))return ng_edit(g,key,"0123456789",(unsigned)g->data[0]);
 unsigned n=(unsigned)g->data[0];if(strlen(g->input)!=n){ng_message(g,"Enter every digit before SUBMIT.");return true;}
 unsigned mask=0;for(unsigned i=0;i<n;i++){unsigned bit=1u<<(g->input[i]-'0');if(baseball_unique(g)&&(mask&bit)){ng_message(g,"This mode requires distinct digits.");return true;}mask|=bit;}
 char secret[8],line[40];secret_text(g,secret,n);unsigned s,b;gc_baseball(secret,g->input,n,&s,&b);
 if(g->pack_revision>=4)g->data[2+g->moves]=(int32_t)baseball_record(g->input,s,b);
 else {snprintf(line,sizeof(line),"%2u  %.7s     %cS  %cB",(unsigned)g->moves+1,g->input,(int)('0'+s),(int)('0'+b));ng_history(g,line);}
 g->moves++;g->score=g->moves;g->scroll=g->moves>5?g->moves-5:0;g->input[0]=0;
 if(s==n){g->status=NG_WON;ng_message(g,"Code cracked!");}
 else if(g->moves>=(unsigned)g->data[1]){g->status=NG_LOST;snprintf(g->message,sizeof(g->message),"Secret: %s",secret);}
 else if(g->pack_revision>=4 && g->moves+1u==(unsigned)g->data[1]){
  g->phase=1;ng_message(g,"ONE LAST TRY");
 }
 else ng_message(g,"S: exact position. B: correct digit, wrong position.");
 return true;
}

static bool valid_baseball(const NgGame *g){
 if(g->status>NG_LOST||g->mode>=(g->pack_revision>=3?1u:6u)||g->difficulty>3||g->pack_revision<1||g->pack_revision>4||(g->difficulty==3&&g->pack_revision==1))return false;
 unsigned n=baseball_length(g),limit=baseball_limit(g);
 bool compact=g->pack_revision>=4;
 if(g->data[0]!=(int)n||g->data[1]!=(int)limit||g->moves>limit||
  g->history_count!=(compact?0u:g->moves)||!input_ok(g,"0123456789",n))return false;
 unsigned mask=0;for(unsigned i=0;i<n;i++){int v=g->board[i]-'0';if(v<0||v>9||(baseball_unique(g)&&(mask&(1u<<v))))return false;mask|=1u<<v;}
 bool won=false;char secret[8];secret_text(g,secret,n);
 if(compact){
  uint32_t bound=1;for(unsigned i=0;i<n;i++)bound*=10u;
  if(g->phase>2 || (g->phase && g->moves<limit-1u) ||
   (g->status==NG_PLAYING && g->moves==limit-1u && !g->phase) ||
   (g->phase==1 && (g->status!=NG_PLAYING || g->moves!=limit-1u)) ||
   (g->moves==limit && g->phase!=2))return false;
  for(unsigned row=0;row<g->moves;row++){
   uint32_t record=(uint32_t)g->data[2+row];unsigned value=record&UINT32_C(0x00ffffff);
   char guess[8];unsigned strikes,balls;if(value>=bound)return false;
   baseball_guess(record,n,guess);gc_baseball(secret,guess,n,&strikes,&balls);
   if((record>>24&15u)!=strikes || (record>>28&15u)!=balls)return false;
   won=strikes==n;if(won && row+1u<g->moves)return false;
  }
  for(unsigned i=2u+g->moves;i<NG_DATA;i++)if(g->data[i])return false;
  for(unsigned i=0;i<NG_HISTORY;i++)for(unsigned j=0;j<sizeof g->history[i];j++)if(g->history[i][j])return false;
 }else for(unsigned row=0;row<g->history_count;row++){
  char guess[8],expected[40];unsigned length=n,seen=0,strikes,balls;
  if(strlen(g->history[row])!=length+15)return false;
  for(unsigned col=0;col<length;col++){
   char digit=g->history[row][4+col];if(digit<'0'||digit>'9')return false;
   unsigned bit=1u<<(digit-'0');if(baseball_unique(g)&&(seen&bit))return false;seen|=bit;guess[col]=digit;
  }
  guess[length]=0;gc_baseball(secret,guess,length,&strikes,&balls);
  snprintf(expected,sizeof(expected),"%2u  %.7s     %cS  %cB",row+1,guess,(int)('0'+strikes),(int)('0'+balls));
  if(strcmp(expected,g->history[row]))return false;
  won=strikes==length;if(won&&row+1<g->history_count)return false;
 }
 if(g->score!=g->moves||(g->status==NG_WON)!=won)return false;
 if(g->status==NG_PLAYING&&g->moves>=limit)return false;
 if(g->status==NG_LOST&&g->moves!=limit)return false;
 return g->scroll<=((compact?g->moves:g->history_count)>5?(compact?g->moves:g->history_count)-5:0);
}

static void render_baseball(const NgGame *g,NgCanvas *c){
 char line[64];snprintf(line,sizeof(line),"%d DIGITS %s   TRY %u/%d",(int)g->data[0],baseball_unique(g)?"UNIQUE":"REPEAT",(unsigned)g->moves,(int)g->data[1]);ng_text(c,12,31,line,NG_BLUE,1);
 unsigned count=g->pack_revision>=4?g->moves:g->history_count;
 for(unsigned i=0;i<5&&g->scroll+i<count;i++){
  const char *record=g->history[g->scroll+i];
  if(g->pack_revision>=4){
   uint32_t packed=(uint32_t)g->data[2+g->scroll+i];char guess[8];
   baseball_guess(packed,(unsigned)g->data[0],guess);
   snprintf(line,sizeof line,"%2u  %.7s     %uS  %uB",(unsigned)g->scroll+i+1,guess,
    (unsigned)(packed>>24&15u),(unsigned)(packed>>28&15u));record=line;
  }
  ng_text(c,22,53+(int)i*18,record,NG_INK,1);
 }
 if(!count)ng_text(c,22,83,"Your attempts appear here.",NG_MUTED,1);
 ng_list_scrollbar(c,count,5,g->scroll,375,45,55,76,136);
 text_input(g,c,149);
}
static void init_equation(NgGame *g){
 g->puzzle_id=bank_index(g,3);
 const char *s=equation_pack(g);g->data[0]=(int32_t)strlen(s);g->data[1]=g->difficulty==3?12:12-2*g->difficulty;
 for(unsigned i=0;s[i];i++)g->board[i]=(int16_t)s[i];
 ng_message(g,"Enter a true equation of the shown length.");
}
static bool action_equation(NgGame *g,int key){
 if(scroll_history(g,key,4))return true;
 if(g->status!=NG_PLAYING)return false;
 if(!submit(key))return ng_edit(g,key,"0123456789+-*/=",(unsigned)g->data[0]);
 unsigned n=(unsigned)g->data[0];if(strlen(g->input)!=n){ng_message(g,"Equation must have the exact character count.");return true;}
 if(!gc_equation(g->input)){ng_message(g,"Use a true equation; no leading zeros.");return true;}
 char secret[11],line[40];uint8_t feedback[10];secret_text(g,secret,n);gc_feedback(secret,g->input,n,feedback);
 memcpy(line,g->input,n);line[n]='|';bool won=true;for(unsigned i=0;i<n;i++){line[n+1+i]=(char)('0'+feedback[i]);if(feedback[i]!=2)won=false;}line[2*n+1]=0;
 ng_history(g,line);++g->moves;g->score=g->moves;g->scroll=g->history_count>4?g->history_count-4:0;g->input[0]=0;
 if(won){g->status=NG_WON;ng_message(g,"Hidden character sequence found!");}
 else if(g->moves>=(unsigned)g->data[1]){g->status=NG_LOST;snprintf(g->message,sizeof(g->message),"Hidden equation: %s",secret);}
 else ng_message(g,"GREEN exact; YELLOW elsewhere; GREY absent.");
 return true;
}

static bool valid_equation(const NgGame *g){
 if(g->status>NG_LOST||!bank_valid(g,3))return false;
 const char *secret=equation_pack(g);unsigned n=(unsigned)strlen(secret);
 if(g->data[0]!=(int)n||g->data[1]!=(g->difficulty==3?12:12-2*g->difficulty)||g->moves>(unsigned)g->data[1]||g->history_count!=g->moves||!input_ok(g,"0123456789+-*/=",n))return false;
 for(unsigned i=0;i<n;i++)if(g->board[i]!=secret[i])return false;
 bool won=false;
 for(unsigned i=0;i<g->history_count;i++){
  if(strlen(g->history[i])!=2*n+1||g->history[i][n]!='|')return false;
  char guess[11];uint8_t feedback[10];memcpy(guess,g->history[i],n);guess[n]=0;
  if(!gc_equation(guess))return false;
  gc_feedback(secret,guess,n,feedback);won=true;
  for(unsigned j=0;j<n;j++){if(g->history[i][n+1+j]!='0'+feedback[j])return false;if(feedback[j]!=2)won=false;}
  if(won&&i+1<g->history_count)return false;
 }
 if(g->score!=g->moves||(g->status==NG_WON)!=won)return false;
 if(g->status==NG_PLAYING&&g->moves>=(unsigned)g->data[1])return false;
 if(g->status==NG_LOST&&g->moves!=(unsigned)g->data[1])return false;
 return g->scroll<=(g->history_count>4?g->history_count-4:0);
}

static void render_equation(const NgGame *g,NgCanvas *c){
 char text[64];snprintf(text,sizeof(text),"%d CHARACTERS    TRY %u/%d",(int)g->data[0],(unsigned)g->moves,(int)g->data[1]);ng_text(c,12,30,text,NG_BLUE,1);
 unsigned n=(unsigned)g->data[0];for(unsigned row=0;row<4&&row+g->scroll<g->history_count;row++){
  const char *s=g->history[row+g->scroll];int step=n>8?35:38;for(unsigned i=0;i<n;i++){int state=s[n+1+i]-'0';int color=state==2?NG_GREEN:state==1?NG_YELLOW:NG_LINE;char ch[]={s[i],0};int x=(n>8?(396-(int)n*step)/2:26)+(int)i*step,y=48+(int)row*23;ng_rect(c,x,y,32,21,color);ng_center(c,x,y+5,32,ch,state==2?NG_WHITE:NG_INK,1);}
 }
 if(!g->history_count){ng_text(c,30,72,"Match the hidden text exactly.",NG_MUTED,1);ng_text(c,30,94,"Equivalent values alone do not win.",NG_MUTED,1);ng_small(c,30,121,"= : SHIFT+DOT",NG_BLUE);}
 ng_list_scrollbar(c,g->history_count,4,g->scroll,378,43,56,67,129);
 text_input(g,c,150);
}
static void init_mind(NgGame *g){
 g->puzzle_id=bank_index(g,1);MindView p=mind_pack(g);g->data[0]=(int32_t)p.n;g->data[1]=(int32_t)p.alphabet;g->data[2]=(int32_t)p.count;
 for(unsigned i=0;i<p.count;i++){for(unsigned j=0;j<p.n;j++)g->board[i*p.n+j]=p.clues[i*p.stride+j];g->data[16+i]=p.matches[i];}
 ng_message(g,"Only exact-position matches are counted.");
}
static bool action_mind(NgGame *g,int key){
 unsigned n=(unsigned)g->data[0],count=(unsigned)g->data[2];
 if(key==NGK_UP){if(g->scroll)g->scroll--;return true;}if(key==NGK_DOWN){if((unsigned)g->scroll+6<count)g->scroll++;return true;}
 if(g->status!=NG_PLAYING)return false;
 if(hint(key)){
  unsigned col=(unsigned)g->data[3]++%n,mask=(1u<<g->data[1])-1u;
  for(unsigned i=0;i<count;i++)if(!g->data[16+i])mask&=~(1u<<g->board[i*n+col]);
  char allowed[11];unsigned k=0;for(unsigned v=0;v<(unsigned)g->data[1];v++)if(mask&(1u<<v))allowed[k++]=(char)('0'+v);allowed[k]=0;
  snprintf(g->message,sizeof(g->message),"Zero-match clues: position %u permits %s.",col+1,allowed);g->data[3]%= (int)n;g->assisted=1;return true;
 }
 if(!submit(key)){char allowed[11];for(unsigned i=0;i<(unsigned)g->data[1];i++)allowed[i]=(char)('0'+i);allowed[g->data[1]]=0;return ng_edit(g,key,allowed,n);}
 if(strlen(g->input)!=n){ng_message(g,"Enter a complete code.");return true;}
 for(unsigned i=0;i<count;i++){unsigned same=0;for(unsigned j=0;j<n;j++)same+=(g->input[j]-'0')==g->board[i*n+j];if(same!=(unsigned)g->data[16+i]){snprintf(g->message,sizeof(g->message),"Clue %u needs %d exact matches; yours has %u.",i+1,(int)g->data[16+i],same);++g->moves;return true;}}
 ++g->moves;g->status=NG_WON;ng_message(g,"Every public clue is satisfied.");return true;
}
static bool valid_mind(const NgGame *g){
 if(g->status>NG_WON||!bank_valid(g,1))return false;
 MindView p=mind_pack(g);if(g->data[0]!=(int)p.n||g->data[1]!=(int)p.alphabet||g->data[2]!=(int)p.count||g->data[3]<0||g->data[3]>=(int)p.n||strlen(g->input)>p.n||g->scroll>(p.count>6?p.count-6:0))return false;
 for(unsigned i=0;i<p.count;i++){if(g->data[16+i]!=p.matches[i])return false;
 for(unsigned j=0;j<p.n;j++)if(g->board[i*p.n+j]!=p.clues[i*p.stride+j])return false;}
 for(unsigned i=0;g->input[i];i++)if(g->input[i]<'0'||g->input[i]>='0'+(int)p.alphabet)return false;
 if(g->status==NG_WON){
  if(!g->moves||strlen(g->input)!=p.n)return false;
  for(unsigned i=0;i<p.count;i++){unsigned matches=0;for(unsigned j=0;j<p.n;j++)matches+=g->input[j]-'0'==g->board[i*p.n+j];if(matches!=(unsigned)g->data[16+i])return false;}
 }
 return true;
}
static void render_mind(const NgGame *g,NgCanvas *c){
 char s[64];snprintf(s,sizeof(s),"%d DIGITS  |  0..%d, REPEATS ALLOWED",(int)g->data[0],(int)g->data[1]-1);ng_text(c,10,30,s,NG_BLUE,1);
 for(unsigned i=0;i<6&&g->scroll+i<(unsigned)g->data[2];i++){unsigned row=g->scroll+i;char code[7];number_code(g,code,row*(unsigned)g->data[0],(unsigned)g->data[0]);snprintf(s,sizeof(s),"%2u    %s      %d EXACT",row+1,code,(int)g->data[16+row]);ng_text(c,32,49+(int)i*16,s,NG_INK,1);}
 ng_list_scrollbar(c,(unsigned)g->data[2],6,g->scroll,375,42,52,75,133);
 text_input(g,c,152);
}
static bool lock_satisfies(int x,const int32_t *p){
 if(x<p[0]||x>p[1]||(p[2]>=0&&x%2!=p[2]))return false;
 int sum=0,y=x;bool contains=false;do{sum+=y%10;if(y%10==p[8])contains=true;y/=10;}while(y);
 return (p[3]<0||sum==p[3])&&x%p[4]==p[5]&&x%p[6]==p[7]&&(p[8]<0||contains)&&(!p[10]||x%p[10]==p[11]);
}
static void init_lock(NgGame *g){
 g->puzzle_id=bank_index(g,1);
 if(g->difficulty==3){const GcMasterLockPack *p=&gc_master_lock[g->puzzle_id-90];for(unsigned i=0;i<9;i++)g->data[i]=p->p[i];g->data[9]=9999;g->data[10]=p->p[9];g->data[11]=p->p[10];}
 else {const GcLockPack *p=&gc_lock_pack[g->puzzle_id];for(unsigned i=0;i<9;i++)g->data[i]=p->p[i];g->data[9]=p->limit;}
 ng_message(g,"All clues are visible from the start.");
}
static bool action_lock(NgGame *g,int key){
 if(g->status!=NG_PLAYING)return false;
 if(hint(key)){
  int first=-1,count=0;for(int x=g->data[0];x<=g->data[1];x++)if(x%g->data[4]==g->data[5]){if(first<0)first=x;++count;}
  snprintf(g->message,sizeof(g->message),"First MOD clue: start %d, add %d (%d candidates).",first,(int)g->data[4],count);g->assisted=1;return true;
 }
 if(!submit(key))return ng_edit(g,key,"0123456789",g->difficulty==3?4:3);
 int value;if(!integer_input(g->input,&value)){ng_message(g,"Enter an integer in the stated range.");return true;}
 ++g->moves;if(lock_satisfies(value,g->data)){g->status=NG_WON;ng_message(g,"All range, digit and remainder clues hold.");}else ng_message(g,"At least one visible clue is not satisfied.");return true;
}
static bool valid_lock(const NgGame *g){
 if(g->status>NG_WON||!bank_valid(g,1)||!input_ok(g,"0123456789",g->difficulty==3?4:3))return false;
 if(g->difficulty==3){const GcMasterLockPack *p=&gc_master_lock[g->puzzle_id-90];for(unsigned i=0;i<9;i++)if(g->data[i]!=p->p[i])return false;if(g->data[9]!=9999||g->data[10]!=p->p[9]||g->data[11]!=p->p[10])return false;}
 else {const GcLockPack *p=&gc_lock_pack[g->puzzle_id];for(unsigned i=0;i<9;i++)if(g->data[i]!=p->p[i])return false;if(g->data[9]!=p->limit||g->data[10]||g->data[11])return false;}
 if(g->status==NG_WON){int value;if(!g->moves||!integer_input(g->input,&value)||!lock_satisfies(value,g->data))return false;}
 return true;
}
static void render_lock(const NgGame *g,NgCanvas *c){
 char s[80];int y=32;snprintf(s,sizeof(s),"DOMAIN 0..%d     RANGE %d..%d",(int)g->data[9],(int)g->data[0],(int)g->data[1]);ng_text(c,12,y,s,NG_BLUE,1);y+=23;
 if(g->data[2]>=0){snprintf(s,sizeof(s),"The number is %s.",(int)g->data[2]?"ODD":"EVEN");ng_text(c,24,y,s,NG_INK,1);y+=18;}
 if(g->data[3]>=0){snprintf(s,sizeof(s),"Sum of decimal digits = %d",(int)g->data[3]);ng_text(c,24,y,s,NG_INK,1);y+=18;}
 for(unsigned i=4;i<=6;i+=2){snprintf(s,sizeof(s),"Number MOD %d = %d",(int)g->data[i],(int)g->data[i+1]);ng_text(c,24,y,s,NG_INK,1);y+=18;}
 if(g->data[10]){snprintf(s,sizeof(s),"Number MOD %d = %d",(int)g->data[10],(int)g->data[11]);ng_text(c,24,y,s,NG_INK,1);y+=18;}
 if(g->data[8]>=0){snprintf(s,sizeof(s),"Contains digit %d",(int)g->data[8]);ng_text(c,24,y,s,NG_INK,1);}
 text_input(g,c,151);
}
static const char *family(unsigned f){static const char *const names[]={"ARITHMETIC","GEOMETRIC","QUADRATIC","FIBONACCI-LIKE","ALTERNATING","RECURRENCE","TWO-TERM RECURRENCE","TWO DIFFERENT STEPS"};return f<8?names[f]:"?";}
static int sequence_next(const NgGame *g){
 switch(g->data[0]){
 case 0:return g->board[5]+g->data[2];case 1:return g->board[5]*g->data[2];case 2:return g->data[1]+g->data[2]*6+g->data[3]*36;
 case 3:return g->board[4]+g->board[5];case 4:return g->board[4]+g->data[3];case 6:return g->data[1]*g->board[5]+g->data[2]*g->board[4]+g->data[3];case 7:return g->board[4]+g->data[1];default:return g->data[2]*g->board[5]+g->data[3];}
}
static void init_sequence(NgGame *g){g->puzzle_id=bank_index(g,1);const GcSequencePack *p=sequence_pack(g);for(unsigned i=0;i<6;i++)g->board[i]=p->seq[i];g->data[0]=p->family;for(unsigned i=0;i<3;i++)g->data[1+i]=p->p[i];ng_message(g,"Find the next term within the listed grammar.");}
static void sequence_explanation(NgGame *g){
 int a=g->data[1],b=g->data[2],c=g->data[3],next=sequence_next(g);
 switch(g->data[0]){
 case 0:snprintf(g->message,sizeof(g->message),"Add %d: %d + (%d) = %d.",b,g->board[5],b,next);break;
 case 1:snprintf(g->message,sizeof(g->message),"Multiply by %d: %d * (%d) = %d.",b,g->board[5],b,next);break;
 case 2:snprintf(g->message,sizeof(g->message),"n starts at 0: %d + %dn + %dn*n; n=6 gives %d.",a,b,c,next);break;
 case 3:snprintf(g->message,sizeof(g->message),"Add previous two: %d + %d = %d.",g->board[4],g->board[5],next);break;
 case 4:snprintf(g->message,sizeof(g->message),"Alternating lanes each add %d: %d + %d = %d.",c,g->board[4],c,next);break;
 case 6:snprintf(g->message,sizeof(g->message),"Next = %d*(%d) + %d*(%d) + (%d) = %d.",a,g->board[5],b,g->board[4],c,next);break;
 case 7:snprintf(g->message,sizeof(g->message),"Odd/even lanes add %d / %d: %d + (%d) = %d.",a,b,g->board[4],a,next);break;
 default:snprintf(g->message,sizeof(g->message),"Next = %d * previous + (%d): %d.",b,c,next);break;
 }
}
static bool action_sequence(NgGame *g,int key){
 if(g->status!=NG_PLAYING)return false;
 if(hint(key)){g->assisted=1;g->data[8]=1;snprintf(g->message,sizeof(g->message),"Rule family: %s.",family((unsigned)(int)g->data[0]));return true;}
 if(!submit(key))return ng_edit(g,key,"-0123456789",6);
 int answer;if(!integer_input(g->input,&answer)){ng_message(g,"Enter one signed integer.");return true;}
 ++g->moves;if(answer==sequence_next(g)){g->status=NG_WON;sequence_explanation(g);}else ng_message(g,"That next term does not fit the allowed rules.");return true;
}
static bool valid_sequence(const NgGame *g){
 if(g->status>NG_WON||!bank_valid(g,1)||g->data[8]<0||g->data[8]>1||!input_ok(g,"-0123456789",6))return false;
 const GcSequencePack *p=sequence_pack(g);if(g->data[0]!=p->family)return false;
 for(unsigned i=0;i<6;i++)if(g->board[i]!=p->seq[i])return false;
 for(unsigned i=0;i<3;i++)if(g->data[i+1]!=p->p[i])return false;
 if(g->status==NG_WON){int value;if(!g->moves||!integer_input(g->input,&value)||value!=sequence_next(g))return false;}
 return true;
}
static void sequence_arrow(NgCanvas *c,int x1,int x2,int y){
 ng_line(c,x1,y,x2,y,NG_MUTED);
 ng_line(c,x2-3,y-2,x2,y,NG_MUTED);ng_line(c,x2-3,y+2,x2,y,NG_MUTED);
}
static void render_sequence(const NgGame *g,NgCanvas *c){
 ng_text(c,13,30,"NEXT TERM",NG_BLUE,1);
 for(int y=67;y<=106;y+=39){sequence_arrow(c,132,138,y);sequence_arrow(c,258,264,y);}
 ng_line(c,383,67,389,67,NG_MUTED);ng_line(c,389,67,389,86,NG_MUTED);
 ng_line(c,389,86,7,86,NG_MUTED);ng_line(c,7,86,7,106,NG_MUTED);
 sequence_arrow(c,7,12,106);
 for(unsigned i=0;i<6;i++){char s[16];snprintf(s,sizeof(s),"%d",g->board[i]);int x=14+(int)(i%3)*126,y=51+(int)(i/3)*39;ng_card(c,x,y,116,32,s,false,false);}
 if(g->data[8]||g->status==NG_WON)ng_center(c,10,131,376,family((unsigned)g->data[0]),NG_BLUE,1);else ng_center(c,10,131,376,"Finite grammar; see RULES.",NG_MUTED,1);
 if(g->status==NG_WON){
  char row[65];size_t len=strlen(g->message);size_t first=len>62?62:len;
  memcpy(row,g->message,first);row[first]=0;ng_small_expression(c,12,149,row,NG_BLUE);
  if(len>first)ng_small_expression(c,12,162,g->message+first,NG_BLUE);
 }else text_input(g,c,151);
}
static const GcCardPack *card_pack(const NgGame *g){
 if(g->id==7&&g->pack_revision==4&&g->difficulty==2)return &gc_countdown_beta4[g->puzzle_id-120];
 if(g->difficulty==3)return g->id==6?&gc_master_target[g->puzzle_id-180]:&gc_master_countdown[g->puzzle_id-90];
 return g->id==6?&gc_target_pack[g->puzzle_id]:&gc_countdown_pack[g->puzzle_id];
}
/* Revision 3 constructs a guaranteed solvable deck for every requested target.
 * The witness demonstrates solvability only; input is accepted by exact rules. */
static unsigned target_rand(uint32_t *state,unsigned limit){
 uint32_t x=*state;x^=x<<13;x^=x>>17;x^=x<<5;*state=x;return limit?x%limit:0;
}
static uint32_t target_deck(uint32_t seed,unsigned difficulty,unsigned target,int16_t *cards,char *answer,char *tip){
 uint32_t rng=seed?seed:1;unsigned a,b,c,d,e=0,f=0,n=difficulty<2?4:difficulty+3;
 if(difficulty==0){
  b=2+target_rand(&rng,11);a=target/b+1;c=2+target_rand(&rng,9);d=c+a*b-target;
  snprintf(answer,40,"%u*%u+%u-%u",a,b,c,d);
 }else if(difficulty==1){
  d=2+target_rand(&rng,8);a=d+2+target_rand(&rng,18);b=target*d/a+1;c=a*b-target*d;
  snprintf(answer,40,"(%u*%u-%u)/%u",a,b,c,d);
 }else if(difficulty==2){
  d=2+target_rand(&rng,8);e=2+target_rand(&rng,8);a=d+e+2+target_rand(&rng,18);b=target*(d+e)/a+1;c=a*b-target*(d+e);
  snprintf(answer,40,"(%u*%u-%u)/(%u+%u)",a,b,c,d,e);
 }else{
  f=2+target_rand(&rng,8);unsigned k=2+target_rand(&rng,8);d=f*k;e=1+target_rand(&rng,f-1);
  a=d+2+target_rand(&rng,18);unsigned numerator=target*d-e*k;b=numerator/a+1;c=a*b-numerator;
  snprintf(answer,40,"(%u*%u-%u)/%u+%u/%u",a,b,c,d,e,f);
 }
 cards[0]=(int16_t)a;cards[1]=(int16_t)b;cards[2]=(int16_t)c;cards[3]=(int16_t)d;
 if(n>4)cards[4]=(int16_t)e;
 if(n>5)cards[5]=(int16_t)f;
 if(tip)snprintf(tip,64,"One first step: multiply %u by %u.",a,b);
 for(unsigned i=n-1;i;i--){unsigned j=target_rand(&rng,i+1);int16_t tmp=cards[i];cards[i]=cards[j];cards[j]=tmp;}
 return rng;
}
/* Revision4 stores only host-verified candidate indices. Every index/target/
 * level was exhaustively graded; this construction performs no device search. */
static uint32_t target_deck_v4(unsigned index,unsigned difficulty,unsigned target,int16_t *cards,char *answer,char *tip){
 uint32_t rng=UINT32_C(0xb4a61e37)^target*UINT32_C(0x9e3779b9)^difficulty*UINT32_C(0x85ebca6b)^index*UINT32_C(0xc2b2ae35);
 if(!rng)rng=1;
 if(!difficulty)return target_deck(rng,0,target,cards,answer,tip);
 unsigned a,b,c,d,e=0,f=0,n=difficulty<2?4:difficulty+3;
 if(difficulty==1){
  unsigned limit=32767/target;if(limit>200)limit=200;
  unsigned total=4+target_rand(&rng,limit-3);c=2+target_rand(&rng,total-3);d=total-c;
  a=2+target_rand(&rng,target*total-3);b=target*total-a;
  snprintf(answer,40,"(%u+%u)/(%u+%u)",a,b,c,d);
  if(tip)snprintf(tip,64,"One first step: add %u and %u.",a,b);
 }else if(difficulty==2){
  unsigned limit=32767/target;if(limit>128)limit=128;
  unsigned denominator=2+target_rand(&rng,limit-1);c=12+target_rand(&rng,52);d=12+target_rand(&rng,52);e=c*d-denominator;
  a=1+target_rand(&rng,target*denominator-1);b=target*denominator-a;
  snprintf(answer,40,"(%u+%u)/(%u*%u-%u)",a,b,c,d,e);
  if(tip)snprintf(tip,64,"One first step: multiply %u by %u.",c,d);
 }else{
  unsigned unit=2+target_rand(&rng,30),gap=67+target_rand(&rng,133),factor=29+target_rand(&rng,99),k=2+target_rand(&rng,2);
  unsigned low=factor*gap>32767/k?factor*gap-32767/k:1;
  c=low+target_rand(&rng,factor*gap-low);if(c%gap==0)++c;
  a=target*unit;b=unit+gap;d=factor;e=(factor*gap-c)*k;f=gap*k;
  snprintf(answer,40,"%u/(%u-%u/(%u-%u/%u))",a,b,c,d,e,f);
  if(tip)snprintf(tip,64,"One first step: divide %u by %u.",e,f);
 }
 cards[0]=(int16_t)a;cards[1]=(int16_t)b;cards[2]=(int16_t)c;cards[3]=(int16_t)d;
 if(n>4)cards[4]=(int16_t)e;
 if(n>5)cards[5]=(int16_t)f;
 for(unsigned i=n-1;i;i--){unsigned j=target_rand(&rng,i+1);int16_t swap=cards[i];cards[i]=cards[j];cards[j]=swap;}
 return rng;
}
/* Revision5 candidates keep every literal in 1..999. Only host-solved indices
 * are embedded; no clamping or approximate grading occurs on the device. */
static uint32_t target_deck_v5(unsigned index,unsigned difficulty,unsigned target,int16_t *cards,char *answer,char *tip){
 uint32_t rng=UINT32_C(0xb5a61e37)^target*UINT32_C(0x9e3779b9)^difficulty*UINT32_C(0x85ebca6b)^index*UINT32_C(0xc2b2ae35);
 if(!rng)rng=1;
 if(!difficulty)return target_deck(rng,0,target,cards,answer,tip);
 unsigned a=0,b=0,c=0,d=0,e=0,f=0,n=difficulty<2?4:difficulty+3;
 if(difficulty==1){
  unsigned variant=index%3;
  if(!variant&&target<=999){
   unsigned maximum=1998/target;if(maximum>48)maximum=48;
   if(maximum>=2){
    unsigned total=2+target_rand(&rng,maximum-1);
    c=1+target_rand(&rng,total-1);d=total-c;
    unsigned low=target*total>999?target*total-999:1;
    unsigned high=target*total-1;if(high>999)high=999;
    a=low+target_rand(&rng,high-low+1);b=target*total-a;
    snprintf(answer,40,"(%u+%u)/(%u+%u)",a,b,c,d);
    if(tip)snprintf(tip,64,"One first step: add %u and %u.",a,b);
   }
  }
  if(!a){
   unsigned total;
   if(variant==2){c=1+target_rand(&rng,12);d=1+target_rand(&rng,12);total=c*d;}
   else{total=2+target_rand(&rng,31);c=1+target_rand(&rng,total-1);d=total-c;}
   unsigned product=target*total,count=0;
   for(unsigned v=2;v<=999&&v<=product;v++)if(!(product%v)&&product/v<=999)count++;
   if(!count)return 0;
   unsigned pick=target_rand(&rng,count);
   for(unsigned v=2;v<=999&&v<=product;v++)if(!(product%v)&&product/v<=999){if(!pick--){a=v;break;}}
   b=product/a;
   snprintf(answer,40,variant==2?"(%u*%u)/(%u*%u)":"(%u*%u)/(%u+%u)",a,b,c,d);
   if(tip)snprintf(tip,64,"One first step: multiply %u by %u.",a,b);
  }
 }else if(difficulty==2){
  if(index%2)return target_deck(rng,2,target,cards,answer,tip);
  unsigned bound=1998/target;if(bound<2)bound=2;if(bound>48)bound=48;
  unsigned denominator=2+target_rand(&rng,bound-1);
  c=12+target_rand(&rng,52);d=12+target_rand(&rng,52);e=c*d-denominator;
  unsigned total=target*denominator;
  unsigned low=total>999?total-999:1,high=total-1;if(high>999)high=999;
  if(low>high||e<1||e>999)return 0;
  a=low+target_rand(&rng,high-low+1);b=total-a;
  snprintf(answer,40,"(%u+%u)/(%u*%u-%u)",a,b,c,d,e);
  if(tip)snprintf(tip,64,"One first step: multiply %u by %u.",c,d);
 }else{
  bool half=target==1000;
  unsigned gap=7+target_rand(&rng,30);d=7+target_rand(&rng,40);unsigned k=2+target_rand(&rng,5);
  unsigned low,high;
  if(half){
   b=1+gap;unsigned factor=2*b-1;
   low=d*factor>999/k?(d*factor-999/k+1)/2:1;
   high=(d*factor-1)/2;if(high>999)high=999;
   if(low>high)return 0;
   c=low+target_rand(&rng,high-low+1);e=k*(d*factor-2*c);f=k*factor;a=500;
  }else{
   low=d*gap>999/k?d*gap-999/k:1;
   high=d*gap-1;if(high>999)high=999;
   if(low>high)return 0;
   c=low+target_rand(&rng,high-low+1);e=k*(d*gap-c);f=k*gap;a=target;b=1+gap;
  }
  snprintf(answer,40,"%u/(%u-%u/(%u-%u/%u))",a,b,c,d,e,f);
  if(tip)snprintf(tip,64,"One first step: divide %u by %u.",e,f);
 }
 unsigned values[6]={a,b,c,d,e,f};
 for(unsigned i=0;i<n;i++)if(values[i]<1||values[i]>999)return 0;
 for(unsigned i=0;i<n;i++)cards[i]=(int16_t)values[i];
 for(unsigned i=n-1;i;i--){unsigned j=target_rand(&rng,i+1);int16_t swap=cards[i];cards[i]=cards[j];cards[j]=swap;}
 return rng;
}
static int target_beta6_slot(unsigned target)
{
 static const unsigned choices[]={10,24,50,100,200};
 for(unsigned i=0;i<5;i++)if(choices[i]==target)return (int)i;
 return -1;
}
bool gc_target_init(NgGame *g,unsigned target){
 if(!g||g->id!=6||g->difficulty>3||g->mode||(g->pack_revision<3||g->pack_revision>6))return false;
 int slot=target_beta6_slot(target);
 if(g->pack_revision==6){if(target!=NG_TARGET_RANDOM && slot<0)return false;}
 else if(target<1||target>1000)return false;
 memset(g->board,0,sizeof(g->board));memset(g->data,0,sizeof(g->data));memset(g->fixed,0,sizeof(g->fixed));memset(g->notes,0,sizeof(g->notes));
 memset(g->history,0,sizeof(g->history));memset(g->input,0,sizeof(g->input));
 g->status=NG_PLAYING;g->phase=g->cursor=g->history_count=g->scroll=g->notes_mode=g->cpu_pending=0;
 g->moves=g->score=0;g->puzzle_id=g->seed;g->data[0]=(int32_t)target;g->data[1]=(int32_t)(g->difficulty<2?4:g->difficulty+3);g->data[3]=1000000000;
 if(g->pack_revision==6){
  unsigned count=slot<0?1000u:200u;
  g->rng=g->seed;
  unsigned ordinal=ng_bank_pick(g,count);
  unsigned record=g->difficulty*1000u+(slot<0?ordinal:(unsigned)slot*200u+ordinal);
  const GcBeta6CardRecord *p=&gc_target_beta6[record];
  g->puzzle_id=16240u+record;g->data[0]=p->target;
  g->data[2]=slot<0?1:0;g->data[4]=slot<0?0:(int32_t)target;
  for(unsigned i=0;i<(unsigned)g->data[1];i++)g->board[i]=p->cards[i];
  ng_message(g,"Use EVERY card exactly once. Fractions are allowed.");return true;
 }
 char answer[40];
 if(g->pack_revision>=4){
  g->rng=g->seed;unsigned ordinal=ng_bank_pick(g,2),record=(g->difficulty*1000+target-1)*2+ordinal;
  if(g->pack_revision==5){g->puzzle_id=8240+record;g->rng=target_deck_v5(gc_target_beta5_index[record],g->difficulty,target,g->board,answer,NULL);}
  else{g->puzzle_id=240+record;g->rng=target_deck_v4(gc_target_beta4_index[record],g->difficulty,target,g->board,answer,NULL);}
 }else g->rng=target_deck(g->seed,g->difficulty,target,g->board,answer,NULL);
 ng_message(g,"Use EVERY card exactly once. Fractions are allowed.");return true;
}
static void init_cards(NgGame *g){
 if(g->id==6&&g->pack_revision>=3){(void)gc_target_init(g,24);return;}
 if(g->id==7&&g->pack_revision>=5){
  unsigned index=g->difficulty*200u+ng_bank_pick(g,200);
  const GcBeta6CardRecord *p=&gc_countdown_beta6[index];
  g->puzzle_id=200u+index;
  for(unsigned i=0;i<6;i++)g->board[i]=p->cards[i];
  g->data[0]=p->target;g->data[1]=6;g->data[3]=1000000000;
  ng_message(g,"Cards at most once; positive integer steps only.");return;
 }
 g->puzzle_id=bank_index(g,g->id==6?2:1);
 const GcCardPack *p=card_pack(g);unsigned n=g->id==6?4:6;for(unsigned i=0;i<n;i++)g->board[i]=p->cards[i];g->data[0]=p->target;g->data[1]=(int)n;g->data[3]=1000000000;
 ng_message(g,g->id==6?"Use every card exactly once.":"Cards at most once; positive integer steps only.");
}
static bool action_cards(NgGame *g,int key){
 if(g->status!=NG_PLAYING)return false;
 char answer[97],tip[64];const GcCardPack *p=NULL;
 if(g->id==6&&g->pack_revision>=3){
  if(g->pack_revision==6){
   const GcBeta6CardRecord *r=&gc_target_beta6[g->puzzle_id-16240u];
   snprintf(answer,sizeof answer,"%s",gc_target_beta6_answer[g->difficulty]+r->answer_offset);
   snprintf(tip,sizeof tip,"One first step: %d %c %d.",g->board[r->hint_a],r->hint_op,g->board[r->hint_b]);
  }else{int16_t cards[6];if(g->pack_revision==5)(void)target_deck_v5(gc_target_beta5_index[g->puzzle_id-8240],g->difficulty,(unsigned)g->data[0],cards,answer,tip);else if(g->pack_revision==4)(void)target_deck_v4(gc_target_beta4_index[g->puzzle_id-240],g->difficulty,(unsigned)g->data[0],cards,answer,tip);else (void)target_deck(g->seed,g->difficulty,(unsigned)g->data[0],cards,answer,tip);}
 }
 else if(g->id==7&&g->pack_revision>=5){
  const GcBeta6CardRecord *r=&gc_countdown_beta6[g->puzzle_id-200u];
  snprintf(answer,sizeof answer,"%s",gc_countdown_beta6_answer+r->answer_offset);
  snprintf(tip,sizeof tip,"One first step: %d %c %d.",g->board[r->hint_a],r->hint_op,g->board[r->hint_b]);
 }
 else {p=card_pack(g);snprintf(answer,sizeof(answer),"%s",p->answer);snprintf(tip,sizeof(tip),"%s",p->hint);}
 if(hint(key)){ng_message(g,tip);g->assisted=1;return true;}
 if(auxiliary(key)){
  if(g->id==6||key==NGK_ANSWER){snprintf(g->input,sizeof(g->input),"%s",answer);g->assisted=1;ng_message(g,"Example answer copied. EXE checks it.");}
  else if(g->data[3]==1000000000)ng_message(g,"CHECK a valid expression before FINISH.");
  else {g->status=g->data[3]?NG_DRAW:NG_WON;snprintf(g->message,sizeof(g->message),"Best submitted distance: %d. Score: %u.",(int)g->data[3],(unsigned)g->score);}
  return true;
 }
 if(!submit(key))return ng_edit(g,key,"0123456789+-*/()",96);
 GcExpression e;if(!gc_expression(g->input,g->id==7,&e)){ng_message(g,g->id==7?"Invalid: every step must be a positive integer.":"Invalid or oversized expression / zero divisor.");return true;}
 if(!gc_cards(&e,g->board,(unsigned)g->data[1],g->id==6)){ng_message(g,g->id==6?"Use ALL card values exactly once each.":"Use each card at most once; no extra constants.");return true;}
 ++g->moves;
 if(g->id==6){if(e.value.num==(int64_t)g->data[0]*e.value.den){g->status=NG_WON;ng_message(g,"Exact target reached with every card!");}else{snprintf(g->message,sizeof(g->message),"Value %ld/%ld; target %d.",(long)e.value.num,(long)e.value.den,(int)g->data[0]);}return true;}
 int64_t distance=e.value.num-g->data[0];if(distance<0)distance=-distance;
 if(distance<g->data[3]){g->data[3]=(int32_t)distance;g->data[4]=(int32_t)e.value.num;g->score=distance==0?10:distance<=5?7:distance<=10?5:0;}
 if(!distance){g->status=NG_WON;ng_message(g,"Exact target: 10 points.");}
 else snprintf(g->message,sizeof(g->message),"Distance %ld; best %d. Keep trying or FINISH.",(long)distance,(int)g->data[3]);
 return true;
}
static bool valid_cards(const NgGame *g){
 if(!input_ok(g,"0123456789+-*/()",96)||(g->id==6?g->status>NG_WON:(g->status!=NG_PLAYING&&g->status!=NG_WON&&g->status!=NG_DRAW)))return false;
 unsigned n=g->id==6?4:6;
 if(g->id==6&&g->pack_revision>=3){
  if(g->mode||g->difficulty>3||g->data[0]<1||g->data[0]>1000)return false;
  n=g->difficulty<2?4:g->difficulty+3;int16_t cards[6]={0};char answer[40];
  if(g->pack_revision==6){
   unsigned first=16240u+g->difficulty*1000u;
   if(g->puzzle_id<first||g->puzzle_id>=first+1000u)return false;
   unsigned local=g->puzzle_id-first;
   const GcBeta6CardRecord *p=&gc_target_beta6[g->puzzle_id-16240u];
   if(g->data[0]!=p->target||g->data[2]<0||g->data[2]>1)return false;
   if(g->data[2]){if(g->data[4]||g->supply_index>=1000u)return false;}
   else {int slot=target_beta6_slot((unsigned)g->data[4]);
    if(slot<0||local/200u!=(unsigned)slot||g->supply_index>=200u)return false;
   }
   for(unsigned i=0;i<n;i++)cards[i]=p->cards[i];
  }else if(g->pack_revision>=4){
   unsigned base=g->pack_revision==5?8240:240;
   unsigned first=base+(g->difficulty*1000+(unsigned)g->data[0]-1)*2;
   if(g->puzzle_id<first||g->puzzle_id>=first+2)return false;
   uint32_t rng=g->pack_revision==5?target_deck_v5(gc_target_beta5_index[g->puzzle_id-8240],g->difficulty,(unsigned)g->data[0],cards,answer,NULL):target_deck_v4(gc_target_beta4_index[g->puzzle_id-240],g->difficulty,(unsigned)g->data[0],cards,answer,NULL);
   if(!rng||rng!=g->rng)return false;
  }else if(g->pack_revision!=3||g->puzzle_id!=g->seed||target_deck(g->seed,g->difficulty,(unsigned)g->data[0],cards,answer,NULL)!=g->rng)return false;
  for(unsigned i=0;i<NG_CELLS;i++)if(g->board[i]!=(i<n?cards[i]:0)||g->fixed[i]||g->notes[i])return false;
  for(unsigned i=2;i<NG_DATA;i++){
   if(i==3){if(g->data[i]!=1000000000)return false;}
   else if(g->pack_revision!=6 || (i!=2 && i!=4)){if(g->data[i])return false;}
  }
  if(g->rows||g->cols||g->phase||g->cursor||g->history_count||g->scroll||g->notes_mode||g->cpu_pending||g->score)return false;
 }else if(g->id==7&&g->pack_revision>=5){
  unsigned first=200u+g->difficulty*200u;
  if(g->puzzle_id<first||g->puzzle_id>=first+200u)return false;
  const GcBeta6CardRecord *p=&gc_countdown_beta6[g->puzzle_id-200u];
  if(g->data[0]!=p->target)return false;
  for(unsigned i=0;i<NG_CELLS;i++)if(g->board[i]!=(i<6?p->cards[i]:0)||g->fixed[i]||g->notes[i])return false;
  for(unsigned i=2;i<NG_DATA;i++)if(i!=3&&i!=4&&g->data[i])return false;
  if(g->rows||g->cols||g->phase||g->cursor||g->history_count||g->scroll||g->notes_mode||g->cpu_pending)return false;
 }else{
  if(!bank_valid(g,g->id==6?2:1))return false;
  const GcCardPack *p=card_pack(g);if(g->data[0]!=p->target)return false;
  for(unsigned i=0;i<n;i++)if(g->board[i]!=p->cards[i])return false;
 }
 if(g->data[1]!=(int)n||g->data[3]<0||g->data[3]>1000000000||g->data[4]<0||g->data[4]>1000000000)return false;
 if(g->status==NG_WON){GcExpression e;if(!g->moves||!gc_expression(g->input,g->id==7,&e)||!gc_cards(&e,g->board,n,g->id==6)||e.value.num!=(int64_t)g->data[0]*e.value.den)return false;}
 if(g->id==7){
  if(g->data[3]==1000000000){if(g->data[4]||g->score||g->status!=NG_PLAYING)return false;}
  else {int64_t distance=(int64_t)g->data[4]-g->data[0];if(distance<0)distance=-distance;
   unsigned score=distance==0?10:distance<=5?7:distance<=10?5:0;
   if(!g->moves||g->data[4]<=0||distance!=g->data[3]||score!=g->score||(!distance&&g->status!=NG_WON)||(g->status==NG_WON&&distance)||(g->status==NG_DRAW&&!distance))return false;
  }
 }
 return true;
}
static unsigned card_usage(const NgGame *g){
 unsigned used=0;const char *s=g->input;
 while(*s){if(*s>='0'&&*s<='9'){unsigned v=0;while(*s>='0'&&*s<='9'){if(v<1000000)v=v*10+(unsigned)(*s-'0');++s;}for(unsigned i=0;i<(unsigned)g->data[1];i++)if(!(used&(1u<<i))&&v==(unsigned)g->board[i]){used|=1u<<i;break;}}else ++s;}
 return used;
}
static void render_cards(const NgGame *g,NgCanvas *c){
 char s[64];snprintf(s,sizeof(s),"TARGET  %d",(int)g->data[0]);ng_center(c,8,31,380,s,NG_BLUE,2);
 unsigned n=(unsigned)g->data[1],used=card_usage(g);int gap=7,w=(368-(int)(n-1)*gap)/(int)n;
 int row_width=(int)n*w+(int)(n-1)*gap,row_x=(396-row_width)/2;
 for(unsigned i=0;i<n;i++){snprintf(s,sizeof(s),"%d",g->board[i]);ng_card(c,row_x+(int)i*(w+gap),63,w,37,s,false,(used&(1u<<i))!=0);}
 ng_expression(c,12,116,g->id==6?"+ - * / ( )    ALL CARDS ONCE":"+ - * / ( )    POSITIVE INTEGER STEPS",NG_MUTED,1);
 if(g->id==7){if(g->data[3]<1000000000)snprintf(s,sizeof(s),"Best distance %d  |  Score %u",(int)g->data[3],(unsigned)g->score);else snprintf(s,sizeof(s),"Distance 0:10  1..5:7  6..10:5  else:0");ng_small(c,12,133,s,NG_BLUE);}
 text_input(g,c,151);
}
static bool operators_value(const NgGame *g,bool witness,GcExpression *e){
 char expr[64];size_t used=0;unsigned n=(unsigned)g->data[0];for(unsigned i=0;i<n;i++){
  int wrote=snprintf(expr+used,sizeof(expr)-used,"%d",g->board[i]);if(wrote<0||(size_t)wrote>=sizeof(expr)-used)return false;used+=(size_t)wrote;
  if(i+1<n){int op=witness?g->data[16+i]:g->notes[i];if(!op)return false;expr[used++]=(char)op;expr[used]=0;}
 }
 return gc_expression(expr,false,e);
}
static void init_operators(NgGame *g){
 g->data[0]=3+g->difficulty;g->cols=(uint8_t)(g->data[0]-1);g->rows=1;g->puzzle_id=g->seed;
 static const char ops[]="+-*/";GcExpression e;
 if(g->difficulty==3){
  g->puzzle_id=ng_bank_pick(g,30);const GcMasterOperatorsPack *p=&gc_master_operators[g->puzzle_id];
  for(unsigned i=0;i<6;i++)g->board[i]=p->numbers[i];
  for(unsigned i=0;i<5;i++)g->data[16+i]=ops[p->ops[i]];
  g->data[1]=p->target;ng_message(g,"Six numbers; precedence and three operator kinds.");return;
 }
 for(unsigned attempt=0;attempt<128;attempt++){
  for(unsigned i=0;i<(unsigned)g->data[0];i++)g->board[i]=(int16_t)(1+ng_rand(g,9));
  for(unsigned i=0;i<(unsigned)g->data[0]-1;i++)g->data[16+i]=ops[ng_rand(g,g->difficulty==0?3:4)];
  if(operators_value(g,true,&e)&&e.value.den==1&&e.value.num>=-999&&e.value.num<=999){g->data[1]=(int32_t)e.value.num;ng_message(g,"Select ? with arrows; type an operator.");return;}
 }
 for(unsigned i=0;i<(unsigned)g->data[0];i++)g->board[i]=(int16_t)(i+1);
 for(unsigned i=0;i<(unsigned)g->data[0]-1;i++)g->data[16+i]='+';
 (void)operators_value(g,true,&e);g->data[1]=(int32_t)e.value.num;
}
static bool action_operators(NgGame *g,int key){
 if(ng_grid_nav(g,key))return true;
 if(g->status!=NG_PLAYING)return false;
 if(auxiliary(key)){if(g->notes[g->cursor]!=(uint16_t)g->data[16+g->cursor]){g->notes[g->cursor]=(uint16_t)g->data[16+g->cursor];++g->moves;}g->assisted=1;ng_message(g,"Revealed this slot from one valid solution.");return true;}
 if(key==NGK_DEL){if(g->notes[g->cursor]){g->notes[g->cursor]=0;++g->moves;}return true;}
 if(key>0&&key<128&&strchr("+-*/",key)){if(g->notes[g->cursor]!=(uint16_t)key){g->notes[g->cursor]=(uint16_t)key;++g->moves;}return true;}
 if(!submit(key))return false;
 GcExpression e;if(!operators_value(g,false,&e)){ng_message(g,"Fill all operator slots; avoid zero divisors.");return true;}
 if(e.value.num==(int64_t)g->data[1]*e.value.den){g->status=NG_WON;ng_message(g,"Valid operators satisfy the target.");}else ng_message(g,"The equation does not reach the target.");return true;
}
static bool valid_operators(const NgGame *g){
 if(g->status>NG_WON||g->mode||g->difficulty>3||(g->difficulty==3&&g->pack_revision!=2)||g->data[0]!=3+g->difficulty||g->rows!=1||g->cols!=g->data[0]-1||g->cursor>=g->cols||g->data[1]<-999||g->data[1]>999)return false;
 if(g->difficulty==3){
  if(g->puzzle_id>=30)return false;
  const GcMasterOperatorsPack *p=&gc_master_operators[g->puzzle_id];static const char ops[]="+-*/";
  if(g->data[1]!=p->target)return false;
  for(unsigned i=0;i<6;i++)if(g->board[i]!=p->numbers[i])return false;
  for(unsigned i=0;i<5;i++)if(g->data[16+i]!=ops[p->ops[i]])return false;
 }
 for(unsigned i=0;i<(unsigned)g->data[0];i++){if(g->board[i]<1||g->board[i]>9)return false;if(i+1<(unsigned)g->data[0]){if(!op_ok(g->data[16+i])||(g->notes[i]&&!op_ok(g->notes[i])))return false;}}
 GcExpression e;
 if(g->status==NG_WON&&(!g->moves||!operators_value(g,false,&e)||e.value.num!=(int64_t)g->data[1]*e.value.den))return false;
 return operators_value(g,true,&e)&&e.value.den==1&&e.value.num==g->data[1];
}
static void render_operators(const NgGame *g,NgCanvas *c){
 ng_text(c,12,32,"KEEP NUMBER ORDER. STANDARD PRECEDENCE.",NG_BLUE,1);unsigned n=(unsigned)g->data[0];int step=n==6?33:n==5?38:n==4?48:58,x=14;
 for(unsigned i=0;i<n;i++){ng_number(c,x,87,g->board[i],NG_INK,2);x+=step;if(i+1<n){char s[]={(char)(g->notes[i]?g->notes[i]:'?'),0};operator_card(c,x-9,80,s,g->cursor==i);x+=step;}}
 char s[24];snprintf(s,sizeof(s),"= %d",(int)g->data[1]);center_expression(c,10,133,376,s,NG_BLUE,2);ng_small_expression(c,12,169,"Arrows: select slot    + - * /: set    DEL: clear",NG_MUTED);
}
static int eval3(int a,int b,int c,int op1,int op2){
 if(op1==2&&op2==2)return a*b*c;
 if(op1==2)return op2==0?a*b+c:a*b-c;
 if(op2==2)return op1==0?a+b*c:a-b*c;
 int v=op1==0?a+b:a-b;return op2==0?v+c:v-c;
}
static void init_cross(NgGame *g){
 g->puzzle_id=bank_index(g,1);const GcCrossPack *p=cross_pack(g);g->rows=g->cols=3;
 for(unsigned i=0;i<9;i++){g->board[i]=p->givens[i];g->fixed[i]=p->givens[i]!=0;}for(unsigned i=0;i<12;i++)g->data[i]=p->ops[i];for(unsigned i=0;i<6;i++)g->data[12+i]=p->targets[i];ng_message(g,"1..9 once across the whole board. * before +/-.");
}
static bool cross_complete(const NgGame *g){
 unsigned used=0;for(unsigned i=0;i<9;i++){int v=g->board[i];if(v<1||v>9||(used&(1u<<v)))return false;used|=1u<<v;}
 for(unsigned i=0;i<3;i++){if(eval3(g->board[3*i],g->board[3*i+1],g->board[3*i+2],g->data[2*i],g->data[2*i+1])!=g->data[12+i])return false;if(eval3(g->board[i],g->board[i+3],g->board[i+6],g->data[6+2*i],g->data[7+2*i])!=g->data[15+i])return false;}
 return true;
}
static bool action_cross(NgGame *g,int key){
 if(ng_grid_nav(g,key))return true;
 if(g->status!=NG_PLAYING)return false;
 if(submit(key)){if(cross_complete(g)){g->status=NG_WON;ng_message(g,"All six equations and the 1..9 set hold.");}else ng_message(g,"Check the six equations and use 1..9 once.");return true;}
 if(g->fixed[g->cursor]){if((key>='0'&&key<='9')||key==NGK_DEL||auxiliary(key)){ng_message(g,"This number is a fixed clue.");return true;}return false;}
 if(auxiliary(key)){int value=cross_pack(g)->solution[g->cursor];if(g->board[g->cursor]!=value){g->board[g->cursor]=(int16_t)value;++g->moves;}g->assisted=1;ng_message(g,"Selected number REVEALED.");return true;}
 if(key==NGK_DEL||key=='0'){if(g->board[g->cursor]){g->board[g->cursor]=0;++g->moves;}return true;}
 if(key>='1'&&key<='9'){if(g->board[g->cursor]!=key-'0'){g->board[g->cursor]=(int16_t)(key-'0');++g->moves;}return true;}return false;
}
static bool valid_cross(const NgGame *g){
 if(g->status>NG_WON||!bank_valid(g,1)||g->rows!=3||g->cols!=3||g->cursor>=9)return false;
 const GcCrossPack *p=cross_pack(g);for(unsigned i=0;i<9;i++)if(g->board[i]<0||g->board[i]>9||g->fixed[i]!=(p->givens[i]!=0)||(g->fixed[i]&&g->board[i]!=p->givens[i]))return false;
 for(unsigned i=0;i<12;i++)if(g->data[i]!=p->ops[i])return false;
 for(unsigned i=0;i<6;i++)if(g->data[12+i]!=p->targets[i])return false;
 return g->status!=NG_WON||(g->moves&&cross_complete(g));
}
static void render_cross(const NgGame *g,NgCanvas *c){
 static const char ops[]="+-*";for(unsigned r=0;r<3;r++)for(unsigned col=0;col<3;col++){
  unsigned index=r*3+col;int x=14+(int)col*64,y=30+(int)r*46;char s[8]="";if(g->board[index])snprintf(s,sizeof(s),"%d",g->board[index]);
  ng_rect(c,x,y,33,30,g->fixed[index]?NG_LINE:NG_WHITE);ng_border(c,x,y,33,30,index==g->cursor?NG_BLUE:NG_LINE,index==g->cursor?2:1);ng_center(c,x,y+7,33,s,g->fixed[index]?NG_INK:NG_BLUE,1);
  if(col<2){char op[]={ops[g->data[2*r+col]],0};ng_expression(c,x+44,y+9,op,NG_INK,1);}
  if(r<2){char op[]={ops[g->data[6+2*col+r]],0};ng_expression(c,x+13,y+34,op,NG_INK,1);}
 }
 for(unsigned i=0;i<3;i++){char s[20];snprintf(s,sizeof(s),"= %d",(int)g->data[12+i]);ng_expression(c,195,40+(int)i*46,s,NG_INK,1);snprintf(s,sizeof(s),"%d",(int)g->data[15+i]);center_expression(c,8+(int)i*64,161,48,s,NG_BLUE,1);}
 ng_small(c,282,38,"USE 1..9",NG_BLUE);ng_small(c,282,51,"ONCE EACH",NG_BLUE);ng_small_expression(c,282,81,"* before",NG_MUTED);ng_small_expression(c,282,94,"+ and -",NG_MUTED);ng_small(c,282,124,"Blue: input",NG_MUTED);ng_small(c,282,137,"Grey: clue",NG_MUTED);
}
static void factor_answer(int value,char *out,size_t cap){
 size_t pos=0;out[0]=0;
 for(unsigned p=2;p<=(unsigned)value/p;p++)if(value%(int)p==0){
  unsigned exp=0;do{value/=(int)p;++exp;}while(value%(int)p==0);
  int n=exp==1?snprintf(out+pos,cap-pos,"%s%u",pos?"*":"",p):snprintf(out+pos,cap-pos,"%s%u^%u",pos?"*":"",p,exp);
  if(n<0||(size_t)n>=cap-pos){out[cap-1]=0;return;}pos+=(size_t)n;
 }
 if(value>1)(void)snprintf(out+pos,cap-pos,"%s%d",pos?"*":"",value);
}

static void init_factor(NgGame *g){
 if(g->pack_revision>=5){
  unsigned count=ng_prime_beta6_count(g->difficulty),index=ng_bank_pick(g,count);
  g->puzzle_id=index;g->data[0]=(int32_t)ng_prime_beta6_target(g->difficulty,index);
  ng_message(g,"Factor this composite into prime bases and powers.");return;
 }
 if(g->difficulty==3){
  if(g->pack_revision==4){
   static const unsigned large[]={37,41,43,47,53,59,61,67,71,73},other[]={11,13,17,19,23,29,31};
   unsigned target=108*large[ng_rand(g,10)]*other[ng_rand(g,7)];if(ng_rand(g,2))target*=2;
   g->data[0]=(int32_t)target;g->puzzle_id=g->seed;ng_message(g,"Four primes; powers total 7..8; two primes >=11.");return;
  }
  unsigned pool[]={2,3,5,7,11,13};for(unsigned i=6;i>1;i--){unsigned j=ng_rand(g,i),v=pool[i-1];pool[i-1]=pool[j];pool[j]=v;}unsigned target=pool[0]*pool[1]*pool[2]*pool[3];
  unsigned a=ng_rand(g,4),b=(a+1+ng_rand(g,3))%4;target*=pool[a]*pool[b];
  g->data[0]=(int32_t)target;g->puzzle_id=g->seed;ng_message(g,"Four distinct primes; two repeated prime powers.");return;
 }
 static const unsigned primes[]={2,3,5,7,11,13,17,19,23,29,31};unsigned choices=g->difficulty==0?4:g->difficulty==1?6:11,count=2+g->difficulty+ng_rand(g,3),target=1;
 for(unsigned i=0;i<count;i++){unsigned p=primes[ng_rand(g,choices)];if(target<=1000000/p)target*=p;}
 g->data[0]=(int32_t)target;g->puzzle_id=g->seed;ng_message(g,"Enter prime factors with * and optional ^exponent.");
}
static bool action_factor(NgGame *g,int key){
 if(g->status!=NG_PLAYING)return false;
 if(hint(key)){unsigned p=2;while((unsigned)g->data[0]%p)p++;snprintf(g->message,sizeof(g->message),"A prime divisor is %u: %d / %u = %u.",p,(int)g->data[0],p,(unsigned)(int)g->data[0]/p);g->assisted=1;return true;}
 if(auxiliary(key)){factor_answer(g->data[0],g->input,sizeof(g->input));g->assisted=1;ng_message(g,"Prime factorization revealed; EXE checks.");return true;}
 if(!submit(key))return ng_edit(g,key,"0123456789*^",96);
 ++g->moves;if(gc_factorization(g->input,g->data[0])){g->status=NG_WON;ng_message(g,"All bases prime; their powers give the target.");}else ng_message(g,"Need prime bases, positive powers, exact product.");return true;
}

static bool valid_factor(const NgGame *g){
 if(g->status>NG_WON||!input_ok(g,"0123456789*^",96)||g->mode||g->difficulty>3||(g->difficulty==3&&g->pack_revision!=2&&g->pack_revision!=4&&g->pack_revision!=5)||g->data[0]<4||g->data[0]>1000000||gc_is_prime((unsigned)g->data[0])||(g->status==NG_WON&&(!g->moves||!gc_factorization(g->input,g->data[0]))))return false;
 if(g->pack_revision>=5)return g->puzzle_id<ng_prime_beta6_count(g->difficulty) &&
  g->data[0]==(int32_t)ng_prime_beta6_target(g->difficulty,g->puzzle_id) &&
  ng_prime_beta6_valid(g->difficulty,(unsigned)g->data[0]);
 if(g->difficulty==3){
  if(g->pack_revision==4){
   unsigned value=(unsigned)g->data[0],twos=0,threes=0;
   while(value%2==0){value/=2;++twos;}while(value%3==0){value/=3;++threes;}
   if((twos!=2&&twos!=3)||threes!=3)return false;
   static const unsigned large[]={37,41,43,47,53,59,61,67,71,73},other[]={11,13,17,19,23,29,31};
   for(unsigned a=0;a<10;a++)for(unsigned b=0;b<7;b++)if(value==large[a]*other[b])return true;
   return false;
  }
  unsigned value=(unsigned)g->data[0],distinct=0,repeated=0;
  for(unsigned p=2;p<=13;p++)if(value%p==0){unsigned exponent=0;do{value/=p;++exponent;}while(value%p==0);if(exponent>2)return false;++distinct;repeated+=exponent==2;}
  if(value!=1||distinct!=4||repeated!=2)return false;
 }
 return true;
}

static void render_factor(const NgGame *g,NgCanvas *c){
 enum { PRIME_TARGET_X=88,PRIME_TARGET_Y=55,PRIME_TARGET_W=220,PRIME_TARGET_H=48 };
 char target[16];snprintf(target,sizeof target,"%d",(int)g->data[0]);
 int scale=4;while(scale>1 && (ng_text_width(target,scale)>PRIME_TARGET_W || 11*scale>PRIME_TARGET_H))scale--;
 int x=PRIME_TARGET_X+(PRIME_TARGET_W-ng_text_width(target,scale))/2;
 int y=PRIME_TARGET_Y+(PRIME_TARGET_H-11*scale)/2;
 ng_center(c,10,31,376,"PRIME FACTORIZATION",NG_BLUE,1);ng_text(c,x,y,target,NG_INK,scale);
 ng_center(c,10,110,376,"Prime bases, positive exponents",NG_MUTED,1);center_expression(c,10,129,376,"Example: 2^3*3^2*5  or  2*2*2*3*3*5",NG_MUTED,1);text_input(g,c,151);
}

const NgModule ng_guesscalc[10]={
 {1,"NUMBER BASEBALL","BASEBALL","Find the hidden digit sequence.\nEASY/NORMAL/HARD/MASTER: 4/5/6/7 digits.\nRepeated digits and leading zero are allowed.\nS = correct digit and position.\nB = correct digit, wrong position.\nExact matches are consumed before counting B.\nTotal attempts by level: 20/30/40/50.\nThe last try is included in that total.\nDigits enter; DEL erases; EXE submits.\nUP/DOWN scroll attempts. No guess undo.\nOlder saved games retain their original rules.",0,NULL,"SUBMIT",1,default_mode,init_baseball,action_baseball,NULL,valid_baseball,render_baseball},
 {2,"EQUATION GUESS","EQUATION","Find the hidden equation TEXT exactly.\nMatching its numeric value alone cannot win.\nUse digits + - * / and one =. RHS integer.\nNo parentheses or leading zeros. Unary minus\non the RHS only.\nStandard precedence. Green: exact. Yellow:\nexists elsewhere. Grey: absent. Duplicates\nconsume exact matches first. 7/6/8-char modes.\nEasy +-, Normal +-* , Hard +-*/ corpus.\nHard 7/8-char modes use two operators.\nMASTER STANDARD/SHORT/LONG: 9/8/10 chars,\nmixed precedence, 12 attempts.\nSHIFT+DOT enters =.\nEXE submits true equations. UP/DOWN history.",0,NULL,"SUBMIT",3,equation_mode,init_equation,action_equation,NULL,valid_equation,render_equation},
 {3,"NUMBER MIND","NUMBER MIND","Each clue gives exact-position matches only.\nDigits in wrong positions give no information.\nEasy: 4 digits 0..5. Normal: 4 digits 0..7.\nHard: 5 digits 0..7. MASTER: 6 digits 0..7.\nRepeats and initial 0 OK.\nAll published clues have one solution in that\nfinite domain. Digits: input. EXE: CHECK.\nUP/DOWN: clues. HINT lists values not excluded\nby visible zero-match clues, one spot at a time.",NGF_HINT,NULL,"CHECK",1,default_mode,init_mind,action_mind,NULL,valid_mind,render_mind},
 {4,"CLUE LOCK","CLUE LOCK","Find an integer satisfying EVERY shown clue.\nMOD is the remainder after division. Decimal\ndigit sum and contains ignore leading zeros.\nEasy domain 0..99, Normal 0..499, Hard 0..999.\nHard uses two modular / CRT conditions.\nMASTER: 0..9999, three MOD clues and digit\nsum. All four clues contribute to uniqueness.\nAll clues shown initially; one domain solution.\nHINT explains the first modular progression.\nDigits: enter. EXE: CHECK. DEL: erase.",NGF_HINT,NULL,"CHECK",1,default_mode,init_lock,action_lock,NULL,valid_lock,render_lock},
 {5,"SEQUENCE DETECTIVE","SEQUENCE","Find the next term in a FINITE rule grammar.\nA finite prefix is not universally unique!\nAllowed rules (n starts at 0):\na+bn: a=-9..9, b=-5..5, b nonzero.\na*b^n: a=1..5, b=-3,-2,2,3.\na+bn+cn*n: a=-5..5,b=-4..4,c=1..3.\nFibonacci sum: first two each 1..9.\nAlternating lanes: starts -6..9, step 1..5.\nx'=b*x+c: start -3..6,b=-2,-1,2,3,\nc=-3..3 nonzero. Both lanes share a step.\nMASTER adds x_next=a*x_last+b*x_before+c,\nfirst two -3..6; a,b,c each -2,-1,1,2;\nand alternating lanes with DIFFERENT steps,\nstarts -6..9; each step -5..5 nonzero.\nRevision4 EASY uses AP, GP, shared step.\nNORMAL: quadratic, Fibonacci, offset, and\nunequal lanes: starts1..9, steps1..3.\nHARD keeps the six old rules; MASTER uses\nall old and new rules together. Old saves\nkeep their original grammar.\nPacks reject competing next answers across\ntheir level grammar. HINT reveals RULE FAMILY.\nEnter signed integer, EXE checks.",NGF_HINT,NULL,"CHECK",1,default_mode,init_sequence,action_sequence,NULL,valid_sequence,render_sequence},
 {6,"MAKE TARGET","MAKE TARGET","Choose 10/24/50/100/200/RANDOM at entry.\nRANDOM mixes the five targets in your level.\nUse EVERY card exactly once to reach target.\nEASY/NORMAL/HARD/MASTER: 4/4/5/6 cards.\nUse + - * / and parentheses.\nExact rational intermediate values allowed.\nNo concatenation, powers or extra constants.\nEqual values are separate usable cards.\nNew: 200 verified decks per target and level;\nRANDOM shares those 1,000 decks per level.\nEvery input card is 1..999 (at most 3 digits).\nE: no division needed. N/H: division needed.\nMASTER: every solution needs fractions.\nType expression; DEL erases; EXE checks.\nHINT gives one first step; ANSWER copies one\nexample. Both mark assisted. All legal answers\nare accepted, not just that example.\n96 chars, nesting12, reduced values <= 1e9.\nEarlier saved decks and targets stay playable.",NGF_HINT,"ANSWER","CHECK",1,default_mode,init_cards,action_cards,NULL,valid_cards,render_cards},
 {7,"COUNTDOWN","COUNTDOWN","Reach target 100..999 with up to six cards.\nEach card at most once; unused cards allowed.\nEvery intermediate result MUST be a positive\ninteger; division must be exact. + - * / ( ).\nSmall deck: two copies each of 1..10.\nLarge: 25,50,75,100, without repeats.\nEasy/Normal/Hard: 1/2/3 large cards.\nNew: 200 verified decks in each difficulty.\nHARD: every exact answer needs >=4 cards.\nMASTER: 3 large cards; exact target requires\nall six cards and at least one division.\nUntimed practice. EXE checks each expression.\nExact=10 points, distance 1..5=7,6..10=5,\nelse 0. Best submitted distance is retained.\nFINISH keeps best result. No optimality claim.\nHINT gives first step of one exact solution.",NGF_HINT,"FINISH","CHECK",1,default_mode,init_cards,action_cards,NULL,valid_cards,render_cards},
 {8,"MISSING OPERATORS","OPERATORS","Fill the missing + - * / operators.\nNumber order is fixed; no parentheses.\nStandard precedence: * / before + -.\nEqual precedence is evaluated left to right.\nAll valid operator combinations are accepted.\nEasy/Normal/Hard use 3/4/5 numbers.\nMASTER: 6 numbers; 1..3 solutions, each with\nat least three different operator kinds.\nArrows select slot; operation key sets it.\nDEL clears. EXE checks. REVEAL fills selected\nslot from one solution and marks assisted.",NGF_UNDO,"REVEAL","CHECK",1,default_mode,init_operators,action_operators,NULL,valid_operators,render_operators},
 {9,"CROSS MATH","CROSS MATH","Place 1..9 exactly once over the WHOLE board.\nSatisfy three horizontal and three vertical\nexpressions, reading left/right or top/down.\n* before + or -. No Latin-square rules.\nGrey fixed clues cannot be edited.\nEasy/Normal/Hard have 4/2/0 fixed numbers.\nMASTER has no fixed values and needs tighter\nrow/column coupling: more candidate boards.\nEach generated pack puzzle has one solution.\nArrows select; 1..9 enter; DEL/0 clear.\nEXE checks all rules. REVEAL selected number\nmarks the game assisted.",NGF_UNDO,"REVEAL","CHECK",1,default_mode,init_cross,action_cross,NULL,valid_cross,render_cross},
 {10,"PRIME FACTOR","PRIME FACTOR","Factor the composite target into primes.\nUse prime bases with optional positive powers.\nExample 360: 2^3*3^2*5, or repeat factors.\nAny factor order is accepted. 1 is not prime\nor composite; composite bases are rejected.\nExponents 1..20; at most 16 written factors.\nNew targets: EASY 3 digits; NORMAL 3/4;\nHARD 4/5; MASTER 5/6. Largest prime <=97.\nStructure, not digit count alone, sets levels.\nOlder saves retain their original factors.\nType digits, * and ^. DEL erases, EXE checks.\nHINT proves one prime divisor. ANSWER reveals\na factorization. Both mark assisted.",NGF_HINT,"ANSWER","CHECK",1,default_mode,init_factor,action_factor,NULL,valid_factor,render_factor}
};
