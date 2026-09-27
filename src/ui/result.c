#include "ui.h"
#include <stdio.h>
#include <string.h>

/* Only metrics with an actual game meaning are named on the result screen. */
void ng_result_summary(const NgGame *g,bool show_time,char out[96])
{
 out[0]=0;
 switch(g->id){
 case 1:case 2:
  snprintf(out,96,"TRIES %lu",(unsigned long)g->moves);break;
 case 7:
  if(g->data[3]==1000000000)
   snprintf(out,96,"SCORE %lu    NO SUBMISSION",(unsigned long)g->score);
  else snprintf(out,96,"SCORE %lu    BEST DIST %ld",
   (unsigned long)g->score,(long)g->data[3]);
  break;
 case 21:case 22:case 23:case 24:case 25:case 37:
  snprintf(out,96,"TURNS %lu",(unsigned long)g->moves);break;
 case 26:
  snprintf(out,96,"SCORE %lu",(unsigned long)g->score);break;
 case 27:
  snprintf(out,96,"MOVES %lu",(unsigned long)g->moves);break;
 case 28:
  snprintf(out,96,"PRESSES %lu",(unsigned long)g->moves);break;
 case 29:
  snprintf(out,96,"CORRECT %ld/%lu    SCORE %lu",
   (long)g->data[5],(unsigned long)g->moves,(unsigned long)g->score);break;
 case 30:
  snprintf(out,96,"ROUNDS %lu/20    SCORE %lu",
   (unsigned long)g->moves,(unsigned long)g->score);break;
 default:
  if(show_time){
   unsigned long seconds=(unsigned long)(g->elapsed_ms/1000u);
   snprintf(out,96,"TIME %lu:%02lu",seconds/60u,seconds%60u);
  }
  break;
 }
 if(g->assisted){
  size_t used=strlen(out);
  snprintf(out+used,96-used,"%sASSISTED",used?"    ":"");
 }
}
