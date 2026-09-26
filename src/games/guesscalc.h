#ifndef NUMGAME_GUESSCALC_H
#define NUMGAME_GUESSCALC_H
#include "ng.h"
/* Rebuild a fresh revision-3/4 Make Target run for target 1..1000. The common
 * seed/run/supply identity remains unchanged. Module fields and RNG are reset
 * deterministically; revision4 maps the same two-record ordinal into the
 * selected target's verified bank. Default init then this call is one draw.
 * Invalid arguments leave every byte unchanged. Do not call to edit a live run. */
bool gc_target_init(NgGame *g,unsigned target);
extern const NgModule ng_guesscalc_extra[2];
unsigned gc_extra_bank_count(unsigned id,unsigned difficulty,unsigned mode);
/* Pure rules helpers used by the independent host regressions. */
int gc_blackbox_ray(unsigned n,const int16_t *atoms,unsigned port);
bool gc_blackbox_complete(const NgGame *g);
bool gc_cryptarithm_complete(const NgGame *g);
#endif
