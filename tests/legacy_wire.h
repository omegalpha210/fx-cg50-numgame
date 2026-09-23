#ifndef NG_TEST_LEGACY_WIRE_H
#define NG_TEST_LEGACY_WIRE_H
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
/* Independent fixture writer for the published v1/v2 byte layout. It drops
 * modern metadata; no production decoder or raw C structs define this view. */
static size_t legacy_wire(uint8_t *old,const uint8_t *current,size_t length,unsigned levels)
{
 assert(levels==3 || levels==4);unsigned games=1u+current[0];
 bool compact=length==1005u+1842u*games;
 assert(games<=5 && (compact || length==3569u+1842u*games));
 if(compact){memcpy(old,current,5);memset(old+5,0,4);}else memcpy(old,current,9);
 size_t at=9;
 for(unsigned mode=0;mode<8;mode++){
  if(compact)memset(old+at,0,levels*64);
  else memcpy(old+at,current+9+mode*320,levels*64);
  at+=levels*64;
 }
 for(unsigned i=0;i<games;i++){
  const uint8_t *g=current+(compact?1005u:3569u)+i*1842;
  memcpy(old+at,g,44);memcpy(old+at+44,g+60,1782);at+=1826;
 }
 return at;
}
#endif
