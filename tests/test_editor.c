#include "app.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

static NgApp app,cold;
static void key(int code)
{(void)ng_app_event(&app,code,NG_DOWN);(void)ng_app_event(&app,code,NG_UP);}
static void prime(void)
{
 ng_app_init(&app,(NgHooks){0},123);
 ng_new(&app.session.game,10,NG_EASY,0,123,1);
 app.session.stats.started=1;
 app.screen=NG_PLAY;app.active=app.resumable=true;app.selected_id=10;
 app.settings.first_help=0;
}
static void editing(void)
{
 NgGame g={0};
 assert(!ng_editor_move(&g,NGK_LEFT));
 assert(ng_edit_expression(&g,'2',"0123456789*^",96));
 assert(ng_edit_expression(&g,NGK_SQUARE,"0123456789*^",96));
 assert(!strcmp(g.input,"2^2")&&g.edit_cursor==3);
 assert(ng_editor_move(&g,NGK_LEFT));assert(ng_editor_move(&g,NGK_LEFT));
 assert(g.edit_cursor==1);
 assert(ng_edit_expression(&g,'*',"0123456789*^",96));
 assert(!strcmp(g.input,"2*^2")&&g.edit_cursor==2);
 assert(ng_edit_expression(&g,NGK_DEL,"0123456789*^",96));
 assert(!strcmp(g.input,"2^2")&&g.edit_cursor==1);
 assert(ng_edit_expression(&g,NGK_DEL,"0123456789*^",96));
 assert(!strcmp(g.input,"^2")&&g.edit_cursor==0);
 assert(!ng_edit_expression(&g,NGK_DEL,"0123456789*^",96));
 assert(!ng_editor_move(&g,NGK_LEFT));
 for(unsigned i=0;i<94;i++)g.input[i]='1';g.input[94]=0;ng_editor_reset(&g);
 assert(ng_edit_expression(&g,NGK_SQUARE,"0123456789*^",96));
 assert(strlen(g.input)==96&&g.edit_cursor==96);
 memset(g.input,'1',95);g.input[95]=0;ng_editor_reset(&g);
 assert(ng_edit_expression(&g,NGK_SQUARE,"0123456789*^",96));
 assert(strlen(g.input)==95&&!strchr(g.input,'^'));
 g.input[0]=0;ng_editor_reset(&g);
 assert(!ng_edit_expression(&g,NGK_SQUARE,"0123456789+-*/()",96));
 assert(!g.input[0]);
}
static void app_editor(void)
{
 prime();key('2');key('*');key('3');assert(!strcmp(app.session.game.input,"2*3"));
 app.dirty=false;NgGame expected=app.session.game;
 key(NGK_LEFT);key(NGK_LEFT);
 assert(app.session.game.edit_cursor==1&&!app.dirty);
 expected.edit_cursor=1;assert(!memcmp(&expected,&app.session.game,sizeof expected));
 assert(!app.session.undo_count);
 key(NGK_SQUARE);
 assert(!strcmp(app.session.game.input,"2^2*3")&&app.session.game.edit_cursor==3);
 key(NGK_LEFT);key(NGK_DEL);
 assert(!strcmp(app.session.game.input,"22*3")&&app.session.game.edit_cursor==1);
 key(NGK_SHIFT);key(NGK_SQUARE);assert(!strcmp(app.session.game.input,"22*3"));
 key(NGK_RIGHT);key(NGK_RIGHT);key(NGK_RIGHT);key(NGK_RIGHT);
 assert(app.session.game.edit_cursor==4);
 key(NGK_EXE);assert(!strcmp(app.session.game.input,"22*3"));
 key(NGK_LEFT);key(NGK_LEFT);assert(app.session.game.edit_cursor==2);
 /* A cold v5 decode retains the text and defaults the runtime caret to end. */
 uint8_t payload[NG_RECORD_MAX];size_t n=ng_single_encode(&app.session,payload,sizeof payload);
 assert(n);memset(&cold,0,sizeof cold);
 assert(ng_single_decode(&cold.session,payload,n,10));
 assert(!strcmp(cold.session.game.input,app.session.game.input));
 assert(cold.session.game.edit_cursor==strlen(cold.session.game.input));
 assert(cold.session.game.edit_cursor!=app.session.game.edit_cursor);
 key(NGK_F4);assert(app.session.game.edit_cursor==strlen(app.session.game.input));
}
typedef struct {
 int x,y,w,h,caret,ink;
 unsigned char glyphs[21][376];
 unsigned colors[5];
} Draw;
static void paint(void *ctx,int x,int y,int w,int h,uint16_t color)
{
 Draw *d=ctx;assert(x>=d->x&&x+w<=d->x+d->w&&y>=d->y&&y+h<=d->y+d->h);
 if(w==1&&h==12&&color==NG_BLUE)d->caret=x;
 if(w==1&&h==1){
  d->glyphs[y-d->y][x-d->x]=1;
  const int symbols[]={0,'+','-','*','/'};
  for(unsigned i=0;i<5;i++)if(color==ng_operator_color(symbols[i],NG_BLUE))d->colors[i]++;
 }
 d->ink++;
}
static void visual(void)
{
 static Draw d;d=(Draw){.x=50,.y=30,.w=74,.h=21,.caret=-1};NgCanvas canvas={&d,paint};
 ng_input_expression_at(&canvas,d.x,d.y,d.w,"1234567890+1234567890",21);
 assert(d.ink&&d.caret>=d.x+6&&d.caret<d.x+d.w-5);
 int right=d.caret;d.caret=-1;
 ng_input_expression_at(&canvas,d.x,d.y,d.w,"1234567890+1234567890",3);
 assert(d.caret>=d.x+6&&d.caret<d.x+d.w-5&&d.caret!=right);
}
static void visual_gap(void)
{
 static Draw d;
 static const char *const values[]={"","0","1","12","123","999","2^2","12+34",
  "(12+34)/(5-6)","2^3*3^2*5","12+34=46","0123456789+-*/()^=",
  "999+999+999+999+999+999+999+999+999+999+999+999+999+999+999+999+999+999+999+999+999+999+999+999"};
 unsigned cases=0;
 for(unsigned width=0;width<2;width++)for(unsigned v=0;v<sizeof values/sizeof values[0];v++){
  const char *value=values[v];unsigned length=(unsigned)strlen(value);
  unsigned colors[5]={0};
  for(unsigned cursor=0;cursor<=length+1;cursor++){
   d=(Draw){.x=10,.y=30,.w=width?376:74,.h=21,.caret=-1};NgCanvas canvas={&d,paint};
   ng_input_expression_at(&canvas,d.x,d.y,d.w,value,cursor);
   assert(d.caret>=d.x+6&&d.caret<d.x+d.w-5);
   /* The actual glyph raster must leave a blank column on BOTH sides of
      the one-pixel caret, including when the input has scrolled. */
   for(int row=4;row<16;row++)for(int col=d.caret-d.x-1;col<=d.caret-d.x+1;col++){
    if(d.glyphs[row][col])fprintf(stderr,"Caret/glyph gap: value=%s cursor=%u width=%d x=%d\n",value,cursor,d.w,d.caret);
    assert(!d.glyphs[row][col]);
   }
   if(width&&ng_text_width(value,1)<=d.w-16){
    if(!cursor)memcpy(colors,d.colors,sizeof colors);
    else assert(!memcmp(colors,d.colors,sizeof colors));
   }
   cases++;
  }
 }
 printf("Caret raster: %u start/middle/end/clamped/scroll cases, 1px gaps and token colors PASS\n",cases);
}
static void result(void)
{
 NgGame g={0};char text[96];g.id=10;g.moves=7;g.score=99;g.elapsed_ms=125000;
 ng_result_summary(&g,true,text);assert(!strcmp(text,"TIME 2:05"));
 ng_result_summary(&g,false,text);assert(!text[0]);
 g.assisted=1;ng_result_summary(&g,false,text);assert(!strcmp(text,"ASSISTED"));
 g.id=26;g.assisted=0;g.score=0;ng_result_summary(&g,true,text);
 assert(!strcmp(text,"SCORE 0"));
 g.id=7;g.data[3]=0;ng_result_summary(&g,true,text);
 assert(!strcmp(text,"SCORE 0    BEST DIST 0"));
 g.id=27;g.moves=18;ng_result_summary(&g,true,text);
 assert(!strcmp(text,"MOVES 18"));
 g.id=28;ng_result_summary(&g,true,text);assert(!strcmp(text,"PRESSES 18"));
 g.id=1;ng_result_summary(&g,true,text);assert(!strcmp(text,"TRIES 18"));
 g.id=3;ng_result_summary(&g,true,text);assert(!strcmp(text,"TIME 2:05"));
}
int main(void)
{editing();app_editor();visual();visual_gap();result();puts("Editor cursor/square, clipping, save and result policies PASS");return 0;}
