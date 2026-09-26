#include "app.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static NgApp app;
static unsigned rectangles;
static void paint(void *ctx,int x,int y,int w,int h,uint16_t color)
{
 (void)ctx;(void)color;
 assert(x>=0 && y>=0 && w>0 && h>0 && x+w<=396 && y+h<=224);
 rectangles++;
}
static void draw(void){NgCanvas canvas={NULL,paint};ng_render(&app,&canvas);}
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
    for(const char *p="1000";*p;p++)press(*p);
    press(NGK_EXE);assert(app.settings.target==1000);
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
int main(void){catalog();settings_rows();extreme_text();return 0;}
