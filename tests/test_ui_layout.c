#include "app.h"
#include "grids_internal.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static NgApp app;
static int left,top,right=396,bottom=224;
static unsigned rectangles;
static void paint(void *ctx,int x,int y,int w,int h,uint16_t color)
{(void)ctx;(void)color;assert(x>=left && y>=top && x+w<=right && y+h<=bottom);rectangles++;}
static NgCanvas canvas={NULL,paint};
static void draw(void){ng_render(&app,&canvas);}
static void press(int key){ng_app_event(&app,key,NG_DOWN);ng_app_event(&app,key,NG_UP);draw();}
static void enter(unsigned id,unsigned difficulty,unsigned mode)
{
 ng_app_init(&app,(NgHooks){0},4521);app.settings.difficulty[id-1]=(uint8_t)difficulty;app.settings.mode[id-1]=(uint8_t)mode;
 press('1'+(int)(ng_catalog_index(id)/5));press('1'+(int)(ng_catalog_index(id)%5));assert(app.screen==NG_ENTRY);
}
static void menu_matrix(void)
{
 unsigned cases=0;
 for(unsigned index=0;index<NG_GAME_COUNT;index++)for(unsigned d=0;d<ng_difficulty_count(ng_visible_id(index));d++)for(unsigned mode=0;mode<ng_module(ng_visible_id(index))->modes;mode++) {
  unsigned id=ng_visible_id(index);enter(id,d,mode);bool level=ng_entry_level(&app),modes=ng_module(id)->modes>1;
  unsigned count=1+(unsigned)level+(unsigned)modes;assert(ng_entry_count(&app)==count && app.entry_selection==0);
  NgSettings settings=app.settings;uint32_t seed=app.seed;
  press(NGK_F1);press(NGK_F2);press(NGK_F3);press('1'+(int)count);
  assert(!memcmp(&settings,&app.settings,sizeof settings) && seed==app.seed && app.screen==NG_ENTRY);
  press(NGK_UP);assert(app.entry_selection==count-1);press(NGK_DOWN);assert(app.entry_selection==0);
  if(level){
   app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_LEVEL);
   for(unsigned i=0;i<5;i++)press(NGK_LEFT);assert(app.settings.difficulty[id-1]==0);
   app.settings_dirty=false;press(NGK_LEFT);assert(!app.settings_dirty);
   for(unsigned i=0;i<5;i++)press(NGK_RIGHT);assert(app.settings.difficulty[id-1]+1==ng_regular_difficulty_count(id));
   app.settings_dirty=false;press(NGK_RIGHT);assert(!app.settings_dirty);app.settings.difficulty[id-1]=(uint8_t)d;
  }
  if(modes){
   app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_MODE);
   for(unsigned i=0;i<9;i++)press(NGK_LEFT);assert(app.settings.mode[id-1]==0);
   app.settings_dirty=false;press(NGK_LEFT);assert(!app.settings_dirty);
   for(unsigned i=0;i<9;i++)press(NGK_RIGHT);assert(app.settings.mode[id-1]+1==ng_module(id)->modes);
   app.settings_dirty=false;press(NGK_RIGHT);assert(!app.settings_dirty && ng_entry_action(&app,app.entry_selection)==NG_ENTRY_MODE);
  }
  /* Every visible row: digits only focus; both OPEN keys launch using unchanged options. */
  for(unsigned row=0;row<count;row++)for(unsigned k=0;k<2;k++){
   enter(id,d,mode);press('1'+(int)row);assert(app.screen==NG_ENTRY && app.entry_selection==row);
   press(k?NGK_EXE:NGK_F6);assert(app.screen==NG_PLAY && ng_valid(&app.session.game));
   assert(app.session.game.difficulty==(id==26 && !mode?NG_NORMAL:d) && app.session.game.mode==mode && !app.session.game.moves);
  }
  NgGame saved=app.session.game;press(NGK_EXIT);assert(app.screen==NG_ENTRY && app.entry_selection==ng_entry_row(&app,NG_ENTRY_NEW));
  press(NGK_F4);assert(app.modal==NG_MODAL_RECORDS);
  if(id==26){unsigned rd=app.record_difficulty,n=mode?4:3;press(NGK_F4);press(NGK_RIGHT);press(NGK_LEFT);assert(app.record_difficulty==(rd+1)%n);}
  if(!modes){press(NGK_F3);assert(app.record_mode==mode);}
  press(NGK_EXIT);
  if(level)app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_LEVEL);
  ng_app_event(&app,NGK_F6,NG_DOWN);assert(app.modal==NG_MODAL_NEW);
  for(unsigned repeat=0;repeat<5;repeat++)ng_app_event(&app,NGK_F6,NG_HOLD);
  assert(app.modal==NG_MODAL_NEW && !memcmp(&saved,&app.session.game,sizeof saved));ng_app_event(&app,NGK_F6,NG_UP);
  press(NGK_EXIT);app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_RESUME);press(NGK_F6);
  assert(app.screen==NG_PLAY && !memcmp(&saved,&app.session.game,sizeof saved));cases++;
 }
 printf("Entry UI: %u configurations, all-row EXE/F6 launches, digit focus, clamped options, NEW confirmation and explicit RESUME PASS\n",cases);
}
static void catalog_identity(void)
{
 bool seen[NG_ID_MAX+1]={0};
 for(unsigned i=0;i<NG_GAME_COUNT;i++){unsigned id=ng_visible_id(i);assert(id&&id<=NG_ID_MAX&&!seen[id]);seen[id]=true;assert(ng_catalog_index(id)==(int)i&&ng_module(id)->id==id);}
 assert(!seen[29]&&!seen[30]&&seen[31]&&seen[32]&&!ng_visible_id(30));
 ng_app_init(&app,(NgHooks){0},1);app.settings.last_game=30;app.summary[29].exists=1;press(NGK_F3);assert(app.screen==NG_MAIN&&!app.active);
 assert(ng_module(29)&&ng_module(30)&&ng_catalog_index(29)<0&&ng_catalog_index(30)<0);
 puts("Catalog: exactly30 visible unique IDs, legacy29/30 retained but never Main RESUME or board aliases PASS");
}
static void box_limits(void)
{
 char long_text[192];memset(long_text,'W',sizeof long_text-1);long_text[sizeof long_text-1]=0;
 left=41;top=31;right=168;bottom=75;
 ng_text_fit(&canvas,left,top,127,long_text,NG_INK,3);
 ng_center(&canvas,left,top,127,long_text,NG_INK,2);
 ng_small_fit(&canvas,left,top,127,long_text,NG_INK);
 ng_wrap(&canvas,left,top,127,12,3,long_text,NG_INK,false);
 left=top=0;right=396;bottom=224;
 const char *source="alpha beta\ngamma delta epsilon";char line[32],joined[96]="";
 while(ng_text_line(line,sizeof line,&source,80,false)){assert(ng_text_width(line,1)<=80);if(*joined)strcat(joined," ");strcat(joined,line);}
 assert(!strcmp(joined,"alpha beta gamma delta epsilon"));
 unsigned compact=0;
 for(unsigned i=90;i<180;i++) {
  const GridsPuzzle *p=grids_record(i);assert(p && p->id==12);int size=156/p->n;if(size>36)size=36;
  for(unsigned cell=0;cell<(unsigned)p->n*p->n;cell++) {
   unsigned cage=(unsigned)p->a[cell];int op=p->b[cage*2],target=p->b[cage*2+1];char clue[20];
   snprintf(clue,sizeof clue,"%d%c",target," +*-/"[op]);int width=ng_small_width(clue);
   if(width>size-5){assert(op==2);snprintf(clue,sizeof clue,"%d",target);width=ng_small_width(clue)+4;compact++;}
   assert(width<=size-5);
  }
 }
 assert(compact);
 puts("Text boxes: fitted/centered/wrapped text stays inside declared rectangle PASS");
}
static void stress_views(void)
{
 for(unsigned index=0;index<NG_GAME_COUNT;index++)for(unsigned d=0;d<ng_difficulty_count(ng_visible_id(index));d++)for(unsigned mode=0;mode<ng_module(ng_visible_id(index))->modes;mode++) {
  unsigned id=ng_visible_id(index);
  enter(id,d,mode);press(NGK_F6);
  app.session.game.elapsed_ms=UINT32_MAX;draw();
  press(NGK_F5);for(unsigned i=0;i<20;i++)press(NGK_DOWN);press(NGK_EXIT);
  if(id>=26 && id<=28){app.session.game.moves=UINT32_MAX;app.session.game.score=UINT32_MAX;draw();}
  if(id==26){for(unsigned i=0;i<16;i++)app.session.game.board[i]=(int16_t)(i+15);app.session.game.data[0]=30;draw();}
  app.modal=NG_MODAL_RESULT;app.session.game.status=NG_WON;app.session.game.moves=UINT32_MAX;app.session.game.score=UINT32_MAX;
  strcpy(app.session.game.message,"A long result sentence with every important detail kept inside the result panel, including the final answer: 1234567890.");draw();
  app.modal=NG_MODAL_RECORDS;app.record_mode=(uint8_t)mode;app.record_difficulty=(uint8_t)d;
  NgBest *b=&app.session.stats.best[mode][d][0];*b=(NgBest){.completed=UINT32_MAX,.wins=UINT32_MAX/3,.losses=UINT32_MAX/3,.draws=UINT32_MAX/3,.best_score=UINT32_MAX,.best_moves=UINT32_MAX,.best_ms=UINT32_MAX,.best_aux=id==26?30:0};draw();
 }
 ng_app_init(&app,(NgHooks){0},1);app.screen=NG_STATS;app.stats_category=6;
 for(unsigned i=0;i<NG_ID_MAX;i++){app.summary[i].played=app.summary[i].active_ms=UINT32_MAX;app.summary[i].completed=app.summary[i].assisted=UINT32_MAX/2;}
 for(unsigned cat=0;cat<6;cat++){app.stats_page=(uint8_t)cat;draw();}
 puts("All mode/difficulty/rules views and maximum-width result/stat/tile fixtures PASS");
}
int main(void){catalog_identity();box_limits();menu_matrix();stress_views();assert(rectangles);puts("UI layout audit PASS");return 0;}
