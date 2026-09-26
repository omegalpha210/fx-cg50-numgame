#include "ng.h"
#include "app.h"
#include "storage.h"
#include "ui.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static const NgModule *const baseball=&ng_guesscalc[0];
static uint16_t pixels[396u*224u];

static void rect(void *unused,int x,int y,int width,int height,uint16_t color)
{
 (void)unused;
 assert(x>=0 && y>=0 && width>=0 && height>=0 && x+width<=396 && y+height<=224);
 for(int row=y;row<y+height;row++)for(int col=x;col<x+width;col++)pixels[row*396+col]=color;
}
static uint16_t pixel(int x,int y){return pixels[y*396+x];}
static void draw(const NgGame *g)
{
 for(unsigned i=0;i<396u*224u;i++)pixels[i]=NG_WHITE;
 NgCanvas canvas={NULL,rect};baseball->render(g,&canvas);
}

static void press(NgGame *g,int key)
{
 assert(baseball->action(g,key));
 assert(baseball->valid(g));
}

static void type(NgGame *g,const char *digits)
{
 for(unsigned i=0;digits[i];i++)press(g,digits[i]);
 press(g,NGK_EXE);
}

static void start(NgGame *g,unsigned difficulty,unsigned revision)
{
 memset(g,0,sizeof *g);
 g->id=1;g->difficulty=(uint8_t)difficulty;g->pack_revision=revision;
 g->seed=g->rng=171u+difficulty;g->run_id=1;
 baseball->init(g);
 assert(baseball->valid(g));
}

static void secret(NgGame *g,const char *digits)
{
 assert(strlen(digits)==(size_t)g->data[0]);
 for(unsigned i=0;digits[i];i++)g->board[i]=(int16_t)digits[i];
 assert(baseball->valid(g));
}

static void roundtrip(const NgGame *g)
{
 NgSession original={0},loaded={0};uint8_t encoded[4096];
 original.game=*g;original.stats.started=1;
 size_t length=ng_single_encode(&original,encoded,sizeof encoded);
 assert(length==1872u);
 assert(ng_single_decode(&loaded,encoded,length,1));
 assert(!memcmp(&loaded.game,g,sizeof *g));
}

static void check_level(unsigned difficulty)
{
 static const unsigned limits[4]={20,30,40,50};
 NgGame g;start(&g,difficulty,4);
 unsigned n=4u+difficulty,limit=limits[difficulty];
 assert(g.data[0]==(int32_t)n && g.data[1]==(int32_t)limit);
 char wrong[8],answer[8];
 unsigned wrong_value=0;
 for(unsigned i=0;i<n;i++){wrong[i]='1';answer[i]='9';wrong_value=wrong_value*10u+1u;}
 wrong[n]=answer[n]=0;secret(&g,answer);
 /* Incomplete input and erase do not spend an attempt. */
 press(&g,'1');press(&g,NGK_EXE);assert(!g.moves);
 press(&g,NGK_DEL);assert(!g.input[0]);
 for(unsigned attempt=1;attempt<limit;attempt++){
  type(&g,wrong);
  assert(g.moves==attempt && g.score==attempt && g.status==NG_PLAYING);
  assert(g.history_count==0);
  assert(((uint32_t)g.data[1+attempt]&UINT32_C(0x00ffffff))==wrong_value);
  assert(g.phase==(attempt==limit-1?1:0));
  assert(g.scroll==(attempt>5?attempt-5:0));
  if(attempt==1 || attempt==limit-1)roundtrip(&g);
 }
 /* Warning acknowledgement cannot consume the final attempt. */
 NgGame before=g;press(&g,NGK_EXE);
 assert(g.phase==2 && g.moves==limit-1);
 assert(!memcmp(g.data,before.data,sizeof g.data));
 roundtrip(&g);
 /* Reading history leaves a partially entered guess intact. */
 press(&g,'1');press(&g,NGK_UP);assert(g.scroll==limit-7 && !strcmp(g.input,"1"));
 for(unsigned i=0;i<limit;i++)press(&g,NGK_UP);
 assert(g.scroll==0 && !strcmp(g.input,"1"));
 for(unsigned i=0;i<limit;i++)press(&g,NGK_DOWN);
 assert(g.scroll==limit-6 && !strcmp(g.input,"1"));
 press(&g,NGK_DEL);
 type(&g,wrong);
 assert(g.moves==limit && g.status==NG_LOST && g.phase==2);
 assert(((uint32_t)g.data[1+limit]&UINT32_C(0x00ffffff))==wrong_value && g.scroll==limit-5);
 roundtrip(&g);
 assert(!baseball->action(&g,'0') && !baseball->action(&g,NGK_EXE));
 NgGame damaged=g;damaged.data[1+limit]^=UINT32_C(1)<<24;assert(!baseball->valid(&damaged));
 damaged=g;damaged.data[2+limit]=1;assert(!baseball->valid(&damaged));
 damaged=g;damaged.scroll=(uint8_t)(limit-4);assert(!baseball->valid(&damaged));
 damaged=g;damaged.moves++;assert(!baseball->valid(&damaged));

 /* The included final guess can still win. */
 start(&g,difficulty,4);secret(&g,answer);
 for(unsigned attempt=1;attempt<limit;attempt++)type(&g,wrong);
 press(&g,NGK_EXIT);assert(g.phase==2 && g.moves==limit-1);
 type(&g,answer);assert(g.status==NG_WON && g.moves==limit);
 roundtrip(&g);
}

