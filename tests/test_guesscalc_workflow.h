#ifndef TEST_GUESSCALC_WORKFLOW_H
#define TEST_GUESSCALC_WORKFLOW_H
/* HOST TEST SUPPORT ONLY. Deliberately sees witnesses; never ship this header. */
#include "ng.h"
#include "../assets/guesscalc_packs.h"
#include "../assets/guesscalc_master.h"
#include "../assets/guesscalc_beta4.h"
#include <stdio.h>
#include <string.h>
typedef void (*GcTestKey)(void *ctx,int key);
static inline void gc_test_type(void *ctx,GcTestKey press,const char *text){for(unsigned i=0;text[i];i++)press(ctx,(unsigned char)text[i]);}
static inline void gc_test_solve(NgGame *g,void *ctx,GcTestKey press){
 char answer[97]={0};
 switch(g->id){
 case 1:case 2:for(unsigned i=0;i<(unsigned)g->data[0];i++)answer[i]=(char)g->board[i];break;
 case 3:{if(g->difficulty==3){for(unsigned i=0;i<6;i++)answer[i]=(char)('0'+gc_master_mind[g->puzzle_id-90].solution[i]);}else{const GcMindPack *p=&gc_mind_pack[g->puzzle_id];for(unsigned i=0;i<p->n;i++)answer[i]=(char)('0'+p->solution[i]);}break;}
 case 4:snprintf(answer,sizeof(answer),"%d",g->difficulty==3?gc_master_lock[g->puzzle_id-90].solution:gc_lock_pack[g->puzzle_id].solution);break;
 case 5:snprintf(answer,sizeof(answer),"%d",g->pack_revision==4&&g->difficulty<2?gc_sequence_beta4[g->puzzle_id-120].seq[6]:g->difficulty==3?gc_master_sequence[g->puzzle_id-90].seq[6]:gc_sequence_pack[g->puzzle_id].seq[6]);break;
 case 6:if(g->pack_revision>=3){NgGame copy=*g;(void)ng_guesscalc[5].action(&copy,NGK_ANSWER);snprintf(answer,sizeof(answer),"%s",copy.input);}else snprintf(answer,sizeof(answer),"%s",g->difficulty==3?gc_master_target[g->puzzle_id-180].answer:gc_target_pack[g->puzzle_id].answer);break;
 case 7:if(g->pack_revision>=5){NgGame copy=*g;(void)ng_guesscalc[6].action(&copy,NGK_ANSWER);snprintf(answer,sizeof answer,"%s",copy.input);}else snprintf(answer,sizeof(answer),"%s",g->pack_revision==4&&g->difficulty==2?gc_countdown_beta4[g->puzzle_id-120].answer:g->difficulty==3?gc_master_countdown[g->puzzle_id-90].answer:gc_countdown_pack[g->puzzle_id].answer);break;
 case 8:for(unsigned i=0;i<(unsigned)g->data[0]-1;i++){while(g->cursor!=i)press(ctx,NGK_RIGHT);press(ctx,g->data[16+i]);}press(ctx,NGK_EXE);return;
 case 9:for(unsigned i=0;i<9;i++){while(g->cursor/3!=i/3)press(ctx,NGK_DOWN);while(g->cursor%3!=i%3)press(ctx,NGK_RIGHT);if(!g->fixed[i])press(ctx,'0'+(g->difficulty==3?gc_master_cross[g->puzzle_id-90].solution[i]:gc_cross_pack[g->puzzle_id].solution[i]));}press(ctx,NGK_EXE);return;
 case 10:{int n=g->data[0];size_t pos=0;for(int p=2;p<=n;p++)while(n%p==0){int k=snprintf(answer+pos,sizeof(answer)-pos,"%s%d",pos?"*":"",p);pos+=(size_t)k;n/=p;}break;}
 default:return;
 }
 while(g->input[0])press(ctx,NGK_DEL);
 gc_test_type(ctx,press,answer);press(ctx,NGK_EXE);
}
#endif
