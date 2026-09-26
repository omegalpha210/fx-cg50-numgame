#include "grids_internal.h"
#include <string.h>
typedef struct {const uint8_t *bytes;unsigned size,pos;bool ok;} Reader;
static unsigned width(unsigned maximum){unsigned bits=1;while(maximum>>bits)bits++;return bits;}
static unsigned get(Reader *r,unsigned bits)
{
 if(bits>16||r->pos+bits>r->size*8){r->ok=false;return 0;}
 unsigned value=0;for(unsigned k=0;k<bits;k++,r->pos++)value|=((r->bytes[r->pos/8]>>(r->pos%8))&1u)<<k;
 return value;
}
bool grids_decode(uint32_t stable_id,GridsPuzzle *p)
{
 if(!p)return false;
 if(stable_id>=GRIDS_FREE_MAGIC_ID&&stable_id<GRIDS_FREE_MAGIC_ID+4u){
  /* Construction witnesses support diagnostics only; completion uses sums.
     One shared decoded cache remains sufficient, with no extra runtime state. */
  static const uint8_t witness[86]={
   2,7,6,9,5,1,4,3,8,
   16,2,3,13,5,11,10,8,9,7,6,12,4,14,15,1,
   17,24,1,8,15,23,5,7,14,16,4,6,13,20,22,10,12,19,21,3,11,18,25,2,9,
   35,1,6,26,19,24,3,32,7,21,23,25,31,9,2,22,27,20,8,28,33,17,10,15,30,5,34,12,14,16,4,36,29,13,18,11
  };
  static const uint8_t offset[4]={0,9,25,50};unsigned d=stable_id-GRIDS_FREE_MAGIC_ID,n=3+d;
  memset(p,0,sizeof *p);p->id=19;p->difficulty=(uint8_t)d;p->n=(uint8_t)n;
  memcpy(p->solution,witness+offset[d],n*n);return true;
 }
 if(stable_id>=GRIDS_PACK_COUNT)return false;
 uint32_t start=grids_pack_offsets[stable_id],end=grids_pack_offsets[stable_id+1];
 if(start>=end||end>GRIDS_PACK_BYTES)return false;
 Reader r={grids_pack_bytes+start,(unsigned)(end-start),0,true};memset(p,0,sizeof *p);
 p->id=(uint8_t)get(&r,8);p->difficulty=(uint8_t)get(&r,8);p->n=(uint8_t)get(&r,8);
 unsigned id=p->id,n=p->n,nn=n*n;if(id<11||id>20||n<3||n>9||p->difficulty>4)return false;
 unsigned cv=id==17?2:id==13?1:(id==18||id==19)?nn:id==20?12:n;
 unsigned sv=(id==16||id==17||id==20)?1:id==13?9:(id==18||id==19)?nn:n;
 for(unsigned i=0;i<nn;i++){unsigned v=get(&r,width(cv));if(v>cv)return false;p->cells[i]=(uint8_t)(id==17&&v==2?255:id==13&&v?255:v);}
 for(unsigned i=0;i<nn;i++){unsigned v=get(&r,width(sv));if(v>sv)return false;p->solution[i]=(uint8_t)v;}
 if(id==12){
  unsigned cages=get(&r,6);if(!cages||cages>40)return false;
  for(unsigned i=0;i<nn;i++){unsigned v=get(&r,width(cages-1));if(v>=cages)return false;p->a[i]=(int16_t)v;}
  for(unsigned i=0;i<cages;i++){p->b[2*i]=(int16_t)get(&r,3);p->b[2*i+1]=(int16_t)get(&r,15);}
 }else if(id==13){
  for(unsigned i=0;i<nn;i++){unsigned a=get(&r,1),b=get(&r,1);if(a)p->a[i]=(int16_t)get(&r,6);if(b)p->b[i]=(int16_t)get(&r,6);}
 }else if(id==14){
  for(unsigned side=0;side<2;side++)for(unsigned i=0;i<nn;i++){unsigned v=get(&r,2);if(v>2)return false;(side?p->b:p->a)[i]=(int16_t)(v==2?-1:(int)v);}
 }else if(id==15){for(unsigned i=0;i<4*n;i++)p->a[i]=(int16_t)get(&r,width(n));}
 else if(id==20){for(unsigned i=0;i<2*n;i++)p->a[i]=(int16_t)get(&r,7);}
 return r.ok&&(r.pos+7)/8==r.size;
}
const GridsPuzzle *grids_record(uint32_t stable_id)
{
 static GridsPuzzle cache;static uint32_t cache_id=UINT32_MAX;
 if(stable_id>=GRIDS_PACK_COUNT&&!(stable_id>=GRIDS_FREE_MAGIC_ID&&stable_id<GRIDS_FREE_MAGIC_ID+4u))return NULL;
 if(stable_id!=cache_id){cache_id=UINT32_MAX;if(!grids_decode(stable_id,&cache))return NULL;cache_id=stable_id;}
 return &cache;
}
unsigned grids_bank_count(unsigned id,unsigned difficulty,unsigned mode)
{
 if(id<11||id>20||difficulty>4||mode)return 0;
 return grids_bank_groups[(id-11)*5+difficulty][1];
}
uint32_t grids_bank_id(unsigned id,unsigned difficulty,unsigned ordinal)
{
 if(id<11||id>20||difficulty>4)return UINT32_MAX;
 const uint16_t *group=grids_bank_groups[(id-11)*5+difficulty];
 return ordinal<group[1]?grids_bank_ids[group[0]+ordinal]:UINT32_MAX;
}
