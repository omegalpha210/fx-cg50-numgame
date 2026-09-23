/* Scalar OS queries documented by libfxcg's system.h and syscall stubs.
   No OS settings are written and no fixed OS data address is read. */
.text
.align 2
.global _ng_os_backlight_duration
_ng_os_backlight_duration:
 mov.l .Lbl,r0
 mov.l .Lsys,r2
 jmp @r2
 nop
.global _ng_os_apo_minutes
_ng_os_apo_minutes:
 mov.l .Lapo,r0
 mov.l .Lsys,r2
 jmp @r2
 nop
.align 2
.Lbl: .long 0x12d9
.Lapo: .long 0x1e91
.Lsys: .long 0x80020070
