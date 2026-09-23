#include "storage.h"
#include <stdio.h>
#include <string.h>
static void quote(const char *s)
{putchar('"');for(;s&&*s;s++){if(*s=='"'||*s=='\\')putchar('\\');if(*s=='\n')fputs("\\n",stdout);else putchar(*s);}putchar('"');}
int main(void)
{
 printf("{\"game_size_host\":%zu,\"session_size_host\":%zu,\"games\":[",sizeof(NgGame),sizeof(NgSession));
 for(unsigned index=0;index<NG_GAME_COUNT;index++){
  unsigned id=ng_visible_id(index);const NgModule *m=ng_module(id);if(index)putchar(',');printf("{\"id\":%u,\"name\":",id);quote(m->name);
  printf(",\"category\":%u,\"slot\":%u,\"difficulties\":%u,\"regular_difficulties\":%u,\"has_hell\":%s,\"generation_policy\":%u,\"flags\":%u,\"aux\":",index/5,index%5,ng_difficulty_count(id),ng_regular_difficulty_count(id),ng_has_hell(id)?"true":"false",ng_generation_policy(id),m->flags);quote(m->aux_label);fputs(",\"primary\":",stdout);quote(m->primary_label);
  fputs(",\"rules\":",stdout);quote(m->rules);fputs(",\"modes\":[",stdout);
  for(unsigned mode=0;mode<m->modes;mode++){if(mode)putchar(',');quote(m->mode_name?m->mode_name(mode):"STANDARD");}fputs("],\"bank_counts_by_mode\":[",stdout);
  for(unsigned mode=0;mode<m->modes;mode++){
   if(mode)putchar(',');putchar('[');for(unsigned level=0;level<ng_difficulty_count(id);level++){if(level)putchar(',');printf("%u",ng_bank_count(id,level,mode));}putchar(']');
  }fputs("]}",stdout);
 }
 puts("]}");return 0;
}
