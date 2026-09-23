/* Standalone harness registration only; omit when linking the actual core. */
#include "../src/games/boards.h"
#include "diagnostics.h"
#include <string.h>
const NgModule *ng_module(unsigned id){return id>=31&&id<=32?&ng_boards[id-31]:NULL;}
unsigned ng_difficulty_count(unsigned id){return id>=31&&id<=32?4:0;}
unsigned ng_level_generation_policy(unsigned id,unsigned difficulty,unsigned mode){(void)id;(void)difficulty;(void)mode;return NG_SUPPLY_BANK;}
void ng_diag_codec(size_t bytes){(void)bytes;}
void ng_new(NgGame *g,unsigned id,unsigned difficulty,unsigned mode,uint32_t seed,uint32_t run_id){
 memset(g,0,sizeof *g);const NgModule *m=ng_module(id);if(!m)return;g->id=(uint8_t)id;g->difficulty=(uint8_t)(difficulty<4?difficulty:1);g->mode=(uint8_t)mode;g->seed=g->rng=seed?seed:1;g->run_id=run_id?run_id:1;g->pack_revision=2;g->generation_policy=NG_SUPPLY_BANK;m->init(g);
}
bool ng_valid(const NgGame *g){
 const NgModule *m=ng_module(g->id);if(!m||g->difficulty>3||g->mode>=m->modes||g->status>NG_DRAW||g->assisted>1||g->recorded>1||g->reserved||!g->seed||!g->rng||!g->run_id)return false;
 if(!memchr(g->input,0,sizeof g->input)||!memchr(g->message,0,sizeof g->message))return false;
 for(unsigned i=0;i<NG_HISTORY;i++)if(!memchr(g->history[i],0,sizeof g->history[i]))return false;
 return m->valid(g);
}
