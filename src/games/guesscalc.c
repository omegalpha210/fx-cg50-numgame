#include "ng.h"
#include "ui.h"
#include "guesscalc_math.h"
#include "../../assets/guesscalc_packs.h"
#include "../../assets/guesscalc_master.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char *default_mode(unsigned mode){(void)mode;return "STANDARD";}
static const char *baseball_mode(unsigned mode){static const char *const m[]={"MID UNIQUE","SHORT UNIQUE","LONG UNIQUE","MID REPEAT","SHORT REPEAT","LONG REPEAT"};return mode<6?m[mode]:"?";}
static const char *equation_mode(unsigned mode){static const char *const m[]={"STANDARD","SHORT","LONG"};return mode<3?m[mode]:"?";}
static const char *target_mode(unsigned mode){return mode==1?"TARGET 10":"TARGET 24";}
unsigned gc_bank_count(unsigned id,unsigned difficulty,unsigned mode){
 if(id<1||id>10||difficulty>3)return 0;
 if(id==1||id==10||(id==8&&difficulty<3))return 0;
 if(mode>=(id==2?3u:id==6?2u:1u))return 0;
 return 30;
}
static unsigned bank_index(NgGame *g,unsigned legacy_modes){
 if(g->difficulty==3)return legacy_modes*90+g->mode*30+ng_bank_pick(g,30);
 return g->mode*90+g->difficulty*30+ng_bank_pick(g,30);
}
static bool bank_valid(const NgGame *g,unsigned modes){
 if(g->difficulty>3||g->mode>=modes||(g->difficulty==3&&g->pack_revision!=2))return false;
 unsigned start=g->difficulty==3?modes*90+g->mode*30:g->mode*90u+g->difficulty*30u;
 return g->puzzle_id>=start&&g->puzzle_id<start+30;
}
static unsigned baseball_length(const NgGame *g){static const unsigned n[]={4,3,5,4,3,5};return n[g->mode]+(g->difficulty==3?2u:0u);}
static unsigned baseball_limit(const NgGame *g){static const unsigned limits[]={16,12,10,16};return limits[g->difficulty];}
static const char *equation_pack(const NgGame *g){return g->difficulty==3?gc_master_equation[g->puzzle_id-270]:gc_equation_pack[g->puzzle_id];}
typedef struct {unsigned n,alphabet,count,stride;const unsigned char *clues,*matches,*solution;} MindView;
static MindView mind_pack(const NgGame *g){
 if(g->difficulty==3){const GcMasterMindPack *p=&gc_master_mind[g->puzzle_id-90];return (MindView){6,8,p->count,6,&p->clues[0][0],p->matches,p->solution};}
 const GcMindPack *p=&gc_mind_pack[g->puzzle_id];return (MindView){p->n,p->alphabet,p->count,5,&p->clues[0][0],p->matches,p->solution};
}
static const GcSequencePack *sequence_pack(const NgGame *g){return g->difficulty==3?&gc_master_sequence[g->puzzle_id-90]:&gc_sequence_pack[g->puzzle_id];}
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
 for(unsigned i=0;i<n;i++){unsigned d=ng_rand(g,10);if(g->mode<3){while(used&(1u<<d))d=(d+1)%10;used|=1u<<d;}g->board[i]=(int16_t)('0'+d);}
 g->puzzle_id=g->seed;ng_message(g,"Leading zero allowed. Enter a code.");
}
static bool action_baseball(NgGame *g,int key){
 if(scroll_history(g,key,5))return true;
 if(g->status!=NG_PLAYING)return false;
 if(!submit(key))return ng_edit(g,key,"0123456789",(unsigned)g->data[0]);
 unsigned n=(unsigned)g->data[0];if(strlen(g->input)!=n){ng_message(g,"Enter every digit before SUBMIT.");return true;}
 unsigned mask=0;for(unsigned i=0;i<n;i++){unsigned bit=1u<<(g->input[i]-'0');if(g->mode<3&&(mask&bit)){ng_message(g,"This mode requires distinct digits.");return true;}mask|=bit;}
 char secret[8],line[40];secret_text(g,secret,n);unsigned s,b;gc_baseball(secret,g->input,n,&s,&b);
 snprintf(line,sizeof(line),"%2u  %.7s     %cS  %cB",(unsigned)g->moves+1,g->input,(int)('0'+s),(int)('0'+b));ng_history(g,line);g->moves++;g->score=g->moves;g->scroll=g->history_count>5?g->history_count-5:0;g->input[0]=0;
 if(s==n){g->status=NG_WON;ng_message(g,"Code cracked!");}
 else if(g->moves>=(unsigned)g->data[1]){g->status=NG_LOST;snprintf(g->message,sizeof(g->message),"Secret: %s",secret);}
 else ng_message(g,"S: exact position. B: wrong position.");
 return true;
}

