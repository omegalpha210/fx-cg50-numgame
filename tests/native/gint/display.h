#ifndef NG_NATIVE_TEST_DISPLAY_H
#define NG_NATIVE_TEST_DISPLAY_H
#include <stdint.h>
extern uint16_t *gint_vram;
void dsetvram(uint16_t *first,uint16_t *second);
void drect(int x1,int y1,int x2,int y2,int color);
void dupdate(void);
#endif
