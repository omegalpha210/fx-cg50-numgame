#include "storage.h"
#include "diagnostics.h"
#include <limits.h>
#include <string.h>
typedef struct {uint8_t *out;const uint8_t *in;size_t pos,size;bool ok;} Codec;
static void u8(Codec *c,uint8_t *v)
{if(c->pos>=c->size){c->ok=false;return;}if(c->out)c->out[c->pos]=*v;else *v=c->in[c->pos];c->pos++;}
static void u16(Codec *c,uint16_t *v)
{uint8_t a=(uint8_t)*v,b=(uint8_t)(*v>>8);u8(c,&a);u8(c,&b);if(!c->out)*v=(uint16_t)(a|((uint16_t)b<<8));}
static void u32(Codec *c,uint32_t *v)
{uint16_t a=(uint16_t)*v,b=(uint16_t)(*v>>16);u16(c,&a);u16(c,&b);if(!c->out)*v=(uint32_t)a|((uint32_t)b<<16);}
static void bytes(Codec *c,char *s,size_t n)
{for(size_t i=0;i<n;i++){uint8_t x=(uint8_t)s[i];u8(c,&x);if(!c->out)s[i]=(char)x;}}
static void game(Codec *c,NgGame *g,bool modern)
{
 u8(c,&g->id);u8(c,&g->difficulty);u8(c,&g->mode);u8(c,&g->status);
 u8(c,&g->phase);u8(c,&g->assisted);u8(c,&g->recorded);u8(c,&g->turn);
 u8(c,&g->rows);u8(c,&g->cols);u8(c,&g->cursor);u8(c,&g->history_count);
 u8(c,&g->scroll);u8(c,&g->notes_mode);u8(c,&g->cpu_pending);u8(c,&g->reserved);
 u32(c,&g->seed);u32(c,&g->rng);u32(c,&g->run_id);u32(c,&g->elapsed_ms);
 u32(c,&g->moves);u32(c,&g->score);u32(c,&g->puzzle_id);
 if(modern){u32(c,&g->supply_seed);u32(c,&g->supply_index);u32(c,&g->pack_revision);u32(c,&g->generation_policy);}
 else {g->pack_revision=1;g->generation_policy=ng_level_generation_policy(g->id,g->difficulty,g->mode);}
 for(unsigned i=0;i<NG_CELLS;i++){uint16_t v=(uint16_t)g->board[i];u16(c,&v);if(!c->out)g->board[i]=v<=INT16_MAX?(int16_t)v:(int16_t)(-1-(int)(UINT16_MAX-v));}
 for(unsigned i=0;i<NG_CELLS;i++)u8(c,&g->fixed[i]);
 for(unsigned i=0;i<NG_CELLS;i++)u16(c,&g->notes[i]);
 for(unsigned i=0;i<NG_DATA;i++){uint32_t v=(uint32_t)g->data[i];u32(c,&v);if(!c->out)g->data[i]=v<=INT32_MAX?(int32_t)v:-1-(int32_t)(UINT32_MAX-v);}
 bytes(c,g->input,sizeof(g->input));bytes(c,g->message,sizeof(g->message));
 for(unsigned i=0;i<NG_HISTORY;i++)bytes(c,g->history[i],sizeof(g->history[i]));
}
static bool supplies(Codec *c,NgSession *s)
{
 for(unsigned m=0;m<NG_MODES;m++)for(unsigned d=0;d<NG_LEVEL_COUNT;d++){
  NgSupply *b=&s->supply[m][d];u32(c,&b->shuffle);u16(c,&b->next);u16(c,&b->count);
  for(unsigned i=0;i<4;i++)u32(c,&b->recent[i]);
  u8(c,&b->recent_count);
  if(b->count>4096 || b->next>b->count || b->recent_count>4 || (b->count && !b->shuffle))return false;
 }
 return c->ok;
}
static void stats(Codec *c,NgStats *s,unsigned levels)
{
 u32(c,&s->started);u32(c,&s->active_ms);
 for(unsigned m=0;m<NG_MODES;m++)for(unsigned d=0;d<levels;d++)for(unsigned a=0;a<2;a++){
  NgBest *b=&s->best[m][d][a];u32(c,&b->completed);u32(c,&b->wins);u32(c,&b->losses);
  u32(c,&b->draws);u32(c,&b->best_score);u32(c,&b->best_moves);u32(c,&b->best_ms);u32(c,&b->best_aux);
 }
}
uint32_t ng_crc32(const void *data,size_t size)
{
 const uint8_t *p=data;uint32_t crc=UINT32_MAX;
 for(size_t i=0;i<size;i++){crc^=p[i];for(unsigned b=0;b<8;b++)crc=(crc>>1)^((0u-(crc&1))&0xedb88320u);}
 return ~crc;
}
static size_t encode(const NgSession *s,uint8_t *out,size_t capacity)
{
 if(s->undo_count>NG_UNDO || !ng_valid(&s->game) ||
  (s->undo_count && !(ng_module(s->game.id)->flags&NGF_UNDO)))return 0;
 Codec c={.out=out,.size=capacity,.ok=true};
 /* Only named fixed-width members are encoded. No struct padding/pointers. */
 uint8_t count=s->undo_count;u8(&c,&count);
 stats(&c,(NgStats *)&s->stats,NG_LEVEL_COUNT);
 if(!supplies(&c,(NgSession *)s))return 0;
 game(&c,(NgGame *)&s->game,true);
 for(unsigned i=0;i<count;i++)game(&c,(NgGame *)&s->undo[i],true);
 if(c.ok)ng_diag_codec(c.pos);
 return c.ok?c.pos:0;
}
size_t ng_encode(const NgSession *s,uint8_t *out,size_t capacity)
{NgDiagScope scope=ng_diag_begin(NGOP_ENCODE,s->game.id);size_t bytes=encode(s,out,capacity);ng_diag_end(scope);return bytes;}
bool ng_decode(NgSession *s,const uint8_t *data,size_t length,unsigned expected_id)
{
 memset(s,0,sizeof(*s));Codec c={.in=data,.size=length,.ok=true};
 u8(&c,&s->undo_count);if(s->undo_count>NG_UNDO)return false;
 /* Exact old wire lengths disambiguate three/four levels without relabeling. */
 unsigned levels=length==1545u+1826u*(1u+s->undo_count)?3u:
  length==2057u+1826u*(1u+s->undo_count)?4u:NG_LEVEL_COUNT;
 bool modern=levels==NG_LEVEL_COUNT;
 stats(&c,&s->stats,levels);if(modern && !supplies(&c,s))return false;
 game(&c,&s->game,modern);
 if(!c.ok || s->game.id!=expected_id || !ng_valid(&s->game))return false;
 if(s->undo_count && !(ng_module(expected_id)->flags&NGF_UNDO))return false;
 for(unsigned i=0;i<s->undo_count;i++) {
  game(&c,&s->undo[i],modern);const NgGame *u=&s->undo[i];
  if(!c.ok || !ng_valid(u) || u->id!=expected_id || u->seed!=s->game.seed ||
   u->run_id!=s->game.run_id || u->mode!=s->game.mode || u->difficulty!=s->game.difficulty ||
   u->puzzle_id!=s->game.puzzle_id || u->supply_seed!=s->game.supply_seed ||
   u->supply_index!=s->game.supply_index || u->pack_revision!=s->game.pack_revision || u->generation_policy!=s->game.generation_policy ||
   u->status!=NG_PLAYING || u->cpu_pending)return false;
 }
 uint64_t total=0;
 for(unsigned m=0;m<NG_MODES;m++)for(unsigned d=0;d<NG_LEVEL_COUNT;d++)for(unsigned a=0;a<2;a++){
  const NgBest *b=&s->stats.best[m][d][a];
  if((uint64_t)b->wins+b->losses+b->draws!=b->completed || b->completed>s->stats.started)return false;
  if(b->best_aux>(expected_id==26?30u:0u))return false;
  if(!b->wins && (b->best_moves || b->best_ms))return false;
  if(!b->completed && (b->best_score || b->best_aux))return false;
  if((m>=ng_module(expected_id)->modes || d>=ng_difficulty_count(expected_id)) && b->completed)return false;
  total+=b->completed;
 }
 if(total>s->stats.started)return false;
 return c.ok && c.pos==length;
}
static uint32_t satadd(uint32_t a,uint32_t b){return b>UINT32_MAX-a?UINT32_MAX:a+b;}
void ng_summarize(const NgSession *s,NgSummary *summary)
{
 *summary=(NgSummary){.played=s->stats.started,.active_ms=s->stats.active_ms,
  .exists=1,.difficulty=s->game.difficulty,.mode=s->game.mode};
 for(unsigned m=0;m<NG_MODES;m++)for(unsigned d=0;d<NG_LEVEL_COUNT;d++){
  summary->completed=satadd(summary->completed,s->stats.best[m][d][0].completed);
  summary->assisted=satadd(summary->assisted,s->stats.best[m][d][1].completed);
 }
}
void ng_record_result(NgSession *s)
{
 NgGame *g=&s->game;if(!g->status || g->recorded)return;
 NgBest *b=&s->stats.best[g->mode][g->difficulty][g->assisted];
 if(b->completed<UINT32_MAX)b->completed++;
 uint32_t *outcome=g->status==NG_WON?&b->wins:g->status==NG_LOST?&b->losses:&b->draws;
 if(*outcome<UINT32_MAX)(*outcome)++;
 if(g->id<=5){if(g->status==NG_WON && (!b->best_score || g->score<b->best_score))b->best_score=g->score;}
 else if(g->score>b->best_score)b->best_score=g->score;
 if(g->id==26 && g->data[0]>(int32_t)b->best_aux)b->best_aux=(uint32_t)g->data[0];
 if(g->status==NG_WON){if(b->wins==1 || g->moves<b->best_moves)b->best_moves=g->moves;
  if(b->wins==1 || g->elapsed_ms<b->best_ms)b->best_ms=g->elapsed_ms;}
 g->recorded=1;
}