static bool valid_baseball(const NgGame *g){
 if(g->status>NG_LOST||g->mode>=6||g->difficulty>3||(g->difficulty==3&&g->pack_revision!=2))return false;
 unsigned n=baseball_length(g),limit=baseball_limit(g);
 if(g->data[0]!=(int)n||g->data[1]!=(int)limit||g->moves>limit||g->history_count!=g->moves||!input_ok(g,"0123456789",n))return false;
 unsigned mask=0;for(unsigned i=0;i<n;i++){int v=g->board[i]-'0';if(v<0||v>9||(g->mode<3&&(mask&(1u<<v))))return false;mask|=1u<<v;}
 bool won=false;char secret[8];secret_text(g,secret,n);
 for(unsigned row=0;row<g->history_count;row++){
  char guess[8],expected[40];unsigned length=n,seen=0,strikes,balls;
  if(strlen(g->history[row])!=length+15)return false;
  for(unsigned col=0;col<length;col++){
   char digit=g->history[row][4+col];if(digit<'0'||digit>'9')return false;
   unsigned bit=1u<<(digit-'0');if(g->mode<3&&(seen&bit))return false;seen|=bit;guess[col]=digit;
  }
  guess[length]=0;gc_baseball(secret,guess,length,&strikes,&balls);
  snprintf(expected,sizeof(expected),"%2u  %.7s     %cS  %cB",row+1,guess,(int)('0'+strikes),(int)('0'+balls));
  if(strcmp(expected,g->history[row]))return false;
  won=strikes==length;if(won&&row+1<g->history_count)return false;
 }
 if(g->score!=g->moves||(g->status==NG_WON)!=won)return false;
 if(g->status==NG_PLAYING&&g->moves>=limit)return false;
 if(g->status==NG_LOST&&g->moves!=limit)return false;
 return g->scroll<=(g->history_count>5?g->history_count-5:0);
}

