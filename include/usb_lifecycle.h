#ifndef CG_USB_LIFECYCLE_H
#define CG_USB_LIFECYCLE_H
#include <stdbool.h>
/* Foreground-only edge latch. Unknown samples never invent an unplug.
   The first valid sample is a baseline, including launch with a cable attached. */
typedef struct { bool known,connected,pending,handling; } UsbLifecycle;
static inline void usb_observe(UsbLifecycle *s,int sample)
{
    if(sample<0)return;
    bool connected=sample!=0;
    if(s->known && !s->connected && connected && !s->handling)s->pending=true;
    s->known=true;s->connected=connected;
}
static inline void usb_initialize(UsbLifecycle *s,int sample)
{*s=(UsbLifecycle){0};usb_observe(s,sample);}
static inline bool usb_take_request(UsbLifecycle *s)
{bool pending=s->pending;s->pending=false;return pending;}
/* A normal MENU/OFF also absorbs any insertion during its checkpoint.
   This guards reentry; it never calls OS/storage code itself. */
static inline bool usb_handoff_begin(UsbLifecycle *s,int sample)
{
    if(s->handling)return false;
    usb_observe(s,sample);s->pending=false;s->handling=true;return true;
}
static inline void usb_handoff_end(UsbLifecycle *s,int sample)
{
    usb_observe(s,sample);s->pending=false;s->handling=false;
}
#endif
