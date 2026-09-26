#include "support.h"
#include "test_guesscalc_workflow.h"
#include "boards.h"
#include "test_boards_fixtures.h"
#include <sys/stat.h>
#include <errno.h>
static uint16_t pixels[396*224];
static TestDisk disk;static NgApp app,fixture;static FILE *manifest;
static void paint(void *ctx,int x,int y,int w,int h,uint16_t color)
{(void)ctx;for(int r=y;r<y+h;r++)for(int col=x;col<x+w;col++)pixels[r*396+col]=color;}
static void capture(const char *name)
{
 fprintf(manifest,"%s.ppm\n",name);NgCanvas c={NULL,paint};ng_render(&app,&c);char path[256];snprintf(path,sizeof(path),"captures/%s.ppm",name);
 FILE *f=fopen(path,"wb");assert(f);fprintf(f,"P6\n396 224\n255\n");
 for(unsigned i=0;i<396*224;i++){unsigned p=pixels[i];unsigned char rgb[3]={(unsigned char)(((p>>11)&31)*255/31),(unsigned char)(((p>>5)&63)*255/63),(unsigned char)((p&31)*255/31)};assert(fwrite(rgb,1,3,f)==3);}assert(!fclose(f));
}
static void main_screen(void){while(app.modal)tap(&app,NGK_EXIT);while(app.screen!=NG_MAIN)tap(&app,NGK_EXIT);}
static void press_capture(void *ctx,int key){tap(ctx,key);}
static void solve_board(void)
{
 NgGame *g=&app.session.game;unsigned n=g->rows,index=g->puzzle_id;
 if(g->id==31){const uint8_t (*r)[4]=boards_test_rects[index];for(unsigned i=1;i<=r[0][0];i++){
  unsigned corners[2]={r[i][0]*n+r[i][1],(r[i][0]+r[i][2]-1)*n+r[i][1]+r[i][3]-1};
  for(unsigned k=0;k<2;k++){while(g->cursor/n!=corners[k]/n)tap(&app,NGK_DOWN);while(g->cursor%n!=corners[k]%n)tap(&app,NGK_RIGHT);tap(&app,NGK_EXE);}
 }}else for(unsigned e=0;e<nb_edge_count(n);e++)if(boards_test_edges[index][e]){
  unsigned split=n*(n+1),base=e>=split?split:0,cols=e>=split?n+1:n;if((g->cursor>=split)!=(e>=split))tap(&app,NGK_F4);
  while((g->cursor-base)/cols!=(e-base)/cols)tap(&app,NGK_DOWN);while((g->cursor-base)%cols!=(e-base)%cols)tap(&app,NGK_RIGHT);tap(&app,NGK_EXE);
 }
 assert(g->status==NG_WON&&app.modal==NG_MODAL_RESULT);
}
int main(void)
{
 if(mkdir("captures",0755)<0)assert(errno==EEXIST);
 manifest=fopen("captures/manifest.txt","w");assert(manifest);
 disk.write_budget=-1;ng_app_init(&app,test_hooks(&disk),76543);capture("00-main");
 for(unsigned category=0;category<6;category++){tap(&app,'1'+(int)category);char name[32];snprintf(name,sizeof(name),"00-category-%u",category+1);capture(name);tap(&app,NGK_EXIT);}
 for(unsigned index=0;index<NG_GAME_COUNT;index++){
  unsigned id=ng_visible_id(index);
  open_game(&app,id);if(id==1){gc_test_type(&app,press_capture,"0123");tap(&app,NGK_EXE);}
  if(id==2){gc_test_type(&app,press_capture,"12+7=19");tap(&app,NGK_EXE);}
  if(id==6){gc_test_type(&app,press_capture,"(3+8)/");}
  if(id==11){tap(&app,NGK_F4);tap(&app,'1');tap(&app,'3');tap(&app,'9');}
  if(id>=21 && id<=23){tap(&app,'1');tap(&app,NGK_EXE);}
  if(id==26){tap(&app,NGK_LEFT);tap(&app,NGK_DOWN);}
  if(id==29)tap(&app,'2');
  char name[40];snprintf(name,sizeof(name),"%02u-play",id);capture(name);
 }
 open_game(&app,1);tap(&app,NGK_EXIT);tap(&app,NGK_EXIT);
 assert(app.screen==NG_CATEGORY && app.resumable);capture("01-tile-resume-badge");
 main_screen();unsigned crypt_index=(unsigned)ng_catalog_index(34);
 tap(&app,'1'+(int)(crypt_index/6));
 while(app.selection!=crypt_index%6)tap(&app,NGK_RIGHT);
 capture("34-cryptarithm-icon");tap(&app,NGK_EXE);tap(&app,NGK_F6);
 capture("34-cryptarithm-aligned-layout");
 open_game(&app,1);gc_test_solve(&app.session.game,&app,press_capture);
 capture("31-result-win");capture("31-modal-view-result");
 tap(&app,NGK_EXIT);capture("32-frozen-result-f6-new");
 tap(&app,NGK_EXIT);capture("32-completed-entry-no-resume");
 open_game(&app,1);tap(&app,NGK_EXIT);capture("32-same-game-entry-resume-new");
 main_screen();capture("32-main-f1-resume");
 tap(&app,'1');tap(&app,'2');capture("32-different-game-entry-no-resume");
 tap(&app,NGK_F6);
 tap(&app,'0');tap(&app,NGK_F6);capture("33-input-error");tap(&app,NGK_F1);capture("34-init-confirm");tap(&app,NGK_EXIT);
 tap(&app,NGK_F5);capture("35-rules");tap(&app,NGK_EXIT);main_screen();tap(&app,NGK_F1);capture("36-main-no-stats");
 open_game(&app,31);tap(&app,NGK_EXE);tap(&app,NGK_RIGHT);tap(&app,NGK_DOWN);capture("37-shikaku-corner");tap(&app,NGK_EXIT);
 app.settings.difficulty[31]=2;open_game(&app,32);app.session.game.cursor=143;tap(&app,NGK_DEL);capture("38-slitherlink-edge");
 open_game(&app,26);fixture=app;for(unsigned i=0;i<16;i++)app.session.game.board[i]=(int16_t)(i<12?i+1:0);app.session.game.data[0]=12;app.session.game.data[1]=1;capture("39-2048-large");app=fixture;
 for(unsigned id=11;id<=20;id++){app.settings.difficulty[id-1]=2;open_game(&app,id);char name[40];snprintf(name,sizeof(name),"%02u-hard",id);capture(name);}
 main_screen();tap(&app,NGK_F2);capture("40-settings");
 /* Entry UI cases use real key dispatch. Later maximum-width cases are
    explicitly labelled renderer fixtures, not earned scores or played runs. */
 ng_app_init(&app,(NgHooks){0},8765);tap(&app,'6');tap(&app,'1');capture("42-entry-2048-new");
 tap(&app,NGK_F6);tap(&app,NGK_EXIT);capture("47-entry-2048-resume");
 main_screen();tap(&app,'3');tap(&app,'1');capture("43-entry-single-mode");
 main_screen();app.settings.difficulty[10]=3;tap(&app,'3');tap(&app,'1');app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_LEVEL);capture("44-entry-master");
 main_screen();app.settings.mode[23]=1;tap(&app,'5');tap(&app,'4');tap(&app,NGK_DOWN);capture("45-entry-cpu-first");
 main_screen();tap(&app,'1');tap(&app,'1');tap(&app,NGK_DOWN);capture("46-entry-difficulty");
 main_screen();tap(&app,NGK_F1);capture("48-main-six-categories");
 open_game(&app,26);fixture=app;tap(&app,NGK_EXIT);tap(&app,NGK_F4);capture("49-entry-resume-no-records");app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_RESUME);tap(&app,NGK_F6);
 for(unsigned i=0;i<16;i++)app.session.game.board[i]=(int16_t)(i+15);app.session.game.data[0]=30;app.session.game.score=UINT32_MAX;capture("50-max-2048");app=fixture;
 open_game(&app,10);fixture=app;app.modal=NG_MODAL_RESULT;app.session.game.status=NG_WON;app.session.game.moves=UINT32_MAX;app.session.game.score=UINT32_MAX;
 strcpy(app.session.game.message,"A long result sentence with every important detail kept inside the result panel, including the final answer: 1234567890.");capture("51-long-result");app=fixture;
 open_game(&app,12);fixture=app;for(unsigned seed=1;seed<10000;seed++){ng_new(&app.session.game,12,2,0,seed,1);if(app.session.game.puzzle_id==166)break;}assert(app.session.game.puzzle_id==166);capture("52-cage-clue");app=fixture;
 open_game(&app,6);fixture=app;memset(app.session.game.input,'8',96);app.session.game.input[96]=0;capture("53-long-input");app=fixture;
 for(unsigned i=0;i<NG_GAME_COUNT;i++){
  unsigned id=ng_visible_id(i);app.settings.difficulty[id-1]=NG_MASTER;if(id==26)app.settings.mode[id-1]=1;
  open_game(&app,id);char name[40];snprintf(name,sizeof name,"%02u-master",id);capture(name);
  tap(&app,NGK_EXIT);app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_LEVEL);snprintf(name,sizeof name,"%02u-master-entry",id);capture(name);
  if(ng_has_hell(id)){
   tap(&app,NGK_F3);assert(app.settings.difficulty[id-1]==NG_HELL);snprintf(name,sizeof name,"%02u-hell-entry",id);capture(name);
   tap(&app,NGK_F6);if(app.modal==NG_MODAL_NEW)tap(&app,NGK_EXE);assert(app.session.game.difficulty==NG_HELL);
   snprintf(name,sizeof name,"%02u-hell",id);capture(name);
  }
 }
 for(unsigned id=31;id<=32;id++)for(unsigned d=0;d<4;d++){app.settings.difficulty[id-1]=(uint8_t)d;open_game(&app,id);char name[40];snprintf(name,sizeof name,"%02u-level-%u",id,d);capture(name);solve_board();snprintf(name,sizeof name,"%02u-level-%u-complete",id,d);capture(name);}
 main_screen();tap(&app,NGK_F2);tap(&app,NGK_F3);