static void render_baseball(const NgGame *g,NgCanvas *c){
 char line[64];snprintf(line,sizeof(line),"%d DIGITS %s   TRY %u/%d",(int)g->data[0],g->mode<3?"UNIQUE":"REPEAT",(unsigned)g->moves,(int)g->data[1]);ng_text(c,12,31,line,NG_BLUE,1);
 for(unsigned i=0;i<5&&g->scroll+i<g->history_count;i++)ng_text(c,22,53+(int)i*18,g->history[g->scroll+i],NG_INK,1);
 if(!g->history_count)ng_text(c,22,83,"Your attempts appear here.",NG_MUTED,1);
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
static void render_sequence(const NgGame *g,NgCanvas *c){
 ng_text(c,13,30,"NEXT TERM",NG_BLUE,1);for(unsigned i=0;i<6;i++){char s[16];snprintf(s,sizeof(s),"%d",g->board[i]);int x=12+(int)(i%3)*126,y=51+(int)(i/3)*39;ng_card(c,x,y,116,32,s,false,false);}
 if(g->data[8]||g->status==NG_WON)ng_center(c,10,131,376,family((unsigned)g->data[0]),NG_BLUE,1);else ng_center(c,10,131,376,"Finite grammar; see RULES.",NG_MUTED,1);
 if(g->status==NG_WON){
  char row[65];size_t len=strlen(g->message);size_t first=len>62?62:len;
  memcpy(row,g->message,first);row[first]=0;ng_small_expression(c,12,149,row,NG_BLUE);
  if(len>first)ng_small_expression(c,12,162,g->message+first,NG_BLUE);
 }else text_input(g,c,151);
}
static const GcCardPack *card_pack(const NgGame *g){
 if(g->difficulty==3)return g->id==6?&gc_master_target[g->puzzle_id-180]:&gc_master_countdown[g->puzzle_id-90];
 return g->id==6?&gc_target_pack[g->puzzle_id]:&gc_countdown_pack[g->puzzle_id];
}
static void init_cards(NgGame *g){
 g->puzzle_id=bank_index(g,g->id==6?2:1);
 const GcCardPack *p=card_pack(g);unsigned n=g->id==6?4:6;for(unsigned i=0;i<n;i++)g->board[i]=p->cards[i];g->data[0]=p->target;g->data[1]=(int)n;g->data[3]=1000000000;
 ng_message(g,g->id==6?"Use every card exactly once.":"Cards at most once; positive integer steps only.");
}
static bool action_cards(NgGame *g,int key){
 if(g->status!=NG_PLAYING)return false;
 const GcCardPack *p=card_pack(g);
 if(hint(key)){ng_message(g,p->hint);g->assisted=1;return true;}
 if(auxiliary(key)){
  if(g->id==6||key==NGK_ANSWER){snprintf(g->input,sizeof(g->input),"%s",p->answer);g->assisted=1;ng_message(g,"Example answer copied. EXE checks it.");}
  else if(g->data[3]==1000000000)ng_message(g,"CHECK a valid expression before FINISH.");
  else {g->status=g->data[3]?NG_DRAW:NG_WON;snprintf(g->message,sizeof(g->message),"Best submitted distance: %d. Score: %u.",(int)g->data[3],(unsigned)g->score);}
  return true;
 }
 if(!submit(key))return ng_edit(g,key,"0123456789+-*/()",96);
 GcExpression e;if(!gc_expression(g->input,g->id==7,&e)){ng_message(g,g->id==7?"Invalid: every step must be a positive integer.":"Invalid or oversized expression / zero divisor.");return true;}
 if(!gc_cards(&e,g->board,(unsigned)g->data[1],g->id==6)){ng_message(g,g->id==6?"Use the four card values exactly once each.":"Use each card at most once; no extra constants.");return true;}
 ++g->moves;
 if(g->id==6){if(e.value.num==(int64_t)g->data[0]*e.value.den){g->status=NG_WON;ng_message(g,"Exact target reached with all four cards!");}else{snprintf(g->message,sizeof(g->message),"Value %ld/%ld; target %d.",(long)e.value.num,(long)e.value.den,(int)g->data[0]);}return true;}
 int64_t distance=e.value.num-g->data[0];if(distance<0)distance=-distance;
 if(distance<g->data[3]){g->data[3]=(int32_t)distance;g->data[4]=(int32_t)e.value.num;g->score=distance==0?10:distance<=5?7:distance<=10?5:0;}
 if(!distance){g->status=NG_WON;ng_message(g,"Exact target: 10 points.");}
 else snprintf(g->message,sizeof(g->message),"Distance %ld; best %d. Keep trying or FINISH.",(long)distance,(int)g->data[3]);
 return true;
}
static bool valid_cards(const NgGame *g){
 if(!input_ok(g,"0123456789+-*/()",96)||(g->id==6?g->status>NG_WON:(g->status!=NG_PLAYING&&g->status!=NG_WON&&g->status!=NG_DRAW))||!bank_valid(g,g->id==6?2:1))return false;
 const GcCardPack *p=card_pack(g);unsigned n=g->id==6?4:6;if(g->data[0]!=p->target||g->data[1]!=(int)n||g->data[3]<0||g->data[3]>1000000000||g->data[4]<0||g->data[4]>1000000000)return false;
 for(unsigned i=0;i<n;i++)if(g->board[i]!=p->cards[i])return false;
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
 unsigned n=(unsigned)g->data[1],used=card_usage(g);int w=n==4?83:55,gap=9;
 for(unsigned i=0;i<n;i++){snprintf(s,sizeof(s),"%d",g->board[i]);ng_card(c,14+(int)i*(w+gap),63,w,37,s,false,(used&(1u<<i))!=0);}
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
 if(g->difficulty==3){
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
 if(g->status>NG_WON||!input_ok(g,"0123456789*^",96)||g->mode||g->difficulty>3||(g->difficulty==3&&g->pack_revision!=2)||g->data[0]<4||g->data[0]>1000000||gc_is_prime((unsigned)g->data[0])||(g->status==NG_WON&&(!g->moves||!gc_factorization(g->input,g->data[0]))))return false;
 if(g->difficulty==3){
  unsigned value=(unsigned)g->data[0],distinct=0,repeated=0;
  for(unsigned p=2;p<=13;p++)if(value%p==0){unsigned exponent=0;do{value/=p;++exponent;}while(value%p==0);if(exponent>2)return false;++distinct;repeated+=exponent==2;}
  if(value!=1||distinct!=4||repeated!=2)return false;
 }
 return true;
}

static void render_factor(const NgGame *g,NgCanvas *c){
 ng_center(c,10,31,376,"PRIME FACTORIZATION",NG_BLUE,1);ng_number(c,135,66,g->data[0],NG_INK,3);
 ng_center(c,10,110,376,"Prime bases, positive exponents",NG_MUTED,1);center_expression(c,10,129,376,"Example: 2^3*3^2*5  or  2*2*2*3*3*5",NG_MUTED,1);text_input(g,c,151);
}

const NgModule ng_guesscalc[10]={
 {1,"NUMBER BASEBALL","BASEBALL","Find the hidden digit sequence. Leading zero\nis allowed. Modes: 3/4/5 digits, UNIQUE or\nREPEAT. S = exact digit and position.\nB = right digit, wrong position. Exact\nmatches are consumed before counting B.\nEasy/Normal/Hard: 16/12/10 attempts.\nMASTER: 6/5/7 digits, 16 attempts.\nMID/SHORT/LONG select those lengths.\nDigits: enter. DEL: erase. EXE: submit.\nUP/DOWN: scroll attempts. No guess undo.",0,NULL,"SUBMIT",6,baseball_mode,init_baseball,action_baseball,NULL,valid_baseball,render_baseball},
 {2,"EQUATION GUESS","EQUATION","Find the hidden equation TEXT exactly.\nMatching its numeric value alone cannot win.\nUse digits + - * / and one =. RHS integer.\nNo parentheses or leading zeros. Unary minus\non the RHS only.\nStandard precedence. Green: exact. Yellow:\nexists elsewhere. Grey: absent. Duplicates\nconsume exact matches first. 7/6/8-char modes.\nEasy +-, Normal +-* , Hard +-*/ corpus.\nHard 7/8-char modes use two operators.\nMASTER STANDARD/SHORT/LONG: 9/8/10 chars,\nmixed precedence, 12 attempts.\nSHIFT+DOT enters =.\nEXE submits true equations. UP/DOWN history.",0,NULL,"SUBMIT",3,equation_mode,init_equation,action_equation,NULL,valid_equation,render_equation},
 {3,"NUMBER MIND","NUMBER MIND","Each clue gives exact-position matches only.\nDigits in wrong positions give no information.\nEasy: 4 digits 0..5. Normal: 4 digits 0..7.\nHard: 5 digits 0..7. MASTER: 6 digits 0..7.\nRepeats and initial 0 OK.\nAll published clues have one solution in that\nfinite domain. Digits: input. EXE: CHECK.\nUP/DOWN: clues. HINT lists values not excluded\nby visible zero-match clues, one spot at a time.",NGF_HINT,NULL,"CHECK",1,default_mode,init_mind,action_mind,NULL,valid_mind,render_mind},
 {4,"CLUE LOCK","CLUE LOCK","Find an integer satisfying EVERY shown clue.\nMOD is the remainder after division. Decimal\ndigit sum and contains ignore leading zeros.\nEasy domain 0..99, Normal 0..499, Hard 0..999.\nHard uses two modular / CRT conditions.\nMASTER: 0..9999, three MOD clues and digit\nsum. All four clues contribute to uniqueness.\nAll clues shown initially; one domain solution.\nHINT explains the first modular progression.\nDigits: enter. EXE: CHECK. DEL: erase.",NGF_HINT,NULL,"CHECK",1,default_mode,init_lock,action_lock,NULL,valid_lock,render_lock},
 {5,"SEQUENCE DETECTIVE","SEQUENCE","Find the next term in a FINITE rule grammar.\nA finite prefix is not universally unique!\nAllowed rules (n starts at 0):\na+bn: a=-9..9, b=-5..5, b nonzero.\na*b^n: a=1..5, b=-3,-2,2,3.\na+bn+cn*n: a=-5..5,b=-4..4,c=1..3.\nFibonacci sum: first two each 1..9.\nAlternating lanes: starts -6..9, step 1..5.\nx'=b*x+c: start -3..6,b=-2,-1,2,3,\nc=-3..3 nonzero. Both lanes share a step.\nMASTER adds x_next=a*x_last+b*x_before+c,\nfirst two -3..6; a,b,c each -2,-1,1,2;\nand alternating lanes with DIFFERENT steps,\nstarts -6..9; each step -5..5 nonzero.\nMASTER checks old and new rules together.\nOther levels use the six old rules only.\nPacks reject competing next answers across\ntheir level grammar. HINT reveals RULE FAMILY.\nEnter signed integer, EXE checks.",NGF_HINT,NULL,"CHECK",1,default_mode,init_sequence,action_sequence,NULL,valid_sequence,render_sequence},
 {6,"MAKE TARGET","MAKE TARGET","Make target 24 or 10 using ALL four cards,\neach exactly once. + - * / and parentheses.\nExact rational intermediate values allowed.\nNo concatenation, powers or extra constants.\nEqual card values are separate usable cards.\nType expression; DEL erases; EXE checks.\nUsed cards are shaded. HINT: one first step.\nANSWER copies a full example; both assisted.\n96 characters, depth 12, 16 literals, 31 ops.\nReduced numerator/denominator <= 1000000000.\nMASTER cards 1..24 require fractions and\na nested expression; integers alone cannot win.",NGF_HINT,"ANSWER","CHECK",2,target_mode,init_cards,action_cards,NULL,valid_cards,render_cards},
 {7,"COUNTDOWN","COUNTDOWN","Reach target 100..999 with up to six cards.\nEach card at most once; unused cards allowed.\nEvery intermediate result MUST be a positive\ninteger; division must be exact. + - * / ( ).\nSmall deck: two copies each of 1..10.\nLarge: 25,50,75,100, without repeats.\nEasy/Normal/Hard: 1/2/3 large cards.\nMASTER: 3 large cards; exact target requires\nall six cards and at least one division.\nUntimed practice. EXE checks each expression.\nExact=10 points, distance 1..5=7,6..10=5,\nelse 0. Best submitted distance is retained.\nFINISH keeps best result. No optimality claim.\nHINT gives first step of one exact solution.",NGF_HINT,"FINISH","CHECK",1,default_mode,init_cards,action_cards,NULL,valid_cards,render_cards},
 {8,"MISSING OPERATORS","OPERATORS","Fill the missing + - * / operators.\nNumber order is fixed; no parentheses.\nStandard precedence: * / before + -.\nEqual precedence is evaluated left to right.\nAll valid operator combinations are accepted.\nEasy/Normal/Hard use 3/4/5 numbers.\nMASTER: 6 numbers; 1..3 solutions, each with\nat least three different operator kinds.\nArrows select slot; operation key sets it.\nDEL clears. EXE checks. REVEAL fills selected\nslot from one solution and marks assisted.",NGF_UNDO,"REVEAL","CHECK",1,default_mode,init_operators,action_operators,NULL,valid_operators,render_operators},
 {9,"CROSS MATH","CROSS MATH","Place 1..9 exactly once over the WHOLE board.\nSatisfy three horizontal and three vertical\nexpressions, reading left/right or top/down.\n* before + or -. No Latin-square rules.\nGrey fixed clues cannot be edited.\nEasy/Normal/Hard have 4/2/0 fixed numbers.\nMASTER has no fixed values and needs tighter\nrow/column coupling: more candidate boards.\nEach generated pack puzzle has one solution.\nArrows select; 1..9 enter; DEL/0 clear.\nEXE checks all rules. REVEAL selected number\nmarks the game assisted.",NGF_UNDO,"REVEAL","CHECK",1,default_mode,init_cross,action_cross,NULL,valid_cross,render_cross},
 {10,"PRIME FACTOR","PRIME FACTOR","Factor the composite target (4..1000000).\nUse prime bases with optional positive powers.\nExample 360: 2^3*3^2*5, or repeat factors.\nAny factor order is accepted. 1 is not prime\nor composite; composite bases are rejected.\nExponents 1..20; at most 16 written factors.\nMASTER: four distinct primes through 13;\ntwo of those primes appear squared.\nType digits, * and ^. DEL erases, EXE checks.\nHINT proves one prime divisor. ANSWER reveals\na factorization. Both mark assisted.",NGF_HINT,"ANSWER","CHECK",1,default_mode,init_factor,action_factor,NULL,valid_factor,render_factor}
};
