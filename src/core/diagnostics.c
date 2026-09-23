#include "diagnostics.h"
#include "storage.h"
#include <string.h>
NgDiagnostics ng_diagnostics;
static uint32_t current_ticks;
#ifdef NG_DIAGNOSTIC
static uint32_t (*diag_clock)(void);
/* This translation unit is explicitly excluded from function instrumentation. */
static void stack_sample(uintptr_t address,uintptr_t function) __attribute__((no_instrument_function));
static void stack_sample(uintptr_t address,uintptr_t function)
{
 NgDiagnostics *d=&ng_diagnostics;
 if(!d->stack_base)return;
#if defined(__sh__)
 /* P1/P2 aliases refer to the same RAM. Never dereference the sampled range. */
 address&=UINT32_C(0x1fffffff);
#endif
 if(d->stack_range_verified && (address<d->stack_floor || address>d->stack_ceiling)){d->stack_rejected++;return;}
 d->stack_samples++;
 if(address<d->stack_low){d->stack_low=address;d->stack_function=function;d->peak_operation=d->operation;d->peak_game=d->game;}
}
void __cyg_profile_func_enter(void *function,void *caller) __attribute__((no_instrument_function));
void __cyg_profile_func_exit(void *function,void *caller) __attribute__((no_instrument_function));
void __cyg_profile_func_enter(void *function,void *caller)
{
 (void)caller;uintptr_t stack;
#if defined(__sh__)
 __asm__ volatile("mov r15, %0":"=r"(stack));
#else
 stack=(uintptr_t)__builtin_frame_address(0);
#endif
 stack_sample(stack,(uintptr_t)function);NgDiagnostics *d=&ng_diagnostics;
 d->instrument_calls++;if(d->call_depth<UINT16_MAX)d->call_depth++;
 if(d->call_depth>d->peak_depth)d->peak_depth=d->call_depth;
}
void __cyg_profile_func_exit(void *function,void *caller)
{(void)function;(void)caller;if(ng_diagnostics.call_depth)ng_diagnostics.call_depth--;}
#endif
void ng_diag_reset(uintptr_t stack)
{
 memset(&ng_diagnostics,0,sizeof ng_diagnostics);
#if defined(__sh__) && defined(NG_DIAGNOSTIC)
 stack&=UINT32_C(0x1fffffff);
#endif
 ng_diagnostics.stack_base=ng_diagnostics.stack_low=stack;
 ng_diagnostics.heap_live=ng_diagnostics.heap_free=-1;current_ticks=0;
}
void ng_diag_clear_counters(void)
{
 NgDiagnostics *d=&ng_diagnostics;
 /* Keep live resources and the original stack origin; no invented zero live count. */
 d->sequence=d->menu_requests=d->menu_entries=d->menu_returns=d->save_errors=0;
 d->count=d->next=0;d->peak_handles=d->handles;d->peak_timers=d->timers;d->stack_low=d->stack_base;
#ifdef NG_DIAGNOSTIC
 d->stack_samples=d->stack_rejected=d->codec_peak=d->instrument_calls=0;
 d->stack_function=0;d->peak_operation=NGOP_IDLE;d->peak_game=0;
 d->peak_depth=d->call_depth;d->stress_done=d->stress_failures=0;d->stress_running=false;
 d->read_calls=d->write_calls=d->read_bytes=d->write_bytes=0;
 memset(d->operation_max,0,sizeof d->operation_max);memset(d->load,0,sizeof d->load);memset(d->ready,0,sizeof d->ready);
 /* gint's arena lifetime peaks cannot be reset through its public API. */
#endif
}
void ng_diag_context(unsigned screen,unsigned game)
{ng_diagnostics.screen=(uint8_t)screen;ng_diagnostics.game=(uint8_t)game;}
void ng_diag_emit(unsigned code,uint32_t value,unsigned detail)
{
 NgDiagnostics *d=&ng_diagnostics;
 char marker;
#ifdef NG_DIAGNOSTIC
 stack_sample((uintptr_t)&marker,0);
#else
 if(d->stack_base && (uintptr_t)&marker<d->stack_low)d->stack_low=(uintptr_t)&marker;
#endif
 switch(code){
 case NGD_MENU_REQUEST:d->menu_requests++;break;case NGD_MENU_ENTER:d->menu_entries++;break;
 case NGD_MENU_RETURN:d->menu_returns++;break;case NGD_SAVE_ERROR:d->save_errors++;break;
 case NGD_OPEN:d->handles++;if(d->handles>d->peak_handles)d->peak_handles=d->handles;break;
 case NGD_CLOSE:if(d->handles)d->handles--;break;
 case NGD_TIMER_START:d->timers++;if(d->timers>d->peak_timers)d->peak_timers=d->timers;d->callback_generation++;break;
 case NGD_TIMER_STOP:if(d->timers)d->timers--;break;
 }
 if(d->frozen)return;
 d->events[d->next]=(NgDiagEvent){++d->sequence,current_ticks,value,(uint8_t)code,d->screen,d->game,(uint8_t)detail};
 d->next=(uint16_t)((d->next+1)%NG_DIAG_EVENTS);if(d->count<NG_DIAG_EVENTS)d->count++;
}
void ng_diag_sample(uint32_t ticks,uintptr_t stack,int32_t live,int32_t free_bytes)
{
 current_ticks=ticks;
#ifdef NG_DIAGNOSTIC
 stack_sample(stack,0);
#else
 if(stack<ng_diagnostics.stack_low)ng_diagnostics.stack_low=stack;
#endif
 ng_diagnostics.heap_live=live;ng_diagnostics.heap_free=free_bytes;
}
void ng_diag_stack_range(uintptr_t floor,uintptr_t ceiling)
{
#ifdef NG_DIAGNOSTIC
#if defined(__sh__)
 floor&=UINT32_C(0x1fffffff);ceiling&=UINT32_C(0x1fffffff);
#endif
 NgDiagnostics *d=&ng_diagnostics;
 d->stack_range_verified=floor<ceiling && d->stack_base>=floor && d->stack_base<=ceiling;
 if(d->stack_range_verified){d->stack_floor=floor;d->stack_ceiling=ceiling;d->stack_base=ceiling;}
#else
 (void)floor;(void)ceiling;
#endif
}
void ng_diag_arena(unsigned index,uint32_t capacity,uint32_t used,uint32_t free_bytes,uint32_t peak_used,int32_t blocks,int32_t peak_blocks)
{
#ifdef NG_DIAGNOSTIC
 if(index>=2)return;
 ng_diagnostics.arena[index]=(NgDiagArena){capacity,used,free_bytes,peak_used,
  used<=capacity && free_bytes<=capacity-used?capacity-used-free_bytes:0,blocks,peak_blocks,true};
#else
 (void)index;(void)capacity;(void)used;(void)free_bytes;(void)peak_used;(void)blocks;(void)peak_blocks;
#endif
}
void ng_diag_clock(uint32_t (*clock_ticks)(void))
{
#ifdef NG_DIAGNOSTIC
 diag_clock=clock_ticks;
#else
 (void)clock_ticks;
#endif
}
NgDiagScope ng_diag_begin(unsigned operation,unsigned game)
{
 NgDiagScope scope={0};
#ifdef NG_DIAGNOSTIC
 NgDiagnostics *d=&ng_diagnostics;scope=(NgDiagScope){diag_clock?diag_clock():0,d->operation,d->game};
 d->operation=(uint8_t)operation;d->game=(uint8_t)game;
#else
 (void)operation;(void)game;
#endif
 return scope;
}
#ifdef NG_DIAGNOSTIC
static void timing_sample(NgDiagTiming *t,uint32_t elapsed) __attribute__((no_instrument_function));
static void timing_sample(NgDiagTiming *t,uint32_t elapsed)
{
 if(t->count<UINT32_MAX)t->count++;
 t->total=elapsed>UINT32_MAX-t->total?UINT32_MAX:t->total+elapsed;if(elapsed>t->max)t->max=elapsed;
 t->samples[t->next]=(uint16_t)(elapsed>UINT16_MAX?UINT16_MAX:elapsed);t->next=(uint8_t)((t->next+1)%16);if(t->used<16)t->used++;
}
#endif
void ng_diag_ready(unsigned game,uint32_t ticks)
{
#ifdef NG_DIAGNOSTIC
 if(game<=NG_ID_MAX)timing_sample(&ng_diagnostics.ready[game],ticks);
#else
 (void)game;(void)ticks;
#endif
}
void ng_diag_end(NgDiagScope scope)
{
#ifdef NG_DIAGNOSTIC
 NgDiagnostics *d=&ng_diagnostics;uint32_t now=diag_clock?diag_clock():0;
 uint32_t elapsed=now>=scope.tick?now-scope.tick:UINT32_C(11059200)-scope.tick+now;
 if(d->operation<NGOP_COUNT && elapsed>d->operation_max[d->operation])d->operation_max[d->operation]=elapsed;
 if(d->operation==NGOP_NEW && d->game<=NG_ID_MAX)timing_sample(&d->load[d->game],elapsed);
 d->operation=scope.operation;d->game=scope.game;
#else
 (void)scope;
#endif
}
void ng_diag_codec(size_t bytes)
{
#ifdef NG_DIAGNOSTIC
 if(bytes>ng_diagnostics.codec_peak)ng_diagnostics.codec_peak=(uint32_t)bytes;
#else
 (void)bytes;
#endif
}
void ng_diag_io(bool writing,ptrdiff_t bytes)
{
#ifdef NG_DIAGNOSTIC
 uint32_t *calls=writing?&ng_diagnostics.write_calls:&ng_diagnostics.read_calls;
 uint32_t *volume=writing?&ng_diagnostics.write_bytes:&ng_diagnostics.read_bytes;
 if(*calls<UINT32_MAX)(*calls)++;
 if(bytes>0)*volume=(uint64_t)*volume+(uint64_t)bytes>UINT32_MAX?UINT32_MAX:*volume+(uint32_t)bytes;
#else
 (void)writing;(void)bytes;
#endif
}
const char *ng_diag_operation_name(unsigned operation)
{static const char *const names[NGOP_COUNT]={"idle","new","validate","input","hint","cpu","encode","load","save"};return operation<NGOP_COUNT?names[operation]:"unknown";}
void ng_diag_stress_start(void)
{
#ifdef NG_DIAGNOSTIC
 ng_diagnostics.stress_done=ng_diagnostics.stress_failures=0;ng_diagnostics.stress_running=true;
#endif
}
bool ng_diag_stress_active(void)
{
#ifdef NG_DIAGNOSTIC
 return ng_diagnostics.stress_running;
#else
 return false;
#endif
}
void ng_diag_stress_cancel(void)
{
#ifdef NG_DIAGNOSTIC
 ng_diagnostics.stress_running=false;
#endif
}
bool ng_diag_stress_step(void)
{
#ifdef NG_DIAGNOSTIC
 NgDiagnostics *d=&ng_diagnostics;if(!d->stress_running)return false;
 unsigned ordinal=d->stress_done,id=ng_visible_id(ordinal%NG_GAME_COUNT),round=ordinal/NG_GAME_COUNT;
 unsigned level=round%ng_difficulty_count(id),mode=(round/ng_difficulty_count(id))%ng_module(id)->modes;
 if(!ng_storage_fixture(id,level,mode,UINT32_C(20260922)+ordinal))d->stress_failures++;
 d->stress_done++;if(d->stress_done>=1000)d->stress_running=false;
 return !d->stress_running || d->stress_done%10==0;
#else
 return false;
#endif
}
