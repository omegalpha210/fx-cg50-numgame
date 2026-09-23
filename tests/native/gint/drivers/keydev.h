#ifndef NG_NATIVE_TEST_KEYDEV_H
#define NG_NATIVE_TEST_KEYDEV_H
#include <gint/keyboard.h>
typedef struct {int unused;} keydev_t;
typedef struct {int enabled;int (*repeater)(int,int,int);} keydev_transform_t;
enum {KEYDEV_TR_REPEATS=0x10};
keydev_t *keydev_std(void);
void keydev_set_transform(keydev_t *device,keydev_transform_t transform);
key_event_t keydev_read(keydev_t *device,bool wait,volatile int *timeout);
#endif
