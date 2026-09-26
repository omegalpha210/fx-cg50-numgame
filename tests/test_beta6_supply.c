#include "support.h"

static NgApp app,cold;
static TestDisk disk;

static void press(NgApp *a,int key)
{
 (void)ng_app_event(a,key,NG_DOWN);
 (void)ng_app_event(a,key,NG_UP);
}
static void begin(NgApp *a,unsigned difficulty,unsigned target)
{
 a->settings.first_help=0;a->settings.target=(uint16_t)target;
 a->settings.difficulty[5]=(uint8_t)difficulty;
 a->selected_id=6;a->screen=NG_ENTRY;
 a->entry_selection=(uint8_t)ng_entry_row(a,NG_ENTRY_NEW);
 press(a,NGK_F6);
 assert(a->screen==NG_PLAY && !a->modal && ng_valid(&a->session.game));
}
static void next(NgApp *a)
{
 assert(a->screen==NG_PLAY);
 press(a,NGK_EXIT);assert(a->screen==NG_ENTRY);
 a->entry_selection=(uint8_t)ng_entry_row(a,NG_ENTRY_NEW);
 press(a,NGK_F6);
 assert(a->screen==NG_PLAY && !a->modal && ng_valid(&a->session.game));
}
static void test_fixed_and_random(void)
{
 ng_app_init(&app,(NgHooks){0},14253);
 begin(&app,NG_HARD,200);
 bool fixed[200]={0};uint32_t last=0;
 for(unsigned i=0;i<201;i++){
  if(i)next(&app);
  const NgGame *g=&app.session.game;
  assert(g->data[0]==200 && g->data[2]==0 && g->data[4]==200);
  assert(app.session.supply[0][NG_HARD].count==200);
  unsigned slot=g->puzzle_id-(16240u+NG_HARD*1000u+4u*200u);
  assert(slot<200);
  if(i<200){assert(!fixed[slot]);fixed[slot]=true;
   assert(app.session.supply[0][NG_HARD].next==i+1);
  }else assert(g->puzzle_id!=last && app.session.supply[0][NG_HARD].next==1);
  last=g->puzzle_id;
 }
 ng_app_init(&app,(NgHooks){0},93251);
 begin(&app,NG_NORMAL,NG_TARGET_RANDOM);
 bool seen[1000]={0};unsigned counts[5]={0};last=0;
 for(unsigned i=0;i<1001;i++){
  if(i)next(&app);
  const NgGame *g=&app.session.game;
  assert(g->data[2]==1 && g->data[4]==0);
  unsigned slot=g->puzzle_id-(16240u+NG_NORMAL*1000u);
  assert(slot<1000 && app.session.supply[0][NG_NORMAL].count==1000);
  static const unsigned targets[]={10,24,50,100,200};
  assert(g->data[0]==(int32_t)targets[slot/200]);
  for(unsigned c=0;c<(unsigned)g->data[1];c++)assert(g->board[c]>=1 && g->board[c]<=999);
  if(i<1000){assert(!seen[slot]);seen[slot]=true;counts[slot/200]++;
   assert(app.session.supply[0][NG_NORMAL].next==i+1);
  }else assert(g->puzzle_id!=last && app.session.supply[0][NG_NORMAL].next==1);
  if(i==255 || i==256 || i==999)assert(g->supply_index==i);
  if(i==999){cold=app;cold.seed=UINT32_C(90909091);}
  if(i==1000){next(&cold);assert(cold.session.game.puzzle_id==g->puzzle_id);
   assert(cold.session.game.seed==g->seed);}
  last=g->puzzle_id;
 }
 for(unsigned i=0;i<5;i++)assert(counts[i]==200);
}
static void test_resume_and_result_new(void)
{
 memset(&disk,0,sizeof disk);disk.write_budget=-1;
 ng_app_init(&app,test_hooks(&disk),17);
 app.settings.first_help=0;app.settings.target=NG_TARGET_RANDOM;
 app.settings.difficulty[5]=NG_EASY;open_game(&app,6);
 for(unsigned i=0;i<257;i++)next(&app);
 NgGame saved=app.session.game;NgSupply supply=app.session.supply[0][NG_EASY];
 assert(saved.supply_index==257 && supply.next==258);
 ng_app_init(&cold,test_hooks(&disk),9000001);
 assert(cold.resumable && !memcmp(&cold.session.game,&saved,sizeof saved));
 assert(!memcmp(&cold.session.supply[0][NG_EASY],&supply,sizeof supply));
 press(&cold,NGK_F1);
 assert(cold.screen==NG_PLAY && !memcmp(&cold.session.game,&saved,sizeof saved));
 next(&app);next(&cold);
 assert(app.session.game.puzzle_id==cold.session.game.puzzle_id);
 assert(app.session.game.seed==cold.session.game.seed);
 assert(!memcmp(&app.session.supply[0][NG_EASY],&cold.session.supply[0][NG_EASY],sizeof supply));
 uint32_t before=cold.session.game.puzzle_id;
 press(&cold,NGK_F4);press(&cold,NGK_F6);
 assert(cold.modal==NG_MODAL_RESULT && cold.session.game.status==NG_WON);
 press(&cold,NGK_F6);
 assert(cold.screen==NG_PLAY && cold.session.game.data[2]==1 && cold.session.game.data[4]==0);
 assert(cold.session.game.puzzle_id!=before && cold.session.supply[0][NG_EASY].next==260);
}
static void test_countdown_and_prime_cycles(void)
{
 static const struct {unsigned id,difficulty,count,first;} cases[]={
  {7,NG_HARD,200,200+NG_HARD*200},
  {10,NG_EASY,128,0},
  {10,NG_MASTER,256,0},
 };
 for(unsigned c=0;c<sizeof cases/sizeof cases[0];c++){
  const unsigned id=cases[c].id,d=cases[c].difficulty,n=cases[c].count;
  bool seen[256]={0};uint32_t last=UINT32_MAX;
  ng_app_init(&app,(NgHooks){0},(uint32_t)(38481+c));
  app.settings.first_help=0;app.settings.difficulty[id-1]=(uint8_t)d;
  app.selected_id=(uint8_t)id;app.screen=NG_ENTRY;
  app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_NEW);
  press(&app,NGK_F6);
  assert(app.screen==NG_PLAY && ng_valid(&app.session.game));
  for(unsigned i=0;i<=n;i++){
   if(i)next(&app);
   const NgGame *g=&app.session.game;
   assert(g->id==id && g->difficulty==d);
   assert(app.session.supply[0][d].count==n);
   assert(g->puzzle_id>=cases[c].first);
   unsigned slot=g->puzzle_id-cases[c].first;
   assert(slot<n);
   if(i<n){assert(!seen[slot]);seen[slot]=true;}
   else assert(g->puzzle_id!=last);
   last=g->puzzle_id;
  }
 }
}
static void test_countdown_cold_resume(void)
{
 memset(&disk,0,sizeof disk);disk.write_budget=-1;
 ng_app_init(&app,test_hooks(&disk),88123);
 app.settings.first_help=0;app.settings.difficulty[6]=NG_MASTER;
 app.selected_id=7;app.screen=NG_ENTRY;
 app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_NEW);
 press(&app,NGK_F6);
 assert(app.screen==NG_PLAY && ng_valid(&app.session.game));
 for(unsigned i=0;i<37;i++)next(&app);
 NgGame saved=app.session.game;NgSupply supply=app.session.supply[0][NG_MASTER];
 ng_app_init(&cold,test_hooks(&disk),91923);
 assert(cold.resumable && !memcmp(&cold.session.game,&saved,sizeof saved));
 assert(!memcmp(&cold.session.supply[0][NG_MASTER],&supply,sizeof supply));
 press(&cold,NGK_F1);
 assert(cold.screen==NG_PLAY && !memcmp(&cold.session.game,&saved,sizeof saved));
 next(&cold);next(&app);
 assert(cold.session.game.puzzle_id==app.session.game.puzzle_id);
 assert(!memcmp(&cold.session.supply[0][NG_MASTER],&app.session.supply[0][NG_MASTER],sizeof supply));
}
int main(void)
{
 test_fixed_and_random();test_resume_and_result_new();test_countdown_and_prime_cycles();
 test_countdown_cold_resume();
 puts("beta.6 target/countdown/prime cycles, boundaries, cold RESUME and result NEW PASS");
 return 0;
}
