#ifndef NUMGAME_NG_H
#define NUMGAME_NG_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define NG_GAME_COUNT 30
#define NG_ID_MAX 32
#define NG_LEVEL_COUNT 5
#define NG_CELLS 81
#define NG_DATA 128
#define NG_HISTORY 16
#define NG_INPUT 97
#define NG_UNDO 4
/* IDs29/30 are legacy only; visible IDs1..28,31,32. Never reuse save IDs. */
enum { NG_PLAYING, NG_WON, NG_LOST, NG_DRAW };
/* Persisted values: MASTER was always 3. HELL is a new, separate value. */
enum { NG_EASY, NG_NORMAL, NG_HARD, NG_MASTER, NG_HELL };
enum { NG_SUPPLY_RUNTIME, NG_SUPPLY_BANK, NG_SUPPLY_TRANSFORMS,
 NG_SUPPLY_HYBRID, NG_SUPPLY_RULES };
enum { NGK_UP=256, NGK_RIGHT, NGK_DOWN, NGK_LEFT, NGK_EXE,
 NGK_DEL, NGK_HINT, NGK_AUX, NGK_CPU, NGK_ANSWER,
 NGK_F1, NGK_F2, NGK_F3, NGK_F4, NGK_F5, NGK_F6,
 NGK_EXIT, NGK_MENU, NGK_SHIFT, NGK_ALPHA, NGK_ACON };
enum { NGF_UNDO=1, NGF_HINT=2, NGF_CPU=4, NGF_TIMED=8 };
typedef struct {
 uint8_t id, difficulty, mode, status;
 uint8_t phase, assisted, recorded, turn;
 uint8_t rows, cols, cursor, history_count;
 uint8_t scroll, notes_mode, cpu_pending, reserved;
 uint32_t seed, rng, run_id, elapsed_ms, moves, score, puzzle_id;
 uint32_t supply_seed,supply_index,pack_revision,generation_policy;
 int16_t board[NG_CELLS]; /* blank convention belongs to module */
 uint8_t fixed[NG_CELLS];
 uint16_t notes[NG_CELLS];
 int32_t data[NG_DATA]; /* module-owned, explicit little endian on disk */
 char input[NG_INPUT];
 char message[128];
 char history[NG_HISTORY][40];
} NgGame;
struct NgCanvas;
typedef struct {
 uint8_t id;
 const char *name, *short_name;
 const char *rules; /* newline-separated, scrollable; lines <= 48 chars */
 uint8_t flags;
 const char *aux_label, *primary_label;
 uint8_t modes;
 const char *(*mode_name)(unsigned mode);
 void (*init)(NgGame *g); /* common fields set, rest zeroed by core */
 bool (*action)(NgGame *g,int key); /* enforce legality; no IO */
 bool (*tick)(NgGame *g,uint32_t delta_ms); /* optional; active time only */
 bool (*valid)(const NgGame *g); /* all persisted module field bounds */
 void (*render)(const NgGame *g,struct NgCanvas *c); /* content y=27..184 */
} NgModule;
extern const NgModule ng_guesscalc[10],ng_grids[10],ng_strategyquick[10],ng_boards[2];
const NgModule *ng_module(unsigned id);
unsigned ng_difficulty_count(unsigned id);
unsigned ng_regular_difficulty_count(unsigned id);
bool ng_has_hell(unsigned id);
unsigned ng_generation_policy(unsigned id);
unsigned ng_level_generation_policy(unsigned id,unsigned difficulty,unsigned mode);
unsigned ng_bank_count(unsigned id,unsigned difficulty,unsigned mode);
const char *ng_level_name(unsigned difficulty);
/* Ordinal within a bank; stable puzzle ID mapping belongs to the module. */
unsigned ng_bank_pick(NgGame *g,unsigned count);
unsigned ng_visible_id(unsigned index);
int ng_catalog_index(unsigned id);
uint32_t ng_random(NgGame *g); /* deterministic xorshift32, state saved */
unsigned ng_rand(NgGame *g,unsigned limit);
void ng_new(NgGame *g,unsigned id,unsigned difficulty,unsigned mode,uint32_t seed,uint32_t run_id);
void ng_new_supply(NgGame *g,unsigned id,unsigned difficulty,unsigned mode,
 uint32_t seed,uint32_t run_id,uint32_t supply_seed,uint32_t supply_index);
bool ng_valid(const NgGame *g);
bool ng_edit(NgGame *g,int key,const char *allowed,unsigned limit);
void ng_message(NgGame *g,const char *message);
void ng_history(NgGame *g,const char *line);
/* Shared wrapping grid cursor. Returns true only for arrow keys. */
bool ng_grid_nav(NgGame *g,int key);
#endif