static void check_scrollbar(void)
{
 NgGame g;start(&g,0,4);secret(&g,"9999");
 draw(&g);assert(pixel(375,55)==NG_WHITE);
 for(unsigned i=0;i<5;i++)type(&g,"1111");
 draw(&g);assert(pixel(375,55)==NG_WHITE);
 type(&g,"1111");assert(g.scroll==1);
 draw(&g);assert(pixel(377,45)==NG_BLUE && pixel(377,136)==NG_LINE);
 assert(pixel(375,55)==NG_LINE);
 press(&g,NGK_UP);assert(g.scroll==0);
 draw(&g);assert(pixel(377,45)==NG_LINE && pixel(377,136)==NG_BLUE);
}

typedef struct {
 NgSettings settings;
 NgSession session;
 bool active;
 unsigned writes;
} MemorySave;

static int load_state(void *context,NgSettings *settings,NgSession *session,bool *active)
{
 MemorySave *saved=context;
 if(!saved->writes)return NG_LOAD_ABSENT;
 *settings=saved->settings;*session=saved->session;*active=saved->active;
 return NG_LOAD_OK;
}
static bool save_state(void *context,NgSettings *settings,const NgSession *session,bool active)
{
 MemorySave *saved=context;saved->settings=*settings;saved->session=*session;
 saved->active=active;saved->writes++;return true;
}
static NgHooks hooks(MemorySave *saved)
{
 NgHooks h={0};h.context=saved;h.load_state=load_state;h.save_state=save_state;return h;
}
static void event(NgApp *app,int key)
{(void)ng_app_event(app,key,NG_DOWN);(void)ng_app_event(app,key,NG_UP);}
static void app_type(NgApp *app,const char *digits)
{for(unsigned i=0;digits[i];i++)event(app,digits[i]);event(app,NGK_EXE);}

