#ifndef NG_NATIVE_TEST_KEYBOARD_H
#define NG_NATIVE_TEST_KEYBOARD_H
#include <stdbool.h>
/* Numeric constants checked against the installed gint keycodes.h. */
enum {KEY_F1=0x91,KEY_F2=0x92,KEY_F3=0x93,KEY_F4=0x94,KEY_F5=0x95,KEY_F6=0x96,
 KEY_SHIFT=0x81,KEY_MENU=0x84,KEY_LEFT=0x85,KEY_UP=0x86,KEY_ALPHA=0x71,
 KEY_POWER=0x73,KEY_EXIT=0x74,KEY_DOWN=0x75,KEY_RIGHT=0x76,
 KEY_FD=0x52,KEY_LEFTP=0x53,KEY_RIGHTP=0x54,
 KEY_7=0x41,KEY_8=0x42,KEY_9=0x43,KEY_DEL=0x44,
 KEY_4=0x31,KEY_5=0x32,KEY_6=0x33,KEY_MUL=0x34,KEY_DIV=0x35,
 KEY_1=0x21,KEY_2=0x22,KEY_3=0x23,KEY_ADD=0x24,KEY_SUB=0x25,
 KEY_0=0x11,KEY_DOT=0x12,KEY_NEG=0x14,KEY_EXE=0x15,KEY_ACON=0x07};
enum {KEYEV_NONE,KEYEV_DOWN,KEYEV_UP,KEYEV_HOLD};
typedef struct {unsigned time:16,reserved:2,mod:1,shift:1,alpha:1,type:3,key:8;} key_event_t;
void clearevents(void);
bool keydown(int key);
#endif
