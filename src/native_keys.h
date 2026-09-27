#ifndef NUMGAME_NATIVE_KEYS_H
#define NUMGAME_NATIVE_KEYS_H
#include "ng.h"
#include <gint/keyboard.h>

/* Physical matrix to one logical action. Keep modifier and square distinct. */
typedef struct {int native,logical;} NgNativeKey;
static const NgNativeKey ng_native_keys[]={
 {KEY_0,'0'},{KEY_1,'1'},{KEY_2,'2'},{KEY_3,'3'},{KEY_4,'4'},
 {KEY_5,'5'},{KEY_6,'6'},{KEY_7,'7'},{KEY_8,'8'},{KEY_9,'9'},
 {KEY_ADD,'+'},{KEY_SUB,'-'},{KEY_MUL,'*'},{KEY_DIV,'/'},
 {KEY_LEFTP,'('},{KEY_RIGHTP,')'},{KEY_DOT,'.'},{KEY_POWER,'^'},
 {KEY_UP,NGK_UP},{KEY_RIGHT,NGK_RIGHT},{KEY_DOWN,NGK_DOWN},
 {KEY_LEFT,NGK_LEFT},{KEY_EXE,NGK_EXE},{KEY_DEL,NGK_DEL},
 {KEY_F1,NGK_F1},{KEY_F2,NGK_F2},{KEY_F3,NGK_F3},
 {KEY_F4,NGK_F4},{KEY_F5,NGK_F5},{KEY_F6,NGK_F6},
 {KEY_EXIT,NGK_EXIT},{KEY_MENU,NGK_MENU},{KEY_SHIFT,NGK_SHIFT},
 {KEY_ALPHA,NGK_ALPHA},{KEY_ACON,NGK_ACON},{KEY_NEG,'-'},
 {KEY_SQUARE,NGK_SQUARE}
};
static inline int ng_native_key_lookup(int native)
{
 for(unsigned i=0;i<sizeof ng_native_keys/sizeof ng_native_keys[0];i++)
  if(ng_native_keys[i].native==native)return ng_native_keys[i].logical;
 return 0;
}
#endif
