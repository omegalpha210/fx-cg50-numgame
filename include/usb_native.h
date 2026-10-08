#ifndef CG_USB_NATIVE_H
#define CG_USB_NATIVE_H
#include <gint/mpu/cpg.h>
#include <gint/mpu/power.h>
#include <gint/mpu/usb.h>
/* Installed gint 2.11: usb.c:hpowered and mpu/usb.h:VBSTS.
   Read-only observation, never usb_open(), register writes or a new ISR.
   Powered-off/clock-disabled hardware is UNKNOWN, not disconnected.
   This is a guarded register adapter, not a public gint cable-event helper.
   See docs/USB_LIFECYCLE_AUDIT.md. Actual detection is HARDWARE TEST REQUIRED. */
static inline int usb_native_sample(void)
{
    if(SH7305_CPG.USBCLKCR.CLKSTP || SH7305_POWER.MSTPCR2.USB0)return -1;
    if(!SH7305_USB.SYSCFG.SCKE)return -1;
    return SH7305_USB.INTSTS0.VBSTS ? 1:0;
}
#endif
