#ifndef NUMGAME_GRIDS_EXTRA_H
#define NUMGAME_GRIDS_EXTRA_H
#include "ng.h"
#define GRIDS_EXTRA_BANK 30u
extern const NgModule ng_grids_extra[2];
typedef struct { uint8_t n,count,pos[25],clue[25],solution[25]; } GridsHashiPuzzle;
typedef struct { uint8_t n; uint32_t clue[18]; uint16_t solution[9]; } GridsNonoPuzzle;
extern const GridsHashiPuzzle grids_hashi_pack[150];
extern const GridsNonoPuzzle grids_nono_pack[120];
unsigned grids_extra_bank_count(unsigned id,unsigned difficulty,unsigned mode);
unsigned grids_extra_bank_id(unsigned id,unsigned difficulty,unsigned ordinal);
bool grids_extra_complete(const NgGame *g);
bool grids_hashi_rules(const NgGame *g,const GridsHashiPuzzle *p);
bool grids_nono_rules(const NgGame *g,const GridsNonoPuzzle *p);
/* Test/capture witness access is explicit; gameplay reads it only for REVEAL. */
bool grids_extra_witness(unsigned id,unsigned puzzle_id,int16_t out[NG_CELLS]);
#endif
