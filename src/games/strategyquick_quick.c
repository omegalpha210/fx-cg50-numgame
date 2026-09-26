#include "strategyquick.h"
#include "../../assets/strategyquick/master_quick.h"
#include "../../assets/strategyquick/sliding_bands.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

static uint32_t add_sat(uint32_t a,uint32_t b){return UINT32_MAX-a<b?UINT32_MAX:a+b;}
bool sq_2048_line(int16_t line[4],uint32_t *score){
 int16_t before[4],packed[4]={0},out[4]={0};unsigned n=0,k=0;
 memcpy(before,line,sizeof before);
 for(unsigned i=0;i<4;i++)if(line[i])packed[n++]=line[i];
 for(unsigned i=0;i<n;i++){
  int e=packed[i];
  if(i+1<n && e==packed[i+1] && e<30){e++;i++;*score=add_sat(*score,UINT32_C(1)<<e);}
  out[k++]=(int16_t)e;
 }
 memcpy(line,out,sizeof out);return memcmp(before,out,sizeof out)!=0;
}
bool sq_2048_can_move(const NgGame *g){
 for(unsigned i=0;i<16;i++){
  if(!g->board[i])return true;
  if(g->board[i]<30 && ((i%4!=3 && g->board[i]==g->board[i+1]) || (i<12 && g->board[i]==g->board[i+4])))return true;
 }
 return false;
}
static void spawn(NgGame *g){
 unsigned empty[16],n=0;for(unsigned i=0;i<16;i++)if(!g->board[i])empty[n++]=i;
 if(n){unsigned i=empty[ng_rand(g,n)];g->board[i]=(int16_t)(ng_rand(g,10)?1:2);}
}
static unsigned tile_index(unsigned line,unsigned p,int key){return key==NGK_LEFT?line*4+p:key==NGK_RIGHT?line*4+3-p:key==NGK_UP?p*4+line:(3-p)*4+line;}
static bool move_2048(NgGame *g,int key){
 if(key<NGK_UP||key>NGK_LEFT)return false;
 bool changed=false;
 for(unsigned i=0;i<4;i++){
  int16_t line[4];for(unsigned p=0;p<4;p++)line[p]=g->board[tile_index(i,p,key)];
  changed|=sq_2048_line(line,&g->score);
  for(unsigned p=0;p<4;p++)g->board[tile_index(i,p,key)]=line[p];
 }
 if(!changed)return false;
 g->moves++;spawn(g);g->message[0]='\0';
 for(unsigned i=0;i<16;i++)if(g->board[i]>g->data[0])g->data[0]=g->board[i];
 if(g->data[0]>=11 && !g->data[1]){g->data[1]=1;ng_message(g,"2048 reached! Continue playing");}
 if(g->score==UINT32_MAX){g->data[2]=1;ng_message(g,"Score capped at 4294967295");}
 if(g->mode==1&&g->data[0]>=g->data[3]){g->status=NG_WON;ng_message(g,"Target tile reached!");}
 else if(!sq_2048_can_move(g)){g->status=NG_LOST;ng_message(g,"No legal moves: game over");}
 return true;
}
static bool sliding_goal(const NgGame *g){unsigned n=g->rows*g->cols;for(unsigned i=0;i<n;i++)if(g->board[i]!=(int)((i+1)%n))return false;return true;}
bool sq_sliding_solvable(const NgGame *g){
 unsigned n=g->rows*g->cols,inv=0,blank=0;
 for(unsigned i=0;i<n;i++){if(!g->board[i])blank=i;for(unsigned j=i+1;j<n;j++)if(g->board[i]&&g->board[j]&&g->board[i]>g->board[j])inv++;}
 return g->cols%2?inv%2==0:(inv+g->rows-blank/g->cols)%2==1;
}
static bool slide(NgGame *g,int key){
 int p=g->cursor,q=p;
 if(key==NGK_UP && p>=g->cols)q-=g->cols;
 else if(key==NGK_DOWN && p<(g->rows-1)*g->cols)q+=g->cols;
 else if(key==NGK_LEFT && p%g->cols)q--;
 else if(key==NGK_RIGHT && p%g->cols<g->cols-1)q++;
 if(q==p)return false;
 g->board[p]=g->board[q];g->board[q]=0;g->cursor=(uint8_t)q;return true;
}
static unsigned manhattan(const NgGame *g){unsigned d=0;for(unsigned i=0;i<g->rows*g->cols;i++)if(g->board[i]){unsigned p=(unsigned)g->board[i]-1;int x=(int)(i%g->cols)-(int)(p%g->cols),y=(int)(i/g->cols)-(int)(p/g->cols);d+=(unsigned)(x<0?-x:x)+(unsigned)(y<0?-y:y);}return d;}
static const uint8_t *sliding_banded(const NgGame *g){return g->mode?sq_sliding_banded4[g->difficulty][g->puzzle_id]:sq_sliding_banded3[g->difficulty][g->puzzle_id];}
static void sliding_init(NgGame *g){
 if(g->pack_revision>=4 && g->difficulty<3){
  g->puzzle_id=ng_bank_pick(g,SQ_SLIDING_BAND_COUNT);const uint8_t *p=sliding_banded(g);unsigned cells=g->rows*g->cols;
  for(unsigned i=0;i<cells;i++){g->board[i]=(int16_t)((p[i/2]>>((i%2)*4))&15u);if(!g->board[i])g->cursor=(uint8_t)i;}
  g->data[0]=g->data[1]=p[(cells+1)/2];
  snprintf(g->message,sizeof g->message,"Start shortest solution: %ld moves",(long)g->data[1]);return;
 }
 if(g->difficulty==3){
  g->puzzle_id=ng_bank_pick(g,sq_bank_count(27,3,g->mode));const uint8_t *p=g->mode?sq_master_sliding4[g->puzzle_id]:sq_master_sliding3[g->puzzle_id];
  for(unsigned i=0;i<g->rows*g->cols;i++){g->board[i]=p[i];if(!p[i])g->cursor=(uint8_t)i;}
  g->data[0]=g->mode?400:31;g->data[1]=g->mode?(int32_t)manhattan(g):31;
  ng_message(g,g->mode?"MASTER: at least 48 moves from the goal":"MASTER: shortest solution is 31 moves");return;
 }
 unsigned n=g->rows*g->cols,steps=(g->difficulty+1u)*80u;
 for(unsigned attempt=0;attempt<16;attempt++){
  for(unsigned i=0;i<n;i++)g->board[i]=(int16_t)((i+1)%n);
  g->cursor=(uint8_t)(n-1);int previous=-1;
  for(unsigned t=0;t<steps;t++){
   int candidates[4],count=0;
   for(int k=NGK_UP;k<=NGK_LEFT;k++){
    int q=(int)g->cursor+(k==NGK_UP?-(int)g->cols:k==NGK_DOWN?g->cols:k==NGK_LEFT?-1:1);
    bool legal=(k==NGK_UP?g->cursor>=g->cols:k==NGK_DOWN?g->cursor<(g->rows-1)*g->cols:k==NGK_LEFT?g->cursor%g->cols!=0:g->cursor%g->cols<g->cols-1);
    if(legal && q!=previous)candidates[count++]=k;
   }
   int old=g->cursor;slide(g,candidates[ng_rand(g,(unsigned)count)]);previous=old;
  }
  if(manhattan(g)>=6u+g->difficulty*4u)break;
 }
 if(sliding_goal(g))slide(g,NGK_UP);
 g->data[0]=(int32_t)steps;
}
static void toggle(NgGame *g,unsigned p){
 unsigned cols=g->cols;g->board[p]^=1;
 if(p>=cols)g->board[p-cols]^=1;
 if(p+cols<(unsigned)(g->rows*cols))g->board[p+cols]^=1;
 if(p%cols)g->board[p-1]^=1;
 if(p%cols+1<cols)g->board[p+1]^=1;
}
bool sq_lights_solution(const NgGame *g,uint32_t *solution){
 unsigned n=g->rows*g->cols,rank=0,pivots[25];uint32_t rows[25]={0};
 for(unsigned p=0;p<n;p++){
  rows[p]=UINT32_C(1)<<p;
  if(p>=g->cols)rows[p]|=UINT32_C(1)<<(p-g->cols);
  if(p+g->cols<n)rows[p]|=UINT32_C(1)<<(p+g->cols);
  if(p%g->cols)rows[p]|=UINT32_C(1)<<(p-1);
  if(p%g->cols+1<g->cols)rows[p]|=UINT32_C(1)<<(p+1);
  if(g->board[p])rows[p]|=UINT32_C(1)<<n;
 }
 for(unsigned col=0;col<n;col++){
  unsigned p=rank;while(p<n && !(rows[p]&(UINT32_C(1)<<col)))p++;
  if(p==n)continue;
  uint32_t tmp=rows[rank];rows[rank]=rows[p];rows[p]=tmp;pivots[rank]=col;
  for(unsigned r=0;r<n;r++)if(r!=rank && (rows[r]&(UINT32_C(1)<<col)))rows[r]^=rows[rank];
  rank++;
 }
 for(unsigned r=rank;r<n;r++)if(rows[r]&(UINT32_C(1)<<n))return false;
 *solution=0;for(unsigned r=0;r<rank;r++)if(rows[r]&(UINT32_C(1)<<n))*solution|=UINT32_C(1)<<pivots[r];
 return true;
}
static bool lights_off(const NgGame *g){for(unsigned i=0;i<g->rows*g->cols;i++)if(g->board[i])return false;return true;}
/* Complete 4x4 toggle nullspace. Every solution for a chosen press mask is
 * mask XOR one of these 16 words. Host row-chasing independently verifies it. */
