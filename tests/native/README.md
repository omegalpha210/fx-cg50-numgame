# Native adapter host regression

Run `tests/native/run.sh` from the repository. It compiles the actual production
`src/main.c` (included with main renamed) and `src/storage/native.c` with
`FXCG50`, strict C11 warnings, assertions, and UBSan. All other production core,
codec, renderer and game code is linked. No substitute application or storage
adapter is tested.

The small gint declarations are original test fixtures. Function signatures,
key matrix constants, 128Hz/day RTC semantics, timer frequency, and Fugue BFile
return conventions were checked against the already-installed gint headers.
The mock callback descriptor implements the two production callback signatures;
it does not emulate the SH calling convention. Physical events enter the actual
native map/keydev loop, and mock held-key state is reconstructed by the actual
native release barrier. Scripted future physical events have not yet entered the
device event queue, so `clearevents` does not discard them.

Assertions cover:

- Native `NG00A/B.dat` settings and `NG01A/B.dat`–`NG30A/B.dat` namespace only.
- World switch surrounds BFile operations; OS MENU/OFF run outside it.
- Two-slot generation, exact EOF bounds for Fugue's permissive reads, interrupted
  writes, zero-byte writes, readback corruption, read errors, and bounded close
  failure with later descriptor cleanup. A good slot remains readable.
- Checkpoint finishes before MENU and SHIFT+AC/ON; ordinary AC/ON, a consumed
  SHIFT modifier, and ALPHA+SHIFT+AC/ON do not power off.
- Failed checkpoint preserves RAM, reports the error, and still performs the
  requested MENU/OFF exactly once, without an infinite retry.
- CPU pending processing uses the actual no-event branch exactly once.
- Timed wakeups, pause/resume, memory exposure and conceal barriers, and a held
  physical digit that cannot enter after the hidden-input transition.
- 2048 directional HOLD cannot make a second move or consume RNG.
- Midnight RTC wrap, 83 minutes of active untimed inactivity without 32-bit
  tick multiplication overflow, and exclusion of time spent in the OS.
- Gint timer allocation failure falls back to RTC 16Hz; failure of both clock sources checkpoints
  and refuses timed play instead of displaying a memory sequence indefinitely.

These are host tests of production adapters, not hardware tests. They do not
establish the real calculator's syscall timing, power-loss behavior, installed
OS/Fugue stability, physical key scanning, or SDK/firmware ABI behavior.

Independent shared-code review regressions are part of the default run:
RESULT → RULES → EXIT returns to the result; Main RESUME cannot clear a failed
checkpoint or replace the previous active run; invalid tile exponents, outcome
counts, cross-bucket totals and empty-record fields are rejected; a deliberately
malformed native record with recomputed valid CRCs is rejected before rendering;
terminal and CPU-pending undo snapshots are rejected. The optional
`NG_NATIVE_REPRO_STATS=1 tests/native/run.sh` isolates the tile-exponent case.
