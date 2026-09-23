#ifndef NUMGAME_GUESSCALC_MATH_H
#define NUMGAME_GUESSCALC_MATH_H
#include <stdbool.h>
#include <stdint.h>
#define GC_MAX_LITERALS 16
/* Exact bounded parser: 96 characters, 12 nesting levels, 31 operations,
 * 16 literals, literal <= 1000000, reduced numerator/denominator <= 1e9.
 * Text is NUL-terminated or backed by 97 accessible bytes. Unterminated
 * 97-byte input fields are safely rejected by all three text parsers. */
typedef struct { int64_t num, den; } GcRational;
typedef struct { GcRational value; int literals[GC_MAX_LITERALS]; unsigned count, operations; } GcExpression;
bool gc_expression(const char *text, bool positive_integer_steps, GcExpression *out);
bool gc_cards(const GcExpression *expr,const int16_t *cards,unsigned count,bool require_all);
bool gc_equation(const char *text);
void gc_feedback(const char *secret,const char *guess,unsigned length,uint8_t *feedback);
void gc_baseball(const char *secret,const char *guess,unsigned length,unsigned *strikes,unsigned *balls);
bool gc_factorization(const char *text,int target);
bool gc_is_prime(unsigned n);
#endif
