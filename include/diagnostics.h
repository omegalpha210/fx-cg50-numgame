#ifndef NUMGAME_DIAGNOSTICS_H
#define NUMGAME_DIAGNOSTICS_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "ng.h"
#define NG_DIAG_EVENTS 96
#define NG_DIAG_EXPORT_MAX 24576u
#if defined(__GNUC__)
#define NG_NO_INSTRUMENT __attribute__((no_instrument_function))
#else
#define NG_NO_INSTRUMENT
#endif
enum { NGOP_IDLE,NGOP_NEW,NGOP_VALIDATE,NGOP_INPUT,NGOP_HINT,NGOP_CPU,NGOP_ENCODE,NGOP_LOAD,NGOP_SAVE,NGOP_COUNT };
typedef struct {uint32_t tick;uint8_t operation,game;} NgDiagScope;
typedef struct {
 uint32_t capacity,used,free_bytes,peak_used,overhead;
 int32_t blocks,peak_blocks;
 bool available;
} NgDiagArena;
typedef struct {uint32_t count,total,max;uint16_t samples[16];uint8_t next,used;} NgDiagTiming;
enum {NGD_INPUT=1,NGD_SAVE_BEGIN,NGD_SAVE_END,NGD_SAVE_ERROR,NGD_OPEN,NGD_CLOSE,
 NGD_MENU_REQUEST,NGD_MENU_ENTER,NGD_MENU_RETURN,NGD_OFF_ENTER,NGD_OFF_RETURN,
 NGD_TIMER_START,NGD_TIMER_STOP,NGD_DIM,NGD_RESTORE,NGD_SAMPLE,NGD_IO_ERROR};
typedef struct {uint32_t sequence,ticks,value;uint8_t code,screen,game,detail;} NgDiagEvent;
typedef struct {
 NgDiagEvent events[NG_DIAG_EVENTS];
 uint32_t sequence,menu_requests,menu_entries,menu_returns,save_errors,callback_generation;
 uintptr_t stack_base,stack_low;
 int32_t heap_live,heap_free;
 uint16_t count,next;
 uint8_t screen,game,handles,peak_handles,timers,peak_timers;
 bool frozen;
#ifdef NG_DIAGNOSTIC
 uintptr_t stack_floor,stack_ceiling,stack_function;
 uint32_t stack_samples,stack_rejected,codec_peak,operation_max[NGOP_COUNT];
 uint32_t stress_done,stress_failures,instrument_calls;
 uint32_t read_calls,write_calls,read_bytes,write_bytes;
 uint16_t call_depth,peak_depth;
 uint8_t operation,peak_operation,peak_game;
 bool stress_running,stack_range_verified;
 NgDiagArena arena[2];
 NgDiagTiming load[NG_ID_MAX+1],ready[NG_ID_MAX+1];
#endif
} NgDiagnostics;
extern NgDiagnostics ng_diagnostics;
NG_NO_INSTRUMENT void ng_diag_reset(uintptr_t stack_base);
NG_NO_INSTRUMENT void ng_diag_context(unsigned screen,unsigned game);
NG_NO_INSTRUMENT void ng_diag_emit(unsigned code,uint32_t value,unsigned detail);
NG_NO_INSTRUMENT void ng_diag_sample(uint32_t ticks,uintptr_t stack,int32_t heap_live,int32_t heap_free);
NG_NO_INSTRUMENT bool ng_diag_export(void);
NG_NO_INSTRUMENT void ng_diag_clear_counters(void);
NG_NO_INSTRUMENT void ng_diag_stack_range(uintptr_t floor,uintptr_t ceiling);
NG_NO_INSTRUMENT void ng_diag_arena(unsigned index,uint32_t capacity,uint32_t used,uint32_t free_bytes,
 uint32_t peak_used,int32_t blocks,int32_t peak_blocks);
NG_NO_INSTRUMENT void ng_diag_clock(uint32_t (*clock_ticks)(void));
NG_NO_INSTRUMENT NgDiagScope ng_diag_begin(unsigned operation,unsigned game);
NG_NO_INSTRUMENT void ng_diag_end(NgDiagScope scope);
NG_NO_INSTRUMENT void ng_diag_ready(unsigned game,uint32_t ticks);
NG_NO_INSTRUMENT void ng_diag_codec(size_t bytes);
NG_NO_INSTRUMENT void ng_diag_io(bool writing,ptrdiff_t bytes);
NG_NO_INSTRUMENT void ng_diag_stress_start(void);
NG_NO_INSTRUMENT bool ng_diag_stress_step(void);
NG_NO_INSTRUMENT bool ng_diag_stress_active(void);
NG_NO_INSTRUMENT void ng_diag_stress_cancel(void);
NG_NO_INSTRUMENT const char *ng_diag_operation_name(unsigned operation);
#endif
