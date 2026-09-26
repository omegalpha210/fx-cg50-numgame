#include "support.h"
#include "legacy_wire.h"
#include "diagnostics.h"
static NgApp app;
static TestDisk disk;
static NgSession original,cold;
static uint8_t bytes[NG_RECORD_MAX],old[NG_RECORD_MAX];
static void setup(unsigned id,unsigned d,unsigned mode)
{
 ng_app_init(&app,(NgHooks){0},UINT32_C(0xffffffff));
 app.selected_id=(uint8_t)id;app.screen=NG_ENTRY;
 app.settings.difficulty[id-1]=(uint8_t)d;app.settings.mode[id-1]=(uint8_t)mode;
 app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_NEW);
}
static void open_next(void)
{
 if(app.screen==NG_PLAY)tap(&app,NGK_EXIT);
 app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_NEW);tap(&app,NGK_F6);
 if(app.modal==NG_MODAL_NEW)tap(&app,NGK_EXE);
 assert(app.screen==NG_PLAY && !app.modal && ng_valid(&app.session.game));
}
static void selectors(void)
{
 for(unsigned id=11;id<=35;id++){
  if(!ng_has_hell(id))continue;
  for(unsigned previous=0;previous<4;previous++){
  setup(id,previous,0);tap(&app,NGK_F3);assert(app.settings.difficulty[id-1]==previous);
  app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_LEVEL);tap(&app,NGK_F3);
  assert(app.settings.difficulty[id-1]==NG_HELL);tap(&app,NGK_RIGHT);assert(app.settings.difficulty[id-1]==NG_HELL);
  tap(&app,NGK_F3);assert(app.settings.difficulty[id-1]==previous);
  tap(&app,NGK_F3);tap(&app,NGK_LEFT);assert(app.settings.difficulty[id-1]==NG_MASTER);
  tap(&app,NGK_RIGHT);assert(app.settings.difficulty[id-1]==NG_MASTER);
  tap(&app,NGK_F3);tap(&app,NGK_UP);assert(app.settings.difficulty[id-1]==NG_HELL);
  tap(&app,NGK_F6);assert(app.screen==NG_PLAY && app.session.game.difficulty==NG_HELL);
  NgGame saved=app.session.game;tap(&app,NGK_EXIT);
  app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_LEVEL);tap(&app,NGK_LEFT);
  assert(!memcmp(&saved,&app.session.game,sizeof saved));
  app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_RESUME);tap(&app,NGK_F6);
  assert(app.screen==NG_PLAY && !memcmp(&saved,&app.session.game,sizeof saved));
  }
 }
 for(unsigned i=0;i<NG_GAME_COUNT;i++){
  unsigned id=ng_visible_id(i);const NgModule *m=ng_module(id);if(m->modes<2)continue;
  setup(id,1,0);app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_MODE);tap(&app,NGK_F3);
  assert(!app.modal && !app.settings.mode[id-1]);
  for(unsigned n=0;n<10;n++)tap(&app,NGK_RIGHT);assert(app.settings.mode[id-1]==m->modes-1);
  tap(&app,NGK_F6);assert(app.screen==NG_PLAY && app.session.game.mode==m->modes-1);
 }
 setup(26,NG_MASTER,0);assert(!ng_entry_level(&app));open_next();assert(app.session.game.difficulty==NG_NORMAL && app.settings.difficulty[25]==NG_MASTER);
 puts("MASTER/HELL values, F3 return label, inline modes, explicit resume and fixed CLASSIC PASS");
}
static void bank_cycles(void)
{
 unsigned groups=0,runs=0;
 for(unsigned i=0;i<NG_GAME_COUNT;i++){
  unsigned id=ng_visible_id(i);for(unsigned d=0;d<ng_difficulty_count(id);d++)for(unsigned mode=0;mode<ng_module(id)->modes;mode++){
   unsigned n=ng_bank_count(id,d,mode);if(!n)continue;assert(n<=4096);setup(id,d,mode);
   uint32_t seen[4096],previous=UINT32_MAX;
   for(unsigned cycle=0;cycle<2;cycle++)for(unsigned k=0;k<n;k++){
    open_next();uint32_t puzzle=app.session.game.puzzle_id;
    for(unsigned j=0;j<k;j++)assert(seen[j]!=puzzle);seen[k]=puzzle;
    if(n>1)assert(previous!=puzzle);previous=puzzle;runs++;
    if(k==n/2){size_t length=ng_encode(&app.session,bytes,sizeof bytes);assert(length && ng_decode(&cold,bytes,length,id));app.session=cold;}
   }
   /* Restart must keep this precise puzzle and leave the shuffle cycle alone. */
   NgSupply supply=app.session.supply[mode][d];uint32_t puzzle=app.session.game.puzzle_id;
   tap(&app,NGK_F1);if(app.modal==NG_MODAL_INIT)tap(&app,NGK_EXE);
   assert(app.session.game.puzzle_id==puzzle && !memcmp(&supply,&app.session.supply[mode][d],sizeof supply));groups++;
  }
 }
 /* Coprime permutation including previous low16-bit rollover trouble. */
 for(unsigned n=1;n<=100;n++)for(unsigned seed=65530;seed<=65540;seed++){
  bool seen[100]={0};NgGame g={.supply_seed=seed};
  for(unsigned k=0;k<n;k++){g.supply_index=k;unsigned v=ng_bank_pick(&g,n);assert(v<n && !seen[v]);seen[v]=true;}
 }
 printf("Persistent supply: %u banks, %u app starts, complete cycles/no immediate repeats, mid-cycle codec and INIT PASS\n",groups,runs);
}
static void old_formats(void)
{
 unsigned cases=0;
 for(unsigned id=2;id<=32;id++)for(unsigned d=0;d<3;d++)for(unsigned mode=0;mode<ng_module(id)->modes;mode++){
  if(id==6 || id==29 || id==30 || (id==26 && mode))continue;
  memset(&original,0,sizeof original);
  /* Legacy payloads must use their original supply rules. Revisions 3/4
     may have different banks, grading or virtual puzzle IDs. */
  if(id==5 || id==7 || id==10 || (id==19 && mode==1) || id==27 || id==28)
   ng_new_supply_version(&original.game,id,d,mode,20260922,1,0,0,1);
  else ng_new(&original.game,id,d,mode,20260922,1);
  original.stats.started=1;
  /* An old payload must reference content that actually existed then. */
  if(id>=11 && id<=20 && !(id==19 && mode==1))for(unsigned seed=1;original.game.puzzle_id>=900 && seed<10000;seed++)ng_new(&original.game,id,d,mode,seed,1);
  if(id>=11 && id<=20)assert(original.game.puzzle_id<900);
  if(!original.game.cpu_pending && (ng_module(id)->flags&NGF_UNDO)){original.undo_count=4;for(unsigned j=0;j<4;j++)original.undo[j]=original.game;}
  size_t n=ng_encode(&original,bytes,sizeof bytes);assert(n);
  for(unsigned levels=3;levels<=4;levels++){
   size_t len=legacy_wire(old,bytes,n,levels);
   if(!ng_decode(&cold,old,len,id))fprintf(stderr,"Legacy decode failed: id=%u level=%u mode=%u wire_levels=%u revision=%lu puzzle=%lu\n",id,d,mode,levels,(unsigned long)original.game.pack_revision,(unsigned long)original.game.puzzle_id);
   assert(ng_decode(&cold,old,len,id));
   assert(cold.game.pack_revision==1 && !cold.game.supply_seed && cold.undo_count==original.undo_count);
   NgGame expected=original.game;expected.pack_revision=1;assert(!memcmp(&expected,&cold.game,sizeof expected));
   assert(!cold.stats.best[0][NG_HELL][0].completed);
   size_t modern=ng_encode(&cold,bytes,sizeof bytes);assert(modern && ng_decode(&cold,bytes,modern,id));
   n=ng_encode(&original,bytes,sizeof bytes);assert(n);cases++;
  }
 }
 const unsigned old_master_ids[]={11,12,14,15};
 for(unsigned i=0;i<4;i++){
  unsigned id=old_master_ids[i];memset(&original,0,sizeof original);
  for(unsigned seed=1;seed<10000;seed++){ng_new(&original.game,id,NG_MASTER,0,seed,1);if(original.game.puzzle_id<980)break;}
  assert(original.game.puzzle_id>=900 && original.game.puzzle_id<980);original.stats.started=1;
  size_t n=ng_encode(&original,bytes,sizeof bytes),len=legacy_wire(old,bytes,n,4);assert(ng_decode(&cold,old,len,id));
  assert(cold.game.difficulty==NG_MASTER && cold.game.pack_revision==1 && cold.game.puzzle_id==original.game.puzzle_id);
  setup(id,NG_HELL,0);app.session=cold;app.active=true;app.screen=NG_PLAY;uint32_t puzzle=cold.game.puzzle_id;
  tap(&app,NGK_F1);if(app.modal==NG_MODAL_INIT)tap(&app,NGK_EXE);
  assert(app.session.game.difficulty==NG_MASTER && app.session.game.pack_revision==1 && app.session.game.puzzle_id==puzzle && ng_valid(&app.session.game));
 }
 for(unsigned id=11;id<=15;id++)for(unsigned d=NG_MASTER;d<=NG_HELL;d++){
  memset(&original,0,sizeof original);ng_new(&original.game,id,d,0,5678,1);original.stats.started=1;
  original.stats.best[0][d][0]=(NgBest){.completed=1,.wins=1,.best_score=123,.best_moves=55,.best_ms=789};
  size_t n=ng_encode(&original,bytes,sizeof bytes);assert(n && ng_decode(&cold,bytes,n,id));
  assert(cold.stats.started==1 && !cold.stats.best[0][d][0].wins);
 }
 printf("Save migration: %u old3/4-level views, current+4undos and compact run/supply roundtrip PASS\n",cases);
}
int main(void)
{
 disk.write_budget=-1;ng_app_init(&app,test_hooks(&disk),13);open_game(&app,21);
 selectors();bank_cycles();old_formats();puts("Mastery common integration PASS");return 0;
}
