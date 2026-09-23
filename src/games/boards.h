#ifndef NUMGAME_BOARDS_H
#define NUMGAME_BOARDS_H
#include "ng.h"
#include "ui.h"
#define NB_SIDE 8
#define NB_CELLS 64
#define NB_EDGES 144
#define NB_PACK_COUNT 120
typedef struct {uint8_t size,clues[NB_CELLS];} NbPuzzle;
extern const NbPuzzle nb_shikaku_pack[NB_PACK_COUNT],nb_slitherlink_pack[NB_PACK_COUNT];
extern const NgModule ng_boards[2];
unsigned nb_bank_count(unsigned id,unsigned difficulty,unsigned mode);
/* Independent rule validators; they do not read puzzle witnesses. */
bool nb_shikaku_complete(const NgGame *g);
bool nb_shikaku_partial(const NgGame *g);
bool nb_slither_complete(const NgGame *g);
unsigned nb_edge_count(unsigned n);
unsigned nb_edge_get(const NgGame *g,unsigned edge);
bool nb_edge_set(NgGame *g,unsigned edge,unsigned value);
void nb_edge_vertices(unsigned n,unsigned edge,unsigned *a,unsigned *b);
#endif
