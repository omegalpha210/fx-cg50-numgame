#define _POSIX_C_SOURCE 200809L
#include "storage.h"
#include "diagnostics.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdarg.h>
#ifdef FXCG50
#include <gint/bfile.h>
#include <gint/gint.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
static const char *directory=".";
void ng_storage_directory(const char *path){directory=path;}
#endif
/* Transactions are synchronous, main-thread only, and never hold two files. */
static int pending_close=-1;
static size_t position,record_base,record_limit;
static const int archive_context=1;
static const int compact_context=2;
static size_t compact_size;
static bool compact_writer;
static char compact_write_name[24];
static char pending_remove_name[24];
static bool raw_remove(const char *name);
#ifdef NG_DIAGNOSTIC
static const char prefix[]="ND";
#else
static const char prefix[]="NG";
#endif
static uint32_t get32(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static void put32(uint8_t *p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(8*i));}
static int raw_close(int fd)
{
#ifdef FXCG50
 return BFile_Close(fd);
#else
 return close(fd);
#endif
}
static bool cleanup(void)
{
 if(pending_close>=0){
  int fd=pending_close;if(raw_close(fd)<0)return false;
  pending_close=-1;ng_diag_emit(NGD_CLOSE,(uint32_t)fd,1);
 }
 if(pending_remove_name[0]){
  if(!raw_remove(pending_remove_name))return false;
  pending_remove_name[0]=0;
 }
 return true;
}
static int file_close(void *ctx,int fd)
{
 (void)ctx;if(raw_close(fd)>=0){ng_diag_emit(NGD_CLOSE,(uint32_t)fd,0);return 0;}
 if(raw_close(fd)<0)pending_close=fd;else ng_diag_emit(NGD_CLOSE,(uint32_t)fd,1);
 ng_diag_emit(NGD_IO_ERROR,(uint32_t)fd,1);return -1;
}
static void name_for(char *name,size_t size,unsigned id,unsigned slot,bool archive)
{
 if(archive)snprintf(name,size,"%sARC%c.dat",prefix,slot?'B':'A');
 else if(id==99)snprintf(name,size,"%sDIAG.txt",prefix);
 else snprintf(name,size,"%s%02u%c.dat",prefix,id,slot?'B':'A');
}
static void compact_name(char *name,size_t size,unsigned id,unsigned slot)
{snprintf(name,size,"%s2%02u%c.dat",prefix,id,slot?'B':'A');}
static void state_name(char *name,size_t size,unsigned slot)
{snprintf(name,size,"%sSTATE%c.dat",prefix,slot?'B':'A');}
#ifdef FXCG50
static void native_path(uint16_t *path,const char *name)
{const char *root="\\\\fls0\\";unsigned n=0;while(*root)path[n++]=(unsigned char)*root++;do{path[n++]=(unsigned char)*name;}while(*name++);}
#endif
static int raw_open(const char *name,bool writing,bool archive,bool *created)
{
 *created=false;
#ifdef FXCG50
 uint16_t path[40];native_path(path,name);
 if(writing && !archive){int r=BFile_Remove(path);if(r<0 && r!=BFile_EntryNotFound)return -1;}
 int fd=BFile_Open(path,writing?BFile_ReadWrite:BFile_ReadOnly);
 if(writing && fd==BFile_EntryNotFound){int size=archive?(int)NG_ARCHIVE_BYTES:0;if(BFile_Create(path,BFile_File,&size)<0)return -1;*created=true;fd=BFile_Open(path,BFile_ReadWrite);}
 if(fd<0){if(*created)(void)BFile_Remove(path);return fd==BFile_EntryNotFound?-2:-1;}
#else
 char path[512];int n=snprintf(path,sizeof path,"%s/%s",directory,name);if(n<0 || (size_t)n>=sizeof path)return -1;
 int fd=open(path,writing?O_RDWR|(archive?0:O_TRUNC):O_RDONLY,0600);
 if(writing && fd<0 && errno==ENOENT){fd=open(path,O_RDWR|O_CREAT|O_EXCL,0600);if(fd>=0){*created=true;if(archive && ftruncate(fd,NG_ARCHIVE_BYTES)<0){if(close(fd)==0)(void)unlink(path);else{pending_close=fd;ng_diag_emit(NGD_OPEN,(uint32_t)fd,1);}return -1;}}}
 if(fd<0)return errno==ENOENT?-2:-1;
#endif
 ng_diag_emit(NGD_OPEN,(uint32_t)fd,writing);return fd;
}
static ptrdiff_t raw_read(int fd,void *out,size_t size,size_t offset)
{
#ifdef FXCG50
 int total=BFile_Size(fd);if(total<0 || offset>(size_t)total)return -1;
 if(size>(size_t)total-offset)size=(size_t)total-offset;
 ptrdiff_t result=size?BFile_Read(fd,out,(int)size,(int)offset):0;
#else
 ptrdiff_t result=pread(fd,out,size,(off_t)offset);
#endif
 ng_diag_io(false,result);return result;
}
static ptrdiff_t raw_write(int fd,const void *in,size_t size,size_t offset)
{
#ifdef FXCG50
 if(size>INT_MAX || offset>INT_MAX || BFile_Seek(fd,(int)offset)<0)return -1;
 ptrdiff_t result=BFile_Write(fd,in,(int)size);
#else
 ptrdiff_t result=pwrite(fd,in,size,(off_t)offset);
#endif
 ng_diag_io(true,result);return result;
}
static bool raw_remove(const char *name)
{
#ifdef FXCG50
 uint16_t path[40];native_path(path,name);return BFile_Remove(path)>=0;
#else
 char path[512];int n=snprintf(path,sizeof path,"%s/%s",directory,name);return n>0 && (size_t)n<sizeof path && unlink(path)==0;
#endif
}
static int exact_open(const char *name,size_t bytes)
{
 bool created=false;
 int old=raw_open(name,false,false,&created);
 if(old>=0){
  /* NG2xxA/B is a private namespace. An interrupted create may contain no
     header yet, so ownership is the path rather than a possibly partial CRC. */
  if(file_close(NULL,old)<0)return -1;
  if(!raw_remove(name))return -1;
 }else if(old!=-2)return -1;
#ifdef FXCG50
 uint16_t path[40];native_path(path,name);int size=(int)bytes;
 if(BFile_Create(path,BFile_File,&size)<0)return -1;
 int fd=BFile_Open(path,BFile_ReadWrite);
 if(fd<0){(void)BFile_Remove(path);return -1;}
#else
 char path[512];int n=snprintf(path,sizeof path,"%s/%s",directory,name);
 if(n<0 || (size_t)n>=sizeof path)return -1;
 int fd=open(path,O_RDWR|O_CREAT|O_EXCL,0600);
 if(fd<0)return -1;
 if(ftruncate(fd,(off_t)bytes)<0){if(close(fd)==0)(void)unlink(path);else pending_close=fd;return -1;}
#endif
 ng_diag_emit(NGD_OPEN,(uint32_t)fd,1);return fd;
}
static void make_archive_header(uint8_t header[32],bool ready)
{
 memset(header,0,32);memcpy(header,ready?"NGARCH02":"NGINIT02",8);put32(header+8,2);put32(header+12,NG_ARCHIVE_LEGACY_IDS+1);put32(header+16,NG_RECORD_MAX);put32(header+20,NG_ARCHIVE_BYTES);put32(header+28,ng_crc32(header,28));
}
/* Initializing archives contain no committed records. Their valid ownership
   marker allows a later write to resume initialization after interruption. */
