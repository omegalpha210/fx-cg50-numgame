#include "storage.h"
#include "diagnostics.h"
#include <string.h>
#define HEADER 32u
/* Reused across all games, never one codec/solver workspace per game. */
static uint8_t buffer[NG_RECORD_MAX];
static NgSession ng_storage_probe;
bool ng_storage_fixture(unsigned id,unsigned difficulty,unsigned mode,uint32_t seed)
{
 /* Sequential main-thread diagnostics only. Reuse existing BSS, no extra
  * session/14K buffer and no BFile operation or user's session reference. */
 NgSession *s=&ng_storage_probe;memset(s,0,sizeof *s);ng_new(&s->game,id,difficulty,mode,seed,1);s->stats.started=1;
 const NgModule *m=ng_module(id);if(!ng_valid(&s->game))return false;
 if(s->game.cpu_pending)(void)m->action(&s->game,NGK_CPU);
 if(m->flags&NGF_HINT)(void)m->action(&s->game,NGK_HINT);
 if(!ng_valid(&s->game))return false;
 if(!s->game.status && !s->game.cpu_pending && (m->flags&NGF_UNDO)){
  s->undo_count=NG_UNDO;for(unsigned i=0;i<NG_UNDO;i++)s->undo[i]=s->game;
 }
 ng_record_result(s);size_t n=ng_encode(s,buffer,sizeof buffer);if(!n)return false;
 uint32_t crc=ng_crc32(buffer,n);if(!ng_decode(s,buffer,n,id))return false;
 size_t again=ng_encode(s,buffer,sizeof buffer);return again==n && ng_crc32(buffer,again)==crc;
}
uint32_t ng_session_fingerprint(const NgSession *s)
{size_t n=ng_encode(s,buffer,sizeof buffer);return n?ng_crc32(buffer,n):0;}
static uint32_t get32(const uint8_t *p)
{return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static void put32(uint8_t *p,uint32_t v)
{for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));}
bool ng_settings_decode(NgSettings *s,const uint8_t *p,size_t n)
{
 unsigned ids=n==63?30:n==67?32:NG_ID_MAX;
 bool modern=n==2*NG_ID_MAX+3+1+NG_RECENT_LIMIT+2+2;
 if((!modern && n!=2*ids+3) || p[0]>NG_ID_MAX || p[1+2*ids]>1 || p[2+2*ids]>1)return false;
 for(unsigned i=0;i<ids;i++) {
  const NgModule *m=ng_module(i+1);
  unsigned old_modes=modern?0u:i==0?6u:i==5?2u:i>=20&&i<=24?3u:0u;
  if(p[1+i]>=ng_difficulty_count(i+1) || p[1+ids+i]>=(old_modes?old_modes:(m?m->modes:1)))return false;
 }
 memset(s,0,sizeof *s);memset(s->difficulty,1,sizeof s->difficulty);s->last_game=p[0];
 memcpy(s->difficulty,p+1,ids);memcpy(s->mode,p+1+ids,ids);s->first_help=p[1+2*ids];s->show_time=p[2+2*ids];
 if(!modern){
  s->mode[0]=0;s->mode[5]=0;
  for(unsigned i=20;i<=24;i++)if(s->mode[i]>1)s->mode[i]=0;
  s->target=24;return true;
 }
 size_t pos=2*NG_ID_MAX+3;s->recent_count=p[pos++];
 if(s->recent_count>NG_RECENT_LIMIT)return false;
 for(unsigned i=0;i<NG_RECENT_LIMIT;i++){
  uint8_t id=p[pos++];if(i<s->recent_count){
   if(ng_catalog_index(id)<0)return false;
   for(unsigned j=0;j<i;j++)if(s->recent[j]==id)return false;
   s->recent[i]=id;
  }else if(id)return false;
 }
 s->pending_delete=p[pos++];
 if(s->pending_delete && ng_catalog_index(s->pending_delete)<0)return false;
 for(unsigned i=0;i<s->recent_count;i++)if(s->recent[i]==s->pending_delete)return false;
 s->migration_complete=p[pos++];if(s->migration_complete>1)return false;
 s->target=(uint16_t)p[pos]|((uint16_t)p[pos+1]<<8);
 return s->target>=1 && s->target<=1000;
}
size_t ng_settings_encode(const NgSettings *s,uint8_t *p,size_t capacity)
{
 if(capacity<2*NG_ID_MAX+3+1+NG_RECENT_LIMIT+2+2)return 0;
 p[0]=s->last_game;memcpy(p+1,s->difficulty,NG_ID_MAX);memcpy(p+1+NG_ID_MAX,s->mode,NG_ID_MAX);
 p[1+2*NG_ID_MAX]=s->first_help;p[2+2*NG_ID_MAX]=s->show_time;
 size_t pos=2*NG_ID_MAX+3;
 p[pos++]=s->recent_count;for(unsigned i=0;i<NG_RECENT_LIMIT;i++)p[pos++]=s->recent[i];
 p[pos++]=s->pending_delete;
 p[pos++]=s->migration_complete;
 unsigned target=s->target?s->target:24;p[pos++]=(uint8_t)target;p[pos++]=(uint8_t)(target>>8);
 NgSettings probe;return ng_settings_decode(&probe,p,pos)?pos:0;
}
static bool settings_valid(const uint8_t *p,size_t n){NgSettings probe;return ng_settings_decode(&probe,p,n);}
static int read_slot(const NgIO *io,unsigned id,unsigned slot,uint32_t *generation,size_t *length)
{
 int fd=io->open(io->context,id,slot,false);
 if(fd==-2)return NG_LOAD_ABSENT;
 if(fd<0)return NG_LOAD_IO_ERROR;
 size_t pos=0;bool error=false;
 while(pos<sizeof(buffer)) {
  ptrdiff_t got=io->read(io->context,fd,buffer+pos,sizeof(buffer)-pos);
  if(got<0 || (size_t)got>sizeof(buffer)-pos){error=true;break;}
  if(got==0)break;
  pos+=(size_t)got;
 }
 /* An overlong file cannot masquerade as a bounded record. */
 if(pos==sizeof(buffer)){uint8_t byte;ptrdiff_t n=io->read(io->context,fd,&byte,1);if(n!=0)error=true;}
 if(io->close(io->context,fd)<0)error=true;
 if(error)return NG_LOAD_IO_ERROR;
 if(pos<HEADER || memcmp(buffer,"NGSAVE01",8) || get32(buffer+8)!=id ||
  (get32(buffer+12)<1 || get32(buffer+12)>4) || get32(buffer+20)!=pos-HEADER ||
  ng_crc32(buffer,28)!=get32(buffer+28) ||
  ng_crc32(buffer+HEADER,pos-HEADER)!=get32(buffer+24))return NG_LOAD_INVALID;
 bool valid=id?ng_decode(&ng_storage_probe,buffer+HEADER,pos-HEADER,id):settings_valid(buffer+HEADER,pos-HEADER);
 if(!valid)return NG_LOAD_INVALID;
 *generation=get32(buffer+16);*length=pos;return NG_LOAD_OK;
}
static unsigned newest(const uint32_t gen[2],const int rc[2])
{return rc[1]==NG_LOAD_OK && (rc[0]!=NG_LOAD_OK || (gen[1]!=gen[0] && (uint32_t)(gen[1]-gen[0])<0x80000000u))?1:0;}
bool ng_slot_valid(const NgIO *io,unsigned id,unsigned slot)
{uint32_t generation;size_t length;return read_slot(io,id,slot,&generation,&length)==NG_LOAD_OK;}
static int read_best(const NgIO *io,unsigned id,uint32_t *generation,size_t *length)
{
 uint32_t gen[2]={0,0};size_t len[2]={0,0};
 int rc[2];rc[0]=read_slot(io,id,0,&gen[0],&len[0]);rc[1]=read_slot(io,id,1,&gen[1],&len[1]);
 if(rc[0]!=NG_LOAD_OK && rc[1]!=NG_LOAD_OK) {
  if(rc[0]==NG_LOAD_IO_ERROR || rc[1]==NG_LOAD_IO_ERROR)return NG_LOAD_IO_ERROR;
  return rc[0]==NG_LOAD_ABSENT && rc[1]==NG_LOAD_ABSENT?NG_LOAD_ABSENT:NG_LOAD_INVALID;
 }
 unsigned best=newest(gen,rc);int last=read_slot(io,id,best,generation,length);
 if(last!=NG_LOAD_OK && rc[1-best]==NG_LOAD_OK) {last=read_slot(io,id,1-best,generation,length);if(last==NG_LOAD_OK)return NG_LOAD_RECOVERED;}
 if(last!=NG_LOAD_OK)return last;
 return rc[1-best]==NG_LOAD_INVALID || rc[1-best]==NG_LOAD_IO_ERROR?NG_LOAD_RECOVERED:NG_LOAD_OK;
}
int ng_load_io(NgSession *s,unsigned id,const NgIO *io)
{
 if(id<1 || id>NG_ID_MAX)return NG_LOAD_INVALID;
 uint32_t gen=0;size_t length=0;int rc=read_best(io,id,&gen,&length);
 if(rc==NG_LOAD_OK || rc==NG_LOAD_RECOVERED) {
  if(!ng_decode(s,buffer+HEADER,length-HEADER,id))return NG_LOAD_INVALID;
  s->generation=gen;
 }
 return rc;
}
static bool write_record(const NgIO *io,unsigned id,NgSession *session,NgSettings *settings)
{
 uint32_t gen[2]={0,0};size_t len[2]={0,0};int rc[2];
 rc[0]=read_slot(io,id,0,&gen[0],&len[0]);rc[1]=read_slot(io,id,1,&gen[1],&len[1]);
 if(rc[0]==NG_LOAD_IO_ERROR || rc[1]==NG_LOAD_IO_ERROR)return false;
 unsigned best=newest(gen,rc),slot=rc[best]==NG_LOAD_OK?1-best:0;
 uint32_t next=rc[best]==NG_LOAD_OK?gen[best]+1:1;
 size_t payload;
 if(id)payload=ng_encode(session,buffer+HEADER,sizeof(buffer)-HEADER);
 else payload=ng_settings_encode(settings,buffer+HEADER,sizeof buffer-HEADER);
 if(!payload)return false;
 memcpy(buffer,"NGSAVE01",8);put32(buffer+8,id);put32(buffer+12,4);
 put32(buffer+16,next);put32(buffer+20,(uint32_t)payload);
 uint32_t crc=ng_crc32(buffer+HEADER,payload);put32(buffer+24,crc);put32(buffer+28,ng_crc32(buffer,28));
 size_t total=HEADER+payload,pos=0;
 if(io->prepare && !io->prepare(io->context,id,slot,total))return false;
 int fd=io->open(io->context,id,slot,true);
 if(fd<0)return false;
 bool ok=true;
 while(pos<total) {
  ptrdiff_t n=io->write(io->context,fd,buffer+pos,total-pos);
  if(n<=0 || (size_t)n>total-pos){ok=false;break;}pos+=(size_t)n;
 }
 if(io->close(io->context,fd)<0)ok=false;
 if(!ok)return false;
 uint32_t readgen=0;size_t readlen=0;
 if(read_slot(io,id,slot,&readgen,&readlen)!=NG_LOAD_OK || readgen!=next || readlen!=total || get32(buffer+24)!=crc)return false;
 if(id)session->generation=next;else settings->generation=next;
 return true;
}
bool ng_save_io(NgSession *s,const NgIO *io)
{return ng_valid(&s->game) && write_record(io,s->game.id,s,NULL);}
int ng_settings_load_io(NgSettings *s,const NgIO *io)
{
 uint32_t gen=0;size_t length=0;int rc=read_best(io,0,&gen,&length);
 if(rc==NG_LOAD_OK || rc==NG_LOAD_RECOVERED) {
  if(!ng_settings_decode(s,buffer+HEADER,length-HEADER))return NG_LOAD_INVALID;
  s->generation=gen;
 }
 return rc;
}
bool ng_settings_save_io(NgSettings *s,const NgIO *io){return write_record(io,0,NULL,s);}
