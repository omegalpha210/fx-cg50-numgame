/* Read unchanged beta.6 v5 payloads using the packed-bank runtime. */
#include "storage.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static NgSession session;
static uint8_t before[NG_RECORD_MAX],after[NG_RECORD_MAX];
static void check(const char *path,unsigned random)
{
 FILE *file=fopen(path,"rb");assert(file);
 size_t length=fread(before,1,sizeof before,file);assert(!ferror(file));
 assert(!fclose(file)&&length>0&&length<sizeof before);
 assert(ng_single_decode(&session,before,length,6));
 assert(ng_valid(&session.game));
 assert(session.game.pack_revision==6&&session.game.data[2]==(int32_t)random);
 assert(ng_single_encode(&session,after,sizeof after)==length);
 assert(!memcmp(before,after,length));
 const NgModule *module=ng_module(6);assert(module);
 assert(module->action(&session.game,NGK_HINT));
 assert(!strncmp(session.game.message,"One first step:",15));
 assert(module->action(&session.game,NGK_ANSWER));
 assert(session.game.input[0]&&ng_valid(&session.game));
 printf("PASS old %s payload %zu bytes, ID %u, HINT/ANSWER\n",
        random?"RANDOM":"FIXED",length,session.game.puzzle_id);
}
int main(int argc,char **argv)
{
 assert(argc==3);
 check(argv[1],0);check(argv[2],1);
 return 0;
}
