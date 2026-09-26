/* Host probe of the exact native beta.6 ROM payload and expression parser. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "guesscalc_math.h"
#include "../../assets/guesscalc_target_beta6.h"
#include "../../assets/guesscalc_countdown_beta6.h"

static void emit(unsigned game,unsigned id,unsigned level,const GcBeta6CardRecord *r,const char *answer,unsigned n){
 GcExpression e;
 assert(strlen(answer)<40);
 assert(gc_expression(answer,game==7,&e));
 assert(gc_cards(&e,r->cards,n,game==6));
 assert(e.value.den==1&&e.value.num==r->target);
 assert(r->hint_a<n&&r->hint_b<n&&r->hint_a!=r->hint_b);
 assert(strchr("+-*/",r->hint_op));
 assert(r->cards[r->hint_a]>=1&&r->cards[r->hint_b]>=1);
 printf("{\"game_id\":%u,\"puzzle_id\":%u,\"difficulty\":%u,\"target\":%u,\"cards\":[",
        game,id,level,(unsigned)r->target);
 for(unsigned i=0;i<n;i++)printf("%s%d",i?",":"",r->cards[i]);
 printf("],\"answer\":\"%s\",\"hint\":[%u,%u,\"%c\"]}\n",answer,
        (unsigned)r->hint_a,(unsigned)r->hint_b,(char)r->hint_op);
}

int main(void){
 for(unsigned d=0;d<4;d++)for(unsigned t=0;t<5;t++)for(unsigned slot=0;slot<200;slot++){
  unsigned ordinal=(d*5+t)*200+slot;
  const GcBeta6CardRecord *r=&gc_target_beta6[ordinal];
  static const unsigned targets[]={10,24,50,100,200};
  assert(r->target==targets[t]);
  unsigned n=d<2?4:d+3;
  for(unsigned i=0;i<n;i++)assert(r->cards[i]>=1&&r->cards[i]<=999);
  for(unsigned i=n;i<6;i++)assert(r->cards[i]==0);
  emit(6,16240+ordinal,d,r,gc_target_beta6_answer[d]+r->answer_offset,n);
 }
 for(unsigned d=0;d<4;d++)for(unsigned slot=0;slot<200;slot++){
  unsigned ordinal=d*200+slot;
  const GcBeta6CardRecord *r=&gc_countdown_beta6[ordinal];
  assert(r->target>=100&&r->target<=999);
  emit(7,200+ordinal,d,r,gc_countdown_beta6_answer+r->answer_offset,6);
 }
 return 0;
}
