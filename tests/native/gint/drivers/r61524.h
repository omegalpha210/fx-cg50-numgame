#ifndef NG_TEST_LCD_H
#define NG_TEST_LCD_H
#include <stdint.h>
uint16_t r61524_get(int reg);
void r61524_set(int reg,uint16_t value);
void r61524_display_rect(uint16_t *vram,int xmin,int xmax,int ymin,int ymax);
#endif
