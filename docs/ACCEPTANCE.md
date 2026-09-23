# Runtime, UI and 30-game acceptance

Visible catalog: **30 games, IDs1–28,31,32**. Arithmetic Rush29 and Number Memory30
are legacy-only decoders/engines, not public games. All visible games have actual
input/renderers, rules, content, transactional saves and executable validation.
The [per-game matrix](acceptance-matrix.csv) does not count variants as games.

**The user's persistent flashing MENU symptom remains unconfirmed on hardware.**
Host reproduction found a different concrete boundary: MENU/OFF DOWN was dropped
when a timed phase changed in the same loop. That is fixed and covered in the
production-main harness. File count, memory exhaustion or a gint OS timer failure
have not been established as the device cause. The diagnostic app and physical
procedure are deliverables; they do not turn hardware-pending work into PASS.

| Check | Executed scope | Result |
|---|---|---|
| Host C/UBSan |13 CTest targets in each release/diagnostic configuration, C11, -Wall -Wextra -Werror, assertions | PASS |
| Native main/BFile |Production source,1000 game/MENU switches per release/diagnostic variant,2 fixed archives,0 residual handles,1 peak timer; ten screen/modal classes, OFF/error paths | PASS |
| Clock/power mocks |60s no-input clock/HUD+PWM dim, exact wake key, APO boundary, OS/off exclusion, wrap/long gaps, fallback, fractional RTC | PASS |
| Entry/layout |213 visible mode/level configurations; every row EXE/F6, digit focus, clamped options, contextual F3 HELL/MODE, confirmation, explicit RESUME, long text/max values | PASS |
| Public lifecycle |30 real renderer/input completions, checkpoint and cold result, cross-game drafts/RNG/CPU/undo | PASS |
| Storage codec |v1/v2/v3, five independent level buckets,324 old-wire matrix views, four undo copies, wrap-safe generations and malformed valid-CRC data | PASS |
| Native migration/faults |16 interrupted-write cut points/retry; create→open failure; foreign/conflicting records preserved; two-copy readback; per-record corruption isolation; retired IDs/preferences | PASS |
| Guess/Calc |Parser/state/alternate-answer audits; explicit entry policy;146 wrapping/clipping/prose/variable-operator color cases | PASS |
| Guess/Calc independent packs |1,230 records,344,898 independent MASTER checks;900 originals retained | PASS |
| Grids |1,750 native witness/codec/lifecycle checks; compact single-record decoder; original980 retained; independent full-pack quality and770 seed replays | PASS; human rating provisional |
| MASTER/HELL |30 visible MASTER; LOGIC11–15 have separate HELL, all bank cycles; human rating provisional | Host workflow and content PASS |
| CPU/retained board engines |Exact finite domains in AI_AUDIT;900 strategy games;2048 merge/RNG/bounds,1200 Sliding,1200 Lights seeds and65535 press sets | PASS |
| Strategy app/legacy |84 public configurations (81 distinct after 2048 CLASSIC normalization) plus9 legacy engine/codec cases | PASS |
| New boards |240 native action solutions; independent uniqueness/D4 structure; exhaustive2×2 rule comparisons;8 complete app lifecycles and1665 common-renderer frames | PASS |
| Embedded consistency |GC/grid JSON→C and independently regenerated AI bytes; board native initializer parsed against JSON | PASS |
| SH release and diagnostic |Fresh existing SDK/GCC14.1/gint2.11 builds, C11, strict warnings, frame limit2048 | PASS |
| Packages |Release16 and diagnostic15 container/size checks, distinct app identities and NG/ND namespaces; final dist hashes checked separately | PASS |
| Rendered evidence |172 actual396×224 release frames plus two diagnostic frames, all30 MASTER and five HELL, mode chooser, board completion, clock, save error/recovery | PASS for host pixels |
| ASan |Fresh target built; runtime sampled in InitializeShadowMemory/StaticSpinMutex before main, terminated after timeout | **UNVERIFIED** |
| Physical fx-CG50 |Reported MENU defect, LCD brightness, OS queries, APO, flash interruption, latency, heap/stack peaks | **HARDWARE TEST REQUIRED** |

Reproduce with `bash tools/test.sh`, `bash tools/verify_content.sh`, both variants
of `bash tools/clean_build.sh`, and the [inventory](CONTENT_INVENTORY.md),
[memory method](MEMORY_METHOD.md) and [device guide](DIAGNOSTICS.md). Raw
machine-specific ASan process samples are excluded from the public source;
the minimal and suite binaries timed out before main, so ASan remains unverified.
Native mocks prove ordering/bounds and supplied failure handling, not CASIO OS
behavior or physical flash atomicity. Renderer captures are not LCD photographs.

Additional concrete fixes in this round: malformed nonterminated arithmetic
input bounds; impossible Countdown result state validation; offline Sudoku
solution counter accepting contradictory fully-filled givens; source-context
loss across expression wrap/clipping; lost idle fractional milliseconds;
migration workspace alias and interrupted creation/retry/conflict handling.
The archive defects were caught during implementation review before delivery.
The original game rules, core RNG algorithm, exact HARD CPU policies and legacy
puzzle records are retained; new bank selection and shuffle-cycle state are
documented separately. See family audits for exact reproductions and independent
reference coverage.

The initial diagnostic package failed checksum validation after its distinct
internal name was edited by fxgxa. The installed edit mode does not recompute the
whole G3A checksum; the local build now invokes its supported repair command
before the independent15-check validator. The SDK was not changed.

[MEMORY_AUDIT.md](MEMORY_AUDIT.md) reports current normal sections/frames and
binary hash; [diagnostic metrics](diagnostic-build-metrics.json) report the
instrumented ELF separately. [PERFORMANCE_AUDIT.md](PERFORMANCE_AUDIT.md)
contains per-game host measurements. [RUNTIME_AUDIT.md](RUNTIME_AUDIT.md),
[STORAGE_ARCHIVE_AUDIT.md](STORAGE_ARCHIVE_AUDIT.md) and
[POWER_TIMER_AUDIT.md](POWER_TIMER_AUDIT.md) distinguish observation from hypothesis.
No device resource peak, guaranteed MENU cure or human-calibrated MASTER rating
is claimed. Follow [HARDWARE_RETEST.md](HARDWARE_RETEST.md) for remaining checks.

Reference repositories and SDK remain read-only. Public source is assembled as
a clean allowlisted snapshot; original local Git history is not published.
