/* Host-only independent export of the actual C decoder, not shipped code. */
#include "grids_internal.h"
#include <stdio.h>
int main(void)
{
 for(unsigned index=0;index<GRIDS_PACK_COUNT;index++){
  GridsPuzzle p;if(!grids_decode(index,&p))return 1;
  printf("[%u,%u,%u",p.id,p.difficulty,p.n);
  for(unsigned i=0;i<81;i++)printf(",%u",p.cells[i]);
  for(unsigned i=0;i<81;i++)printf(",%u",p.solution[i]);
  for(unsigned i=0;i<81;i++)printf(",%d",p.a[i]);
  for(unsigned i=0;i<81;i++)printf(",%d",p.b[i]);
  puts("]");
 }
 return 0;
}
