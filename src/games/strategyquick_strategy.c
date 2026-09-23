#include "strategyquick.h"
#include "../../assets/strategyquick/tables.h"
#include "../../assets/strategyquick/master_strategy.h"
#include <stdio.h>
#include <string.h>

static unsigned bit_at(const uint8_t *p,unsigned i){return (p[i/8]>>(i%8))&1u;}
static bool triple(unsigned mask){
 for(unsigned a=0;a<7;a++)for(unsigned b=a+1;b<8;b++)for(unsigned c=b+1;c<9;c++)
  if(a+b+c+3==15 && (mask&(1u<<a)) && (mask&(1u<<b)) && (mask&(1u<<c)))return true;
 return false;
}
static unsigned cards(const NgGame *g,unsigned p){unsigned m=0;for(unsigned i=0;i<9;i++)if(g->board[i]==(int)p)m|=1u<<i;return m;}
static int fifteen_masks(unsigned mine,unsigned theirs){
 unsigned code=0,pow=1;
 for(unsigned i=0;i<9;i++,pow*=3){if(mine&(1u<<i))code+=pow;else if(theirs&(1u<<i))code+=2*pow;}
 return (int)((sq_fifteen_values[code/4]>>((code%4)*2))&3u)-1;
}
int sq_strategy_value(unsigned id,const NgGame *g){
 if(id==21){unsigned n=0;for(unsigned i=0;i<g->cols;i++)n^=(unsigned)g->board[i];return n?1:-1;}
 if(id==22)return bit_at(sq_wythoff_bits,(unsigned)g->board[0]*41u+(unsigned)g->board[1])?1:-1;
 if(id==23)return bit_at(sq_euclid_bits,(unsigned)g->board[0]*100u+(unsigned)g->board[1])?1:-1;
 if(id==24)return fifteen_masks(cards(g,g->turn+1u),cards(g,2u-g->turn));
 if(id==25)return (g->data[1]-g->board[0])%(g->data[2]+1)?1:-1;
 return 0;
}
bool sq_strategy_legal(const NgGame *g,SqMove m){
 if(g->status!=NG_PLAYING || g->mode>1 || m.amount<1)return false;
 switch(g->id){
 case 21:return m.choice>=0 && m.choice<g->cols && m.amount<=g->board[m.choice];
 case 22:return m.choice>=0 && m.choice<=2 && m.amount<=(m.choice==0?g->board[0]:m.choice==1?g->board[1]:(g->board[0]<g->board[1]?g->board[0]:g->board[1]));
 case 23:return g->board[0]>0 && m.choice==0 && m.amount<=g->board[1]/g->board[0];
 case 24:return m.choice>=0 && m.choice<9 && !g->board[m.choice] && m.amount==1;
 case 25:return m.choice==0 && m.amount<=g->data[2] && g->board[0]+m.amount<=g->data[1];
 default:return false;
 }
}
static unsigned terminal_kind(const NgGame *g,unsigned player){
 bool win=false,draw=false;
 switch(g->id){
 case 21:win=true;for(unsigned i=0;i<g->cols;i++)if(g->board[i])win=false;break;
 case 22:win=!g->board[0]&&!g->board[1];break;
 case 23:win=!g->board[0]||!g->board[1];break;
 case 24:win=triple(cards(g,player+1));draw=!win;for(unsigned i=0;i<9;i++)if(!g->board[i])draw=false;break;
 case 25:win=g->board[0]==g->data[1];break;
 }
 return win?1:draw?2:0;
}
static bool strategy_terminal(NgGame *g,unsigned player){
 unsigned kind=terminal_kind(g,player);
 if(kind==1){g->data[0]=(int32_t)player+1;g->status=(g->mode==2||player==0)?NG_WON:NG_LOST;snprintf(g->message,sizeof g->message,"%s wins",g->mode==2?(player?"Player 2":"Player 1"):(player?"CPU":"You"));}
 if(kind==2){g->status=NG_DRAW;ng_message(g,"All cards taken: draw");}
 return kind!=0;
}
bool sq_strategy_apply(NgGame *g,SqMove m){
 if(!sq_strategy_legal(g,m))return false;
 unsigned player=g->turn;
 switch(g->id){
 case 21:g->board[m.choice]-=(int16_t)m.amount;break;
 case 22:if(m.choice!=1)g->board[0]-=(int16_t)m.amount;if(m.choice!=0)g->board[1]-=(int16_t)m.amount;break;
 case 23:{int a=g->board[0],b=g->board[1]-m.amount*a;g->board[0]=(int16_t)(a<b?a:b);g->board[1]=(int16_t)(a<b?b:a);break;}
 case 24:g->board[m.choice]=(int16_t)(player+1);break;
 case 25:g->board[0]+=(int16_t)m.amount;break;
 }
 g->moves++;g->input[0]='\0';g->message[0]='\0';g->cpu_pending=0;
 if(!strategy_terminal(g,player)){g->turn^=1;g->cpu_pending=(g->mode<2 && g->turn==1);}
 return true;
}
/* Evaluate a child from small rule state, never copy the 1.8KB whole game. */
static int move_value(const NgGame *g,SqMove m){
 if(g->id==22){unsigned a=(unsigned)g->board[0]-(m.choice!=1?(unsigned)m.amount:0),b=(unsigned)g->board[1]-(m.choice!=0?(unsigned)m.amount:0);return (!a&&!b)||!bit_at(sq_wythoff_bits,a*41u+b)?1:-1;}
 if(g->id==23){unsigned a=(unsigned)g->board[0],b=(unsigned)g->board[1]-(unsigned)m.amount*a;if(!b)return 1;unsigned x=a<b?a:b,y=a<b?b:a;return bit_at(sq_euclid_bits,x*100u+y)?-1:1;}
 if(g->id==24){unsigned mine=cards(g,g->turn+1u)|(1u<<m.choice),theirs=cards(g,2u-g->turn);if(triple(mine))return 1;return -fifteen_masks(theirs,mine);}
 if(g->id==25)return (g->data[1]-g->board[0]-m.amount)%(g->data[2]+1)?-1:1;
 return 0;
}
SqMove sq_strategy_pick(NgGame *g,bool exact){
 SqMove chosen={-1,0};int best=-2;unsigned ties=0;
 if(exact && g->id==21){unsigned sum=0;for(unsigned i=0;i<g->cols;i++)sum^=(unsigned)g->board[i];if(sum){for(unsigned i=0;i<g->cols;i++){unsigned remain=(unsigned)g->board[i]^sum;if(remain<(unsigned)g->board[i]){SqMove m={(int)i,g->board[i]-(int)remain};return m;}}}exact=false;}
 int count=g->id==21?g->cols:g->id==22?3:g->id==24?9:1;
 for(int c=0;c<count;c++)for(int n=1;n<100;n++){
  SqMove m={c,n};if(!sq_strategy_legal(g,m))break;
  int value=0;
  if(exact)value=move_value(g,m);
  if(value>best){best=value;chosen=m;ties=1;}
  else if(value==best){ties++;if(!ng_rand(g,ties))chosen=m;}
 }
 return chosen;
}
const char *sq_strategy_mode(unsigned mode){static const char *names[]={"YOU FIRST","CPU FIRST","LEGACY 2P"};return names[mode<3?mode:0];}
unsigned sq_bank_count(unsigned id,unsigned difficulty,unsigned mode){
 if(difficulty!=3)return 0;
 if(id>=21&&id<=25&&mode<3)return sq_master_start_count[id-21][mode==1];
 if(id==27&&mode<2)return mode?30:2;
 if(id==28&&mode<2)return 30;
 return 0;
}
void sq_strategy_init(NgGame *g){
 g->rows=1;g->cols=1;g->turn=g->mode==1;g->data[3]=g->turn;
 g->cpu_pending=g->mode==1;
 switch(g->id){
 case 21:g->cols=g->difficulty>=2?4:3;for(unsigned i=0;i<g->cols;i++)g->board[i]=(int16_t)(1+ng_rand(g,g->difficulty==0?7:g->difficulty==1?15:31));break;
 case 22:g->cols=3;g->board[0]=(int16_t)(1+ng_rand(g,g->difficulty==0?10:g->difficulty==1?24:40));g->board[1]=(int16_t)(1+ng_rand(g,g->difficulty==0?10:g->difficulty==1?24:40));break;
 case 23:{int a=1+(int)ng_rand(g,g->difficulty==0?12:g->difficulty==1?40:99),b=1+(int)ng_rand(g,g->difficulty==0?12:g->difficulty==1?40:99);g->board[0]=(int16_t)(a<b?a:b);g->board[1]=(int16_t)(a<b?b:a);break;}
 case 24:g->rows=g->cols=3;break;
 case 25:g->data[1]=g->difficulty==0?21:g->difficulty==1?31:23;g->data[2]=g->difficulty==2?4:3;break;
 }
 if(g->difficulty==3){
  g->puzzle_id=ng_bank_pick(g,sq_bank_count(g->id,3,g->mode));const SqStart *p=&sq_master_starts[g->id-21][g->mode==1][g->puzzle_id];
  for(unsigned i=0;i<9;i++)g->board[i]=(int16_t)(g->id==24&&g->mode==1&&p->board[i]?3-p->board[i]:p->board[i]);
  g->data[1]=p->target;g->data[2]=p->max_add;g->data[4]=p->initial_plies;
  ng_message(g,"MASTER: a forced win is available to you");
 }
}
static int input_number(const char *s){int n=0;if(!*s)return 0;for(unsigned i=0;s[i];i++){if(s[i]<'0'||s[i]>'9')return 0;n=n*10+s[i]-'0';if(n>99)return 0;}return n;}
bool sq_strategy_action(NgGame *g,int key){
 if(g->status!=NG_PLAYING || g->mode>1)return false;
 if(g->cpu_pending){
  if(key!=NGK_CPU)return false;
  bool exact=g->difficulty>=2 || (g->difficulty==1 && ng_rand(g,4)!=0);
  SqMove move=sq_strategy_pick(g,exact);return sq_strategy_apply(g,move);
 }
 if(key==NGK_CPU)return false;
 if(g->id==24){
  if(ng_grid_nav(g,key))return true;
  if(key>='1'&&key<='9'){g->cursor=(uint8_t)(key-'1');key=NGK_EXE;}
  if(key==NGK_EXE){SqMove m={g->cursor,1};if(sq_strategy_apply(g,m))return true;ng_message(g,"That card is already taken");return true;}
  return false;
 }
 if((g->id==21||g->id==22) && ng_grid_nav(g,key))return true;
 if(key==NGK_EXE){SqMove m={(g->id==21||g->id==22)?g->cursor:0,input_number(g->input)};if(!sq_strategy_apply(g,m))ng_message(g,"Enter a legal positive amount");return true;}
 return ng_edit(g,key,"0123456789",2);
}
bool sq_strategy_valid(const NgGame *g){
 if(g->id<21||g->id>25||g->difficulty>3||g->mode>2||(g->mode==2&&g->pack_revision>2)||g->turn>1||g->phase||g->data[0]<0||g->data[0]>2||g->data[3]<0||g->data[3]>1)return false;
 if(g->difficulty==3){if(g->puzzle_id>=sq_bank_count(g->id,3,g->mode))return false;const SqStart *p=&sq_master_starts[g->id-21][g->mode==1][g->puzzle_id];if(g->data[1]!=p->target||g->data[2]!=p->max_add||g->data[4]!=p->initial_plies)return false;}
 else if(g->data[4])return false;
 if(g->mode!=1 && g->data[3]!=0)return false;
 if(g->mode==1 && g->data[3]!=1)return false;
 if(g->cpu_pending!=(g->status==NG_PLAYING && g->mode<2 && g->turn==1))return false;
 if(g->rows!=(g->id==24?3:1)||g->cols!=(g->id==21?(g->difficulty>=2?4:3):g->id==22||g->id==24?3:1)||g->cursor>=g->rows*g->cols)return false;
 size_t length=0;while(length<NG_INPUT && g->input[length]){if(length>=2 || g->input[length]<'0'||g->input[length]>'9')return false;length++;}if(length==NG_INPUT)return false;
 if(g->id==21){for(unsigned i=0;i<g->cols;i++)if(g->board[i]<0||g->board[i]>31)return false;}
 if(g->id==22){if(g->board[0]<0||g->board[0]>40||g->board[1]<0||g->board[1]>40)return false;}
 if(g->id==23){if(g->board[0]<0||g->board[0]>g->board[1]||g->board[1]>99||g->board[1]<1)return false;}
 if(g->id==24){unsigned count=0,counts[2]={0};for(unsigned i=0;i<9;i++){if(g->board[i]<0||g->board[i]>2)return false;if(g->board[i]){count++;counts[g->board[i]-1]++;}}if(g->moves>9||count!=g->moves+(unsigned)g->data[4])return false;
  unsigned first=(unsigned)g->data[3];if(counts[first]!=(count+1)/2||counts[first^1]!=count/2)return false;
  if(triple(cards(g,1))&&triple(cards(g,2)))return false;
 }
 if(g->id==25){if(g->difficulty<3&&(g->data[1]!=(g->difficulty==0?21:g->difficulty==1?31:23)||g->data[2]!=(g->difficulty==2?4:3)))return false;if(g->board[0]<0||g->board[0]>g->data[1])return false;}
 if(g->status==NG_PLAYING){
  if(g->data[0] || g->turn!=((g->moves+(unsigned)g->data[3])%2))return false;
  if(g->id==24 && (triple(cards(g,1))||triple(cards(g,2))))return false;
  if(terminal_kind(g,g->turn))return false;
 }else{
  if(!g->moves || g->turn!=((g->moves-1u+(unsigned)g->data[3])%2))return false;
  unsigned kind=terminal_kind(g,g->turn);if(!kind)return false;
  unsigned status=kind==2?NG_DRAW:(g->mode==2||g->turn==0)?NG_WON:NG_LOST;
  if(g->status!=status||g->data[0]!=(kind==1?(int)g->turn+1:0))return false;
 }
 return true;
}
static void strategy_panel(NgCanvas *c,int x,int y,int w,int h,const char *label,bool selected){
 ng_rect(c,x,y,w,h,NG_WHITE);ng_border(c,x,y,w,h,selected?NG_BLUE:NG_LINE,selected?2:1);ng_center(c,x,y+9,w,label,NG_MUTED,1);
}
void sq_strategy_render(const NgGame *g,NgCanvas *c){
 char s[64];const char *who=g->mode==2?(g->turn?"PLAYER 2":"PLAYER 1"):(g->turn?"CPU":"YOUR TURN");
 ng_text(c,12,31,g->mode==2?"LEGACY 2P - READ ONLY":g->cpu_pending?"CPU THINKING...":who,NG_BLUE,1);
 if(g->id==21){
  int w=g->cols==4?82:108;
  for(unsigned i=0;i<g->cols;i++){
   int x=14+(int)i*(w+12);snprintf(s,sizeof s,"PILE %u",i+1);strategy_panel(c,x,55,w,80,s,g->cursor==i);ng_number(c,x+w/2-10,84,g->board[i],NG_INK,2);
   for(int t=0;t<g->board[i] && t<16;t++)ng_rect(c,x+8+(t%8)*8,113+(t/8)*7,5,5,NG_BLUE);
  }
  ng_text(c,14,151,"REMOVE",NG_INK,1);ng_input(c,99,144,90,g->input);ng_text(c,205,151,"ONE PILE",NG_MUTED,1);
 }else if(g->id==22){
  strategy_panel(c,16,56,116,78,"PILE A",g->cursor==0);ng_number(c,57,87,g->board[0],NG_INK,2);
  strategy_panel(c,144,56,116,78,"PILE B",g->cursor==1);ng_number(c,186,87,g->board[1],NG_INK,2);
  strategy_panel(c,272,56,108,78,"BOTH",g->cursor==2);ng_text(c,282,95,"SAME #",NG_MUTED,1);
  ng_text(c,16,151,"REMOVE",NG_INK,1);ng_input(c,102,144,92,g->input);
 }else if(g->id==23){
  strategy_panel(c,24,59,150,60,"SMALL",false);strategy_panel(c,211,59,160,60,"LARGE",false);ng_number(c,80,85,g->board[0],NG_INK,2);ng_number(c,265,85,g->board[1],NG_INK,2);
  snprintf(s,sizeof s,"LARGE - k x SMALL  k:1..%d",g->board[0]?g->board[1]/g->board[0]:0);ng_text(c,16,130,s,NG_INK,1);ng_input(c,150,148,90,g->input);
 }else if(g->id==24){
  for(unsigned i=0;i<9;i++){
   int x=15+(int)(i%3)*56,y=54+(int)(i/3)*40;snprintf(s,sizeof s,"%u",i+1);ng_card(c,x,y,46,34,s,g->cursor==i,g->board[i]!=0);
   if(g->board[i]){ng_rect(c,x+5,y+25,36,4,g->board[i]==1?NG_BLUE:NG_RED);}
  }
  ng_text(c,202,59,"THREE SUM TO 15",NG_INK,1);
  for(unsigned p=1;p<=2;p++){snprintf(s,sizeof s,"%s:",g->mode==2?(p==1?"P1":"P2"):(p==1?"YOU":"CPU"));size_t n=strlen(s);for(unsigned i=0;i<9;i++)if(g->board[i]==(int)p && n+3<sizeof s)n+=(size_t)snprintf(s+n,sizeof s-n," %u",i+1);ng_text(c,202,88+(int)(p-1)*32,s,p==1?NG_BLUE:NG_RED,1);}
 }else{
  snprintf(s,sizeof s,"TARGET %ld",(long)g->data[1]);ng_center(c,12,61,372,s,NG_MUTED,1);ng_center(c,12,85,372,"",NG_INK,2);ng_number(c,174,87,g->board[0],NG_BLUE,3);
  snprintf(s,sizeof s,"ADD 1..%ld",(long)g->data[2]);ng_text(c,45,145,s,NG_INK,1);ng_input(c,189,139,100,g->input);
 }
}
