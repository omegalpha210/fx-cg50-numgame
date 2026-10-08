#ifndef MOCK_USB_REGISTERS_H
#define MOCK_USB_REGISTERS_H
typedef struct { struct { unsigned SCKE; } SYSCFG; struct { unsigned VBSTS; } INTSTS0; } MockUsbRegisters;
extern MockUsbRegisters mock_usb_registers;
#define SH7305_USB mock_usb_registers
#endif
