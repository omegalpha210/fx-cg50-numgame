#include "ui.h"

/* Original, pixel-aligned menu pictograms. All geometry stays within 40x32.
   These are presentation only: no game state, random numbers or puzzle data. */
static void box(NgCanvas *c,int x,int y,int w,int h,int color)
{ng_rect(c,x,y,w,h,NG_WHITE);ng_border(c,x,y,w,h,color,1);}
static void digit(NgCanvas *c,int x,int y,char value,int color)
{char text[2]={value,0};ng_small(c,x,y,text,color);}
static void grid(NgCanvas *c,int x,int y,const char *values,int accent,unsigned shaded)
{
 for(unsigned i=0;i<9;i++) {
  int xx=x+(int)(i%3)*9,yy=y+(int)(i/3)*9;
  bool dark=(shaded&(1u<<i))!=0;
  ng_rect(c,xx,yy,10,10,dark?accent:NG_WHITE);
  ng_border(c,xx,yy,10,10,accent,1);
  if(values[i]!=' ')digit(c,xx+2,yy+1,values[i],dark?NG_WHITE:NG_INK);
 }
}
static void ring(NgCanvas *c,int x,int y,int r,int color)
{
 int dx=r,dy=0,error=1-r;
 while(dx>=dy) {
  const int px[8]={dx,dy,-dy,-dx,-dx,-dy,dy,dx};
  const int py[8]={dy,dx,dx,dy,-dy,-dx,-dx,-dy};
  for(unsigned i=0;i<8;i++)ng_rect(c,x+px[i],y+py[i],1,1,color);
  dy++;if(error<0)error+=2*dy+1;else{dx--;error+=2*(dy-dx)+1;}
 }
}
static void timer(NgCanvas *c,int x,int y,int accent)
{
 box(c,x+15,y,10,4,accent);ng_rect(c,x+19,y+4,2,3,accent);
 ring(c,x+20,y+19,11,accent);ng_line(c,x+20,y+19,x+20,y+11,NG_INK);
 ng_line(c,x+20,y+19,x+26,y+19,NG_INK);
}
void ng_game_icon(NgCanvas *c,int x,int y,unsigned id,int accent)
{
 switch(id) {
 case 1: /* Number code, exact and misplaced feedback. Shared with GUESS. */
  for(int i=0;i<4;i++){box(c,x+i*10,y+3,9,16,accent);digit(c,x+2+i*10,y+7,"12?4"[i],NG_INK);}
  for(int i=0;i<4;i++){ng_rect(c,x+2+i*10,y+24,5,5,i<2?accent:NG_WHITE);ng_border(c,x+2+i*10,y+24,5,5,accent,1);}
  break;
 case 2: /* A hidden equation with two feedback cells. */
  box(c,x+1,y+3,38,25,accent);ng_small(c,x+5,y+6,"2+3=5",NG_INK);
  for(int i=0;i<5;i++){ng_rect(c,x+5+i*6,y+18,4,5,i<2?accent:NG_LINE);}break;
 case 3: /* Public code clues and exact-position counts. */
  for(int i=0;i<3;i++){ng_small(c,x+2,y+2+i*10,i==0?"1234":i==1?"1256":"7836",NG_INK);box(c,x+29,y+i*10,10,10,accent);digit(c,x+32,y+1+i*10,(char)('1'+i),accent);}break;
 case 4: /* Combination lock. */
  ng_border(c,x+11,y+1,18,16,accent,2);box(c,x+5,y+12,30,19,accent);
  ng_small(c,x+12,y+18,"42",NG_INK);break;
 case 5: /* A sequence stepping toward its next term. */
  ng_small(c,x+1,y+2,"2 4 6",NG_INK);ng_line(c,x+4,y+20,x+30,y+20,accent);
  ng_line(c,x+30,y+20,x+26,y+16,accent);ng_line(c,x+30,y+20,x+26,y+24,accent);
  digit(c,x+33,y+17,'?',accent);for(int i=0;i<3;i++)ng_rect(c,x+4+i*9,y+18,3,5,accent);break;
 case 6: /* Number cards for a target expression. */
  box(c,x+2,y+8,14,23,accent);digit(c,x+7,y+16,'3',NG_INK);
  box(c,x+12,y+1,14,23,accent);digit(c,x+17,y+9,'8',NG_INK);
  ng_small(c,x+28,y+4,"24",accent);ng_line(c,x+29,y+22,x+37,y+22,accent);ng_line(c,x+33,y+18,x+33,y+26,accent);break;
 case 7:timer(c,x,y,accent);break;
 case 8: /* Missing operator. */
  digit(c,x+1,y+7,'3',NG_INK);box(c,x+10,y+3,13,16,accent);digit(c,x+14,y+7,'?',accent);digit(c,x+29,y+7,'2',NG_INK);
  ng_small(c,x+15,y+23,"=5",accent);break;
 case 9: /* Crossing arithmetic rows and columns. */
  ng_small(c,x+3,y+2,"2+3=5",NG_INK);digit(c,x+3,y+12,'+',accent);digit(c,x+16,y+12,'*',accent);
  ng_small(c,x+3,y+23,"4-1=3",NG_INK);break;
 case 10: /* Factor tree. */
  ng_small(c,x+14,y,"12",NG_INK);ng_line(c,x+19,y+9,x+8,y+20,accent);ng_line(c,x+20,y+9,x+31,y+20,accent);
  box(c,x+2,y+20,13,11,accent);digit(c,x+6,y+22,'3',NG_INK);
  box(c,x+25,y+20,13,11,accent);digit(c,x+29,y+22,'4',NG_INK);break;
 case 11:grid(c,x+6,y+2,"1  2 3  9",accent,0);ng_border(c,x+6,y+2,28,28,accent,2);break;
 case 12: /* Heavy cage boundaries and a sum clue. */
  grid(c,x+6,y+2,"      2  ",NG_LINE,0);ng_border(c,x+6,y+2,19,19,accent,2);
  ng_border(c,x+24,y+2,10,28,accent,2);ng_border(c,x+6,y+20,19,10,accent,2);ng_small(c,x+9,y+5,"6+",NG_INK);break;
 case 13: /* Kakuro diagonal clue blocks. */
  grid(c,x+6,y+2,"    3  4 ",accent,15);
  ng_line(c,x+15,y+2,x+24,y+11,NG_WHITE);ng_line(c,x+24,y+2,x+33,y+11,NG_WHITE);
  ng_line(c,x+6,y+11,x+15,y+20,NG_WHITE);break;
 case 14: /* Two values and an inequality. */
  box(c,x+1,y+2,13,13,accent);digit(c,x+5,y+5,'1',NG_INK);
  box(c,x+26,y+2,13,13,accent);digit(c,x+30,y+5,'3',NG_INK);
  ng_line(c,x+22,y+5,x+17,y+8,accent);ng_line(c,x+17,y+8,x+22,y+11,accent);
  box(c,x+1,y+18,13,13,accent);digit(c,x+5,y+21,'2',NG_INK);
  box(c,x+26,y+18,13,13,accent);digit(c,x+30,y+21,'1',NG_INK);break;
 case 15: /* A skyline with different building heights. */
  for(int i=0;i<4;i++){int h=(i==0?12:i==1?25:i==2?18:8);box(c,x+2+i*9,y+30-h,7,h,accent);for(int yy=30-h+3;yy<27;yy+=5)ng_rect(c,x+4+i*9,y+yy,2,2,accent);}
  ng_line(c,x,y+30,x+39,y+30,accent);break;
 case 16:grid(c,x+6,y+2,"121332213",accent,(1u<<0)|(1u<<5)|(1u<<7));break;
 case 17: /* Binary puzzle. */
  ng_small(c,x+3,y+2,"0 1 0",NG_INK);ng_small(c,x+3,y+12,"1 0 1",accent);ng_small(c,x+3,y+22,"0 1 1",NG_INK);break;
 case 18: /* Consecutive path through a grid. */
  grid(c,x+6,y+2,"123654789",accent,0);ng_border(c,x+6,y+2,28,10,accent,2);ng_border(c,x+6,y+20,28,10,accent,2);break;
 case 19:grid(c,x+6,y+2,"816357492",accent,0);break;
 case 20: /* Keep/remove a number to meet a row sum. */
  grid(c,x+1,y+2,"2341 22 3",accent,0);ng_line(c,x+11,y+4,x+18,y+10,NG_RED);
  ng_small(c,x+29,y+4,"6",accent);ng_small(c,x+29,y+13,"3",accent);ng_small(c,x+29,y+22,"5",accent);break;
 case 21: /* Nim matchstick piles. */
  for(int row=0;row<3;row++)for(int i=0;i<3+row*2;i++){int xx=x+3+i*5;ng_rect(c,xx,y+2+row*10,2,8,accent);ng_rect(c,xx,y+2+row*10,2,2,NG_INK);}
  break;
 case 22: /* Two piles and equal removal. */
  for(int row=0;row<4;row++)box(c,x+3,y+23-row*6,12,5,accent);
  for(int row=0;row<3;row++)box(c,x+24,y+23-row*6,12,5,accent);
  ng_line(c,x+16,y+10,x+22,y+16,NG_INK);ng_line(c,x+16,y+16,x+22,y+10,NG_INK);break;
 case 23: /* Repeated subtraction as lengths. */
  ng_small(c,x+1,y,"15",NG_INK);for(int i=0;i<3;i++)box(c,x+1+i*12,y+10,12,7,accent);
  ng_small(c,x+1,y+23,"6",NG_INK);box(c,x+13,y+22,15,7,accent);ng_line(c,x+31,y+25,x+38,y+25,accent);break;
 case 24: /* Three cards totaling fifteen. */
  for(int i=0;i<3;i++){box(c,x+1+i*13,y+1,12,18,accent);digit(c,x+5+i*13,y+6,"249"[i],NG_INK);}ng_small(c,x+11,y+24,"=15",accent);break;
 case 25: /* Race along a numbered track to the flag. */
  ng_line(c,x+2,y+23,x+36,y+23,accent);for(int i=0;i<6;i++)ng_line(c,x+3+i*6,y+20,x+3+i*6,y+26,accent);
  ng_rect(c,x+12,y+19,5,5,accent);ng_line(c,x+33,y+2,x+33,y+23,NG_INK);
  ng_rect(c,x+34,y+2,6,7,accent);ng_small(c,x+2,y+4,"21",NG_INK);break;
 case 26: /* Merging numbered tiles. */
  for(int i=0;i<4;i++){int xx=x+4+(i%2)*17,yy=y+(i/2)*16,bg,fg;ng_2048_colors(i<3?(unsigned)i+1:0,&bg,&fg);ng_rect(c,xx,yy,16,15,bg);ng_border(c,xx,yy,16,15,accent,1);digit(c,xx+5,yy+4,"248 "[i],fg);}break;
 case 27: /* Sliding board with a visible empty square. */
  grid(c,x+6,y+2,"12345678 ",accent,0);ng_rect(c,x+25,y+21,8,8,NG_LINE);break;
 case 28: /* Lit cross in a Lights Out grid. */
  grid(c,x+6,y+2,"         ",accent,186);break;
 case 29: /* Timed arithmetic, with a lightning bolt. */
  timer(c,x-7,y,accent);ng_rect(c,x+7,y+10,13,16,NG_WHITE);
  ng_line(c,x+17,y+9,x+9,y+19,accent);ng_line(c,x+9,y+19,x+17,y+19,accent);ng_line(c,x+17,y+19,x+10,y+28,accent);
  ng_small(c,x+28,y+6,"+",NG_INK);ng_small(c,x+28,y+20,"*",NG_INK);break;
 case 30: /* Digits to remember, then a concealed card. */
  for(int i=0;i<3;i++){box(c,x+1+i*13,y+7,12,18,accent);digit(c,x+5+i*13,y+12,"72?"[i],i==2?accent:NG_INK);}
  ng_line(c,x+5,y+3,x+33,y+3,accent);ng_line(c,x+5,y+3,x+5,y+6,accent);ng_line(c,x+33,y+3,x+33,y+6,accent);break;
 case 31: /* Rectangles each containing their area clue. */
  box(c,x+2,y+2,18,28,accent);box(c,x+19,y+2,19,14,accent);box(c,x+19,y+15,19,15,accent);
  digit(c,x+9,y+13,'6',NG_INK);digit(c,x+26,y+5,'3',NG_INK);digit(c,x+26,y+19,'3',NG_INK);break;
 case 32: /* Dotted lattice and one loop around clues. */
  for(int row=0;row<4;row++)for(int col=0;col<5;col++)ng_rect(c,x+col*9,y+row*10,2,2,NG_INK);
  ng_border(c,x,y,38,22,accent,2);ng_small(c,x+11,y+7,"2 3",NG_INK);break;
 case 33: /* BLACK BOX: a ray enters, bends around hidden atoms and exits. */
  box(c,x+6,y+3,28,27,accent);ng_line(c,x,y+9,x+12,y+9,accent);
  ng_line(c,x+12,y+9,x+18,y+15,accent);ng_line(c,x+18,y+15,x+28,y+15,accent);
  ng_rect(c,x+19,y+22,5,5,NG_INK);ng_rect(c,x+27,y+6,4,4,NG_INK);
  ng_line(c,x+28,y+15,x+39,y+15,accent);break;
 case 34: /* CRYPTARITHM: letters resolve to a digit sum. */
  ng_small(c,x+5,y+1,"AB",NG_INK);ng_small(c,x+1,y+11,"+BA",accent);
  ng_line(c,x+1,y+21,x+36,y+21,accent);ng_small(c,x+10,y+23,"??",NG_INK);break;
 case 35: /* HASHI: numbered islands with two parallel bridges. */
  ring(c,x+6,y+8,5,accent);ring(c,x+33,y+8,5,accent);
  digit(c,x+4,y+5,'2',NG_INK);digit(c,x+31,y+5,'3',NG_INK);
  ng_line(c,x+11,y+6,x+28,y+6,accent);ng_line(c,x+11,y+10,x+28,y+10,accent);
  ring(c,x+20,y+26,5,accent);digit(c,x+18,y+23,'1',NG_INK);
  ng_line(c,x+33,y+13,x+23,y+23,accent);break;
 case 36: /* NONOGRAM: row/column clues and revealed squares. */
  ng_small(c,x+13,y+1,"1 2",accent);ng_small(c,x,y+10,"2",accent);
  ng_small(c,x,y+21,"1",accent);
  for(int row=0;row<2;row++)for(int col=0;col<3;col++){
   int xx=x+13+col*8,yy=y+10+row*10;
   ng_rect(c,xx,yy,8,10,(row==0 && col<2)||(row==1 && col==2)?accent:NG_WHITE);
   ng_border(c,xx,yy,8,10,accent,1);
  }break;
 case 37: /* REVERSI: black and white discs around a legal flip. */
  box(c,x+4,y+2,32,29,accent);
  for(int row=0;row<2;row++)for(int col=0;col<2;col++){
   int xx=x+10+col*14,yy=y+6+row*13;
   ring(c,xx+4,yy+4,4,accent);
   if(row==col)ng_rect(c,xx+2,yy+2,5,5,NG_INK);
  }break;
 case 38: /* NET: rotated pipe tiles joining a central hub. */
  for(int row=0;row<3;row++)for(int col=0;col<3;col++)box(c,x+5+col*10,y+1+row*10,10,10,accent);
  ng_rect(c,x+18,y+14,4,4,NG_INK);
  ng_line(c,x+20,y+5,x+20,y+25,accent);ng_line(c,x+10,y+16,x+30,y+16,accent);
  ng_line(c,x+30,y+16,x+30,y+26,accent);break;
 default:break;
 }
}