static unsigned lights4_minimum(uint32_t mask){
 static const uint16_t kernel[16]={0x0000,0x135e,0x278b,0x34d5,0x4e1d,0x5d43,0x6996,0x7ac8,0x8ca7,0x9ff9,0xab2c,0xb872,0xc2ba,0xd1e4,0xe531,0xf66f};
 unsigned best=16;
 for(unsigned i=0;i<16;i++){
  uint32_t bits=mask^kernel[i];unsigned count=0;
  for(unsigned bit=0;bit<16;bit++)count+=(bits>>bit)&1u;
  if(count<best)best=count;
 }
 return best;
}
static void lights4_graded(NgGame *g){
 static const unsigned targets[3]={2,4,5};unsigned target=targets[g->difficulty];uint32_t chosen=0;
 /* 16 candidates maximum. The deterministic fallback 0x1f has minimum five;
  * two/four chosen presses are already minimal (kernel minimum weight eight). */
 for(unsigned attempt=0;attempt<16;attempt++){
  chosen=0;
  for(unsigned i=0;i<target;i++){
   unsigned choices[16],count=0;for(unsigned p=0;p<16;p++)if(!(chosen&(UINT32_C(1)<<p)))choices[count++]=p;
   chosen|=UINT32_C(1)<<choices[ng_rand(g,count)];
  }
  if(lights4_minimum(chosen)==target)break;
  if(attempt==15)chosen=UINT32_C(0x1f);
 }
 for(unsigned p=0;p<16;p++)if(chosen&(UINT32_C(1)<<p))toggle(g,p);
 g->data[0]=-1;g->data[1]=(int32_t)target;
 snprintf(g->message,sizeof g->message,"Start requires at least %u presses",target);
}
static void lights_init(NgGame *g){
 if(g->difficulty==3){
  g->puzzle_id=ng_bank_pick(g,30);uint32_t b=sq_master_lights[g->mode][g->puzzle_id];
  for(unsigned i=0;i<g->rows*g->cols;i++)g->board[i]=(int16_t)((b>>i)&1u);
  g->data[0]=-1;g->data[1]=sq_master_lights_min[g->mode][g->puzzle_id];
  snprintf(g->message,sizeof g->message,"MASTER: minimum %ld presses from start",(long)g->data[1]);return;
 }
 if(g->rows==4 && g->pack_revision>=3){lights4_graded(g);return;}
 unsigned n=g->rows*g->cols,count=g->difficulty==0?4:g->difficulty==1?8:g->rows==4?12:18;uint32_t chosen=0;
 for(unsigned i=0;i<count;i++){
  unsigned choices[25],k=0;for(unsigned j=0;j<n;j++)if(!(chosen&(UINT32_C(1)<<j)))choices[k++]=j;
  unsigned p=choices[ng_rand(g,k)];chosen|=UINT32_C(1)<<p;toggle(g,p);
 }
 if(lights_off(g))toggle(g,0);
 g->data[0]=-1;g->data[1]=(int32_t)count;
}
/* Rush: data 0=a 1=b 2=c 3=operation 4=answer 5=correct 6=wrong
 * 7=combo 8=best combo 9=remaining ms 10=result exposure ms. */
