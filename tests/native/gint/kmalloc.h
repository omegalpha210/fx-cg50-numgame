#ifndef NG_TEST_KMALLOC_H
#define NG_TEST_KMALLOC_H
#include <stdint.h>
typedef struct {void *start,*end;struct {int live_blocks,peak_live_blocks;} stats;} kmalloc_arena_t;
typedef struct {uint32_t free_memory,used_memory,peak_used_memory;} kmalloc_gint_stats_t;
kmalloc_arena_t *kmalloc_get_arena(const char *name);
kmalloc_gint_stats_t *kmalloc_get_gint_stats(kmalloc_arena_t *arena);
#endif
