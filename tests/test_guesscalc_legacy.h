#ifndef TEST_GUESSCALC_LEGACY_H
#define TEST_GUESSCALC_LEGACY_H
/* Portable hashes of all original game fields, generated from guesscalc.c at
 * 4e7da73bef82e7e20dce2a8d80ff85d9b8f66639 (new supply fields excluded).
 * Order: id1..10, mode0..modes-1, difficulty0..2, seed1/17/UINT32_MAX.
 * A regression fingerprint, not a security/authentication primitive. */
#include "ng.h"
static inline uint32_t gc_legacy_byte(uint32_t hash,unsigned byte){return (hash^(byte&255u))*UINT32_C(16777619);}
static inline uint32_t gc_legacy_word(uint32_t hash,uint32_t word,unsigned bytes){for(unsigned i=0;i<bytes;i++)hash=gc_legacy_byte(hash,word>>(i*8));return hash;}
static inline uint32_t gc_legacy_fingerprint(const NgGame *g){
 uint32_t h=UINT32_C(2166136261);
 const uint8_t prefix[]={g->id,g->difficulty,g->mode,g->status,g->phase,g->assisted,g->recorded,g->turn,g->rows,g->cols,g->cursor,g->history_count,g->scroll,g->notes_mode,g->cpu_pending,g->reserved};
 const uint32_t words[]={g->seed,g->rng,g->run_id,g->elapsed_ms,g->moves,g->score,g->puzzle_id};
 for(unsigned i=0;i<sizeof(prefix);i++)h=gc_legacy_byte(h,prefix[i]);
 for(unsigned i=0;i<sizeof(words)/sizeof(*words);i++)h=gc_legacy_word(h,words[i],4);
 for(unsigned i=0;i<NG_CELLS;i++)h=gc_legacy_word(h,(uint16_t)g->board[i],2);
 for(unsigned i=0;i<NG_CELLS;i++)h=gc_legacy_byte(h,g->fixed[i]);
 for(unsigned i=0;i<NG_CELLS;i++)h=gc_legacy_word(h,g->notes[i],2);
 for(unsigned i=0;i<NG_DATA;i++)h=gc_legacy_word(h,(uint32_t)g->data[i],4);
 for(unsigned i=0;i<sizeof(g->input);i++)h=gc_legacy_byte(h,(unsigned char)g->input[i]);
 for(unsigned i=0;i<sizeof(g->message);i++)h=gc_legacy_byte(h,(unsigned char)g->message[i]);
 for(unsigned i=0;i<NG_HISTORY;i++)for(unsigned j=0;j<sizeof(g->history[i]);j++)h=gc_legacy_byte(h,(unsigned char)g->history[i][j]);
 return h;
}
static const uint32_t gc_legacy_states[162]={
2889126609u,1387819557u,722285066u,519123150u,1775222166u,1644202249u,
1148679489u,2896938997u,3540528254u,2670143029u,662382335u,2990909067u,
837702602u,4194556956u,2399780460u,1307626481u,2656081231u,3569314835u,
3405461567u,3851374649u,1843036346u,1997356972u,3802320070u,1266098337u,
840264467u,193697613u,629230070u,3023005343u,2306472290u,2734643270u,
1201910848u,2006591097u,2453664525u,4230721343u,1920753206u,3356017714u,
3000192837u,2878022986u,804576693u,2405392330u,1971263217u,3502104394u,
4098154849u,1141549550u,1551391357u,1496210845u,1910556238u,2951499114u,
4174323390u,1771313081u,333400193u,1894107913u,1122976294u,4225441086u,
1605114331u,3373446818u,785818524u,3440171329u,1395011610u,1439801687u,
3806443321u,2862883975u,2255335146u,1162411465u,164047099u,1395938569u,
3068158955u,4156203936u,2089965204u,3627688035u,1909967091u,677312132u,
1870769002u,893351617u,576877233u,4076133396u,2964527542u,2220145521u,
1907078522u,2250055561u,187967009u,1510548370u,4019855321u,2112283052u,
678379053u,956097057u,3533839121u,3157913518u,3507297458u,2668686464u,
2167583845u,4254173244u,2786222809u,2679910871u,572524563u,111637342u,
3805484205u,3931545920u,4184694891u,2521265952u,3210228637u,2981277873u,
3331123457u,1850737536u,214068734u,3965731377u,3516969304u,4142752812u,
933076014u,3859201112u,2076524617u,1243367418u,828206221u,2606338587u,
1440253373u,886325u,2807207883u,853173323u,258178602u,2964178835u,
573250845u,2055341569u,1066196778u,2338762857u,2921997780u,1869270881u,
2007855992u,3055316525u,823915388u,2663791051u,3107368727u,1913057471u,
374049978u,2391924922u,3723888271u,958669512u,2020979970u,968880895u,
2039686265u,1884680333u,1434717040u,519130022u,1221259389u,1553554355u,
3777182507u,3007869319u,2930162488u,2607180333u,1394721794u,82238673u,
3470310824u,3981043418u,243457404u,544250378u,2993245011u,1637526640u,
4136529085u,2950895919u,451368105u,809899779u,2597623482u,1720971126u,
};
#endif
