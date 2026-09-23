# Power and timer audit

One allocated250ms gint timer wakes the sleeping main loop in every screen.
Its callback sets a flag only. RTC16Hz is the allocation-failure fallback. If
both wake sources fail, play is refused with a visible notice; MENU/OFF remain
available. There is no per-game timer allocation or empty busy-wait loop.

Active time uses RTC128Hz deltas with64-bit conversion and fractional remainder,
including thought time without keys. Rules, stats, entry, modal, pause, OS and
power suspension are excluded. A plain dim does not pause. The active HUD is
updated through the actual LCD rectangular-update API once its seconds change;
ordinary untimed wakeups do not redraw the full game. Clock midnight wrap,
long gaps, resume and CPU single-turn behavior are tested.

Idle time advances independently; timer flags, HUD redraw, CPU and saves never
reset it. Actual key DOWN/HOLD resets it, restores a dimmed backlight and is
processed once. A key arriving at the APO boundary takes priority over APO.
Automatic power-off occurs in the main loop after the normal dirty checkpoint,
through the same safe supported OFF hook as SHIFT+AC/ON. Checkpoint failure
does not become a retry loop or a permanent MENU/OFF trap.

The read-only libfxcg-documented OS queries are GetBacklightDuration (syscall
0x12D9, half-minute units) and GetAutoPowerOffTime (0x1E91, minutes). Values are
accepted only for30/60/180seconds and10/60minutes. Unsupported values use the
explicit NUM GAME fallback60seconds/10minutes. The settings page identifies
OS-derived versus fallback policy. No OS setting is written, and no guessed
fixed OS data address is read.

Dim changes the actual LCD PWM through installed gint r61524_get/set at the
brightness register0x5A1, whose purpose is established by the installed driver
and primary hardware code below. Its exact original value is captured and
restored on input or before MENU/OFF. Other LCD/port settings are unchanged.
This is hardware dimming, not a black overlay or a claim of backlight power-off.
Installed GETKEY_BACKLIGHT is compiled for monochrome FX hardware and is not
used as a CG50 auto-dim feature.

Primary references:
- [CASIO fx-CG50 manual, Power Properties](https://www.casio.com/content/dam/casio/global/support/manuals/calculators/pdf/004-en/f/fx-CG50_Soft_v360_EN.pdf)
- [libfxcg system query declarations](https://github.com/Jonimoose/libfxcg/blob/master/include/fxcg/system.h)
- [GetBacklightDuration syscall](https://github.com/Jonimoose/libfxcg/blob/master/libfxcg/syscalls/GetBacklightDuration.S)
- [GetAutoPowerOffTime syscall](https://github.com/Jonimoose/libfxcg/blob/master/libfxcg/syscalls/GetAutoPowerOffTime.S)
- [Utilities author hardware reference](https://github.com/gbl08ma/utilities/blob/master/src/hardwareProvider.cpp)

Only API signatures, syscall identifiers and documented register meaning are
used; no Utilities implementation is bundled. Gint itself remains unchanged.
Actual device brightness/OS version behavior, APO/wake and MENU failure diagnosis
remain **HARDWARE TEST REQUIRED**.

Final boundary regressions also cover160 fractional8-tick RTC wakeups equaling
exactly10,000ms, key-up at APO, and MENU/SHIFT+AC/ON arriving at a phase barrier.
Blocked modifier keys from a previous screen cannot be re-authorized by that
barrier. Native clock/HUD tests use the same main.c as the SH build. The source
is a24-hour RTC tick value: intervals spanning a full day without any scheduler
observation are not inferred as elapsed days and remain a documented limit.
