#ifndef NG_TEST_SUPPORT_H
#define NG_TEST_SUPPORT_H
#include "app.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct {
 uint8_t bytes[NG_ID_MAX+1][2][NG_RECORD_MAX];
 size_t lengths[NG_ID_MAX+1][2],position;
 unsigned id,slot,saves,opens,menu,off;
 bool writing,fail_open,fail_read,fail_close;
 int write_budget;
} TestDisk;
static int test_open(void *ctx,unsigned id,unsigned slot,bool writing)
{
 TestDisk *d=ctx;assert(id<=NG_ID_MAX && slot<=1);d->opens++;
 if(d->fail_open)return -1;
 if(!writing && !d->lengths[id][slot])return -2;
 d->id=id;d->slot=slot;d->writing=writing;d->position=0;
 if(writing){d->lengths[id][slot]=0;d->saves++;}return 1;
}
static ptrdiff_t test_read(void *ctx,int fd,void *out,size_t n)
{
 TestDisk *d=ctx;assert(fd==1);if(d->fail_read)return -1;
 size_t remain=d->lengths[d->id][d->slot]-d->position;if(n>remain)n=remain;
 memcpy(out,d->bytes[d->id][d->slot]+d->position,n);d->position+=n;return (ptrdiff_t)n;
}
static ptrdiff_t test_write(void *ctx,int fd,const void *in,size_t n)
{
 TestDisk *d=ctx;assert(fd==1);if(d->write_budget==0)return -1;
 if(d->write_budget>0 && n>(size_t)d->write_budget)n=(size_t)d->write_budget;
 assert(d->position+n<=NG_RECORD_MAX);memcpy(d->bytes[d->id][d->slot]+d->position,in,n);
 d->position+=n;d->lengths[d->id][d->slot]=d->position;
 if(d->write_budget>0)d->write_budget-=(int)n;return (ptrdiff_t)n;
}
static int test_close(void *ctx,int fd){TestDisk *d=ctx;assert(fd==1);return d->fail_close?-1:0;}
static NgIO test_io(TestDisk *d){return (NgIO){d,test_open,test_read,test_write,test_close};}
static int test_load_hook(void *ctx,NgSession *s,unsigned id){NgIO io=test_io(ctx);return ng_load_io(s,id,&io);}
static bool test_save_hook(void *ctx,NgSession *s){NgIO io=test_io(ctx);return ng_save_io(s,&io);}
static int test_settings_load_hook(void *ctx,NgSettings *s){NgIO io=test_io(ctx);return ng_settings_load_io(s,&io);}
static bool test_settings_save_hook(void *ctx,NgSettings *s){NgIO io=test_io(ctx);return ng_settings_save_io(s,&io);}
static void test_menu(void *ctx){((TestDisk *)ctx)->menu++;}
static void test_off(void *ctx){((TestDisk *)ctx)->off++;}
static NgHooks test_hooks(TestDisk *d){return (NgHooks){d,test_load_hook,test_save_hook,test_settings_load_hook,test_settings_save_hook,test_menu,test_off};}
static void test_pixel(void *ctx,int x,int y,int w,int h,uint16_t color)
{unsigned *draws=ctx;assert(x>=0 && y>=0 && x+w<=396 && y+h<=224 && w>0 && h>0);(void)color;(*draws)++;}
static void test_render(const NgApp *a){unsigned draws=0;NgCanvas c={&draws,test_pixel};ng_render(a,&c);assert(draws>0);}
static void tap(NgApp *a,int key){ng_app_event(a,key,NG_DOWN);ng_app_event(a,key,NG_UP);test_render(a);}
static void open_game(NgApp *a,unsigned id)
{
 while(a->modal)tap(a,NGK_EXIT);
 while(a->screen!=NG_MAIN){tap(a,NGK_EXIT);assert(!a->modal);}
 tap(a,'1'+(int)(ng_catalog_index(id)/5));tap(a,'1'+(int)(ng_catalog_index(id)%5));assert(a->screen==NG_ENTRY);
 tap(a,'1'+(int)ng_entry_row(a,NG_ENTRY_NEW));tap(a,NGK_F6);if(a->modal==NG_MODAL_NEW)tap(a,NGK_EXE);
 assert(a->screen==NG_PLAY && a->session.game.id==id);assert(ng_valid(&a->session.game));
}
#endif
