#ifndef NG_NATIVE_TEST_GINT_H
#define NG_NATIVE_TEST_GINT_H
#include <stdbool.h>
typedef struct {int (*noarg)(void);int (*argfn)(void *);void *arg;} gint_call_t;
#define NG_MOCK_CALL_pulse(...) ((gint_call_t){.noarg=pulse})
#define NG_MOCK_CALL_dispatch(value) ((gint_call_t){.argfn=dispatch,.arg=(value)})
#define GINT_CALL(function,...) NG_MOCK_CALL_##function(__VA_ARGS__)
#define NG_MOCK_CALL_read_power_settings(value) ((gint_call_t){.argfn=read_power_settings,.arg=(value)})
int gint_world_switch(gint_call_t call);
void gint_osmenu(void);
void gint_poweroff(bool show_message);
#endif
