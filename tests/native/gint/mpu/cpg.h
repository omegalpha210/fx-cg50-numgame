#ifndef MOCK_USB_CPG_H
#define MOCK_USB_CPG_H
typedef struct { struct { unsigned CLKSTP; } USBCLKCR; } MockUsbCpg;
extern MockUsbCpg mock_usb_cpg;
#define SH7305_CPG mock_usb_cpg
#endif
