/* Compare beta.6 app-level fixed/RANDOM sequences and v5 resume bytes. */
#include "app.h"
#include <assert.h>
#include <stdio.h>

static NgApp app;
static uint8_t wire[NG_RECORD_MAX];
static void press(int key)
{
 (void)ng_app_event(&app,key,NG_DOWN);
 (void)ng_app_event(&app,key,NG_UP);
}
static void begin(unsigned difficulty,unsigned target)
{
 ng_app_init(&app,(NgHooks){0},14253u+difficulty*100u+target);
 app.settings.first_help=0;app.settings.target=(uint16_t)target;
 app.settings.difficulty[5]=(uint8_t)difficulty;
 app.selected_id=6;app.screen=NG_ENTRY;
 app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_NEW);
 press(NGK_F6);
 assert(app.screen==NG_PLAY&&!app.modal&&ng_valid(&app.session.game));
}
static void next(void)
{
 press(NGK_EXIT);assert(app.screen==NG_ENTRY);
 app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_NEW);
 press(NGK_F6);assert(app.screen==NG_PLAY&&!app.modal&&ng_valid(&app.session.game));
}
static void checkpoint(const char *prefix,const char *kind)
{
 char path[512];int n=snprintf(path,sizeof path,"%s.%s",prefix,kind);
 assert(n>0&&(size_t)n<sizeof path);
 size_t length=ng_single_encode(&app.session,wire,sizeof wire);assert(length);
 FILE *file=fopen(path,"wb");assert(file);
 assert(fwrite(wire,1,length,file)==length);assert(!fclose(file));
}
static void scenario(const char *prefix,unsigned difficulty,unsigned target,unsigned count,const char *kind)
{
 begin(difficulty,target);
 for(unsigned i=0;i<count;i++){
  if(i)next();
  const NgGame *g=&app.session.game;
  printf("%s %u %u %d %u",kind,i,g->puzzle_id,(int)g->data[0],g->supply_index);
  for(unsigned c=0;c<(unsigned)g->data[1];c++)printf(" %d",(int)g->board[c]);
  putchar('\n');
  if(i==count/2)checkpoint(prefix,kind);
 }
}
int main(int argc,char **argv)
{
 assert(argc==2);
 scenario(argv[1],NG_HARD,200,200,"fixed");
 scenario(argv[1],NG_NORMAL,NG_TARGET_RANDOM,1000,"random");
 return 0;
}
