/* Optional real-app renderer audit for owned MASTER entries. */
#include "app.h"
#include "ui.h"
#include "strategyquick.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static NgApp app;
static uint16_t pixels[396*224];
static void paint(void *ctx,int x,int y,int w,int h,uint16_t color){
 (void)ctx;assert(x>=0&&y>=0&&w>0&&h>0&&x+w<=396&&y+h<=224);
 for(int row=y;row<y+h;row++)for(int col=x;col<x+w;col++)pixels[row*396+col]=color;
}
static void save(const char *dir,const char *name){
 char path[512];int length=snprintf(path,sizeof path,"%s/%s.ppm",dir,name);assert(length>0&&(size_t)length<sizeof path);
 NgCanvas canvas={NULL,paint};memset(pixels,255,sizeof pixels);ng_render(&app,&canvas);assert(ng_valid(&app.session.game));
 FILE *f=fopen(path,"wb");assert(f);fprintf(f,"P6\n396 224\n255\n");
 for(unsigned p=0;p<396*224;p++){unsigned v=pixels[p];uint8_t rgb[3]={(uint8_t)(((v>>11)&31)*255/31),(uint8_t)(((v>>5)&63)*255/63),(uint8_t)((v&31)*255/31)};assert(fwrite(rgb,1,3,f)==3);}
 assert(!fclose(f));
}
static void press(int key){ng_app_event(&app,key,NG_DOWN);ng_app_event(&app,key,NG_UP);}
int main(int argc,char **argv){
 assert(argc==2 || (argc==3&&!strcmp(argv[2],"recent")));const unsigned ids[]={21,22,23,24,25,26,27,28,31,32,37,38};
 for(unsigned i=argc==3?10:0;i<sizeof ids/sizeof ids[0];i++){
  unsigned id=ids[i],mode=id>=26&&id<=28?1:0;ng_app_init(&app,(NgHooks){0},412);app.screen=NG_PLAY;app.selected_id=(uint8_t)id;app.active=app.resumable=true;app.session.stats.started=1;app.settings.difficulty[id-1]=3;app.settings.mode[id-1]=(uint8_t)mode;
  ng_new_supply(&app.session.game,id,3,mode,717,1,43,0);char name[40];snprintf(name,sizeof name,"master-%u",id);save(argv[1],name);
  if(id==37 || id==38){
   NgGame *g=&app.session.game;
   for(unsigned step=0;g->status==NG_PLAYING&&step<64;step++){
    if(id==38)press(NGK_F3);
    else if(g->cpu_pending)assert(ng_app_cpu(&app));
    else{NgGame decision=*g;int p=sq_reversi_pick(&decision,1);assert(p>=0);g->cursor=(uint8_t)p;press(NGK_EXE);}
    assert(ng_valid(g));
    if(id==37 && step==0 && g->cpu_pending)save(argv[1],"recent-37-cpu");
   }
   assert(g->status&&app.modal==NG_MODAL_RESULT);snprintf(name,sizeof name,"recent-%u-result-modal",id);save(argv[1],name);
   press(NGK_EXIT);assert(app.screen==NG_PLAY&&app.result_view&&!app.modal);snprintf(name,sizeof name,"recent-%u-result",id);save(argv[1],name);
   press(NGK_F6);assert(app.screen==NG_PLAY&&g->status==NG_PLAYING&&g->difficulty==3&&g->mode==mode);snprintf(name,sizeof name,"recent-%u-new",id);save(argv[1],name);
  }
  if(id==31){ng_app_event(&app,NGK_EXE,NG_DOWN);ng_app_event(&app,NGK_EXE,NG_UP);ng_app_event(&app,NGK_RIGHT,NG_DOWN);ng_app_event(&app,NGK_RIGHT,NG_UP);save(argv[1],"master-31-selection");}
 }
 puts("Owned actual app renderer: selected MASTER entries, result/NEW flow and pixel bounds PASS");return 0;
}
