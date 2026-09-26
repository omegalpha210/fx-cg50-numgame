#ifndef STRATEGYQUICK_H
#define STRATEGYQUICK_H
#include "ng.h"
#include "ui.h"
typedef struct { int choice, amount; } SqMove;
#define SQ_SLIDING_BAND_COUNT 128u
unsigned sq_bank_count(unsigned id,unsigned difficulty,unsigned mode);
/* Pure helper API exposed for independent host audits. */
int sq_strategy_value(unsigned id,const NgGame *g);
SqMove sq_strategy_pick(NgGame *g,bool exact);
bool sq_strategy_legal(const NgGame *g,SqMove m);
bool sq_strategy_apply(NgGame *g,SqMove m);
void sq_strategy_init(NgGame *g);
bool sq_strategy_action(NgGame *g,int key);
bool sq_strategy_valid(const NgGame *g);
void sq_strategy_render(const NgGame *g,NgCanvas *c);
const char *sq_strategy_mode(unsigned mode);
void sq_quick_init(NgGame *g);
bool sq_quick_action(NgGame *g,int key);
bool sq_quick_tick(NgGame *g,uint32_t delta_ms);
bool sq_quick_valid(const NgGame *g);
void sq_quick_render(const NgGame *g,NgCanvas *c);
const char *sq_sliding_mode(unsigned mode);
const char *sq_2048_mode(unsigned mode);
const char *sq_lights_mode(unsigned mode);
const char *sq_rush_mode(unsigned mode);
/* 2048 tiles are exponents 0..30; return false for an unchanged line. */
bool sq_2048_line(int16_t line[4],uint32_t *score);
bool sq_2048_can_move(const NgGame *g);
bool sq_sliding_solvable(const NgGame *g);
bool sq_lights_solution(const NgGame *g,uint32_t *solution);
/* Compact, pure rule helpers for independent Reversi/Net audits. */
unsigned sq_reversi_moves(const int16_t board[64],unsigned player,uint8_t moves[64]);
bool sq_reversi_place(int16_t board[64],unsigned player,unsigned position);
int sq_reversi_pick(NgGame *g,unsigned level);
bool sq_net_complete(const NgGame *g);
unsigned sq_net_connected(const NgGame *g);
#endif
