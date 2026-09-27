#include "target_beta6_pack.h"
#include "../../assets/guesscalc_target_beta6_packed.h"
#include <string.h>

_Static_assert(sizeof(gc_target_beta6_packed) == 63000, "Make Target pack size");

static bool address(unsigned ordinal, unsigned *difficulty, const uint8_t **record)
{
 static const uint32_t starts[4] = {0,13000,26000,43000};
 static const uint8_t widths[4] = {13,13,17,20};
 if(ordinal >= 4000 || !difficulty || !record) return false;
 unsigned level = ordinal / 1000;
 uint32_t offset = starts[level] + (ordinal % 1000) * widths[level];
 if(offset > sizeof(gc_target_beta6_packed) ||
    widths[level] > sizeof(gc_target_beta6_packed) - offset) return false;
 *difficulty = level;
 *record = gc_target_beta6_packed + offset;
 return true;
}

bool gc_target_beta6_read(unsigned ordinal, GcTargetBeta6 *out)
{
 static const uint8_t card_bytes[4] = {5,5,7,8};
 static const uint16_t targets[5] = {10,24,50,100,200};
 const uint8_t *record;
 unsigned difficulty;
 if(!out || !address(ordinal,&difficulty,&record)) return false;
 GcTargetBeta6 value = {0};
 value.card_count = difficulty < 2 ? 4 : (uint8_t)(difficulty + 3);
 value.target = targets[(ordinal % 1000) / 200];
 for(unsigned i=0;i<value.card_count;i++){
  uint16_t card = 0;
  for(unsigned b=0;b<10;b++){
   unsigned bit = i*10+b;
   if(bit/8 >= card_bytes[difficulty]) return false;
   card |= (uint16_t)(((record[bit/8] >> (bit%8)) & 1u) << b);
  }
  if(card < 1 || card > 999) return false;
  value.cards[i] = (int16_t)card;
 }
 for(unsigned bit=value.card_count*10;bit<card_bytes[difficulty]*8;bit++)
  if((record[bit/8] >> (bit%8)) & 1u) return false;
 uint8_t hint = record[card_bytes[difficulty]];
 value.hint_a = hint & 7u;
 value.hint_b = (hint >> 3) & 7u;
 if(value.hint_a >= value.card_count || value.hint_b >= value.card_count ||
    value.hint_a == value.hint_b) return false;
 value.hint_op = (uint8_t)"+-*/"[hint >> 6];
 *out = value;
 return true;
}

bool gc_target_beta6_answer_text(unsigned ordinal, char *out, size_t capacity)
{
 static const uint8_t card_bytes[4] = {5,5,7,8};
 static const uint8_t answer_bytes[4] = {7,7,9,11};
 static const char operators[] = "+-*/()";
 const uint8_t *record;
 unsigned difficulty;
 GcTargetBeta6 value;
 if(!out || !address(ordinal,&difficulty,&record) ||
    !gc_target_beta6_read(ordinal,&value)) return false;
 const uint8_t *stream = record + card_bytes[difficulty] + 1;
 char decoded[40];
 unsigned length = 0, tokens = 0, used = 0;
 for(unsigned i=0;i<answer_bytes[difficulty]*2u;i++){
  uint8_t code = (i & 1u) ? (stream[i/2] >> 4) : (stream[i/2] & 15u);
  if(code == 15u){
   if(i != answer_bytes[difficulty]*2u-1 || !(i & 1u)) return false;
   break;
  }
  tokens++;
  if(code < 6u){
   if(code >= value.card_count || (used & (1u << code))) return false;
   used |= 1u << code;
   unsigned card = (unsigned)value.cards[code];
   if(card >= 100u){if(length >= sizeof(decoded)-1) return false;decoded[length++] = (char)('0'+card/100u);}
   if(card >= 10u){if(length >= sizeof(decoded)-1) return false;decoded[length++] = (char)('0'+card/10u%10u);}
   if(length >= sizeof(decoded)-1) return false;
   decoded[length++] = (char)('0'+card%10u);
  }else{
   if(code > 11u || length >= sizeof(decoded)-1) return false;
   decoded[length++] = operators[code-6u];
  }
 }
 if(tokens < 4u*value.card_count-3u ||
    used != (1u << value.card_count)-1u || length+1u > capacity) return false;
 decoded[length] = 0;
 memcpy(out,decoded,length+1u);
 return true;
}
