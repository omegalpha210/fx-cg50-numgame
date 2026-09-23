#ifndef NG_NATIVE_TEST_TIMER_H
#define NG_NATIVE_TEST_TIMER_H
#include <stdint.h>
#include <gint/gint.h>
enum {TIMER_ANY=-1,TIMER_CONTINUE=0};
int timer_configure(int timer,uint64_t delay,gint_call_t callback);
void timer_start(int timer);
void timer_pause(int timer);
#endif
