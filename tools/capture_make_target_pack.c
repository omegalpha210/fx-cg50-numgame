/* Host-only deterministic HINT/ANSWER renderer comparison harness. */
#include "app.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static NgApp app;
static uint16_t pixels[396u*224u];
static void paint(void *context,int x,int y,int width,int height,uint16_t color)
{
 (void)context;
 assert(x>=0&&y>=0&&width>0&&height>0&&x+width<=396&&y+height<=224);
 for(int row=y;row<y+height;row++)for(int col=x;col<x+width;col++)pixels[row*396+col]=color;
}
static void press(int key)
{
 (void)ng_app_event(&app,key,NG_DOWN);
 (void)ng_app_event(&app,key,NG_UP);
}
static void capture(const char *directory,const char *name)
{
 memset(pixels,0xff,sizeof pixels);
 NgCanvas canvas={NULL,paint};ng_render(&app,&canvas);
 char path[512];int n=snprintf(path,sizeof path,"%s/%s.ppm",directory,name);
 assert(n>0&&(size_t)n<sizeof path);
 FILE *f=fopen(path,"wb");assert(f);
 assert(fprintf(f,"P6\n396 224\n255\n")>0);
 for(unsigned i=0;i<396u*224u;i++){
  unsigned p=pixels[i];unsigned char rgb[3]={
   (unsigned char)(((p>>11)&31u)*255u/31u),
   (unsigned char)(((p>>5)&63u)*255u/63u),
   (unsigned char)((p&31u)*255u/31u)};
  assert(fwrite(rgb,1,3,f)==3);
 }
 assert(!fclose(f));
}
int main(int argc,char **argv)
{
 assert(argc==2);
 ng_app_init(&app,(NgHooks){0},UINT32_C(0x62b6e4));
 app.settings.first_help=0;
 app.settings.target=24;
 app.settings.difficulty[5]=NG_HARD;
 app.selected_id=6;app.screen=NG_ENTRY;
 app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_NEW);
 press(NGK_F6);
 assert(app.screen==NG_PLAY&&!app.modal&&ng_valid(&app.session.game));
 printf("puzzle=%u target=%d\n",app.session.game.puzzle_id,(int)app.session.game.data[0]);
 press(NGK_F3);capture(argv[1],"hint");
 press(NGK_F4);capture(argv[1],"answer");
 assert(ng_valid(&app.session.game));
 return 0;
}
