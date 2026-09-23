#ifndef NG_TEST_LEGACY_WIRE_H
#define NG_TEST_LEGACY_WIRE_H
#include <assert.h>
#include <stdint.h>
#include <string.h>
/* Independent fixture writer for the published v1/v2 byte layout. It drops
 * modern metadata; no production decoder or raw C structs define this view. */
static size_t legacy_wire(uint8_t *old,const uint8_t *current,size_t length,unsigned levels)
{
 assert(levels==3 || levels==4);unsigned games=1u+current[0];
 assert(games<=5 && length==3569u+1842u*games);
 memcpy(old,current,9);size_t at=9;
 for(unsigned mode=0;mode<8;mode++){
  memcpy(old+at,current+9+mode*320,levels*64);at+=levels*64;
 }
 for(unsigned i=0;i<games;i++){
  const uint8_t *g=current+3569+i*1842;
  memcpy(old+at,g,44);memcpy(old+at+44,g+60,1782);at+=1826;
 }
 return at;
}
#endif
