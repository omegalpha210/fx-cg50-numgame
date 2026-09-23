/* Host-only renderer fixtures. Link with build-host/libnumgame_core.a.
   Usage: guesscalc_capture existing-output-directory
   These are deterministic logical-input/renderer examples, not hardware proof. */
#include "app.h"
#include "../../tests/test_guesscalc_workflow.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static NgApp app;
static uint16_t pixels[396*224];
static void paint(void *ctx,int x,int y,int w,int h,uint16_t color)
{
 (void)ctx;
 assert(x>=0&&y>=0&&w>0&&h>0&&x+w<=396&&y+h<=224);
 for(int r=y;r<y+h;r++)for(int col=x;col<x+w;col++)pixels[r*396+col]=color;
}
static void press(void *ctx,int key)
{
 NgGame *g=ctx;(void)ng_guesscalc[g->id-1].action(g,key);assert(ng_valid(g));
}
static NgGame *start(unsigned id,unsigned difficulty)
{
 ng_app_init(&app,(NgHooks){0},51);
 ng_new(&app.session.game,id,difficulty,0,51,1);
 app.screen=NG_PLAY;app.selected_id=(uint8_t)id;
 return &app.session.game;
}
static NgGame *pack(unsigned id,unsigned index)
{
 NgGame *g=start(id,index/30);
 for(unsigned seed=1;seed<10000;seed++){
  ng_new(g,id,index/30,0,seed,1);
  if(g->puzzle_id==index)return g;
 }
 assert(!"fixture pack unavailable");return g;
}
static void capture(const char *directory,const char *name)
{
 NgCanvas canvas={NULL,paint};ng_render(&app,&canvas);
 char path[1024];int n=snprintf(path,sizeof(path),"%s/%s.ppm",directory,name);
 assert(n>0&&(size_t)n<sizeof(path));FILE *file=fopen(path,"wb");assert(file);
 fprintf(file,"P6\n396 224\n255\n");
 for(unsigned i=0;i<396*224;i++){
  unsigned p=pixels[i];unsigned char rgb[3]={
   (unsigned char)(((p>>11)&31)*255/31),(unsigned char)(((p>>5)&63)*255/63),(unsigned char)((p&31)*255/31)};
  assert(fwrite(rgb,1,3,file)==3);
 }
 assert(!fclose(file));
}
int main(int argc,char **argv)
{
 assert(argc==2);NgGame *g=start(2,0);capture(argv[1],"02-equation-entry");
 gc_test_type(g,press,"12+7=19");press(g,NGK_EXE);assert(g->status==NG_PLAYING);
 gc_test_type(g,press,"8/4+2=4");capture(argv[1],"02-equation-feedback");
 g=pack(6,60);gc_test_type(g,press,"8/(3-8/3)");capture(argv[1],"06-target-rational");
 g=pack(7,60);gc_test_type(g,press,"100+(75-25)/(3+2)*1");press(g,NGK_EXE);
 assert(g->data[3]==10&&g->score==5);capture(argv[1],"07-countdown-distance");
 g=start(8,2);for(unsigned i=0;i<4;i++){press(g,"+-*/"[i]);if(i<3)press(g,NGK_RIGHT);}
 capture(argv[1],"08-operators-four-colors");
 (void)start(9,1);capture(argv[1],"09-cross-arithmetic");
 g=start(10,1);press(g,NGK_AUX);capture(argv[1],"10-factor-example");
 g=start(5,1);gc_test_solve(g,g,press);capture(argv[1],"05-sequence-solved");
 for(unsigned id=1;id<=10;id++){
  g=start(id,NG_MASTER);
  if(id==1||id==2){ng_new(g,id,NG_MASTER,2,51,1);gc_test_solve(g,g,press);}
  else if(id==3){for(unsigned row=0;row<10;row++)press(g,NGK_DOWN);press(g,NGK_HINT);}
  else if(id==5||id==9)gc_test_solve(g,g,press);
  else if(id==6||id==10)press(g,NGK_AUX);
  else if(id==7)gc_test_type(g,press,gc_master_countdown[g->puzzle_id-90].answer);
  else if(id==8){for(unsigned i=0;i<5;i++){press(g,g->data[16+i]);if(i<4)press(g,NGK_RIGHT);}}
  char name[32];snprintf(name,sizeof(name),"%02u-master",id);capture(argv[1],name);
 }
 puts("Eighteen guesscalc renderer fixtures including all10 MASTER games PASS");return 0;
}
