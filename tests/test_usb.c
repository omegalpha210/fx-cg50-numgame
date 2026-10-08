#include "usb_lifecycle.h"
#include "usb_native.h"
#include <assert.h>
#include <stdio.h>
MockUsbCpg mock_usb_cpg;
MockUsbPower mock_usb_power;
MockUsbRegisters mock_usb_registers;
int main(void)
{
    for(unsigned clk=0;clk<2;clk++)for(unsigned stopped=0;stopped<2;stopped++)
    for(unsigned scke=0;scke<2;scke++)for(unsigned cable=0;cable<2;cable++){
        mock_usb_cpg.USBCLKCR.CLKSTP=clk;mock_usb_power.MSTPCR2.USB0=stopped;
        mock_usb_registers.SYSCFG.SCKE=scke;mock_usb_registers.INTSTS0.VBSTS=cable;
        int expected=clk || stopped || !scke ? -1:(int)cable;
        assert(usb_native_sample()==expected);
    }
    UsbLifecycle state;usb_initialize(&state,0);
    usb_observe(&state,1);assert(usb_take_request(&state));assert(!usb_take_request(&state));
    for(unsigned i=0;i<10000;i++){usb_observe(&state,1);assert(!state.pending);}
    usb_observe(&state,-1);usb_observe(&state,1);assert(!state.pending);
    usb_observe(&state,0);usb_observe(&state,1);assert(usb_take_request(&state));
    /* Absorb insertions before/during a normal MENU/OFF/save transition. */
    assert(usb_handoff_begin(&state,1));assert(!usb_handoff_begin(&state,1));
    usb_observe(&state,0);usb_observe(&state,1);assert(!state.pending);
    usb_handoff_end(&state,1);usb_observe(&state,1);assert(!state.pending);
    usb_observe(&state,0);usb_observe(&state,1);assert(state.pending);
    /* Failed checkpoint consumes one request, without an automatic retry loop. */
    assert(usb_take_request(&state));usb_observe(&state,1);assert(!state.pending);
    usb_initialize(&state,1);usb_observe(&state,1);assert(!state.pending);
    usb_initialize(&state,-1);usb_observe(&state,1);assert(!state.pending);
    usb_observe(&state,0);usb_observe(&state,1);assert(state.pending);
    puts("USB policy PASS: 16 clock/VBUS samples; one insertion, 10000 held samples, rearm, unknown, launch-connected, reentry/races and finite failure. HARDWARE TEST REQUIRED.");
    return 0;
}
