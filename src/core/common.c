#include "ng.h"
#include <stdio.h>
#include <string.h>
uint32_t ng_random(NgGame *g)
{
 uint32_t x=g->rng;if(!x)x=0x6d2b79f5u;
 x^=x<<13;x^=x>>17;x^=x<<5;g->rng=x;return x;
}
unsigned ng_rand(NgGame *g,unsigned limit)
{return limit ? ng_random(g)%limit:0;}
unsigned ng_bank_pick(NgGame *g,unsigned count)
{
 if(!count)return 0;
 if(!g->supply_seed)return ng_rand(g,count);
 /* Affine permutation: coprime stride visits every ordinal exactly once.
  * The low 16 bits are an adjustable rotation; high bits choose the stride. */
 unsigned step=1+(g->supply_seed>>16)%count;
 for(;;){unsigned a=step,b=count;while(b){unsigned r=a%b;a=b;b=r;}if(a==1)break;step=step%count+1;}
 return (unsigned)(((uint64_t)(g->supply_index%count)*step+(g->supply_seed&65535u))%count);
}
void ng_message(NgGame *g,const char *message)
{snprintf(g->message,sizeof(g->message),"%s",message);}
bool ng_edit(NgGame *g,int key,const char *allowed,unsigned limit)
{
 size_t n=strlen(g->input);
 if(key==NGK_DEL){if(n)g->input[n-1]=0;return true;}
 if(key>0 && key<128 && strchr(allowed,key)) {
  if(n<limit && n+1<sizeof(g->input)){g->input[n]=(char)key;g->input[n+1]=0;g->message[0]=0;}
  else ng_message(g,"Input limit reached");
  return true;
 }
 return false;
}
void ng_editor_reset(NgGame *g)
{
 size_t length=strlen(g->input);
 g->edit_cursor=(uint8_t)(length<NG_INPUT?length:NG_INPUT-1);
}
bool ng_editor_move(NgGame *g,int key)
{
 size_t length=strlen(g->input);
 if(g->edit_cursor>length)ng_editor_reset(g);
 if(key==NGK_LEFT && g->edit_cursor){g->edit_cursor--;return true;}
 if(key==NGK_RIGHT && g->edit_cursor<length){g->edit_cursor++;return true;}
 return false;
}
/* Cursor-only navigation lives in app.c; this helper edits one bounded draft. */
bool ng_edit_expression(NgGame *g,int key,const char *allowed,unsigned limit)
{
 size_t length=strlen(g->input);
 if(g->edit_cursor>length)ng_editor_reset(g);
 if(key==NGK_DEL){
  if(!g->edit_cursor)return false;
  memmove(g->input+g->edit_cursor-1,g->input+g->edit_cursor,
          length-g->edit_cursor+1);
  g->edit_cursor--;
  return true;
 }
 char square[2]={'^','2'};
 const char *insert=NULL;
 size_t count=0;
 char single=(char)key;
 if(key==NGK_SQUARE && strchr(allowed,'^')){insert=square;count=2;}
 else if(key>0 && key<128 && strchr(allowed,key)){insert=&single;count=1;}
 if(!insert)return false;
 if(limit>=NG_INPUT)limit=NG_INPUT-1;
 if(length+count>limit){ng_message(g,"Input limit reached");return true;}
 memmove(g->input+g->edit_cursor+count,g->input+g->edit_cursor,
         length-g->edit_cursor+1);
 memcpy(g->input+g->edit_cursor,insert,count);
 g->edit_cursor=(uint8_t)(g->edit_cursor+count);
 g->message[0]=0;
 return true;
}
void ng_history(NgGame *g,const char *line)
{
 if(g->history_count==NG_HISTORY) {
  memmove(g->history,g->history+1,(NG_HISTORY-1)*sizeof(g->history[0]));
  g->history_count--;
 }
 snprintf(g->history[g->history_count++],sizeof(g->history[0]),"%s",line);
 g->scroll=0;
}
bool ng_grid_nav(NgGame *g,int key)
{
 if(!g->rows || !g->cols)return false;
 unsigned r=g->cursor/g->cols,c=g->cursor%g->cols;
 if(key==NGK_UP)r=(r+g->rows-1)%g->rows;
 else if(key==NGK_DOWN)r=(r+1)%g->rows;
 else if(key==NGK_LEFT)c=(c+g->cols-1)%g->cols;
 else if(key==NGK_RIGHT)c=(c+1)%g->cols;
 else return false;
 g->cursor=(uint8_t)(r*g->cols+c);return true;
}
