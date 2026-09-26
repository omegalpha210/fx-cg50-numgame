#include "app.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static NgApp app;
static unsigned rectangles;
static uint16_t rule_pixels[396*224];
static int card_x[6],card_w[6];static unsigned card_count;
static void paint(void *ctx,int x,int y,int w,int h,uint16_t color)
{
 (void)ctx;(void)color;
 assert(x>=0 && y>=0 && w>0 && h>0 && x+w<=396 && y+h<=224);
 rectangles++;
}
static void draw(void){NgCanvas canvas={NULL,paint};ng_render(&app,&canvas);}
static void rule_paint(void *ctx,int x,int y,int w,int h,uint16_t color)
{(void)ctx;for(int row=y;row<y+h;row++)for(int col=x;col<x+w;col++)rule_pixels[row*396+col]=color;}
static void draw_rules(void)
{memset(rule_pixels,0,sizeof rule_pixels);NgCanvas canvas={NULL,rule_paint};ng_render(&app,&canvas);}
static void card_paint(void *ctx,int x,int y,int w,int h,uint16_t color)
{(void)ctx;if(y==63 && h==37 && color==NG_WHITE){assert(card_count<6);card_x[card_count]=x;card_w[card_count++]=w;}}
static void press(int key){ng_app_event(&app,key,NG_DOWN);ng_app_event(&app,key,NG_UP);draw();}
static void enter(unsigned id,unsigned difficulty,unsigned mode)
{
 ng_app_init(&app,(NgHooks){0},4521);
 app.settings.difficulty[id-1]=(uint8_t)difficulty;
 app.settings.mode[id-1]=(uint8_t)mode;
 int index=ng_catalog_index(id);assert(index>=0);
 press('1'+index/6);press('1'+index%6);
 assert(app.screen==NG_ENTRY && app.selected_id==id);
}
static void catalog(void)
{
 bool seen[NG_ID_MAX+1]={0};
 for(unsigned i=0;i<NG_GAME_COUNT;i++){
  unsigned id=ng_visible_id(i);assert(id && id<=NG_ID_MAX && !seen[id]);
  seen[id]=true;assert(ng_catalog_index(id)==(int)i && ng_module(id)->id==id);
 }
 assert(!seen[29] && !seen[30]);
 for(unsigned id=33;id<=38;id++)assert(seen[id]);
 assert(ng_visible_id(NG_GAME_COUNT)==0);
}
static void settings_rows(void)
{
 unsigned configurations=0;
 for(unsigned index=0;index<NG_GAME_COUNT;index++){
  unsigned id=ng_visible_id(index);
  for(unsigned d=0;d<ng_difficulty_count(id);d++)for(unsigned mode=0;mode<ng_module(id)->modes;mode++){
   enter(id,d,mode);
   unsigned expected=1+(unsigned)ng_entry_level(&app)+(id==6)+(ng_module(id)->modes>1);
   assert(ng_entry_count(&app)==expected);
   assert(ng_entry_action(&app,0)==NG_ENTRY_NEW);
   press(NGK_F1);press(NGK_F2);press(NGK_F4);
   assert(app.screen==NG_ENTRY && !app.modal);
   if(ng_entry_level(&app) && d<ng_regular_difficulty_count(id)){
    app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_LEVEL);
    app.settings.difficulty[id-1]=NG_EASY;press(NGK_LEFT);assert(app.settings.difficulty[id-1]==NG_EASY);
    app.settings.difficulty[id-1]=NG_MASTER;press(NGK_RIGHT);assert(app.settings.difficulty[id-1]==NG_MASTER);
    app.settings.difficulty[id-1]=(uint8_t)d;
   }
   if(ng_module(id)->modes>1){
    app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_MODE);
    press(NGK_F3);assert(!app.modal);
    press(NGK_LEFT);assert(app.settings.mode[id-1]==(mode?mode-1:0));
    app.settings.mode[id-1]=(uint8_t)mode;
   }
   if(id==6){
    app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_TARGET);
    for(unsigned step=0;step<4;step++)press(NGK_RIGHT);
    assert(app.settings.target==NG_TARGET_RANDOM);
   }
   app.entry_selection=(uint8_t)ng_entry_row(&app,ng_entry_level(&app)?NG_ENTRY_LEVEL:NG_ENTRY_NEW);
   press(NGK_F6);assert(app.screen==NG_PLAY && ng_valid(&app.session.game));
   assert(app.session.game.id==id && app.session.game.mode==mode);
   assert(app.session.game.difficulty==(id==26 && !mode?NG_NORMAL:d));
   draw();press(NGK_F5);assert(app.modal==NG_MODAL_RULES);draw();press(NGK_EXIT);
   press(NGK_EXIT);assert(app.screen==NG_ENTRY && app.active);
   assert(ng_entry_action(&app,0)==NG_ENTRY_RESUME);
   app.entry_selection=0;press(NGK_F6);assert(app.screen==NG_PLAY);
   configurations++;
  }
 }
 printf("UI: %u visible configurations, inline options, rules, open and resume PASS\n",configurations);
}
static void extreme_text(void)
{
 char text[192];memset(text,'W',sizeof text-1);text[sizeof text-1]=0;
 NgCanvas canvas={NULL,paint};
 ng_text_fit(&canvas,41,31,127,text,NG_INK,3);
 ng_center(&canvas,41,31,127,text,NG_INK,2);
 ng_small_fit(&canvas,41,31,127,text,NG_INK);
 ng_wrap(&canvas,41,31,127,12,3,text,NG_INK,false);
 for(unsigned index=0;index<NG_GAME_COUNT;index++){
  unsigned id=ng_visible_id(index);enter(id,ng_difficulty_count(id)-1,0);
  draw();press(NGK_F6);assert(ng_valid(&app.session.game));
  app.session.game.elapsed_ms=UINT32_MAX;app.session.game.score=UINT32_MAX;draw();
  app.modal=NG_MODAL_RESULT;app.session.game.status=NG_WON;
  strcpy(app.session.game.message,"A long result sentence with every important detail kept inside the result panel, including the final answer: 1234567890.");
  draw();
 }
 assert(rectangles);
 puts("UI: all screens draw within 396x224 with extreme result text PASS");
}
static void rules_scroll(void)
{
 unsigned short_id=0,long_id=0,scrolling=0,one_more=0;
 for(unsigned index=0;index<NG_GAME_COUNT;index++){
  unsigned id=ng_visible_id(index),lines=ng_rules_line_count(id),max=ng_rules_max_scroll(id);
  assert(lines && max==(lines>10?lines-10:0) && max<100);
  if(!max && !short_id)short_id=id;
  if(max==1)one_more++;
  if(max){scrolling++;if(max>=4 && !long_id)long_id=id;}
  enter(id,NG_EASY,0);press(NGK_F5);
  assert(app.modal==NG_MODAL_RULES && app.rules_scroll==0);
  ng_app_event(&app,NGK_UP,NG_DOWN);ng_app_event(&app,NGK_UP,NG_UP);
  assert(app.rules_scroll==0);
  for(unsigned step=0;step<max+3;step++){
   ng_app_event(&app,NGK_DOWN,NG_DOWN);ng_app_event(&app,NGK_DOWN,NG_UP);
  }
  assert(app.rules_scroll==max);
  for(unsigned step=0;step<max+3;step++){
   ng_app_event(&app,NGK_UP,NG_DOWN);ng_app_event(&app,NGK_UP,NG_UP);
  }
  assert(app.rules_scroll==0);
  press(NGK_EXIT);assert(app.modal==NG_MODAL_NONE);
 }
 assert(short_id && long_id && one_more && scrolling);
 enter(short_id,NG_EASY,0);press(NGK_F5);draw_rules();
 assert(rule_pixels[70*396+375]==NG_WHITE);
 press(NGK_EXIT);
 enter(long_id,NG_EASY,0);press(NGK_F5);draw_rules();
 assert(rule_pixels[34*396+377]==NG_LINE);
 assert(rule_pixels[190*396+377]==NG_BLUE);
 assert(rule_pixels[49*396+375]==NG_BLUE);
 unsigned max=ng_rules_max_scroll(long_id);
 ng_app_event(&app,NGK_DOWN,NG_DOWN);assert(app.rules_scroll==1);
 ng_app_event(&app,NGK_DOWN,NG_HOLD);assert(app.rules_scroll==2);
 ng_app_event(&app,NGK_DOWN,NG_UP);
 for(unsigned i=2;i<max/2;i++){ng_app_event(&app,NGK_DOWN,NG_DOWN);ng_app_event(&app,NGK_DOWN,NG_UP);}
 draw_rules();assert(rule_pixels[34*396+377]==NG_BLUE && rule_pixels[190*396+377]==NG_BLUE);
 for(unsigned i=max/2;i<max;i++){ng_app_event(&app,NGK_DOWN,NG_DOWN);ng_app_event(&app,NGK_DOWN,NG_UP);}
 draw_rules();assert(rule_pixels[34*396+377]==NG_BLUE && rule_pixels[190*396+377]==NG_LINE);
 assert(rule_pixels[178*396+375]==NG_BLUE);
 printf("UI: RULES wrap/scroll for 36 games; %u need rail; arrows and bounds PASS\n",scrolling);
}
static void card_and_factor_alignment(void)
{
 enter(6,NG_MASTER,0);press(NGK_F6);
 assert(app.session.game.data[1]==6);
 for(unsigned i=0;i<6;i++)app.session.game.board[i]=999;
 card_count=0;NgCanvas card_canvas={NULL,card_paint};ng_render(&app,&card_canvas);
 assert(card_count==6 && card_x[0]==15 && card_x[5]+card_w[5]==380);
 for(unsigned i=1;i<6;i++)assert(card_w[i]==55 && card_x[i]-card_x[i-1]-card_w[i-1]==7);
 assert(ng_text_width("999",2)<card_w[0]-8);
 enter(10,NG_MASTER,0);press(NGK_F6);
 static const int values[]={9,360,43956,157626,488808};
 for(unsigned i=0;i<sizeof values/sizeof values[0];i++){
  app.session.game.data[0]=values[i];draw_rules();
  int minx=396,maxx=0,miny=224,maxy=0;
  for(int y=55;y<103;y++)for(int x=88;x<308;x++)if(rule_pixels[y*396+x]==NG_INK){
   if(x<minx)minx=x;
   if(x>maxx)maxx=x;
   if(y<miny)miny=y;
   if(y>maxy)maxy=y;
  }
  assert(minx<=maxx && miny<=maxy && maxx<308 && maxy<103);
  assert(minx+maxx>=392 && minx+maxx<=400);
 }
 puts("UI: equal centered six-card row and measured Prime target centering PASS");
}
int main(void){catalog();settings_rows();extreme_text();rules_scroll();card_and_factor_alignment();return 0;}
