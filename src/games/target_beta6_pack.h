#ifndef TARGET_BETA6_PACK_H
#define TARGET_BETA6_PACK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Stable ordinal 0..3999 is puzzle_id - 16240. No heap or bank-wide cache. */
typedef struct {
 int16_t cards[6];
 uint16_t target;
 uint8_t card_count, hint_a, hint_b, hint_op;
} GcTargetBeta6;

bool gc_target_beta6_read(unsigned ordinal, GcTargetBeta6 *out);
bool gc_target_beta6_answer_text(unsigned ordinal, char *out, size_t capacity);

#endif