static int archive_header(int fd,bool created,bool writing)
{
 uint8_t header[32],expected[32];make_archive_header(expected,false);
 if(created && raw_write(fd,expected,32,0)!=32)return -1;
 if(raw_read(fd,header,32,0)!=32)return -1;
 if(!memcmp(header,expected,32)){
  if(!writing)return -2;
  memset(header,0,32);
  for(unsigned id=0;id<=NG_ARCHIVE_LEGACY_IDS;id++)if(raw_write(fd,header,32,NG_ARCHIVE_HEADER+(size_t)id*NG_RECORD_MAX)!=32)return -1;
  make_archive_header(header,true);if(raw_write(fd,header,32,0)!=32)return -1;
 }
 make_archive_header(expected,true);
 return !memcmp(header,expected,32)?0:-1;
}
static int file_open(void *ctx,unsigned id,unsigned slot,bool writing)
{
 bool archive=ctx!=NULL;if((id>NG_ID_MAX && id!=99) || slot>1 || !cleanup())return -1;
 if(archive && id>NG_ARCHIVE_LEGACY_IDS)return -2;
 char name[24];name_for(name,sizeof name,id,slot,archive);bool created;
 int fd=raw_open(name,writing,archive,&created);if(fd<0)return fd;
 record_base=archive?NG_ARCHIVE_HEADER+(size_t)id*NG_RECORD_MAX:0;position=0;record_limit=id==99?NG_DIAG_EXPORT_MAX:NG_RECORD_MAX;
 if(archive){
  int header_rc=archive_header(fd,created,writing);
  if(header_rc<0){int closed=file_close(NULL,fd);if(created && closed==0)(void)raw_remove(name);return header_rc;}
  if(!writing){
   uint8_t header[32];ptrdiff_t n=raw_read(fd,header,sizeof header,record_base);
   if(n!=(ptrdiff_t)sizeof header){file_close(NULL,fd);return -1;}
   bool empty=true;for(unsigned i=0;i<sizeof header;i++)if(header[i])empty=false;
   if(empty){file_close(NULL,fd);return -2;}
   uint32_t length=get32(header+20);if(length<=NG_RECORD_MAX-32)record_limit=length+32;
  }
 }
 return fd;
}
static bool compact_prepare(void *ctx,unsigned id,unsigned slot,size_t bytes)
{(void)ctx;compact_size=bytes;return id<=NG_ID_MAX && slot<=1 && bytes>=32 && bytes<=NG_RECORD_MAX;}
static int compact_open(void *ctx,unsigned id,unsigned slot,bool writing)
{
 (void)ctx;if(id>NG_ID_MAX || slot>1 || !cleanup())return -1;
 char name[24];compact_name(name,sizeof name,id,slot);
 int fd;
 if(writing){if(compact_size<32 || compact_size>NG_RECORD_MAX)return -1;fd=exact_open(name,compact_size);}
 else {bool created=false;fd=raw_open(name,false,false,&created);}
 if(fd<0)return fd;
 compact_writer=writing;
 if(writing)snprintf(compact_write_name,sizeof compact_write_name,"%s",name);
 position=record_base=0;record_limit=NG_RECORD_MAX;return fd;
}
static int compact_close(void *ctx,int fd)
{
 (void)ctx;bool incomplete=compact_writer && position!=compact_size;
 int rc=file_close(NULL,fd);compact_writer=false;
 if(incomplete){
  if(rc==0){if(!raw_remove(compact_write_name)){
   snprintf(pending_remove_name,sizeof pending_remove_name,"%s",compact_write_name);rc=-1;
  }}
  else snprintf(pending_remove_name,sizeof pending_remove_name,"%s",compact_write_name);
 }
 return rc;
}
static int state_open(void *ctx,unsigned id,unsigned slot,bool writing)
{
 (void)ctx;if(id || slot>1 || !cleanup())return -1;
 char name[24];state_name(name,sizeof name,slot);
 int fd;
 if(writing){if(compact_size<32 || compact_size>NG_RECORD_MAX)return -1;fd=exact_open(name,compact_size);}
 else {bool created=false;fd=raw_open(name,false,false,&created);}
 if(fd<0)return fd;
 compact_writer=writing;
 if(writing)snprintf(compact_write_name,sizeof compact_write_name,"%s",name);
 position=record_base=0;record_limit=NG_RECORD_MAX;return fd;
}
static ptrdiff_t file_read(void *ctx,int fd,void *out,size_t size)
{
 (void)ctx;if(position>=record_limit)return 0;
 if(size>record_limit-position)size=record_limit-position;
 ptrdiff_t n=raw_read(fd,out,size,record_base+position);if(n>0 && (size_t)n<=size)position+=(size_t)n;return n;
}
static ptrdiff_t file_write(void *ctx,int fd,const void *in,size_t size)
{
 (void)ctx;if(size>record_limit-position)return -1;
 ptrdiff_t n=raw_write(fd,in,size,record_base+position);if(n>0 && (size_t)n<=size)position+=(size_t)n;return n;
}
static const NgIO legacy={NULL,file_open,file_read,file_write,file_close,NULL};
static const NgIO archive={(void *)&archive_context,file_open,file_read,file_write,file_close,NULL};
static const NgIO compact={(void *)&compact_context,compact_open,file_read,file_write,compact_close,compact_prepare};
static const NgIO state={(void *)&compact_context,state_open,file_read,file_write,compact_close,compact_prepare};
static bool remove_legacy(unsigned id,unsigned slot)
{
 /* Only a semantically valid, owned record may be removed, after two verified copies. */
 if(!ng_slot_valid(&legacy,id,slot))return false;
 char name[24];name_for(name,sizeof name,id,slot,false);return raw_remove(name);
}
static bool load_ok(int rc){return rc==NG_LOAD_OK || rc==NG_LOAD_RECOVERED;}
static int old_game_load(NgSession *s,unsigned id)
{
 int rc=id<=NG_ARCHIVE_LEGACY_IDS?ng_load_io(s,id,&archive):NG_LOAD_ABSENT;
 if(!load_ok(rc) && id<=30){int old=ng_load_io(s,id,&legacy);if(load_ok(old))return old;}
 return rc;
}
static bool remove_old_archives(void)
{
 bool ok=true;
 for(unsigned slot=0;slot<2;slot++){
  char name[24];name_for(name,sizeof name,0,slot,true);bool created=false;
  int fd=raw_open(name,false,true,&created);
  if(fd==-2)continue;
  if(fd<0){ok=false;continue;}
  int owned=archive_header(fd,false,false);int closed=file_close(NULL,fd);
  if(owned || closed || !raw_remove(name))ok=false;
 }
 return ok;
}
static bool old_namespace_present(void)
{
 bool created=false;
 for(unsigned slot=0;slot<2;slot++){
  char name[24];name_for(name,sizeof name,0,slot,true);
  int fd=raw_open(name,false,true,&created);
  if(fd>=0){(void)file_close(NULL,fd);return true;}
  if(fd!=-2)return true;
 }
 for(unsigned id=0;id<=30;id++)for(unsigned slot=0;slot<2;slot++){
  char name[24];name_for(name,sizeof name,id,slot,false);
  int fd=raw_open(name,false,false,&created);
  if(fd>=0){(void)file_close(NULL,fd);return true;}
  if(fd!=-2)return true;
 }
 return false;
}
static bool compact_delete(unsigned id)
{
 bool ok=true;
 for(unsigned slot=0;slot<2;slot++){
  char name[24];compact_name(name,sizeof name,id,slot);bool created=false;
  int fd=raw_open(name,false,false,&created);
  if(fd==-2)continue;
  if(fd<0){ok=false;continue;}
  int closed=file_close(NULL,fd);
  if(closed || !raw_remove(name))ok=false;
 }
 return ok;
}
static bool retire_old_namespace(const NgSettings *options)
{
 if(!old_namespace_present())return true;
 for(unsigned i=0;i<options->recent_count;i++)for(unsigned slot=0;slot<2;slot++)
  if(!ng_slot_valid(&compact,options->recent[i],slot))return false;
 for(unsigned slot=0;slot<2;slot++)if(!ng_slot_valid(&compact,0,slot))return false;
 bool ok=remove_old_archives();
 for(unsigned id=0;id<=30;id++)for(unsigned slot=0;slot<2;slot++){
  if(ng_slot_valid(&legacy,id,slot) && !remove_legacy(id,slot))ok=false;
 }
 return ok;
}
static bool migrate(NgSession *workspace)
{
 NgSettings options;int current=ng_settings_load_io(&options,&compact);
 if(load_ok(current)){
  if(options.pending_delete){
   if(!compact_delete(options.pending_delete))return false;
   options.pending_delete=0;
   if(!ng_settings_save_io(&options,&compact))return false;
  }
  if(options.migration_complete)return true;
  if(!old_namespace_present()){
   options.migration_complete=1;
   return ng_settings_save_io(&options,&compact);
  }
  for(unsigned i=0;i<options.recent_count;i++){
   unsigned id=options.recent[i];
   if(ng_slot_valid(&compact,id,0) && ng_slot_valid(&compact,id,1))continue;
   if(!load_ok(old_game_load(workspace,id)))return false;
   for(unsigned copy=0;copy<2;copy++)if(!ng_save_io(workspace,&compact))return false;
  }
  if(!retire_old_namespace(&options))return false;
  options.migration_complete=1;
  return ng_settings_save_io(&options,&compact);
 }
 if(current==NG_LOAD_IO_ERROR)return false;
 memset(&options,0,sizeof options);
 memset(options.difficulty,1,sizeof options.difficulty);
 options.mode[25]=1;options.show_time=1;options.target=24;
 if(!old_namespace_present()){
  options.migration_complete=1;
  return ng_settings_save_io(&options,&compact) && ng_settings_save_io(&options,&compact);
 }
 int prior=ng_settings_load_io(&options,&archive);
 if(!load_ok(prior)){int legacy_rc=ng_settings_load_io(&options,&legacy);if(load_ok(legacy_rc))prior=legacy_rc;}
 typedef struct {uint8_t id;uint32_t generation;} Candidate;
 Candidate candidates[30];unsigned count=0;
 for(unsigned ordinal=0;ordinal<30;ordinal++){
  unsigned id=ordinal<28?ordinal+1:ordinal+3;
  if(load_ok(old_game_load(workspace,id)) && !(id>=21 && id<=25 && workspace->game.mode==2))
   candidates[count++]=(Candidate){(uint8_t)id,workspace->generation};
 }
 if(!count && !load_ok(prior))return true;
 for(unsigned i=0;i<count;i++)for(unsigned j=i+1;j<count;j++){
  bool swap=(candidates[j].id==options.last_game && candidates[i].id!=options.last_game) ||
   (candidates[i].id!=options.last_game && candidates[j].id!=options.last_game &&
    candidates[j].generation>candidates[i].generation);
  if(swap){Candidate tmp=candidates[i];candidates[i]=candidates[j];candidates[j]=tmp;}
 }
 options.recent_count=0;memset(options.recent,0,sizeof options.recent);
 for(unsigned i=0;i<count && options.recent_count<NG_RECENT_LIMIT;i++){
  unsigned id=candidates[i].id;
  if(!load_ok(old_game_load(workspace,id)))continue;
  if(!ng_save_io(workspace,&compact) || !ng_save_io(workspace,&compact))return false;
  if(!ng_slot_valid(&compact,id,0) || !ng_slot_valid(&compact,id,1))return false;
  options.recent[options.recent_count++]=(uint8_t)id;
 }
 if(options.recent_count && ng_catalog_index(options.last_game)<0)options.last_game=options.recent[0];
 if(!ng_settings_save_io(&options,&compact) || !ng_settings_save_io(&options,&compact))return false;
 if(!retire_old_namespace(&options))return false;
 options.migration_complete=1;
 return ng_settings_save_io(&options,&compact);
}
static bool old_settings_marker_present(void)
{
 bool created=false;
 for(unsigned slot=0;slot<2;slot++)for(unsigned kind=0;kind<3;kind++){
  char name[24];
  if(kind==0)compact_name(name,sizeof name,0,slot);
  else name_for(name,sizeof name,0,slot,kind==1);
  int fd=raw_open(name,false,kind==1,&created);
  if(fd>=0){(void)file_close(NULL,fd);return true;}
  if(fd!=-2)return true;
 }
 return false;
}
static bool retire_state_sources(void)
{
 bool ok=remove_old_archives();
 /* NG2xxA/B is an exact private namespace; a corrupt old slot still belongs
  * to us and must not linger once both version-5 copies are verified. */
 for(unsigned id=1;id<=NG_ID_MAX;id++)if(!compact_delete(id))ok=false;
 for(unsigned id=1;id<=30;id++)for(unsigned slot=0;slot<2;slot++)
  if(ng_slot_valid(&legacy,id,slot) && !remove_legacy(id,slot))ok=false;
 if(ok)for(unsigned slot=0;slot<2;slot++){
  if(ng_slot_valid(&legacy,0,slot) && !remove_legacy(0,slot))ok=false;
 }
 if(ok && !compact_delete(0))ok=false;
 return ok;
}
static bool migrate_state(NgSession *workspace)
{
 NgSettings options;bool active=false;
 int current=ng_state_load_io(&options,workspace,&active,&state);
 if(load_ok(current)){
  /* A verified first copy may have survived an interrupted initial migration. */
  if(!ng_state_slot_valid(&state,0) || !ng_state_slot_valid(&state,1))
   if(!ng_state_save_io(&options,workspace,active,&state))return false;
  return old_settings_marker_present()?retire_state_sources():true;
 }
 if(current==NG_LOAD_IO_ERROR)return false;
 memset(&options,0,sizeof options);memset(options.difficulty,1,sizeof options.difficulty);
 options.mode[25]=1;options.show_time=1;options.target=24;
 if(current==NG_LOAD_ABSENT && !old_settings_marker_present()){
  options.migration_complete=1;
  return ng_state_save_io(&options,workspace,false,&state) &&
   ng_state_save_io(&options,workspace,false,&state);
 }
 NgSettings old;int prior=ng_settings_load_io(&old,&compact);
 if(!load_ok(prior)){
  int arch=ng_settings_load_io(&old,&archive);
  if(load_ok(arch))prior=arch;
  else {int legacy_rc=ng_settings_load_io(&old,&legacy);if(load_ok(legacy_rc))prior=legacy_rc;}
 }
 if(load_ok(prior))options=old;
 bool found=false;
 if(load_ok(prior) && options.recent_count){
  for(unsigned i=0;i<options.recent_count;i++){
   unsigned id=options.recent[i];
   int rc=ng_load_io(workspace,id,&compact);
   if(!load_ok(rc))rc=old_game_load(workspace,id);
   if(load_ok(rc) && workspace->game.status==NG_PLAYING &&
    !(id>=21 && id<=25 && workspace->game.mode==2)){
    options.last_game=(uint8_t)id;found=true;break;
   }
  }
 }
 if(!found && load_ok(prior) && !options.recent_count &&
  ng_catalog_index(options.last_game)>=0){
  unsigned id=options.last_game;
  int rc=ng_load_io(workspace,id,&compact);
  if(!load_ok(rc))rc=old_game_load(workspace,id);
  found=load_ok(rc) && workspace->game.status==NG_PLAYING &&
   !(id>=21 && id<=25 && workspace->game.mode==2);
 }
 /* Old archives had no recency list. Use the last game, then save generations. */
 if(!found && (!load_ok(prior) || !options.recent_count)){
  typedef struct {uint8_t id;uint32_t generation;} Candidate;
  Candidate choices[NG_GAME_COUNT];unsigned count=0;
  for(unsigned i=0;i<NG_GAME_COUNT;i++){
   unsigned id=ng_visible_id(i);
   int rc=ng_load_io(workspace,id,&compact);
   if(!load_ok(rc))rc=old_game_load(workspace,id);
   if(load_ok(rc) && workspace->game.status==NG_PLAYING &&
    !(id>=21 && id<=25 && workspace->game.mode==2))
    choices[count++]=(Candidate){(uint8_t)id,workspace->generation};
  }
  for(unsigned i=0;i<count;i++)for(unsigned j=i+1;j<count;j++){
   bool swap=(choices[j].id==options.last_game && choices[i].id!=options.last_game) ||
    (choices[i].id!=options.last_game && choices[j].id!=options.last_game &&
     choices[j].generation>choices[i].generation);
   if(swap){Candidate tmp=choices[i];choices[i]=choices[j];choices[j]=tmp;}
  }
  if(count){unsigned id=choices[0].id;int rc=ng_load_io(workspace,id,&compact);
   if(!load_ok(rc))rc=old_game_load(workspace,id);
   if(load_ok(rc)){options.last_game=(uint8_t)id;found=true;}
  }
 }
 options.recent_count=0;memset(options.recent,0,sizeof options.recent);
 options.pending_delete=0;options.migration_complete=1;
 if(!ng_state_save_io(&options,workspace,found,&state) ||
  !ng_state_save_io(&options,workspace,found,&state))return false;
 NgSettings verified;bool valid_run=false;
 if(!load_ok(ng_state_load_io(&verified,workspace,&valid_run,&state)) ||
  valid_run!=found)return false;
 return retire_state_sources();
}
static bool diagnostic_line(int fd,const char *format,...)
{
 char line[384];va_list args;va_start(args,format);int n=vsnprintf(line,sizeof line,format,args);va_end(args);
 return n>0 && n<(int)sizeof line && file_write(NULL,fd,line,(size_t)n)==n;
}
static bool export_diagnostics(void)
{
 NgDiagnostics *d=&ng_diagnostics;unsigned handles_before=d->handles;d->frozen=true;
 int fd=file_open(NULL,99,0,true);if(fd<0){d->frozen=false;return false;}
 bool ok=diagnostic_line(fd,"NUM GAME diagnostics v3\nmenu=%lu/%lu/%lu handles_before_export=%u peak=%u timers=%u peak=%u owner=common generation=%lu heap_live=%ld heap_free=%ld stack_sample=%lu\n",
  (unsigned long)d->menu_requests,(unsigned long)d->menu_entries,(unsigned long)d->menu_returns,handles_before,d->peak_handles,d->timers,d->peak_timers,(unsigned long)d->callback_generation,(long)d->heap_live,(long)d->heap_free,(unsigned long)(d->stack_base-d->stack_low));
#ifdef NG_DIAGNOSTIC
 ok=ok && diagnostic_line(fd,"coverage=project_function_entry_and_explicit_samples libraries_OS_interrupts_not_instrumented stack_painting=none\n");
 ok=ok && diagnostic_line(fd,"stack_region=%lx..%lx verified=%u low=%lx function=%lx samples=%lu rejected=%lu depth=%u deepest_game=%u operation=%s\n",
  (unsigned long)d->stack_floor,(unsigned long)d->stack_ceiling,d->stack_range_verified,(unsigned long)d->stack_low,(unsigned long)d->stack_function,(unsigned long)d->stack_samples,(unsigned long)d->stack_rejected,d->peak_depth,d->peak_game,ng_diag_operation_name(d->peak_operation));
 ok=ok && diagnostic_line(fd,"app_heap_live=0 app_heap_peak=0 codec_used_peak=%lu capacity=%u (BSS: not extra heap)\n",(unsigned long)d->codec_peak,NG_RECORD_MAX);
 for(unsigned i=0;ok && i<2;i++){
  const NgDiagArena *a=&d->arena[i];
  ok=diagnostic_line(fd,"arena=%s available=%u capacity=%lu used=%lu free=%lu lifetime_peak=%lu overhead_now=%lu blocks=%ld peak_blocks=%ld\n",i?"_ostk":"_uram",a->available,(unsigned long)a->capacity,(unsigned long)a->used,(unsigned long)a->free_bytes,(unsigned long)a->peak_used,(unsigned long)a->overhead,(long)a->blocks,(long)a->peak_blocks);
 }
 ok=ok && diagnostic_line(fd,"VRAM=177408 payload +96 requested margin/alignment; INCLUDED in _ostk. Arena peaks are separate lifetime peaks, NOT summed.\n");
 ok=ok && diagnostic_line(fd,"stress=%lu/1000 failed=%lu running=%u fixture_io=none read_calls=%lu write_calls=%lu read_bytes=%lu write_bytes=%lu\n",
  (unsigned long)d->stress_done,(unsigned long)d->stress_failures,d->stress_running,(unsigned long)d->read_calls,(unsigned long)d->write_calls,(unsigned long)d->read_bytes,(unsigned long)d->write_bytes);
 ok=ok && diagnostic_line(fd,"timing=RTC128Hz; init=module_only; ready=NEW_dispatch_through_first_display_including_prior_checkpoint; p50/p95=latest16; 0 ticks=<7.8125ms, not zero latency\n");
 for(unsigned phase=0;ok && phase<2;phase++)for(unsigned i=1;ok && i<=NG_ID_MAX;i++)if((phase?d->ready[i]:d->load[i]).used){
  const NgDiagTiming *t=phase?&d->ready[i]:&d->load[i];uint16_t sorted[16];memcpy(sorted,t->samples,t->used*sizeof *sorted);
  for(unsigned j=1;j<t->used;j++){uint16_t v=sorted[j];unsigned k=j;while(k && sorted[k-1]>v){sorted[k]=sorted[k-1];k--;}sorted[k]=v;}
  ok=diagnostic_line(fd,"%s_game=%u count=%lu recent=%u p50_ticks=%u p95_ticks=%u max_ticks=%lu total_ticks=%lu\n",phase?"ready":"init",i,(unsigned long)t->count,t->used,sorted[(t->used-1)/2],sorted[(t->used*95+99)/100-1],(unsigned long)t->max,(unsigned long)t->total);
 }
 for(unsigned op=1;ok && op<NGOP_COUNT;op++)ok=diagnostic_line(fd,"operation=%s max_ticks=%lu\n",ng_diag_operation_name(op),(unsigned long)d->operation_max[op]);
#endif
 for(unsigned i=0;ok && i<d->count;i++){
  const NgDiagEvent *e=&d->events[(d->next+NG_DIAG_EVENTS-d->count+i)%NG_DIAG_EVENTS];
  ok=diagnostic_line(fd,"%lu %lu code=%u screen=%u game=%u detail=%u value=%lu\n",(unsigned long)e->sequence,(unsigned long)e->ticks,e->code,e->screen,e->game,e->detail,(unsigned long)e->value);
 }
 if(file_close(NULL,fd)<0)ok=false;
 d->frozen=false;return ok;
}
typedef struct {void *data;unsigned id;int operation;void *extra;bool *flag;} Request;
static int dispatch(void *opaque)
{
 Request *r=opaque;
 switch(r->operation){
 case 0:{int rc=ng_load_io(r->data,r->id,&compact);return rc==NG_LOAD_ABSENT?old_game_load(r->data,r->id):rc;}
 case 1:return ng_save_io(r->data,&compact);
 case 2:{int rc=ng_settings_load_io(r->data,&compact);if(rc!=NG_LOAD_ABSENT)return rc;rc=ng_settings_load_io(r->data,&archive);if(!load_ok(rc)){int old=ng_settings_load_io(r->data,&legacy);if(load_ok(old))return old;}return rc;}
 case 3:return ng_settings_save_io(r->data,&compact);
 case 4:return cleanup();
 case 6:return migrate(r->data);
 case 7:return compact_delete(r->id);
 case 8:return ng_state_load_io(r->data,r->extra,r->flag,&state);
 case 9:return ng_state_save_io(r->data,r->extra,r->flag && *r->flag,&state);
 case 10:return migrate_state(r->data);
 default:return export_diagnostics();
 }
}
static int transaction(Request *r)
{
 NgDiagScope scope=ng_diag_begin(r->operation==0 || r->operation==2 || r->operation==6 || r->operation==8 || r->operation==10?NGOP_LOAD:r->operation==1 || r->operation==3 || r->operation==9?NGOP_SAVE:NGOP_IDLE,r->id);
#ifdef FXCG50
 int result=gint_world_switch(GINT_CALL(dispatch,(void *)r));
#else
 int result=dispatch(r);
#endif
 ng_diag_end(scope);return result;
}
int ng_storage_load(NgSession *s,unsigned id){Request r={.data=s,.id=id,.operation=0};return transaction(&r);}
bool ng_storage_save(NgSession *s){Request r={.data=s,.id=s->game.id,.operation=1};return transaction(&r)!=0;}
int ng_settings_load(NgSettings *s){Request r={.data=s,.operation=2};return transaction(&r);}
bool ng_settings_save(NgSettings *s){Request r={.data=s,.operation=3};return transaction(&r)!=0;}
bool ng_storage_cleanup(void){Request r={.operation=4};return transaction(&r)!=0;}
bool ng_diag_export(void){Request r={.operation=5};return transaction(&r)!=0;}

bool ng_storage_migrate(NgSession *workspace){Request r={.data=workspace,.operation=6};return transaction(&r)!=0;}
bool ng_state_migrate(NgSession *workspace){Request r={.data=workspace,.operation=10};return transaction(&r)!=0;}
int ng_state_load(NgSettings *settings,NgSession *session,bool *active)
{Request r={.data=settings,.extra=session,.flag=active,.operation=8};return transaction(&r);}
bool ng_state_save(NgSettings *settings,const NgSession *session,bool active)
{Request r={.data=settings,.extra=(void *)session,.flag=&active,.operation=9};return transaction(&r)!=0;}
bool ng_storage_delete(unsigned id){if(!id || id>NG_ID_MAX)return false;Request r={.id=id,.operation=7};return transaction(&r)!=0;}
