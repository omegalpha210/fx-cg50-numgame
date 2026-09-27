/* Same-process host comparison. Not an fx-CG50 latency measurement. */
#define _POSIX_C_SOURCE 200809L
#include "target_beta6_pack.h"
#include "../assets/guesscalc_target_beta6.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static volatile unsigned observed;
static double seconds(void)
{
 struct timespec value;
 assert(!clock_gettime(CLOCK_MONOTONIC,&value));
 return (double)value.tv_sec+(double)value.tv_nsec/1e9;
}
static unsigned ordinal(unsigned i){return (i*1597u)%4000u;}
static void run(unsigned mode,unsigned packed)
{
 char answer[40],hint[64];
 for(unsigned i=0;i<4000;i++){
  unsigned id=ordinal(i),level=id/1000;
  const GcBeta6CardRecord *old=&gc_target_beta6[id];
  if(mode==0){
   if(packed){GcTargetBeta6 record;assert(gc_target_beta6_read(id,&record));observed+=(unsigned)record.cards[0]+record.target;}
   else observed+=(unsigned)old->cards[0]+old->target;
  }else if(mode==1){
   if(packed){assert(gc_target_beta6_answer_text(id,answer,sizeof answer));}
   else snprintf(answer,sizeof answer,"%s",gc_target_beta6_answer[level]+old->answer_offset);
   observed+=(unsigned char)answer[0]+(unsigned)strlen(answer);
  }else{
   unsigned a,b;char op;int left,right;
   if(packed){GcTargetBeta6 record;assert(gc_target_beta6_read(id,&record));a=record.hint_a;b=record.hint_b;op=(char)record.hint_op;left=record.cards[a];right=record.cards[b];}
   else {a=old->hint_a;b=old->hint_b;op=(char)old->hint_op;left=old->cards[a];right=old->cards[b];}
   snprintf(hint,sizeof hint,"One first step: %d %c %d.",left,op,right);
   observed+=(unsigned char)hint[0]+(unsigned)strlen(hint);
  }
 }
}
static int compare(const void *a,const void *b)
{double x=*(const double *)a,y=*(const double *)b;return (x>y)-(x<y);}
int main(void)
{
 static const char *name[]={"load","answer","hint"};
 for(unsigned mode=0;mode<3;mode++)for(unsigned packed=0;packed<2;packed++){
  double times[51];
  run(mode,packed);
  for(unsigned repeat=0;repeat<51;repeat++){
   double begin=seconds();run(mode,packed);times[repeat]=(seconds()-begin)*1000000/4000;
  }
  qsort(times,51,sizeof times[0],compare);
  printf("%s %s host us/record p50=%.3f p95=%.3f\n",name[mode],packed?"packed":"original",times[25],times[48]);
 }
 printf("observed=%u\n",observed);
 return 0;
}
