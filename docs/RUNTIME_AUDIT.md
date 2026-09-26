# Runtime audit

This records the earlier runtime investigation and its historical storage/UI
state. The current single-resume storage and controls are documented in
[STORAGE_FORMAT.md](STORAGE_FORMAT.md) and [CONTROLS.md](CONTROLS.md).

Baseline ad9c34a is tagged backup/pre-runtime-board-20260922. Native device
observation: MENU flashes after switching games, OFF/ON does not resolve it,
rear RESET was needed. **The physical symptom has not been reproduced here and
its cause is not confirmed.** Host mocks cannot reproduce CASIO firmware.

| ID | Evidence / action | Regression / limit |
|---|---|---|
| RT01 | Untimed games passed no timeout to blocking keydev_read. Their elapsed time accumulated on the next key but HUD did not refresh while thinking. One common250ms scheduler now wakes all screens. | Real main mock:60s no-key elapsed/HUD; timer peak1. |
| RT02 | No application idle dim/APO service existed. Game time and user inactivity are now separate. | Real main:dim/restore, key once, pre-APO key, checkpointed APO, OS time exclusion. |
| RT03 | Failed BFile close retains one known handle for bounded later retry. Cleanup is now attempted before MENU/OFF and counts are visible. | Injected close/write/read errors; RAM retained; MENU/OFF still called. Permanent hardware close failure remains a device diagnosis. |
| RT04 | No software MENU failure found in baseline dispatcher: MENU precedes every modal and calls the hook even after a failed checkpoint. No getkey automatic MENU is used. |1000 real-handler native-mock game switches and MENU calls; bounded resources. Not1000 physical OS switches. |
| RT05 | Installed gint osmenu.c tries an internal OS route, then an OS timer fallback; failed timer allocation returns immediately. | Plausible explanation for a flash, **not established as this device's cause**. SDK is unchanged. RAM diagnostics record requests/entry/return. |

There is one flat main loop; screens do not call one another recursively. No
project malloc/calloc/realloc calls or per-game hardware timers exist. CPU work
is bounded in retained engines; timer callbacks only set a wake flag. Before
OS/off the common wake timer is paused, queued input cleared, dimmed hardware
brightness restored, and pending file cleanup attempted. Existing gint supported
gint_osmenu/gint_poweroff are used. OFF returns to the same suspended call and
RAM state; it is not treated as a cold restart or rear RESET.

Diagnostics:96 fixed16-byte events plus fixed counters in RAM. Settings→F3 DIAG
shows MENU request/entry/return, current/peak handles and timers, heap statistics
where the installed allocator supplies them, and sampled stack displacement.
The stack number is a sample, not a high-water bound. F2 EXPORT explicitly
writes one fixed diagnostic text file; no per-key log files are created.
Clock callbacks do not touch the ring or file system. Event code definitions are
in include/diagnostics.h; the export contains screen/game/detail/value fields.

Baseline saving used NG00A/B.dat preferences and NG01..30A/B.dat game records:
max62 fixed names, no timestamps or random temporary files. Those files are
progress, aggregate stats and rollback slots, not disposable cache. Consolidated
archive migration is a separate audited storage phase; it must not be called a
proven fix for the unconfirmed physical MENU symptom.

Reproduction: `bash tools/test.sh`, including the native main/BFile mocks.
Source reference: installed gint revision
badbd0fd2bd8ac796fd55d49b93691741bd8a139, kernel/osmenu.c, kernel/world.c,
keysc/keydev.c, keysc/getkey.c, render-cg/dvram.c. Reference projects remain read-only.

Second review found and fixed a concrete boundary bug: a MENU or SHIFT+AC/ON
DOWN arriving at exactly a timed phase transition was discarded with stale game
input by the native epoch barrier. Global commands now survive that barrier;
ordinary game input remains blocked. Production-main tests cover both commands
at the legacy Memory phase boundary. This is a reproducible code defect, but it
does not establish the cause of the user's persistent flashing-MENU symptom.

The independent idle clock now carries fractional RTC milliseconds. At 16 Hz,
160 eight-tick wakeups now measure exactly 10,000 ms rather than 9,920 ms. Storage
has moved from bounded per-game files to two fixed archives; see
STORAGE_ARCHIVE_AUDIT.md. Migration remains conservative on conflicts and unknown
ownership headers.

## Final integrated evidence

Visible IDs1–28,31,32 now pass1000 production-main game/MENU transitions
per release/diagnostic namespace variant with
one common peak/active timer, two archive files and zero remaining mock handles.
MENU was also exercised from PLAY, RULES, INIT, ENTRY, RECORDS, NEW, category
STATS, SETTINGS, DIAGNOSTICS and MAIN, with modal OFF and failed checkpoints.
SHIFT+DOT tests cover held/latched modifiers, no-repeat, alpha conflict, removal
of the S↔D alias and unchanged per-game grammar. Diagnostic export freezes the
ring contents while still tracking its own descriptor, and two explicit exports
overwrite one bounded file. Export includes common callback owner/generation.

Before at ad9c34a: ELF text604,284/data512/BSS39,184; project BSS38,100. Files
were62 fixed legacy names, not unbounded generated filenames. After integration: ELF text671,408/data512/BSS41,904; project BSS40,820.
Largest individual compiler frame is unchanged at1,880bytes. Current frames
and hash are in MEMORY_AUDIT.md/build-metrics.json.
There are zero project malloc/calloc/realloc calls before and after. Compiler
frames, mock counters and sampled stack addresses are not device peak metrics.

The final ASan attempt again stopped before main; its sample is retained under
private development samples (excluded from the public source snapshot). UBSan, independent content, both clean SH
builds and package validation completed. This does not resolve the physical
MENU hypothesis. NUM DIAG uses internal@NGDIAG and ND files; normal NUM GAME
uses@NUMGAME and NG files. HARDWARE_RETEST.md provides the reproducible next step.

## Expanded diagnostics

The current instrumentation and exact workload are described in
[DIAGNOSTICS.md](DIAGNOSTICS.md). The normal/diagnostic build reports separate
static placement and conditional call paths from host/device observations.
No measured physical device peak or persistent MENU cure is claimed.
