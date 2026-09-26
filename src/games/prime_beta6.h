#ifndef NG_PRIME_BETA6_H
#define NG_PRIME_BETA6_H

#include <stdbool.h>
#include <stdint.h>

/* Revision-5 Prime Factor content. EASY has 128 starts; the other levels have
   128 starts in each of their two adjacent decimal-digit groups. A target of
   zero denotes an invalid level/index. No mutable state or RNG is used here. */
unsigned ng_prime_beta6_count(unsigned difficulty);
uint32_t ng_prime_beta6_target(unsigned difficulty, unsigned index);
bool ng_prime_beta6_valid(unsigned difficulty, uint32_t target);

#endif
