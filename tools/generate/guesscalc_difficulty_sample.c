/* Host-only difficulty inventory: invoke the real module initialization path.
 * No public app state, native IO, or bank/generator code is reimplemented. */
#include "ng.h"
#include "guesscalc.h"
#include "guesscalc_math.h"
#include "../../assets/guesscalc_cryptarithm.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

extern unsigned gc_bank_count(unsigned id,unsigned difficulty,unsigned mode);
static const NgModule *module(unsigned id){return id<=10?&ng_guesscalc[id-1]:&ng_guesscalc_extra[id-33];}
static void initialize(NgGame *g,unsigned id,unsigned level,unsigned mode,
                       uint32_t seed,unsigned ordinal,unsigned target){
 memset(g,0,sizeof(*g));g->id=(uint8_t)id;g->difficulty=(uint8_t)level;
 g->mode=(uint8_t)mode;g->seed=g->rng=seed;g->run_id=1;
 g->pack_revision=id==1||id==6?3:2;
 g->supply_seed=30u<<16;g->supply_index=ordinal;module(id)->init(g);
 if(id==6)assert(gc_target_init(g,target));
 assert(module(id)->valid(g));
}
static void output(unsigned id,unsigned level,unsigned mode,uint32_t seed,
                   unsigned ordinal,unsigned target,const char *cohort){
 NgGame g,copy;initialize(&g,id,level,mode,seed,ordinal,target);
 initialize(&copy,id,level,mode,seed,ordinal,target);assert(!memcmp(&g,&copy,sizeof(g)));
 printf("{\"id\":%u,\"level\":%u,\"mode\":%u,\"seed\":%u,\"ordinal\":%u,\"puzzle_id\":%u,\"cohort\":\"%s\",\"rows\":%u,\"board\":[",id,level,mode,(unsigned)seed,ordinal,(unsigned)g.puzzle_id,cohort,g.rows);
 for(unsigned i=0;i<NG_CELLS;i++)printf("%s%d",i?",":"",g.board[i]);
 printf("],\"fixed\":[");for(unsigned i=0;i<NG_CELLS;i++)printf("%s%u",i?",":"",g.fixed[i]);
 printf("],\"data\":[");for(unsigned i=0;i<NG_DATA;i++)printf("%s%ld",i?",":"",(long)g.data[i]);printf("]");
 if(id==6||id==10){
  assert(module(id)->action(&copy,NGK_ANSWER));assert(module(id)->valid(&copy));
  printf(",\"witness\":\"%s\"",copy.input);
  assert(module(id)->action(&copy,NGK_EXE));assert(copy.status==NG_WON&&module(id)->valid(&copy));
 }
 if(id==7){
  for(unsigned i=0;i<6;i++)if(g.board[i]==g.data[0]){
   snprintf(copy.input,sizeof(copy.input),"%d",g.board[i]);
   assert(module(id)->action(&copy,NGK_EXE));assert(copy.status==NG_WON&&module(id)->valid(&copy));
   break;
  }
 }
 if(id==33){
  int16_t atoms[NG_CELLS];for(unsigned i=0;i<NG_CELLS;i++)atoms[i]=g.fixed[i];
  printf(",\"rays\":[");for(unsigned i=0;i<4u*g.rows;i++){int ray=gc_blackbox_ray(g.rows,atoms,i);assert(ray!=-3);printf("%s%d",i?",":"",ray);}printf("]");
 }
 if(id==34){const GcCryptPack *p=&gc_crypt_pack[g.puzzle_id];printf(",\"words\":[\"%s\",\"%s\",\"%s\"]",p->words[0],p->words[1],p->words[2]);}
 puts("}");
}
int main(void){
 const unsigned ids[]={1,2,3,4,5,6,7,8,9,10,33,34};
 for(unsigned index=0;index<sizeof(ids)/sizeof(*ids);index++){
  unsigned id=ids[index];
  for(unsigned level=0;level<4;level++)for(unsigned mode=0;mode<module(id)->modes;mode++){
   unsigned count=id<=10?gc_bank_count(id,level,mode):gc_extra_bank_count(id,level,mode);
   if(count){for(unsigned ordinal=0;ordinal<count;ordinal++)output(id,level,mode,739,ordinal,24,"bank");}
   else if(id==6){
    for(unsigned target=1;target<=1000;target++)output(id,level,mode,UINT32_C(0x6e554d47),0,target,"target-domain");
    const unsigned targets[]={1,24,1000};
    for(unsigned t=0;t<3;t++)for(unsigned seed=1;seed<=256;seed++)output(id,level,mode,seed,0,targets[t],"seed-sample");
   }else for(unsigned seed=1;seed<=256;seed++)output(id,level,mode,seed,0,24,"seed-sample");
  }
 }
 return 0;
}
