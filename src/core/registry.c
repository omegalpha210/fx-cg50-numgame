#include "ng.h"
#include "diagnostics.h"
#include <string.h>
unsigned gc_bank_count(unsigned id,unsigned difficulty,unsigned mode);
unsigned grids_bank_count(unsigned id,unsigned difficulty,unsigned mode);
unsigned nb_bank_count(unsigned id,unsigned difficulty,unsigned mode);
unsigned sq_bank_count(unsigned id,unsigned difficulty,unsigned mode);
bool ng_has_hell(unsigned id){return id>=11 && id<=15;}
unsigned ng_regular_difficulty_count(unsigned id){return id==29 || id==30?3:4;}
unsigned ng_difficulty_count(unsigned id){return ng_has_hell(id)?5:ng_regular_difficulty_count(id);}
const char *ng_level_name(unsigned difficulty)
{static const char *const names[NG_LEVEL_COUNT]={"EASY","NORMAL","HARD","MASTER","HELL"};return difficulty<NG_LEVEL_COUNT?names[difficulty]:"UNKNOWN";}
unsigned ng_generation_policy(unsigned id)
{
 if(id==1 || id==10 || id==29 || id==30)return NG_SUPPLY_RUNTIME;
 if(id==8 || (id>=21 && id<=25) || id==27 || id==28)return NG_SUPPLY_HYBRID;
 if(id==26)return NG_SUPPLY_RULES;
 return id==19?NG_SUPPLY_TRANSFORMS:NG_SUPPLY_BANK;
}
unsigned ng_level_generation_policy(unsigned id,unsigned difficulty,unsigned mode)
{
 if(id==19)return NG_SUPPLY_TRANSFORMS;
 if(ng_bank_count(id,difficulty,mode))return NG_SUPPLY_BANK;
 if(id>=21 && id<=26)return NG_SUPPLY_RULES;
 return NG_SUPPLY_RUNTIME;
}
unsigned ng_bank_count(unsigned id,unsigned difficulty,unsigned mode)
{
 const NgModule *m=ng_module(id);if(!m || difficulty>=ng_difficulty_count(id) || mode>=m->modes)return 0;
 if(id<=10)return gc_bank_count(id,difficulty,mode);
 if(id<=20)return grids_bank_count(id,difficulty,mode);
 if(id>=21 && id<=28)return sq_bank_count(id,difficulty,mode);
 if(id==31 || id==32)return nb_bank_count(id,difficulty,mode);
 return 0;
}
unsigned ng_visible_id(unsigned index){return index<28?index+1:index<NG_GAME_COUNT?index+3:0;}
int ng_catalog_index(unsigned id){return id>=1 && id<=28?(int)id-1:id==31 || id==32?(int)id-3:-1;}
const NgModule *ng_module(unsigned id)
{
 if(id>=1 && id<=10)return &ng_guesscalc[id-1];
 if(id>=11 && id<=20)return &ng_grids[id-11];
 if(id>=21 && id<=30)return &ng_strategyquick[id-21];
 if(id>=31 && id<=32)return &ng_boards[id-31];
 return NULL;
}
void ng_new(NgGame *g,unsigned id,unsigned difficulty,unsigned mode,uint32_t seed,uint32_t run_id)
{ng_new_supply(g,id,difficulty,mode,seed,run_id,0,0);}
void ng_new_supply(NgGame *g,unsigned id,unsigned difficulty,unsigned mode,uint32_t seed,uint32_t run_id,uint32_t supply_seed,uint32_t supply_index)
{
 memset(g,0,sizeof(*g));const NgModule *m=ng_module(id);
 if(!m || !m->init)return;
 g->id=(uint8_t)id;g->difficulty=(uint8_t)(difficulty<ng_difficulty_count(id)?difficulty:1);
 g->mode=(uint8_t)(mode<m->modes?mode:0);g->seed=seed?seed:1;
 g->rng=g->seed;g->run_id=run_id?run_id:1;g->supply_seed=supply_seed;g->supply_index=supply_index;g->pack_revision=2;
 g->generation_policy=ng_level_generation_policy(g->id,g->difficulty,g->mode);
 NgDiagScope scope=ng_diag_begin(NGOP_NEW,id);m->init(g);ng_diag_end(scope);
}
static bool valid(const NgGame *g)
{
 const NgModule *m=ng_module(g->id);
 if(!m || !m->valid || g->difficulty>=ng_difficulty_count(g->id) || g->mode>=m->modes ||
 g->status>NG_DRAW || g->assisted>1 || g->recorded>1 || g->turn>1 ||
 g->rows>9 || g->cols>9 || g->history_count>NG_HISTORY ||
 g->cursor>=(g->id==32?144:NG_CELLS) || g->scroll>NG_HISTORY || g->notes_mode>1 ||
 g->cpu_pending>1 || g->reserved || !g->seed || !g->rng || !g->run_id ||
 g->pack_revision<1 || g->pack_revision>2 || g->generation_policy!=ng_level_generation_policy(g->id,g->difficulty,g->mode))return false;
 if(!memchr(g->input,0,sizeof(g->input)) || !memchr(g->message,0,sizeof(g->message)))return false;
 for(unsigned i=0;i<NG_HISTORY;i++)if(!memchr(g->history[i],0,sizeof(g->history[i])))return false;
 for(unsigned i=0;i<NG_CELLS;i++)if(g->fixed[i]>1 || g->notes[i]>0x3ff)return false;
 return m->valid(g);
}
bool ng_valid(const NgGame *g)
{NgDiagScope scope=ng_diag_begin(NGOP_VALIDATE,g->id);bool ok=valid(g);ng_diag_end(scope);return ok;}
