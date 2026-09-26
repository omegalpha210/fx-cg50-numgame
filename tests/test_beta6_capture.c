#include "app.h"
#include "guesscalc_math.h"
#include "prime_beta6.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

/* Captures use the same C renderer and game modules as the native build.
   The three-digit card frames are explicitly labelled layout fixtures. */
static NgApp app;
static uint16_t pixels[396u*224u];
static FILE *manifest;

static void paint(void *context,int x,int y,int w,int h,uint16_t color)
{
 (void)context;
 assert(x>=0 && y>=0 && w>0 && h>0 && x+w<=396 && y+h<=224);
 for(int row=y;row<y+h;row++)for(int col=x;col<x+w;col++)
  pixels[row*396+col]=color;
}
static void capture(const char *name)
{
 memset(pixels,0xff,sizeof pixels);
 NgCanvas canvas={NULL,paint};ng_render(&app,&canvas);
 char path[160];int length=snprintf(path,sizeof path,"captures/beta6/%s.ppm",name);
 assert(length>0 && (size_t)length<sizeof path);
 FILE *out=fopen(path,"wb");assert(out);
 assert(fprintf(out,"P6\n396 224\n255\n")>0);
 for(unsigned i=0;i<396u*224u;i++){
  unsigned p=pixels[i];unsigned char rgb[3]={
   (unsigned char)(((p>>11)&31u)*255u/31u),
   (unsigned char)(((p>>5)&63u)*255u/63u),
   (unsigned char)((p&31u)*255u/31u)};
  assert(fwrite(rgb,1,sizeof rgb,out)==sizeof rgb);
 }
 assert(fclose(out)==0);assert(fprintf(manifest,"%s.ppm\n",name)>0);
}
static void press(int key)
{
 (void)ng_app_event(&app,key,NG_DOWN);
 (void)ng_app_event(&app,key,NG_UP);
}
static void type(const char *value)
{for(unsigned i=0;value[i];i++)press((unsigned char)value[i]);}
static void main_screen(void)
{
 for(unsigned i=0;i<8 && (app.screen!=NG_MAIN || app.modal);i++)press(NGK_EXIT);
 assert(app.screen==NG_MAIN && !app.modal);
}
static void entry(unsigned id)
{
 int index=ng_catalog_index(id);assert(index>=0);
 main_screen();press('1'+index/6);press('1'+index%6);
 assert(app.screen==NG_ENTRY && app.selected_id==id && !app.modal);
}
static void open_new(unsigned id,unsigned difficulty)
{
 assert(difficulty<ng_difficulty_count(id));
 app.settings.difficulty[id-1]=(uint8_t)difficulty;entry(id);
 app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_NEW);
 press(NGK_F6);
 for(unsigned i=0;i<3 && (app.modal==NG_MODAL_NEW || app.modal==NG_MODAL_EVICT);i++)press(NGK_EXE);
 if(app.modal==NG_MODAL_RULES)press(NGK_EXIT);
 assert(app.screen==NG_PLAY && !app.modal && app.session.game.id==id);
 assert(app.session.game.difficulty==difficulty && ng_valid(&app.session.game));
}
static void target_frames(void)
{
 static const unsigned choices[]={10,24,50,100,200,NG_TARGET_RANDOM};
 static const char *const names[]={"010","024","050","100","200","random"};
 for(unsigned i=0;i<6;i++){
  app.settings.target=(uint16_t)choices[i];entry(6);
  app.entry_selection=(uint8_t)ng_entry_row(&app,NG_ENTRY_TARGET);
  if(i==0){press(NGK_LEFT);assert(app.settings.target==choices[i]);}
  if(i==5){press(NGK_RIGHT);assert(app.settings.target==choices[i]);}
  char name[80];snprintf(name,sizeof name,"make-target-entry-%s",names[i]);capture(name);
  press(i%2?NGK_EXE:NGK_F6);if(app.modal==NG_MODAL_NEW)press(NGK_EXE);
  assert(app.screen==NG_PLAY && !app.modal && ng_valid(&app.session.game));
  if(i==5){
   unsigned resolved=(unsigned)app.session.game.data[0];bool found=false;
   for(unsigned j=0;j<5;j++)if(resolved==choices[j])found=true;
   assert(found && app.session.game.data[2]==1);
  }else assert(app.session.game.data[0]==(int32_t)choices[i] &&
               app.session.game.data[2]==0);
  snprintf(name,sizeof name,"make-target-play-%s",names[i]);capture(name);
 }
 for(unsigned difficulty=0;difficulty<4;difficulty++){
  app.settings.target=24;open_new(6,difficulty);
  unsigned cards=(unsigned)app.session.game.data[1];
  assert(cards==(difficulty<2?4u:difficulty+3u));
  if(difficulty==0 || difficulty>=2){
   NgGame saved=app.session.game;
   for(unsigned i=0;i<cards;i++)app.session.game.board[i]=999;
   char name[80];snprintf(name,sizeof name,"make-target-%u-card-999-layout-fixture",cards);
   capture(name);app.session.game=saved;
  }
 }
}
static void countdown_frames(void)
{
 static const char *const names[]={"easy","normal","hard","master"};
 for(unsigned difficulty=0;difficulty<4;difficulty++){
  open_new(7,difficulty);char name[80];
  snprintf(name,sizeof name,"countdown-%s",names[difficulty]);capture(name);
 }
}
static unsigned decimal_digits(unsigned value)
{unsigned n=1;while(value>=10){value/=10;n++;}return n;}
static void prime_frames(void)
{
 static const char *const names[]={"easy","normal","hard","master"};
 for(unsigned difficulty=0;difficulty<4;difficulty++){
  open_new(10,difficulty);
  unsigned count=ng_prime_beta6_count(difficulty),low=0,high=0;
  assert(count>0);
  for(unsigned i=1;i<count;i++){
   unsigned value=ng_prime_beta6_target(difficulty,i);
   if(value<ng_prime_beta6_target(difficulty,low))low=i;
   if(value>ng_prime_beta6_target(difficulty,high))high=i;
  }
  unsigned indices[]={low,high};const char *ext[]={"min-digits","max-digits"};
  if(difficulty)assert(decimal_digits(ng_prime_beta6_target(difficulty,low))+1==
                       decimal_digits(ng_prime_beta6_target(difficulty,high)));
  for(unsigned j=0;j<2;j++){
   app.session.game.puzzle_id=indices[j];
   app.session.game.data[0]=(int32_t)ng_prime_beta6_target(difficulty,indices[j]);
   assert(ng_valid(&app.session.game));
   char name[80];snprintf(name,sizeof name,"prime-%s-%s",names[difficulty],ext[j]);
   capture(name);
  }
 }
}
static char wrong_digit(const NgGame *g,unsigned attempt)
{
 unsigned n=(unsigned)g->data[0];char digit=(char)('0'+attempt%10u);bool same=true;
 for(unsigned i=0;i<n;i++)if(g->board[i]!=digit)same=false;
 return same?(digit=='9'?'0':(char)(digit+1)):digit;
}
static void submit_baseball(const char *digits)
{type(digits);press(NGK_EXE);}
static void baseball_frames(void)
{
 static const unsigned caps[]={20,30,40,50};
 for(unsigned difficulty=0;difficulty<4;difficulty++){
  open_new(1,difficulty);NgGame *g=&app.session.game;
  unsigned cap=caps[difficulty],digits=(unsigned)g->data[0];
  assert((unsigned)g->data[1]==cap);
  char guess[8],name[80];
  snprintf(name,sizeof name,"baseball-%u-short-no-scroll",cap);capture(name);
  for(unsigned attempt=0;attempt<cap-1;attempt++){
   memset(guess,wrong_digit(g,attempt),digits);guess[digits]=0;
   submit_baseball(guess);
   assert(g->moves==attempt+1 && g->status==NG_PLAYING && ng_valid(g));
   if(attempt+1<cap-1)assert(!app.modal);
  }
  assert(app.modal==NG_MODAL_LAST_TRY && g->phase==1);
  snprintf(name,sizeof name,"baseball-%u-of-%u-last-try",cap-1,cap);capture(name);
  press(NGK_EXE);assert(!app.modal && g->phase==2 && ng_valid(g));
  snprintf(name,sizeof name,"baseball-%u-of-%u-play",cap-1,cap);capture(name);
  if(cap==50){
   press('7');assert(g->input[0]=='7');
   unsigned max=g->moves-5;
   while(g->scroll)press(NGK_UP);
   assert(g->input[0]=='7' && g->scroll==0);
   capture("baseball-49-history-top-draft");
   while(g->scroll<max/2)press(NGK_DOWN);
   assert(g->input[0]=='7');capture("baseball-49-history-middle-draft");
   while(g->scroll<max)press(NGK_DOWN);
   assert(g->input[0]=='7');capture("baseball-49-history-bottom-draft");
   press(NGK_DEL);assert(!g->input[0]);
  }
  memset(guess,wrong_digit(g,cap-1),digits);guess[digits]=0;
  submit_baseball(guess);
  assert(g->moves==cap && g->status==NG_LOST && app.modal==NG_MODAL_RESULT && ng_valid(g));
  snprintf(name,sizeof name,"baseball-%u-of-%u-fail-result",cap,cap);capture(name);
  press(NGK_EXIT);assert(app.screen==NG_PLAY && !app.modal);
  snprintf(name,sizeof name,"baseball-%u-of-%u-fail-history",cap,cap);capture(name);
 }
 open_new(1,0);NgGame *g=&app.session.game;unsigned digits=(unsigned)g->data[0];
 char secret[8];for(unsigned i=0;i<digits;i++)secret[i]=(char)g->board[i];secret[digits]=0;
 for(unsigned attempt=0;attempt<19;attempt++){
  char guess[8];memset(guess,wrong_digit(g,attempt),digits);guess[digits]=0;
  submit_baseball(guess);
 }
 assert(app.modal==NG_MODAL_LAST_TRY);press(NGK_EXE);
 submit_baseball(secret);
 assert(g->moves==20 && g->status==NG_WON && app.modal==NG_MODAL_RESULT && ng_valid(g));
 capture("baseball-20-of-20-win-result");
}
static void mind_frames(void)
{
 open_new(3,3);NgGame *g=&app.session.game;
 assert(g->data[2]>6);capture("mind-long-top");
 press('1');assert(g->input[0]=='1');
 unsigned max=(unsigned)g->data[2]-6u;
 while(g->scroll<max/2)press(NGK_DOWN);
 assert(g->input[0]=='1');capture("mind-long-middle-draft");
 while(g->scroll<max)press(NGK_DOWN);
 assert(g->input[0]=='1');capture("mind-long-bottom-draft");
}
static void equation_frames(void)
{
 open_new(2,0);NgGame *g=&app.session.game;
 capture("equation-short-no-scroll");
 char secret[11],candidate[16];unsigned n=(unsigned)g->data[0];
 for(unsigned i=0;i<n;i++)secret[i]=(char)g->board[i];secret[n]=0;
 bool found=false;
 for(unsigned a=1;a<100 && !found;a++)for(unsigned b=1;b<100 && !found;b++){
  int length=snprintf(candidate,sizeof candidate,"%u+%u=%u",a,b,a+b);
  if(length==(int)n && strcmp(candidate,secret) && gc_equation(candidate))found=true;
 }
 assert(found);
 for(unsigned i=0;i<5;i++){
  type(candidate);press(NGK_EXE);
  assert(g->moves==i+1 && g->status==NG_PLAYING && ng_valid(g));
 }
 assert(g->history_count==5 && g->scroll==1);
 press(NGK_UP);assert(g->scroll==0);capture("equation-long-top");
 press('1');assert(g->input[0]=='1');press(NGK_DOWN);
 assert(g->scroll==1 && g->input[0]=='1');capture("equation-long-bottom-draft");
}
int main(void)
{
 if(mkdir("captures",0755)<0)assert(errno==EEXIST);
 if(mkdir("captures/beta6",0755)<0)assert(errno==EEXIST);
 manifest=fopen("captures/beta6/manifest.txt","w");assert(manifest);
 ng_app_init(&app,(NgHooks){0},UINT32_C(0x62b6e4));
 app.settings.first_help=0;
 target_frames();countdown_frames();prime_frames();baseball_frames();
 mind_frames();equation_frames();
 assert(fclose(manifest)==0);
 puts("beta.6 actual renderer captures PASS");return 0;
}
