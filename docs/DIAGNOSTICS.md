# NUM DIAG: device memory and runtime measurements

**Hardware measured peak: NOT MEASURED.** This guide describes instrumentation
that must be run on an actual fx-CG50. Host timing, host stack samples and compiler
frames do not establish device peak RAM or latency. The reported persistent MENU
flashing remains unreproduced and its cause is unknown.

NUM GAME is the normal application. NUM DIAG uses the same game engines and
content, a separate menu identity, and compact `ND2xxA/B.dat` saves instead of
normal `NG2xxA/B.dat` saves. It never migrates or writes normal NG progress.
Each namespace retains at most five game saves plus settings, with two exact-length copies each. Diagnostic function-entry hooks
add code, static counters, stack work and time overhead; its observations describe
the instrumented build. Compare the [normal build report](build-metrics.json),
[diagnostic build report](diagnostic-build-metrics.json) and
[current host sample](PERFORMANCE_36.md) for the measured differences. The older
build-metrics files are historical 30-game baseline reports.

## Controls and bounded stress

Open **Main → F2 SET → F3 DIAG** in NUM DIAG.

| Key | Action |
|---|---|
| F1 RESET | Clear app counters, timing samples and observed stack minimum; retain actual live handles/timers and allocator lifetime peaks |
| F2 EXPORT | Write/replace fixed `NDDIAG.txt`, maximum 24,576 bytes |
| F3 PAGE | Switch Memory and Runtime summaries |
| F4 STRESS / STOP | Start or cancel 1,000 RAM-only fixture operations |
| F6 OK / EXIT | Close and cancel stress |
| MENU / SHIFT+AC/ON | Cancel stress, attempt ordinary dirty checkpoints, then use the existing OS action |

The stress scenario covers visible game IDs cyclically, levels, and modes. Each
step starts and validates a fresh game, performs a pending CPU move and a supported
hint, exercises four undo snapshots when legal, and round-trips encoded state and
CRC. It reuses the existing storage workspace. It never references the player's
session or performs BFile writes for fixtures. Real key input has priority between
steps. Cancellation occurs between bounded operations, not in the middle of an
engine validator or synchronous BFile call. An export is an explicit file write;
there is no automatic per-key disk log.

## Reproducible device procedure

1. Back up all existing progress files. Record model, OS, installed G3A SHA256, free storage, and
   whether launch was cold or resumed. Use disposable ND progress for these tests.
2. Open diagnostics, RESET, run STRESS to `1000/1000`, and EXPORT. Expected fixture
   failures are zero. After export, open handles return to zero; the active common
   timer count is one when a timer backend is available. Repeat three times and
   compare arena live usage, blocks and resource counts for continued growth.
3. RESET again. For each visible game, start EASY/NORMAL/HARD/MASTER in each mode
   where offered. 2048 CLASSIC has only its fixed NORMAL bucket; TARGET offers all
   four levels. For the six LOGIC games also select HELL with F3 on the difficulty row. Repeat
   NEW at least 20 times per setting if measuring a latency distribution. Open
   diagnostics and EXPORT after each group to retain its latest 16 timing samples.
4. Exercise Countdown and Number Mind HINT; LOGIC HELL REVEAL/validation; Numbrix
   and Slitherlink edits/checks; strategy MASTER CPU-FIRST; Sudoku maximum notes
   and four undos; and NEW/EXIT/MENU/OFF checkpoints. Repeat expensive actions
   separately from the 1,000 RAM fixture scenario. Test a cold load and legacy
   migration using backed-up disposable data. Record observed pauses and key
   response. Do not alter a live user's save files to inject failures.
5. Compare reset/export observations, including arena usage and handles, before
   and after repeated MENU/OFF returns. Follow [the hardware retest](HARDWARE_RETEST.md)
   for the original MENU symptom, dim/APO, input barriers and physical flash tests.

The automatic RAM scenario does not simulate actual calculator flash, full-app
MENU/OFF transitions, power cuts, or every terminal solution. These are separate
native adapter, engine, and physical-device tests. Timer ticks are not user
activity and do not reset dim/APO.

## Reading the export

- `stack_region`, `verified`, `low`, `samples`, `rejected`, `deepest_game`, and
  `operation`: observed project stack samples. The SH interval uses the installed
  gint stack floor and the end of its documented user-RAM region; P1/P2 aliases
  are normalized. No stack memory is painted, read or modified to measure depth.
  The function address can be resolved against the matching diagnostic ELF.
- Project C function-entry hooks plus explicit samples cover application calls.
  Uninstrumented SDK/libc/OS routines, assembly and interrupt-only deeper frames
  can be missed. Sampling and its own overhead are **not a worst-case stack bound**.
  `depth` is instrumented call depth, not an algorithmic search-depth proof.
- `app_heap_live=0 app_heap_peak=0` expresses the source ownership fact that app
  code performs no heap allocation. It is not a claim of zero SDK/OS allocation.
  `_uram` and `_ostk` use public gint arena statistics: capacity, current payload
  used/free, allocator lifetime peak, and live/peak blocks. `overhead_now` is
  capacity minus contemporaneous used/free, not original requested payload.
  RESET cannot reset allocator lifetime peaks through the public API.
- The one VRAM payload is 177,408 bytes, requested with a 96-byte alignment/margin
  allowance by gint. It is already included in `_ostk` usage. Never add it again,
  add aliased regions twice, or add BSS codec capacity again as heap usage.
- `codec_used_peak` is bytes actually encoded in the existing 14,000-byte BSS
  buffer; the decoder session is also BSS. Neither is an extra dynamic arena.
- `init_game` measures module initialization only. `ready_game` measures actual
  NEW dispatch through validation and the first displayed frame, including any
  previous-run checkpoint in that dispatch. Entry-time archive reads precede it.
  p50/p95 use the latest 16 observations; count/total/max span the reset interval.
  Clock resolution is RTC 128 Hz: zero ticks means below 7.8125 ms resolution.
- Read/write calls and bytes are native adapter activity, including validation
  reads and explicit exports; RAM fixtures perform no file I/O. Operation maxima
  are separate timings, not a sum or simultaneous resource peak. The 96-event
  ring is bounded and overwrites old entries. Export itself has overhead.

Device load targets (bank approximately 0.5 s; runtime p95 approximately 2 s)
remain unmeasured. High-cost uniqueness/rating generation runs on the host and
ships verified banks. Device constructors use bounded iterations; there is no
wall-clock three-second preemption or inside-operation spinner. No requested HELL
is replaced by HARD. Report actual missed targets before adding a cooperative
long-running generator rather than claiming host results prove device speed.