static void rush_problem(NgGame *g){
 int a,b,c=0,answer,op=(int)ng_rand(g,g->difficulty==0?2:4);
 unsigned limit=g->difficulty==0?10:g->difficulty==1?30:99;
 a=(int)ng_rand(g,limit)+1;b=(int)ng_rand(g,g->difficulty==0?10:12)+1;
 if(op==0)answer=a+b;
 else if(op==1){if(g->difficulty==0 && a<b){int tmp=a;a=b;b=tmp;}answer=a-b;}
 else if(op==2)answer=a*b;
 else{answer=a;a*=b;}
 if(g->difficulty==2){c=(int)ng_rand(g,21)-10;answer+=c;}
 g->data[0]=a;g->data[1]=b;g->data[2]=c;g->data[3]=op;g->data[4]=answer;
 g->input[0]='\0';g->phase=0;
}
static bool signed_answer(const char *s,int *value){
 unsigned i=0;int sign=1,n=0;if(s[i]=='-'){sign=-1;i++;}if(!s[i])return false;
 for(;s[i];i++){if(s[i]<'0'||s[i]>'9')return false;n=n*10+s[i]-'0';if(n>99999)return false;}
 *value=n*sign;return true;
}
/* Memory: phase 0 exposure,1 hidden barrier,2 input,3 round correct,
 * 4 final comparison. data 0=length 1=round 2=remaining exposure ms,
 * 3=phase delay ms. Secret digits stored board[], preserve leading zero. */
