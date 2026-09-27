# beta.8 editor, result presentation and code cleanup audit

Baseline: local beta.7 handoff `80a5e63` (implementation `9dbcc3b`), public beta.7 `ebbd6d7`. The baseline passed 27/27 strict C11/UBSan host targets and had normal/diagnostic packages of 559,380/586,884 bytes. This milestone does not generate puzzles, change scoring or difficulty rules, or alter the v5 save wire layout. A local rollback point exists at `80a5e63`.

## Responsibility and file inventory

| Files or symbols | Previous/current purpose | Class | Decision and evidence |
|---|---|---|---|
| `src/main.c`, `src/native_keys.h` | Physical gint key matrix to one logical action | A | Moved the mapping into one inspectable table; the main loop and barrier use it. The installed gint `keycodes.h` names `KEY_SQUARE=0x72` (`KEY_X2`) and `KEY_POWER=0x73` (`KEY_CARET`). The native adapter compiles production `main.c` and checks DOWN/UP/HOLD and modifiers. |
| `src/core/common.c`, four `guesscalc.c` expression actions | Bounded string editing | A | Shared atomic insertion/deletion helper; the module still owns its allowed grammar and length. LEFT/RIGHT is handled in app navigation without dirtying storage. Numeric code, grid, card and history controls retain their handlers. |
| `src/ui/draw.c`, `src/ui/render.c`, `src/ui/result.c` | Glyph-metric clipping and result presentation | A | An input rectangle draws only a visible expression slice, with the caret in bounds and original operator colors. A pure formatter names only meaningful result metrics. |
| `src/core/app.c`, `include/app.h` | Lifecycle and runtime UI state | A | Removed unreachable STATS/RECORDS navigation, rendering and the per-game summary array. Screen/modal enum numbers remain reserved for diagnostic trace stability. Completion, frozen VIEW RESULT, NEW and save flow are unchanged. |
| `src/storage/codec.c`, `transaction.c`, `native.c`, `NgStats` | v5 and older save readers; run counters and two-copy transaction | C | Kept. `edit_cursor` is explicitly omitted from every wire revision and reconstructed at the end of decoded text. `stats.started` still determines run IDs and sequence state; it is not a removed menu. |
| `NgSummary` and `ng_summarize` | Host/legacy summary contract | B/C | Kept because `test_master_integration.c` exercises the decoded summary. Its former 38-entry `NgApp` display cache was unused by reachable UI. |
| Game IDs29/30 and strategy/quick old modes | Older active saves and validation | C | Kept. Stable IDs are not renumbered or reused; legacy action/render and codec paths remain tested. |
| `assets/guesscalc_target_beta4/5.h`, current packed beta.6 header | Older Make Target resumes and current bank | A/C | Kept. The 63,000-byte packed bank remains current; older indices remain for saved games. |
| `assets/guesscalc_target_beta6.h`, JSON source and logical manifest | Independent golden reference and reconstruction | D | Kept in host/public source, excluded from linked SH packages. The packer and native audit compare all 4,000 decoded records to these references. |
| `tools/generate/*`, validators, capture tools, host fixtures | Regeneration and independent checks | B/D | Kept. Names containing beta/legacy are not deletion evidence. |
| `docs/captures/*`, prior audits | Referenced provenance and edge-case images | E | Kept. New README images are curated from current renderer frames under stable `docs/images` names; no old capture was deleted merely for age. |
| `CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/native/run.sh` | Runtime/host source discovery | A/B | Both CMake files currently glob `src/*.c`; inventory found no experimental or abandoned runtime `.c` under `src`. The new result module is included in normal, diagnostic and host builds. An explicit list is not needed for this tree; later experiments must live outside `src`. |

No untracked user files, save files, toolchains or reference projects were deleted. No generated problem array was manually reformatted. The only removed runtime paths are the unreachable STATS/RECORDS screens and their app cache; the persisted statistics and their readers remain.

## Editor contract

`NgGame.edit_cursor` is runtime-only and fits the existing host struct padding. New input begins at the end; cold decode resets to the saved input's end. An action that replaces input (ANSWER) resets the caret. LEFT/RIGHT only changes that caret and redraws: it does not increment moves, alter RNG/undo, or schedule a save. Insert and DEL preserve the suffix, clamp to the original game limit, and leave an incomplete submitted draft intact. The x² logical action inserts both `^` and `2` or neither; only Prime Factor's grammar contains `^`. SHIFT+x² adds nothing. The native table gives square a unique held/blocked bit. The existing arrow repeater is used for caret movement.

