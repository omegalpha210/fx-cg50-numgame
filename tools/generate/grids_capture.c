/* Host-only renderer review utility, no runtime dependency. */
#include "ng.h"
#include "ui.h"
#include "grids_internal.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint16_t pixels[224][396];
static bool expanded;
static unsigned capture_mode;
static void rect(void *context,int x,int y,int w,int h,uint16_t c)
{(void)context;assert(x>=0&&y>=0&&x+w<=396&&y+h<=224);for(int r=y;r<y+h;r++)for(int col=x;col<x+w;col++)pixels[r][col]=c;}
static int capture(unsigned id,unsigned difficulty,unsigned puzzle_id,bool master)
{
 NgCanvas c={NULL,rect};NgGame g={0};
 for(unsigned seed=master?1:77;seed<10000;seed++){
  memset(&g,0,sizeof g);g.id=(uint8_t)id;g.difficulty=(uint8_t)difficulty;g.mode=(uint8_t)capture_mode;g.seed=seed;g.rng=seed;ng_grids[id-11].init(&g);
  if(!master||g.puzzle_id==puzzle_id)break;
 }
 if(master&&g.puzzle_id!=puzzle_id)return 1;
 if(id==11){for(unsigned i=0;i<81;i++)if(!g.fixed[i]){g.cursor=(uint8_t)i;g.notes[i]=511;g.notes_mode=1;break;}}
 ng_rect(&c,0,0,396,224,NG_PAPER);ng_grids[id-11].render(&g,&c);ng_text(&c,10,8,ng_grids[id-11].name,NG_INK,1);
 char path[100];if(expanded)snprintf(path,sizeof path,"assets/grids/expanded/capture-%u-mode%u.ppm",puzzle_id,capture_mode);else if(master)snprintf(path,sizeof path,"assets/grids/master/capture-%u.ppm",puzzle_id);else snprintf(path,sizeof path,"assets/grids/%u.ppm",id);
 FILE *f=fopen(path,"wb");if(!f)return 1;fprintf(f,"P6\n396 224\n255\n");
 for(unsigned y=0;y<224;y++)for(unsigned x=0;x<396;x++){unsigned v=pixels[y][x];unsigned char rgb[3]={(unsigned char)((v>>11)*255/31),(unsigned char)(((v>>5)&63)*255/63),(unsigned char)((v&31)*255/31)};fwrite(rgb,1,3,f);}
 return fclose(f)!=0;
}
int main(int argc,char **argv)
{
 if((argc==3||argc==4)&&strcmp(argv[1],"--record")==0){
  char *end=NULL;unsigned long value=strtoul(argv[2],&end,10);if(!end||*end||value>=GRIDS_PACK_COUNT)return 2;
  const GridsPuzzle *p=grids_record((uint32_t)value);if(!p)return 2;
  unsigned id=p->id,difficulty=p->difficulty;expanded=true;
  if(argc==4){if(strcmp(argv[3],"--free")||id!=19)return 2;capture_mode=1;}
  return capture(id,difficulty,(unsigned)value,true);
 }else if(argc==2&&strcmp(argv[1],"--master")==0){
  /* Highest audit-search count in each family, plus widest product target. */
  const unsigned ids[5]={11,12,14,15,12},puzzles[5]={903,937,947,968,928};
  for(unsigned i=0;i<5;i++)if(capture(ids[i],3,puzzles[i],true))return 1;
 }else if(argc==1){for(unsigned id=11;id<=20;id++)if(capture(id,2,0,false))return 1;}
 else return 2;
 return 0;
}
