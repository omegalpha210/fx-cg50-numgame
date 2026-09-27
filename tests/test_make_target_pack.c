#include "target_beta6_pack.h"
#include "../assets/guesscalc_target_beta6.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
 assert(!gc_target_beta6_read(4000, &(GcTargetBeta6){0}));
 assert(!gc_target_beta6_read(0, NULL));
 assert(!gc_target_beta6_answer_text(4000, (char[40]){0}, 40));
 assert(!gc_target_beta6_answer_text(0, NULL, 40));
 const unsigned edges[] = {0,199,200,999,1000,1199,1200,1999,2000,2999,3000,3999};
 for(unsigned i=0;i<sizeof edges/sizeof edges[0];i++){
  unsigned ordinal=edges[i],level=ordinal/1000;
  GcTargetBeta6 decoded;
  char answer[40],short_buffer[2]={'X','Y'};
  const GcBeta6CardRecord *reference=&gc_target_beta6[ordinal];
  assert(gc_target_beta6_read(ordinal,&decoded));
  assert(decoded.target==reference->target);
  assert(!memcmp(decoded.cards,reference->cards,sizeof decoded.cards));
  assert(decoded.hint_a==reference->hint_a&&decoded.hint_b==reference->hint_b&&decoded.hint_op==reference->hint_op);
  assert(gc_target_beta6_answer_text(ordinal,answer,sizeof answer));
  assert(!strcmp(answer,gc_target_beta6_answer[level]+reference->answer_offset));
  assert(!gc_target_beta6_answer_text(ordinal,short_buffer,sizeof short_buffer));
  assert(short_buffer[0]=='X'&&short_buffer[1]=='Y');
 }
 puts("PASS Make Target packed random-access edges and bounded output");
 return 0;
}