static void memory_round(NgGame *g){
 unsigned base=3+g->difficulty,length=base+(unsigned)g->data[1]/2;if(length>16)length=16;
 g->data[0]=(int32_t)length;
 for(unsigned i=0;i<length;i++)g->board[i]=(int16_t)ng_rand(g,10);
 g->input[0]='\0';g->phase=0;
 g->data[2]=(int32_t)(1400u+length*(g->difficulty==0?700u:g->difficulty==1?500u:350u));
 g->data[3]=0;
}
const char *sq_sliding_mode(unsigned mode){return mode?"4 x 4":"3 x 3";}
const char *sq_2048_mode(unsigned mode){return mode?"TARGET":"CLASSIC";}
const char *sq_lights_mode(unsigned mode){return mode?"5 x 5":"4 x 4";}
const char *sq_rush_mode(unsigned mode){return mode?"UNTIMED PRACTICE":"60 SECONDS";}
void sq_quick_init(NgGame *g){
 if(g->id==26){g->rows=g->cols=4;if(g->mode)g->data[3]=g->difficulty==3?13:9+g->difficulty;spawn(g);spawn(g);for(unsigned i=0;i<16;i++)if(g->board[i]>g->data[0])g->data[0]=g->board[i];}
 else if(g->id==27){g->rows=g->cols=g->mode?4:3;sliding_init(g);}
 else if(g->id==28){g->rows=g->cols=g->mode?5:4;lights_init(g);}
 else if(g->id==29){g->rows=g->cols=1;g->data[9]=60000;if(g->mode)g->assisted=1;rush_problem(g);}
 else if(g->id==30){g->rows=g->cols=1;memory_round(g);}
}
bool sq_quick_action(NgGame *g,int key){
 if(g->status!=NG_PLAYING)return false;
 if(g->id==26)return move_2048(g,key);
 if(g->id==27){if(!slide(g,key))return false;g->moves++;if(sliding_goal(g)){g->status=NG_WON;ng_message(g,"All tiles in order");}return true;}
 if(g->id==28){
  if(ng_grid_nav(g,key))return true;
  if(key==NGK_HINT){uint32_t solution;if(sq_lights_solution(g,&solution)&&solution){unsigned p=0;while(!(solution&(UINT32_C(1)<<p)))p++;g->cursor=(uint8_t)p;g->data[0]=(int32_t)p;g->assisted=1;ng_message(g,"GF(2): press the highlighted cell");return true;}return false;}
  if(key==NGK_EXE || key=='5'){toggle(g,g->cursor);g->moves++;g->data[0]=-1;g->message[0]='\0';if(lights_off(g)){g->status=NG_WON;ng_message(g,"All lights off");}return true;}return false;
 }
 if(g->id==29){
  if(key=='=' && g->mode){g->status=NG_WON;ng_message(g,"Practice finished");return true;}
  if(g->phase==1){if(key==NGK_EXE && g->data[10]==0){g->message[0]='\0';rush_problem(g);return true;}return false;}
  if(key==NGK_EXE){if(g->moves>=1000000){g->status=NG_WON;ng_message(g,"Practice session limit reached");return true;}int answer;if(!signed_answer(g->input,&answer)){ng_message(g,"Enter an integer answer");return true;}
   g->moves++;if(answer==g->data[4]){g->data[5]++;g->data[7]++;if(g->data[7]>g->data[8])g->data[8]=g->data[7];g->score=add_sat(g->score,10u+(unsigned)g->data[7]*2u);ng_message(g,"Correct! EXE for next problem");}
   else{g->data[6]++;g->data[7]=0;snprintf(g->message,sizeof g->message,"Answer: %ld. EXE for next",(long)g->data[4]);}
   g->phase=1;g->data[10]=300;return true;
  }
  if(key=='-' && !g->input[0])return ng_edit(g,key,"-0123456789",6);
  return ng_edit(g,key,"0123456789",6);
 }
 if(g->id==30){
  if(g->phase==0 || g->phase==1 || g->phase==4)return false;
  if(g->phase==3){if(key==NGK_EXE && g->data[3]==0){g->data[1]++;memory_round(g);g->message[0]='\0';return true;}return false;}
  if(key==NGK_EXE){
   if(strlen(g->input)!=(size_t)g->data[0]){ng_message(g,"Enter every digit, including zeros");return true;}
   bool correct=true;for(int i=0;i<g->data[0];i++)if(g->input[i]-'0'!=g->board[i])correct=false;
   g->moves++;
   if(correct){g->score=add_sat(g->score,(unsigned)g->data[0]);g->phase=3;g->data[3]=300;ng_message(g,"Correct sequence! EXE: next round");if(g->data[1]>=19){g->status=NG_WON;g->phase=4;ng_message(g,"20 rounds complete");}}
   else{g->status=NG_LOST;g->phase=4;ng_message(g,"Sequence differs: compare below");}
   return true;
  }
  return ng_edit(g,key,"0123456789",(unsigned)g->data[0]);
 }
 return false;
}
static void consume(int32_t *timer,uint32_t delta){*timer=delta>=(uint32_t)*timer?0:*timer-(int32_t)delta;}
bool sq_quick_tick(NgGame *g,uint32_t delta_ms){
 if(g->status!=NG_PLAYING || !delta_ms)return false;
 if(g->id==29){
  int32_t oldsecond=(g->data[9]+999)/1000;
  if(g->phase==1)consume(&g->data[10],delta_ms);
  if(!g->mode){consume(&g->data[9],delta_ms);if(!g->data[9]){g->status=NG_WON;ng_message(g,"Time! Run complete");return true;}}
  return oldsecond!=(g->data[9]+999)/1000;
 }
 if(g->id==30){
  if(g->phase==0){consume(&g->data[2],delta_ms);if(!g->data[2]){g->phase=1;g->data[3]=350;return true;}}
  else if(g->phase==1){consume(&g->data[3],delta_ms);if(!g->data[3]){g->phase=2;return true;}}
  else if(g->phase==3)consume(&g->data[3],delta_ms);
 }
 return false;
}
bool sq_quick_valid(const NgGame *g){
 if(g->id<26||g->id>30||g->difficulty>(g->id>=29?2:3)||g->turn||g->cpu_pending)return false;
 if(g->mode>(g->id<=29?1:0))return false;
 unsigned size=g->id==26?4:g->id==27?(g->mode?4:3):g->id==28?(g->mode?5:4):1;
 if(g->rows!=size||g->cols!=size||g->cursor>=size*size)return false;
 size_t length=0;while(length<NG_INPUT && g->input[length])length++;if(length==NG_INPUT)return false;
 if(g->id<=28 && g->phase)return false;
 if(g->id==26){
  if(g->data[3]!=(g->mode?(g->difficulty==3?13:9+g->difficulty):0))return false;
  if(g->data[0]<1||g->data[0]>30||g->data[1]<0||g->data[1]>1||g->data[2]<0||g->data[2]>1)return false;
  int highest=0;for(unsigned i=0;i<16;i++){if(g->board[i]<0||g->board[i]>30||g->board[i]>g->data[0])return false;if(g->board[i]>highest)highest=g->board[i];}
  if(highest!=g->data[0]||g->data[1]!=(highest>=11)||g->data[2]!=(g->score==UINT32_MAX))return false;
  if(g->status!=(g->mode&&highest>=g->data[3]?NG_WON:sq_2048_can_move(g)?NG_PLAYING:NG_LOST))return false;
 }
 if(g->id==27){
  uint32_t seen=0;unsigned n=size*size;
  for(unsigned i=0;i<n;i++){if(g->board[i]<0||g->board[i]>=(int)n || (seen&(1u<<g->board[i])))return false;seen|=1u<<g->board[i];}
  if(g->board[g->cursor] || !sq_sliding_solvable(g))return false;
  if(g->difficulty==3){if(g->puzzle_id>=sq_bank_count(27,3,g->mode)||g->data[0]!=(g->mode?400:31)||g->data[1]<(g->mode?48:31)||g->data[1]>(g->mode?80:31))return false;}
  else if(g->pack_revision>=4){if(g->puzzle_id>=SQ_SLIDING_BAND_COUNT)return false;unsigned distance=sliding_banded(g)[(n+1)/2];if(g->data[0]!=(int)distance||g->data[1]!=(int)distance)return false;}
  else if(g->data[0]<80||g->data[0]>240||g->data[1])return false;
  if(g->status!=(sliding_goal(g)?NG_WON:NG_PLAYING))return false;
 }
 if(g->id==28){for(unsigned i=0;i<size*size;i++)if(g->board[i]<0||g->board[i]>1)return false;if(g->data[0]<-1||g->data[0]>=(int)(size*size)||g->data[1]<1||g->data[1]>25)return false;if(g->difficulty==3&&(g->puzzle_id>=30||g->data[1]!=sq_master_lights_min[g->mode][g->puzzle_id]))return false;if(g->pack_revision>=3&&!g->mode&&g->difficulty<3&&g->data[1]!=(g->difficulty==0?2:g->difficulty==1?4:5))return false;uint32_t solution;if(!sq_lights_solution(g,&solution))return false;if(g->status!=(lights_off(g)?NG_WON:NG_PLAYING))return false;}
 if(g->id==29){
  for(unsigned i=0;i<NG_CELLS;i++)if(g->board[i])return false;
  if(g->phase>1||length>6||g->data[0]<1||g->data[0]>1188||g->data[1]<1||g->data[1]>12||g->data[2]<-10||g->data[2]>10||g->data[3]<0||g->data[3]>3||g->data[4]<-100||g->data[4]>1200)return false;
  for(unsigned i=5;i<=8;i++)if(g->data[i]<0||g->data[i]>10000000)return false;
  if(g->data[9]<0||g->data[9]>60000||g->data[10]<0||g->data[10]>300)return false;
  int a=g->data[0],b=g->data[1],op=g->data[3];if(op==3&&a%b)return false;
  if((op==0?a+b:op==1?a-b:op==2?a*b:a/b)+g->data[2]!=g->data[4])return false;
  if((uint32_t)(g->data[5]+g->data[6])!=g->moves||g->data[7]>g->data[8]||g->data[8]>g->data[5])return false;
  if(g->status!=NG_PLAYING && g->status!=NG_WON)return false;
  if(!g->mode && ((g->status==NG_WON)!=(g->data[9]==0)))return false;
  for(size_t i=0;i<length;i++)if((g->input[i]<'0'||g->input[i]>'9') && !(i==0&&g->input[i]=='-'))return false;
 }
 if(g->id==30){
  if(g->phase>4||g->data[0]<3||g->data[0]>16||g->data[1]<0||g->data[1]>19||g->data[2]<0||g->data[2]>12600||g->data[3]<0||g->data[3]>350||length>(size_t)g->data[0])return false;
  for(int i=0;i<g->data[0];i++)if(g->board[i]<0||g->board[i]>9)return false;
  for(size_t i=0;i<length;i++)if(g->input[i]<'0'||g->input[i]>'9')return false;
  if(g->phase!=0 && g->data[2])return false;
  if(g->data[0]!=3+g->difficulty+g->data[1]/2)return false;
  if((g->phase==4)!=(g->status!=NG_PLAYING))return false;
  if(g->moves!=(uint32_t)g->data[1]+(g->phase>=3?1u:0u))return false;
  if(g->phase>=3){bool same=length==(size_t)g->data[0];for(size_t i=0;i<length;i++)if(g->input[i]-'0'!=g->board[i])same=false;
   if(g->phase==3&&!same)return false;
   if(g->phase==4 && (g->status==NG_WON?(!same||g->data[1]!=19):g->status!=NG_LOST||same))return false;
  }
 }
 return true;
}
void sq_quick_render(const NgGame *g,NgCanvas *c){
 char s[96];
 if(g->id==26){
  int x=16,y=29,cell=37;
  for(unsigned i=0;i<16;i++){
   int xx=x+(int)(i%4)*cell,yy=y+(int)(i/4)*cell,e=g->board[i];
   int color,ink;ng_2048_colors((unsigned)e,&color,&ink);
   ng_rect(c,xx,yy,cell-3,cell-3,color);
   if(e){snprintf(s,sizeof s,"%lu",(unsigned long)(UINT32_C(1)<<e));if(strlen(s)>5)snprintf(s,sizeof s,"2^%d",e);ng_center(c,xx+2,yy+12,cell-7,s,ink,1);}
  }
  ng_text(c,190,43,"SCORE",NG_MUTED,1);snprintf(s,sizeof s,"%lu",(unsigned long)g->score);ng_text_fit(c,190,63,196,s,NG_INK,2);
  ng_text(c,190,100,"BEST TILE",NG_MUTED,1);snprintf(s,sizeof s,"%lu",(unsigned long)(UINT32_C(1)<<g->data[0]));ng_text_fit(c,190,119,196,s,NG_BLUE,2);
  if(g->mode){snprintf(s,sizeof s,"TARGET %lu",(unsigned long)(UINT32_C(1)<<g->data[3]));ng_text(c,190,150,s,NG_BLUE,1);ng_text(c,190,168,"ARROWS: SLIDE",NG_MUTED,1);}
  else ng_text(c,190,157,"CLASSIC / ARROWS",NG_MUTED,1);
 }else if(g->id==27 || g->id==28){
  NgGridLayout l=ng_grid_layout(g->rows,g->cols,false);
  for(unsigned i=0;i<g->rows*g->cols;i++){
   s[0]='\0';if(g->id==27 && g->board[i])snprintf(s,sizeof s,"%d",g->board[i]);
   ng_grid_cell(c,l,g->cols,i,s,false,g->cursor==i,g->id==28?(g->board[i]?NG_YELLOW:NG_INK):(g->board[i]?NG_WHITE:NG_LINE));
   if(g->id==28 && g->data[0]==(int)i)ng_border(c,l.x+(int)(i%g->cols)*l.size+4,l.y+(int)(i/g->cols)*l.size+4,l.size-8,l.size-8,NG_MAGENTA,2);
  }
  int x=l.x+(int)g->cols*l.size+14;
  ng_text(c,x,47,"MOVES",NG_MUTED,1);snprintf(s,sizeof s,"%lu",(unsigned long)g->moves);ng_text_fit(c,x,65,386-x,s,NG_INK,2);
  if(g->id==27){ng_text(c,x,115,"ARROWS",NG_BLUE,1);ng_text(c,x,134,"MOVE BLANK",NG_MUTED,1);}
  else{ng_text(c,x,115,"EXE: PRESS",NG_BLUE,1);ng_text(c,x,134,"TURN ALL OFF",NG_MUTED,1);}
  if(g->difficulty==3 || (g->id==27&&g->pack_revision>=4)){snprintf(s,sizeof s,g->id==27&&g->mode&&g->difficulty==3?"START >= %ld":"START MIN %ld",(long)g->data[1]);ng_text_fit(c,x,158,386-x,s,NG_MUTED,1);}
 }else if(g->id==29){
  snprintf(s,sizeof s,"%s  OK %ld  MISS %ld",g->mode?"PRACTICE":"RUSH",(long)g->data[5],(long)g->data[6]);ng_text_fit(c,13,33,g->mode?373:302,s,NG_MUTED,1);
  const char *ops[]={"+","-","x","/"};
  if(g->difficulty==2)snprintf(s,sizeof s,"(%ld %s %ld) %c %ld",(long)g->data[0],ops[g->data[3]],(long)g->data[1],g->data[2]<0?'-':'+',(long)(g->data[2]<0?-g->data[2]:g->data[2]));
  else snprintf(s,sizeof s,"%ld %s %ld",(long)g->data[0],ops[g->data[3]],(long)g->data[1]);
  ng_center(c,8,65,380,s,NG_INK,2);ng_input(c,89,104,216,g->input);
  snprintf(s,sizeof s,"COMBO %ld   %s",(long)g->data[7],g->phase?"EXE: NEXT":"EXE: ANSWER");ng_center(c,8,151,380,s,NG_BLUE,1);
  if(!g->mode){snprintf(s,sizeof s,"%lds",(long)((g->data[9]+999)/1000));ng_text(c,328,33,s,NG_RED,1);}
 }else{
  snprintf(s,sizeof s,"ROUND %ld / 20     %ld DIGITS",(long)g->data[1]+1,(long)g->data[0]);ng_text(c,16,33,s,NG_MUTED,1);
  if(g->phase==0 || g->phase==3 || g->phase==4){
   for(int i=0;i<g->data[0];i++)s[i]=(char)('0'+g->board[i]);
   s[g->data[0]]='\0';
   ng_center(c,8,67,380,s,NG_BLUE,g->data[0]<=12?2:1);
   ng_center(c,8,115,380,g->phase==0?"REMEMBER THE ORDER":g->phase==3?"CORRECT - EXE TO CONTINUE":"ACTUAL SEQUENCE",NG_MUTED,1);
   if(g->phase==4)ng_input(c,64,142,268,g->input);
  }else if(g->phase==1){ng_center(c,8,81,380,". . .",NG_MUTED,2);}
  else{ng_center(c,8,65,380,"ENTER THE HIDDEN SEQUENCE",NG_INK,1);ng_input(c,45,102,306,g->input);snprintf(s,sizeof s,"%u / %ld DIGITS",(unsigned)strlen(g->input),(long)g->data[0]);ng_center(c,8,151,380,s,NG_MUTED,1);}
 }
}
