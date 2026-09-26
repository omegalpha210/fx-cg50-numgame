#include "prime_beta6.h"
#include "../../assets/guesscalc_prime_beta6.h"

enum { PRIME_GROUP_SIZE = 128, PRIME_LEVEL_COUNT = 4 };
_Static_assert(sizeof(ng_prime_beta6_targets) / sizeof(ng_prime_beta6_targets[0]) == 896,
               "Prime Factor target table size must match level offsets");

unsigned ng_prime_beta6_count(unsigned difficulty)
{
 return difficulty >= PRIME_LEVEL_COUNT ? 0u :
        difficulty == 0 ? PRIME_GROUP_SIZE : 2u * PRIME_GROUP_SIZE;
}

static unsigned prime_offset(unsigned difficulty)
{
 return difficulty == 0 ? 0u : PRIME_GROUP_SIZE +
        (difficulty - 1u) * 2u * PRIME_GROUP_SIZE;
}

uint32_t ng_prime_beta6_target(unsigned difficulty, unsigned index)
{
 unsigned count = ng_prime_beta6_count(difficulty);
 return index < count ? ng_prime_beta6_targets[prime_offset(difficulty) + index] : 0u;
}

bool ng_prime_beta6_valid(unsigned difficulty, uint32_t target)
{
 unsigned count = ng_prime_beta6_count(difficulty);
 if(!count || target < 100u || target > 999999u) return false;
 unsigned groups = count / PRIME_GROUP_SIZE, base = prime_offset(difficulty);
 for(unsigned group = 0; group < groups; group++) {
  unsigned low = base + group * PRIME_GROUP_SIZE;
  unsigned high = low + PRIME_GROUP_SIZE;
  while(low < high) {
   unsigned mid = low + (high - low) / 2u;
   uint32_t value = ng_prime_beta6_targets[mid];
   if(value < target) low = mid + 1u;
   else high = mid;
  }
  if(low < base + (group + 1u) * PRIME_GROUP_SIZE &&
     ng_prime_beta6_targets[low] == target) return true;
 }
 return false;
}
