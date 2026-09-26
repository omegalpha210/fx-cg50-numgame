#ifndef NUMGAME_UI_H
#define NUMGAME_UI_H
#include "ng.h"
/* Pure drawing interface, 396x224 RGB565, production and host share renderer. */
#define NG_RGB(r,g,b) (((r)<<11)|((g)<<6)|(b))
enum { NG_INK=NG_RGB(3,5,7), NG_PAPER=NG_RGB(30,30,29),
 NG_WHITE=65535, NG_BLACK=0, NG_BLUE=NG_RGB(1,10,25),
 NG_MUTED=NG_RGB(12,14,15), NG_LINE=NG_RGB(24,26,26),
 NG_GREEN=NG_RGB(3,17,9), NG_RED=NG_RGB(25,3,4),
 NG_PALE=NG_RGB(26,29,30), NG_YELLOW=65504, NG_MAGENTA=63519 };
typedef struct NgCanvas {
 void *context;
 void (*rect)(void *,int,int,int,int,uint16_t);
} NgCanvas;
void ng_rect(NgCanvas *c,int x,int y,int w,int h,int color);
void ng_border(NgCanvas *c,int x,int y,int w,int h,int color,int thick);
void ng_line(NgCanvas *c,int x1,int y1,int x2,int y2,int color);
/* Native gint 8x9 face (11 raster rows), normal weight, matching DIFF EQ. */
int ng_text_width(const char *text,int scale);
void ng_text(NgCanvas *c,int x,int y,const char *text,int color,int scale);
void ng_center(NgCanvas *c,int x,int y,int w,const char *text,int color,int scale);
/* 5x7 normal single-stroke font for tiny notes and cage clues. */
void ng_small(NgCanvas *c,int x,int y,const char *text,int color);
int ng_operator_color(int symbol,int fallback);
void ng_expression(NgCanvas *c,int x,int y,const char *text,int color,int scale);
void ng_wrap_expression(NgCanvas *c,int x,int y,int width,int spacing,unsigned lines,const char *text,int color,bool small);
void ng_small_expression(NgCanvas *c,int x,int y,const char *text,int color);
void ng_input_expression(NgCanvas *c,int x,int y,int w,const char *value);
int ng_small_width(const char *text);
void ng_text_fit(NgCanvas *c,int x,int y,int width,const char *text,int color,int scale);
void ng_small_fit(NgCanvas *c,int x,int y,int width,const char *text,int color);
bool ng_text_line(char *out,unsigned capacity,const char **text,int width,bool small);
void ng_wrap(NgCanvas *c,int x,int y,int width,int spacing,unsigned lines,const char *text,int color,bool small);
void ng_2048_colors(unsigned exponent,int *background,int *foreground);
/* Original 40x32 game pictograms. Stable game IDs1..32; 33 statistics. */
void ng_game_icon(NgCanvas *c,int x,int y,unsigned id,int accent);
void ng_number(NgCanvas *c,int x,int y,int value,int color,int scale);
void ng_input(NgCanvas *c,int x,int y,int w,const char *value);
void ng_card(NgCanvas *c,int x,int y,int w,int h,const char *label,bool selected,bool used);
/* A shared rail for lists longer than their visible rows. */
void ng_list_scrollbar(NgCanvas *c,unsigned total,unsigned visible,unsigned offset,
 int track_x,int up_y,int track_y,int track_h,int down_y);
/* Wrapped RULES layout uses the same font metrics for drawing and key bounds. */
unsigned ng_rules_line_count(unsigned id);
unsigned ng_rules_max_scroll(unsigned id);
/* Cell x/y/size returned for module-specific clues/inequalities. */
typedef struct {int x,y,size;} NgGridLayout;
NgGridLayout ng_grid_layout(unsigned rows,unsigned cols,bool outside_clues);
void ng_grid_cell(NgCanvas *c,NgGridLayout l,unsigned cols,unsigned index,
 const char *label,bool fixed,bool selected,int fill);
#endif