static void check_last_try_app(void)
{
 MemorySave saved={0};NgApp app,cold,other;
 ng_app_init(&app,hooks(&saved),4711);
 app.selected_id=1;app.screen=NG_ENTRY;app.settings.difficulty[0]=NG_EASY;
 app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_NEW);
 event(&app,NGK_EXE);
 assert(app.screen==NG_PLAY && app.active && app.resumable);
 assert(app.session.game.pack_revision==4 && app.session.game.data[1]==20);
 char answer[8],wrong[8];
 for(unsigned i=0;i<4;i++)answer[i]=(char)app.session.game.board[i];answer[4]=0;
 strcpy(wrong,strcmp(answer,"1111")?"1111":"2222");
 for(unsigned attempt=0;attempt<18;attempt++)app_type(&app,wrong);
 assert(app.session.game.moves==18 && app.modal==NG_MODAL_NONE);
 for(unsigned i=0;i<4;i++)event(&app,wrong[i]);
 assert(ng_app_event(&app,NGK_EXE,NG_DOWN));
 assert(app.modal==NG_MODAL_LAST_TRY && app.session.game.phase==1 &&
  app.session.game.moves==19);
 /* The held submit that opened the dialog must not acknowledge or submit. */
 assert(!ng_app_event(&app,NGK_EXE,NG_HOLD));
 assert(app.modal==NG_MODAL_LAST_TRY && app.session.game.moves==19);
 (void)ng_app_event(&app,NGK_EXE,NG_UP);
 event(&app,NGK_MENU);assert(saved.active && saved.session.game.phase==1);
 MemorySave pending=saved;
 ng_app_init(&cold,hooks(&saved),1729);
 assert(cold.resumable && cold.session.game.moves==19);
 event(&cold,NGK_F1);
 assert(cold.screen==NG_PLAY && cold.modal==NG_MODAL_LAST_TRY);
 assert(cold.session.game.phase==1 && cold.session.game.moves==19);
 assert(!memcmp(cold.session.game.data,app.session.game.data,sizeof app.session.game.data));
 event(&cold,NGK_EXIT);
 assert(cold.modal==NG_MODAL_NONE && cold.session.game.phase==2 &&
  cold.session.game.moves==19);
 event(&cold,NGK_MENU);
 NgApp resumed;ng_app_init(&resumed,hooks(&saved),1730);
 event(&resumed,NGK_F1);
 assert(resumed.screen==NG_PLAY && resumed.modal==NG_MODAL_NONE &&
  resumed.session.game.phase==2 && resumed.session.game.moves==19);
 app_type(&resumed,wrong);
 assert(resumed.session.game.status==NG_LOST && resumed.session.game.moves==20);
 assert(resumed.modal==NG_MODAL_RESULT);
 uint32_t lost_seed=resumed.session.game.seed;
 event(&resumed,NGK_EXIT);
 assert(resumed.screen==NG_PLAY && resumed.result_view && resumed.modal==NG_MODAL_NONE);
 assert(resumed.session.game.moves==20 && resumed.session.game.board[0]==answer[0]);
 event(&resumed,NGK_EXIT);
 assert(resumed.screen==NG_ENTRY && !resumed.active && !resumed.resumable);
 assert(!resumed.session.game.id && !resumed.session.game.moves && !resumed.session.game.board[0]);
 assert(!resumed.before.id && !saved.active && resumed.seed==lost_seed);
 event(&resumed,NGK_F6);
 assert(resumed.screen==NG_PLAY && resumed.session.game.status==NG_PLAYING);
 assert(resumed.session.game.seed!=lost_seed);
 bool same_secret=true;
 for(unsigned i=0;i<4;i++)if(resumed.session.game.board[i]!=answer[i])same_secret=false;
 assert(!same_secret);
 /* F6 acknowledgement also leaves the final try untouched and can win. */
 ng_app_init(&other,hooks(&pending),1731);event(&other,NGK_F1);
 assert(other.modal==NG_MODAL_LAST_TRY);
 event(&other,NGK_F6);
 assert(other.modal==NG_MODAL_NONE && other.session.game.phase==2 &&
  other.session.game.moves==19);
 app_type(&other,answer);
 assert(other.session.game.status==NG_WON && other.session.game.moves==20);
 assert(other.modal==NG_MODAL_RESULT);
 event(&app,NGK_EXE);
 assert(app.modal==NG_MODAL_NONE && app.session.game.phase==2 &&
  app.session.game.moves==19);
}

int main(void)
{
 for(unsigned difficulty=0;difficulty<4;difficulty++)check_level(difficulty);
 check_scrollbar();
 check_last_try_app();
 NgGame g;start(&g,0,4);secret(&g,"0012");
 type(&g,"0000");
 assert(((uint32_t)g.data[2]&UINT32_C(0x00ffffff))==0);
 assert(((uint32_t)g.data[2]>>24&15u)==2);
 assert(((uint32_t)g.data[2]>>28&15u)==0);
 roundtrip(&g);type(&g,"0012");assert(g.status==NG_WON && g.moves==2);
 /* Old active saves retain their original cap and string history. */
 for(unsigned difficulty=0;difficulty<4;difficulty++){
  static const unsigned old_limits[4]={16,12,10,16};
  start(&g,difficulty,3);
  assert(g.data[1]==(int32_t)old_limits[difficulty]);
  assert(g.history_count==0 && g.phase==0);
  unsigned n=4u+difficulty;char answer[8],wrong[8];
  for(unsigned i=0;i<n;i++){answer[i]='9';wrong[i]='1';}
  answer[n]=wrong[n]=0;secret(&g,answer);
  for(unsigned i=0;i<old_limits[difficulty];i++)type(&g,wrong);
  assert(g.status==NG_LOST && g.moves==old_limits[difficulty] &&
   g.history_count==old_limits[difficulty] && g.phase==0);
  roundtrip(&g);
 }
 puts("baseball beta6: 20/30/40/50, final-win/loss, 50 durable records, legacy PASS");
 return 0;
}
