#ifndef NUMGAME_GRIDS_INTERNAL_H
#define NUMGAME_GRIDS_INTERNAL_H
#include "ng.h"
/* Public clues and a separately audited solution witness. Never render solution
 * except one-cell explicitly marked REVEAL. Metadata is immutable flash data. */
typedef struct {
 uint8_t id,difficulty,n;
 uint8_t cells[81],solution[81];
 int16_t a[81],b[81];
} GridsPuzzle;
#include "grids_pack_data.h"
#define GRIDS_FREE_MAGIC_ID UINT32_C(0xfffffff0)
/* Revision-3 FREE magic orders use four rules-only IDs, outside the bank. */
/* Decode one stable record. No whole-bank runtime decompression. */
bool grids_decode(uint32_t stable_id,GridsPuzzle *out);
/* One shared 490-byte cache; pointer remains valid only until a different
 * record is requested. Never retain it across another game's validation. */
const GridsPuzzle *grids_record(uint32_t stable_id);
unsigned grids_bank_count(unsigned id,unsigned difficulty,unsigned mode);
uint32_t grids_bank_id(unsigned id,unsigned difficulty,unsigned ordinal);
const GridsPuzzle *grids_puzzle(const NgGame *g);
bool grids_complete(const NgGame *g);
bool grids_rules_complete(const NgGame *g,const GridsPuzzle *p);
#endif