The result formatter is presentation-only. Its 36 visible games have this explicit metric policy:

| Games (stable IDs) | Completion summary |
|---|---|
| Number Baseball 1; Equation Guess 2 | Actual TRIES |
| Number Mind 3; Clue Lock 4; Sequence 5; Black Box 33 | Time when enabled; ASSISTED only when used |
| Make Target 6; Missing Operators 8; Cross Math 9; Prime Factor 10; Cryptarithm 34 | Time when enabled; ASSISTED only when used |
| Countdown 7 | Actual SCORE (including zero) and best submitted target distance, or NO SUBMISSION |
| Sudoku 11; Calcudoku 12; Kakuro 13; Futoshiki 14; Skyscrapers 15; Hashi 35 | Time when enabled; ASSISTED only when used |
| Hitori 16; Binary 17; Numbrix 18; Magic Square 19; Sum Grid 20; Nonogram 36 | Time when enabled; ASSISTED only when used |
| Nim 21; Wythoff 22; Euclid 23; Make Fifteen 24; Race 25; Reversi 37 | Actual TURNS and the existing win/loss/draw title |
| 2048 26 | Actual SCORE (including zero), plus MAX TILE below |
| Sliding 27; Lights Out 28 | Actual MOVES / PRESSES respectively |
| Shikaku 31; Slitherlink 32; Net 38 | Time when enabled; ASSISTED only when used |
| Legacy Rush 29; Memory 30 | Existing correct/round and score summaries retained for old saves |

Prime Factor's old MOVES value was the internal number of submissions, not board movement; its score was not a player-facing metric. Both fields remain for validation and legacy save compatibility. When time is disabled and no assistance was used, the explanatory message moves up rather than leaving a metric placeholder. Its completed factorization is shown in the frozen final game view.

## Verification and measured costs

Targeted editor/native tests cover insertion at both ends and in the middle, deletion, boundaries, capacity 95/96, physical square DOWN/UP/HOLD, unsupported grammar, SHIFT+DOT and one-shot modifiers. Renderer tests bound the caret and glyphs to the input box; the current capture set includes the Prime Factor mid-expression, completion modal and frozen view. The complete strict C11/UBSan host suite passed 28/28 targets. Full content verification and public-source rebuild are recorded separately in the final milestone handoff. Hardware LCD, MENU, flash latency and stack/arena peaks remain **HARDWARE TEST REQUIRED**; ASan remains **UNVERIFIED** on the available macOS runtime.

| Measure | beta.7 | beta.8 | Change |
|---|---:|---:|---:|
| Manually maintained `src`/`include` C/H lines (46 files; excludes generated assets) | 10,753 | 10,864 | +111 |
| Normal `.text` | 146,740 B | 145,796 B | −944 B |
| Normal `.rodata` | 382,960 B | 382,760 B | −200 B |
| Normal `.data` | 80 B | 80 B | 0 |
| Normal `.bss` + `.gint.bss` | 46,096 B | 45,328 B | −768 B |
| Normal package | 559,380 B | 558,236 B | −1,144 B |
| Diagnostic `.text` / `.rodata` | 172,788 / 384,416 B | 172,004 / 384,216 B | −784 / −200 B |
| Diagnostic `.bss` + `.gint.bss` | 50,016 B | 49,248 B | −768 B |
| Diagnostic package | 586,884 B | 585,900 B | −984 B |
| Normal largest statically reported frame | 1,392 B | 1,392 B | 0 |
| Framebuffer accounting | 177,408 B | 177,408 B | 0 |

The changes removed two unreachable runtime presentation branches and a 38-entry display cache. No tracked source or legacy data file was deleted; no host-only file was moved, and referenced capture/history documents remain. The shared editor helper is newly consolidated, while native mapping and result formatting are separated by responsibility. Source line growth reflects the new tests and readable code, not an execution cost. The static sizes are linker measurements, not physical RAM peaks or proof of faster execution.

Manual C style for touched code: C11, the existing indentation convention, braces for multi-line branches, bounded buffers, named responsibilities, and comments for non-obvious persisted-versus-runtime state. Generator-owned arrays retain their existing formatting. This cleanup makes no claim that README changes or source line counts improve execution speed.
