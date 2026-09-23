#include "ui.h"
void ng_2048_colors(unsigned exponent,int *background,int *foreground)
{
 static const unsigned short colors[]={
  NG_RGB(29,29,29), /* Empty */
  NG_RGB(24,25,26), /* 2: grey */
  NG_RGB(22,30,12), /* 4: lime */
  NG_RGB(7,27,30),  /* 8: cyan */
  NG_RGB(3,12,27),  /* 16: blue */
  NG_RGB(31,29,8),  /* 32: yellow */
  NG_RGB(31,17,2),  /* 64: orange */
  NG_RGB(27,4,23),  /* 128: magenta */
  NG_RGB(29,4,4),   /* 256: red */
  NG_RGB(26,3,3),NG_RGB(23,2,2),NG_RGB(20,1,2),NG_RGB(17,1,2),
  NG_RGB(15,1,1),NG_RGB(13,1,1),NG_RGB(11,0,1)
 };
 unsigned index=exponent<16?exponent:15;
 *background=colors[index];
 *foreground=exponent==4 || exponent>=7?NG_WHITE:NG_INK;
}
