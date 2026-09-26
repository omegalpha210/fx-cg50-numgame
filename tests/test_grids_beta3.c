#include "support.h"
#include "grids_internal.h"
#include "grids_extra.h"

static TestDisk disk;
static NgApp app,cold;
static NgSession decoded;
static uint8_t bytes[NG_RECORD_MAX];
static uint16_t image[396*224];
static const char *capture_directory;

static void image_rect(void *context,int x,int y,int w,int h,uint16_t color)
{
 (void)context;assert(x>=0&&y>=0&&w>0&&h>0&&x+w<=396&&y+h<=224);
 for(int r=y;r<y+h;r++)for(int c=x;c<x+w;c++)image[r*396+c]=color;
}
static void capture(const char *name)
{
 if(!capture_directory)return;
 NgCanvas canvas={NULL,image_rect};ng_render(&app,&canvas);
 char path[512];snprintf(path,sizeof path,"%s/%s.ppm",capture_directory,name);FILE *f=fopen(path,"wb");assert(f);
 fprintf(f,"P6\n396 224\n255\n");
 for(unsigned i=0;i<396*224;i++){
  unsigned p=image[i];unsigned char rgb[3]={(unsigned char)(((p>>11)&31)*255/31),(unsigned char)(((p>>5)&63)*255/63),(unsigned char)((p&31)*255/31)};
  assert(fwrite(rgb,1,3,f)==3);
 }
 assert(!fclose(f));
}
static void codec(const NgSession *session)
{
 size_t n=ng_single_encode(session,bytes,sizeof bytes);assert(n&&ng_single_decode(&decoded,bytes,n,session->game.id));
 assert(!memcmp(&session->game,&decoded.game,sizeof session->game));
}
static void nonogram_persistence_and_hold(void)
{
 memset(&disk,0,sizeof disk);disk.write_budget=-1;ng_app_init(&app,test_hooks(&disk),36003);open_game(&app,36);
 int16_t truth[81];assert(grids_extra_witness(36,app.session.game.puzzle_id,truth));
 unsigned nn=(unsigned)app.session.game.rows*app.session.game.cols,empty=nn,black=nn;
 for(unsigned i=0;i<nn;i++){if(truth[i])black=i;else empty=i;}assert(empty<nn&&black<nn);
 app.session.game.cursor=(uint8_t)empty;tap(&app,'0');assert(app.session.game.board[empty]==0);
 assert(app.session.game.board[black]==-1);codec(&app.session);
 NgGame before=app.session.game;tap(&app,NGK_EXIT);ng_app_init(&cold,test_hooks(&disk),912);assert(cold.resumable);tap(&cold,NGK_F1);
 assert(!memcmp(&before,&cold.session.game,sizeof before));app=cold;
 /* G: a real cold storage load retains both X=0 and unknown=-1. */
 assert(app.session.game.board[empty]==0&&app.session.game.board[black]==-1);
 for(unsigned i=0;i<nn;i++)if(truth[i]&&i!=black){app.session.game.cursor=(uint8_t)i;tap(&app,'1');}
 assert(!app.session.game.status&&!app.modal);capture("nonogram-mixed-before-last");
 app.session.game.cursor=(uint8_t)empty;tap(&app,NGK_DEL);assert(app.session.game.board[empty]==-1);
 app.session.game.cursor=(uint8_t)black;
 capture("nonogram-before-last");uint32_t run=app.session.game.run_id;
 ng_app_event(&app,NGK_EXE,NG_DOWN);assert(app.session.game.status==NG_WON&&app.modal==NG_MODAL_RESULT&&!app.resumable);
 NgGame won=app.session.game;capture("nonogram-blank-complete");
 /* H: holding the completing EXE cannot dismiss RESULT or start NEW. */
 for(unsigned i=0;i<50;i++)ng_app_event(&app,NGK_EXE,NG_HOLD);
 assert(app.modal==NG_MODAL_RESULT&&app.session.game.run_id==run&&!memcmp(&won,&app.session.game,sizeof won));
 ng_app_event(&app,NGK_EXE,NG_UP);tap(&app,NGK_EXIT);assert(!app.modal&&app.result_view);capture("nonogram-result-view");
 for(unsigned i=0;i<50;i++)ng_app_event(&app,NGK_EXE,NG_HOLD);
 assert(app.session.game.run_id==run&&!memcmp(&won,&app.session.game,sizeof won));
 tap(&app,NGK_F6);assert(app.session.game.run_id!=run&&app.session.game.status==NG_PLAYING);
 puts("NONOGRAM G/H: cold X/blank resume, last EXE auto-completion, HOLD barrier and explicit F6 NEW PASS");
}
static void magic_persistence(void)
{
 for(unsigned d=0;d<4;d++){
  memset(&disk,0,sizeof disk);disk.write_budget=-1;ng_app_init(&app,test_hooks(&disk),19003+d);
  app.settings.difficulty[18]=(uint8_t)d;app.settings.mode[18]=1;open_game(&app,19);
  NgGame *g=&app.session.game;unsigned n=3+d,nn=n*n;
  assert(g->pack_revision==3&&g->generation_policy==NG_SUPPLY_RULES&&g->rows==n&&g->cols==n&&g->puzzle_id==GRIDS_FREE_MAGIC_ID+d);
  char label[48];snprintf(label,sizeof label,"magic-free-%u-blank",n);capture(label);
  GridsPuzzle witness;assert(grids_decode(g->puzzle_id,&witness));
  for(unsigned i=0;i<nn;i++){
   g->cursor=(uint8_t)i;unsigned value=witness.solution[i];
   if(value>=10)tap(&app,'0'+(int)(value/10));
   tap(&app,'0'+(int)(value%10));tap(&app,NGK_EXE);
   assert(!g->status&&g->board[i]==(int)value);
  }
  assert(grids_complete(g));g->cursor=(uint8_t)(nn-1);codec(&app.session);
  snprintf(label,sizeof label,"magic-free-%u-filled",n);capture(label);
  NgGame original=*g;tap(&app,NGK_EXIT);ng_app_init(&cold,test_hooks(&disk),13);assert(cold.resumable);tap(&cold,NGK_F1);
  assert(!memcmp(&original,&cold.session.game,sizeof original));app=cold;
  tap(&app,NGK_F1);assert(app.modal==NG_MODAL_INIT);tap(&app,NGK_EXE);
  assert(app.session.game.rows==n&&app.session.game.cols==n&&app.session.game.puzzle_id==original.puzzle_id&&app.session.game.run_id==original.run_id);
  for(unsigned i=0;i<81;i++)assert(!app.session.game.board[i]);
 }
 for(unsigned revision=1;revision<=2;revision++)for(unsigned d=0;d<(revision==1?3u:4u);d++){
  memset(&disk,0,sizeof disk);disk.write_budget=-1;ng_app_init(&app,test_hooks(&disk),221);
  for(unsigned seed=1;seed<10000;seed++){
   ng_new_supply_version(&app.session.game,19,d,1,seed,14,0,0,revision);
   if(revision==2||app.session.game.puzzle_id<980)break;
  }
  app.session.stats.started=1;NgGame *g=&app.session.game;assert(ng_valid(g));assert(g->rows==(d>=2?4:3)&&g->generation_policy==NG_SUPPLY_TRANSFORMS);
  g->board[0]=7;g->cursor=1;strcpy(g->input,"12");codec(&app.session);
  app.active=app.resumable=app.dirty=true;app.screen=NG_PLAY;app.selected_id=19;app.settings.last_game=19;app.settings.difficulty[18]=(uint8_t)d;app.settings.mode[18]=1;
  assert(ng_checkpoint(&app));ng_app_init(&cold,test_hooks(&disk),43);assert(cold.resumable);tap(&cold,NGK_F1);
  assert(cold.session.game.rows==g->rows&&cold.session.game.board[0]==7&&!strcmp(cold.session.game.input,"12"));
  app=cold;uint32_t puzzle=app.session.game.puzzle_id;unsigned n=app.session.game.rows;
  tap(&app,NGK_F1);tap(&app,NGK_EXE);assert(app.session.game.rows==n&&app.session.game.puzzle_id==puzzle&&app.session.game.pack_revision==revision);
  assert(!app.session.game.board[0]&&!app.session.game.input[0]&&ng_valid(&app.session.game));
  /* Changing the selected level changes NEW only; RESUME retains old order. */
  tap(&app,NGK_EXIT);app.settings.difficulty[18]=(uint8_t)((d+1)%4);app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_NEW);tap(&app,NGK_F6);
  assert(app.session.game.rows==3+(d+1)%4&&app.session.game.pack_revision==3);
 }
 puts("MAGIC FREE: all four orders, codec/cold resume/INIT, 36, rev1/2 original order and NEW separation PASS");
}
int main(int argc,char **argv)
{
 if(argc==3&&!strcmp(argv[1],"--capture"))capture_directory=argv[2];
 nonogram_persistence_and_hold();magic_persistence();return 0;
}
