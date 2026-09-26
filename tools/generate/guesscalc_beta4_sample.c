/* Host-only C/JSON parity probe for every revision4 target record. */
#include "ng.h"
#include "guesscalc.h"
#include "guesscalc_math.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(int argc,char **argv){
 (void)argv;
 if(argc>1){
  unsigned target,count=0;char line[128],expression[97];
  while(fgets(line,sizeof(line),stdin)){
   assert(sscanf(line,"%u %96[^\n]",&target,expression)==2);GcExpression e;
   assert(gc_expression(expression,false,&e)&&e.value.num==(int64_t)target*e.value.den);++count;
  }
  printf("PASS %u exact expressions in native bounded parser\n",count);return 0;
 }
 for(unsigned d=0;d<4;d++)for(unsigned target=1;target<=1000;target++)for(unsigned ordinal=0;ordinal<2;ordinal++){
  NgGame g={0};g.id=6;g.difficulty=(uint8_t)d;g.seed=g.rng=739;g.run_id=1;g.pack_revision=4;
  g.supply_seed=2u<<16;g.supply_index=ordinal;ng_guesscalc[5].init(&g);assert(gc_target_init(&g,target));assert(ng_guesscalc[5].valid(&g));
  assert(ng_guesscalc[5].action(&g,NGK_ANSWER));
  printf("{\"puzzle_id\":%u,\"cards\":[",(unsigned)g.puzzle_id);
  for(unsigned i=0;i<(unsigned)g.data[1];i++)printf("%s%d",i?",":"",g.board[i]);
  printf("],\"witness\":\"%s\",\"rng\":%u}\n",g.input,(unsigned)g.rng);
  assert(ng_guesscalc[5].action(&g,NGK_EXE));assert(g.status==NG_WON&&ng_guesscalc[5].valid(&g));
 }
 return 0;
}