#ifdef NG_DIAGNOSTIC
 capture("54-diagnostics-memory");tap(&app,NGK_F3);capture("54-diagnostics-runtime");
#else
 capture("54-diagnostics");
#endif
 tap(&app,NGK_EXIT);
 open_game(&app,2);gc_test_type(&app,press_capture,"12+7");tap(&app,NGK_SHIFT);tap(&app,'.');gc_test_type(&app,press_capture,"19");capture("55-equation-input");tap(&app,NGK_EXE);capture("56-equation-feedback");
 open_game(&app,6);fixture=app;strcpy(app.session.game.message,"12+3-4*2/1 = 7");capture("57-operator-footer-fixture");app.modal=NG_MODAL_RESULT;app.session.game.status=NG_WON;capture("58-operator-result-fixture");app=fixture;
 open_game(&app,21);capture("59-idle-000");ng_app_tick(&app,60000);capture("60-idle-060");
 memset(&disk,0,sizeof disk);disk.write_budget=-1;ng_app_init(&app,test_hooks(&disk),9654);open_game(&app,31);ng_app_tick(&app,1);assert(ng_checkpoint(&app));ng_app_tick(&app,1);
 disk.write_budget=0;tap(&app,NGK_EXIT);assert(app.modal==NG_MODAL_SAVE_ERROR);capture("61-save-error");disk.write_budget=-1;tap(&app,NGK_EXE);assert(!app.modal);capture("62-save-retry");
 assert(disk.lengths[0][0]&&disk.lengths[0][1]);disk.bytes[0][1][40]^=1;ng_app_init(&app,test_hooks(&disk),123);capture("63-backup-recovered");
 main_screen();tap(&app,'2');tap(&app,'1');app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_TARGET);
 app.notice[0]=0;
 capture("63-target-inline-entry");
 capture("64-target-select");
 tap(&app,NGK_LEFT);capture("65-target-edit-left");tap(&app,NGK_EXIT);
 tap(&app,NGK_RIGHT);capture("66-target-edit-right");tap(&app,NGK_EXIT);
 tap(&app,'0');tap(&app,NGK_EXE);capture("67-target-invalid");tap(&app,NGK_EXIT);
 tap(&app,'2');tap(&app,'4');tap(&app,'7');tap(&app,NGK_EXE);
 assert(app.screen==NG_ENTRY && !app.target_editing && app.settings.target==247);
 capture("68-target-committed");
 /* Revision-4 content at every level, including the 1000-target card widths.
    These frames come from the real entry and game renderer. */
 static const unsigned revised[]={5,6,7,10};
 for(size_t j=0;j<sizeof revised/sizeof revised[0];j++)for(unsigned d=0;d<4;d++){
  unsigned id=revised[j];main_screen();
  app.settings.difficulty[id-1]=(uint8_t)d;
  if(id==6)app.settings.target=1000;
  tap(&app,'1'+(int)(ng_catalog_index(id)/6));
  tap(&app,'1'+(int)(ng_catalog_index(id)%6));
  assert(app.screen==NG_ENTRY);
  app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_LEVEL);
  char name[40];snprintf(name,sizeof name,"%02u-beta4-%u-entry",id,d);capture(name);
  open_game(&app,id);
  snprintf(name,sizeof name,"%02u-beta4-%u-game",id,d);capture(name);
 }
 assert(!fclose(manifest));
 puts("Renderer capture: main +6 categories +36 games +inline options +edge states PASS");return 0;
}
