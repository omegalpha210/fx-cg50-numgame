#ifndef MOCK_USB_POWER_H
#define MOCK_USB_POWER_H
typedef struct { struct { unsigned USB0; } MSTPCR2; } MockUsbPower;
extern MockUsbPower mock_usb_power;
#define SH7305_POWER mock_usb_power
#endif
