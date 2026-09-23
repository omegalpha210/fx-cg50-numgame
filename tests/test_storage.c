#include "support.h"
static TestDisk disk;
static NgSession original,loaded;
static uint8_t encoded[NG_RECORD_MAX],again[NG_RECORD_MAX];
static void same(const NgSession *a,const NgSession *b)
{size_t n=ng_encode(a,encoded,sizeof(encoded)),m=ng_encode(b,again,sizeof(again));assert(n && n==m && !memcmp(encoded,again,n));}
static void header_word(unsigned id,unsigned slot,unsigned offset,uint32_t value)
{
 uint8_t *p=disk.bytes[id][slot];
 for(unsigned i=0;i<4;i++)p[offset+i]=(uint8_t)(value>>(8*i));
 uint32_t crc=ng_crc32(p,28);
 for(unsigned i=0;i<4;i++)p[28+i]=(uint8_t)(crc>>(8*i));
}
int main(void)
{
 /* Exercise shared workflow helpers so this header stays strict-warning clean. */
 static NgApp app;disk.write_budget=-1;ng_app_init(&app,test_hooks(&disk),321);open_game(&app,21);
 NgIO io=test_io(&disk);unsigned cases=0;
 for(unsigned id=1;id<=NG_ID_MAX;id++)for(unsigned difficulty=0;difficulty<ng_difficulty_count(id);difficulty++)for(unsigned mode=0;mode<ng_module(id)->modes;mode++) {
  memset(&original,0,sizeof(original));ng_new(&original.game,id,difficulty,mode,1000+id*100+difficulty*10+mode,1);original.stats.started=1;
  if(!ng_valid(&original.game)){fprintf(stderr,"Invalid initial %u d%u m%u\n",id,difficulty,mode);return 1;}
  original.undo_count=original.game.cpu_pending || !(ng_module(id)->flags&NGF_UNDO)?0:4;for(unsigned i=0;i<original.undo_count;i++)original.undo[i]=original.game;
  assert(ng_save_io(&original,&io));assert(ng_load_io(&loaded,id,&io)==NG_LOAD_OK);same(&original,&loaded);cases++;
  loaded.game.difficulty=(uint8_t)ng_difficulty_count(id);assert(!ng_save_io(&loaded,&io));
 }
 /* Save A then B. Truncate/checksum-corrupt newest; fall back with exact RNG. */
 memset(&disk,0,sizeof(disk));disk.write_budget=-1;memset(&original,0,sizeof(original));ng_new(&original.game,26,1,0,123,1);original.stats.started=1;
 assert(ng_save_io(&original,&io));NgGame initial=original.game;
 assert(ng_module(26)->action(&original.game,NGK_LEFT));assert(ng_save_io(&original,&io));
 disk.bytes[26][1][100]^=1;assert(ng_load_io(&loaded,26,&io)==NG_LOAD_RECOVERED);assert(!memcmp(&initial,&loaded.game,sizeof(initial)));
 disk.write_budget=61;assert(!ng_save_io(&original,&io));disk.write_budget=-1;
 assert(ng_load_io(&loaded,26,&io)==NG_LOAD_RECOVERED);assert(loaded.game.rng==initial.rng);
 disk.fail_close=true;assert(!ng_save_io(&original,&io));disk.fail_close=false;
 assert(ng_load_io(&loaded,26,&io)==NG_LOAD_RECOVERED);
 disk.fail_read=true;assert(ng_load_io(&loaded,26,&io)==NG_LOAD_IO_ERROR);disk.fail_read=false;
 /* Recomputed header checksums prove version rejection, wrap-safe newest
    selection, and that unsupported versions cannot reset a different game. */
 memset(&disk,0,sizeof(disk));disk.write_budget=-1;original.game=initial;
 assert(ng_save_io(&original,&io));header_word(26,0,16,UINT32_MAX-1);
 original.game.elapsed_ms=11;assert(ng_save_io(&original,&io));assert(original.generation==UINT32_MAX);
 assert(ng_load_io(&loaded,26,&io)==NG_LOAD_OK && loaded.game.elapsed_ms==11);
 original.game.elapsed_ms=22;assert(ng_save_io(&original,&io));assert(original.generation==0);
 assert(ng_load_io(&loaded,26,&io)==NG_LOAD_OK && loaded.generation==0 && loaded.game.elapsed_ms==22);
 memset(&loaded,0,sizeof(loaded));ng_new(&loaded.game,21,1,0,456,2);loaded.stats.started=1;assert(ng_save_io(&loaded,&io));
 header_word(26,0,12,0);assert(ng_load_io(&loaded,26,&io)==NG_LOAD_RECOVERED && loaded.game.elapsed_ms==11);
 header_word(26,1,12,99);assert(ng_load_io(&loaded,26,&io)==NG_LOAD_INVALID);
 assert(ng_load_io(&loaded,21,&io)==NG_LOAD_OK && loaded.game.seed==456);
 /* Exact length and explicit field checks survive a recomputed outer checksum. */
 size_t n=ng_encode(&original,encoded,sizeof(encoded));assert(n);
 encoded[0]=5;assert(!ng_decode(&loaded,encoded,n,26));encoded[0]=0;
 assert(!ng_decode(&loaded,encoded,n-1,26));assert(!ng_decode(&loaded,encoded,n,25));
 n=ng_encode(&original,encoded,sizeof(encoded));assert(n==1005u+1842u*(1u+original.undo_count));
 original.stats.best[0][1][0].best_aux=31;
 size_t compact_n=ng_encode(&original,again,sizeof(again));assert(compact_n==n && !memcmp(encoded,again,n));
 original.stats.best[0][1][0].best_aux=0;
 memset(&disk,0,sizeof(disk));disk.write_budget=-1;
 NgSettings settings={.last_game=26,.show_time=1,.migration_complete=1,.target=1000};assert(ng_settings_save_io(&settings,&io));NgSettings cold={0};assert(ng_settings_load_io(&cold,&io)==NG_LOAD_OK);assert(cold.last_game==26 && cold.target==1000);
 settings.mode[0]=255;assert(!ng_settings_save_io(&settings,&io));
 printf("Storage: %u game/mode/difficulty round trips, CRC, truncation, IO, version isolation, generation wrap, settings PASS\n",cases);
 return 0;
}
