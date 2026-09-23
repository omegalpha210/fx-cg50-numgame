#define _POSIX_C_SOURCE 200809L
#define _DARWIN_C_SOURCE 1
#include "app.h"
#include "diagnostics.h"
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
/* Host evidence only. These ABI, timings and process peaks are not SH values. */
static NgApp app;
static NgSession saved,cold;
static uint16_t pixels[396*224];
static uint8_t wire[NG_RECORD_MAX];
static double samples[4096];
static unsigned rectangles;
static double now(void)
{struct timespec t;assert(!clock_gettime(CLOCK_MONOTONIC,&t));return t.tv_sec+t.tv_nsec/1e9;}
static void paint(void *unused,int x,int y,int w,int h,uint16_t color)
{
 (void)unused;assert(x>=0 && y>=0 && x+w<=396 && y+h<=224);rectangles++;
 for(int r=y;r<y+h;r++)for(int col=x;col<x+w;col++)pixels[r*396+col]=color;
}
static int compare(const void *a,const void *b)
{double x=*(const double *)a,y=*(const double *)b;return (x>y)-(x<y);}
static void summary(unsigned n)
{
 assert(n && n<=4096);qsort(samples,n,sizeof *samples,compare);
 printf("\"n\":%u,\"p50_ms\":%.6f,\"p95_ms\":%.6f,\"max_ms\":%.6f",n,samples[(n-1)/2]*1000,samples[(n*95+99)/100-1]*1000,samples[n-1]*1000);
}
static void draw(void){NgCanvas canvas={NULL,paint};ng_render(&app,&canvas);}
static void prepare(unsigned id,unsigned level,unsigned mode,unsigned seed)
{
 ng_new(&app.session.game,id,level,mode,seed,seed);assert(ng_valid(&app.session.game));
 app.screen=NG_PLAY;app.active=true;app.modal=NG_MODAL_NONE;draw();
}
static unsigned files(const char *folder)
{
 DIR *d=opendir(folder);assert(d);unsigned n=0;struct dirent *entry;
 while((entry=readdir(d)))if(strcmp(entry->d_name,".") && strcmp(entry->d_name,".."))n++;
 assert(!closedir(d));return n;
}
static long rss(void)
{
 struct rusage usage;assert(!getrusage(RUSAGE_SELF,&usage));
#if defined(__APPLE__)
 return usage.ru_maxrss;
#else
 return usage.ru_maxrss*1024;
#endif
}
int main(void)
{
 char stack;ng_diag_reset((uintptr_t)&stack);ng_app_init(&app,(NgHooks){0},20260922);
 printf("{\"platform\":\"host\",\"hardware_peak\":\"NOT MEASURED\",\"pointer_bytes\":%zu,\"game_bytes\":%zu,\"session_bytes\":%zu,\"app_bytes\":%zu,",sizeof(void *),sizeof(NgGame),sizeof(NgSession),sizeof(NgApp));
#ifdef NG_DIAGNOSTIC
 printf("\"instrumented\":true,");
#else
 printf("\"instrumented\":false,");
#endif
 printf("\"load_scope\":\"module_init + semantic_validation + full_RGB565_renderer; host warm process; all modes/levels mixed,64 samples/config\",\"games\":[");
 for(unsigned index=0;index<NG_GAME_COUNT;index++){
  unsigned id=ng_visible_id(index),n=0;const NgModule *m=ng_module(id);
  for(unsigned level=0;level<ng_difficulty_count(id);level++)for(unsigned mode=0;mode<m->modes;mode++)for(unsigned k=0;k<64;k++){
   double t=now();prepare(id,level,mode,20260922+index*1000+level*100+mode*64+k);samples[n++]=now()-t;
  }
  printf("%s{\"id\":%u,",index?",":"",id);summary(n);printf("}");
 }
 printf("],\"heavy\":[");
 const unsigned heavy[]={7,11,12,13,14,15,3,18,32,21,22,23,24,25};
 for(unsigned i=0;i<sizeof heavy/sizeof *heavy;i++){
  unsigned id=heavy[i];const NgModule *m=ng_module(id);unsigned level=ng_has_hell(id)?NG_HELL:NG_MASTER;
  for(unsigned k=0;k<128;k++){
   prepare(id,level,(m->flags&NGF_CPU)?1:0,4321+k);double t=now();
   if(app.session.game.cpu_pending){NgDiagScope s=ng_diag_begin(NGOP_CPU,id);(void)m->action(&app.session.game,NGK_CPU);ng_diag_end(s);}
   if(m->flags&NGF_HINT){NgDiagScope s=ng_diag_begin(NGOP_HINT,id);(void)m->action(&app.session.game,NGK_HINT);ng_diag_end(s);}
   assert(ng_valid(&app.session.game));draw();samples[k]=now()-t;
  }
  printf("%s{\"id\":%u,\"scope\":\"pending_CPU_then_hint_if_supported + validate + render\",",i?",":"",id);summary(128);printf("}");
 }
 printf("],\"ram_fixture\":{");
 long before=rss();unsigned failures=0;ng_diag_emit(NGD_TIMER_START,0,0);
 for(unsigned k=0;k<1000;k++){
  unsigned id=ng_visible_id(k%NG_GAME_COUNT),round=k/NG_GAME_COUNT;double t=now();
  failures+=!ng_storage_fixture(id,round%ng_difficulty_count(id),(round/ng_difficulty_count(id))%ng_module(id)->modes,67123+k);
  samples[k]=now()-t;assert(!ng_diagnostics.handles && ng_diagnostics.timers==1);
 }
 summary(1000);printf(",\"failures\":%u,\"rss_highwater_before_bytes\":%ld,\"rss_highwater_after_bytes\":%ld,\"app_malloc_calls\":0,\"app_heap_bytes\":0,\"handles\":%u,\"timers\":%u,\"fixture_io\":false},",failures,before,rss(),ng_diagnostics.handles,ng_diagnostics.timers);assert(!failures);
 /* Real host archive adapter: maximal undo/notes plus CRC, two-copy recovery
    reads and write verification. This temporary namespace never sees saves. */
 char folder[]="benchmark-fixture-XXXXXX";assert(mkdtemp(folder));ng_storage_directory(folder);
 memset(&saved,0,sizeof saved);ng_new(&saved.game,11,NG_HELL,0,123,1);saved.stats.started=1;
 for(unsigned c=0;c<NG_CELLS;c++)if(!saved.game.fixed[c])saved.game.notes[c]=0x1ff;
 saved.undo_count=NG_UNDO;for(unsigned i=0;i<NG_UNDO;i++)saved.undo[i]=saved.game;
 assert(ng_valid(&saved.game));size_t payload=ng_encode(&saved,wire,sizeof wire);assert(payload);
 double first=now();assert(ng_storage_save(&saved));first=now()-first;assert(ng_storage_save(&saved));
 for(unsigned k=0;k<128;k++){
  double t=now();assert(ng_storage_save(&saved));samples[k]=now()-t;
  assert(ng_storage_load(&cold,11)==NG_LOAD_OK);assert(!memcmp(&cold,&saved,sizeof saved));
  assert(!ng_diagnostics.handles && ng_diagnostics.timers==1);
 }
 printf("\"checkpoint\":{\"scope\":\"POSIX archive save including existing copies+CRC+readback, excludes explicit cold-load assertion\",\"first_create_ms\":%.6f,",first*1000);summary(128);
 printf(",\"payload_bytes\":%zu,\"record_with_header_bytes\":%zu,\"archive_files\":%u,\"total_archive_bytes\":%u,\"handles\":%u},",payload,payload+32,files(folder),2*NG_ARCHIVE_BYTES,ng_diagnostics.handles);assert(files(folder)==2);
#ifdef NG_DIAGNOSTIC
 printf("\"diagnostics\":{\"sampled_stack_bytes\":%lu,\"samples\":%u,\"deepest_game\":%u,\"deepest_operation\":\"%s\",\"codec_peak\":%u,\"read_calls\":%u,\"write_calls\":%u,\"read_bytes\":%u,\"write_bytes\":%u},",(unsigned long)(ng_diagnostics.stack_base-ng_diagnostics.stack_low),ng_diagnostics.stack_samples,ng_diagnostics.peak_game,ng_diag_operation_name(ng_diagnostics.peak_operation),ng_diagnostics.codec_peak,ng_diagnostics.read_calls,ng_diagnostics.write_calls,ng_diagnostics.read_bytes,ng_diagnostics.write_bytes);
 const char *prefix="ND";
#else
 const char *prefix="NG";
#endif
 for(unsigned slot=0;slot<2;slot++){char path[128];snprintf(path,sizeof path,"%s/%sARC%c.dat",folder,prefix,'A'+slot);assert(!unlink(path));}assert(!rmdir(folder));ng_storage_directory(".");
 printf("\"rectangles\":%u,\"limits\":\"No host->SH timing/ABI conversion; RSS highwater includes runtime/pages and is not live heap. App allocation zero is source ownership, not OS allocation coverage. No wall-clock timeout implemented in engines; bounded construction/host banks avoid device search. Hardware latency/fallback budgets unverified.\"}\n",rectangles);return 0;
}
