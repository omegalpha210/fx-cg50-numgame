# Memory and build audit

Measured native build: `build-clean-gdvSH1`. ELF SHA256 `ee9c686a5cf2ae9a9b76b1de9bc9ccb68cd9998c4809c46a63d243903f799799`.
This report separates ELF placement, compiler static analysis, host observations
and device observations. **No total runtime RAM peak is inferred.**
Method and measurement limits: [MEMORY_METHOD.md](MEMORY_METHOD.md).

## A. Static placement

| ELF allocated section | Bytes | VMA | LMA | Flags |
|---|---:|---|---|---|
| `.text` | 145,588 | `0x00300000` | `0x00300000` | CONTENTS, ALLOC, LOAD, READONLY, CODE |
| `.gint.blocks` | 352 | `0x003238c0` | `0x003238c0` | CONTENTS, ALLOC, LOAD, READONLY, CODE |
| `.gint.drivers` | 432 | `0x00323a20` | `0x00323a20` | CONTENTS, ALLOC, LOAD, DATA |
| `.rodata` | 489,520 | `0x00323bd0` | `0x00323bd0` | CONTENTS, ALLOC, LOAD, READONLY, DATA |
| `.bss` | 45,984 | `0x08101400` | `0x08101400` | ALLOC |
| `.data` | 80 | `0x0810c7a0` | `0x0039b400` | CONTENTS, ALLOC, LOAD, DATA |
| `.data.4` | 0 | `0x0810c7f0` | `0x0810c7f0` | CONTENTS, ALLOC, LOAD, DATA |
| `.ilram` | 128 | `0xe5200000` | `0x0039b450` | CONTENTS, ALLOC, LOAD, READONLY, CODE |
| `.gint.bss` | 112 | `0x0810c7f0` | `0x0810c7f0` | ALLOC |

GNU size aggregate text/data/BSS: 635,588/512/46,096 B.
These legacy aggregates do not mean code/RAM/peak: `text` includes readonly
data and ILRAM code; `.gint.drivers` can be ROM metadata despite DATA flags.
Pre-link project object BSS: 45,014 B across 30 objects.
Do not add object totals to the linked sections; they describe overlapping storage.
Linker map: PARSED; allocated-section cross-check: True.

| Selected native symbol | Bytes | ELF section |
|---|---:|---|
| `_cache_id.1` | 4 | `.data` |
| `_sq_master_start_count` | 10 | `.rodata` |
| `_gc_target_beta6_answer` | 16 | `.rodata` |
| `_sq_master_sliding3` | 18 | `.rodata` |
| `_sq_master_lights_min` | 60 | `.rodata` |
| `_grids_bank_groups` | 200 | `.rodata` |
| `_sq_wythoff_bits` | 211 | `.rodata` |
| `_sq_master_lights` | 240 | `.rodata` |
| `_gc_master_operators` | 420 | `.rodata` |
| `_sq_master_sliding4` | 480 | `.rodata` |
| `_cache.0` | 490 | `.bss` |
| `_gc_master_sequence` | 660 | `.rodata` |
| `_gc_master_lock` | 720 | `.rodata` |
| `_gc_master_equation` | 990 | `.rodata` |
| `_sq_euclid_bits` | 1,250 | `.rodata` |
| `_gc_master_cross` | 1,260 | `.rodata` |
| `_gc_sequence_beta4` | 1,320 | `.rodata` |
| `_ng_diagnostics` | 1,588 | `.bss` |
| `_gc_lock_pack` | 1,980 | `.rodata` |
| `_gc_sequence_pack` | 1,980 | `.rodata` |
| `_gc_equation_pack` | 2,430 | `.rodata` |
| `_gc_crypt_pack` | 2,760 | `.rodata` |
| `_gc_countdown_beta4` | 2,820 | `.rodata` |
| `_gc_master_countdown` | 2,820 | `.rodata` |
| `_gc_master_mind` | 2,940 | `.rodata` |
| `_grids_bank_ids` | 3,500 | `.rodata` |
| `_sq_master_starts` | 3,600 | `.rodata` |
| `_gc_cross_pack` | 3,780 | `.rodata` |
| `_sq_fifteen_values` | 4,921 | `.rodata` |
| `_gc_master_target` | 5,640 | `.rodata` |
| `_grids_pack_offsets` | 7,004 | `.rodata` |
| `_nb_shikaku_pack` | 7,800 | `.rodata` |
| `_nb_slitherlink_pack` | 7,800 | `.rodata` |
| `_gc_countdown_pack` | 8,460 | `.rodata` |
| `_gc_mind_pack` | 8,820 | `.rodata` |
| `_ng_storage_probe` | 12,936 | `.bss` |
| `_buffer` | 14,000 | `.bss` |
| `_gc_countdown_beta6_answer` | 14,361 | `.rodata` |
| `_app` | 15,884 | `.bss` |
| `_gc_countdown_beta6` | 16,000 | `.rodata` |
| `_gc_target_beta4_index` | 16,000 | `.rodata` |
| `_gc_target_beta5_index` | 16,000 | `.rodata` |
| `_gc_target_pack` | 16,920 | `.rodata` |
| `_gc_target_beta6_answer_1` | 17,306 | `.rodata` |
| `_gc_target_beta6_answer_0` | 17,713 | `.rodata` |
| `_gc_target_beta6_answer_2` | 24,091 | `.rodata` |
| `_gc_target_beta6_answer_3` | 30,476 | `.rodata` |
| `_grids_pack_bytes` | 79,658 | `.rodata` |
| `_gc_target_beta6` | 80,000 | `.rodata` |

The app/session, transaction probe, codec buffer and optional one-record grid
cache are already contained in these sections. Only the symbols present in
this ELF are counted; no whole-bank RAM cache is assumed.

## B. Compiler static analysis

Compiler: `sh-elf-gcc (GCC) 14.1.0`. Individual-frame limit: 2,048 B; pass: True.
Accepted .su functions: 416; accepted .ci units: 29.

| Function | Individual frame bytes | Kind |
|---|---:|---|
| `src/core/app.c:99:13:start` | 1392 | static |
| `src/ui/draw.c:163:6:ng_wrap_expression` | 576 | static |
| `src/games/guesscalc_math.c:78:6:gc_feedback` | 532 | static |
| `src/storage/native.c:396:13:migrate_state` | 524 | static |
| `src/games/grids.c:95:6:grids_rules_complete` | 512 | static |
| `src/ui/render.c:216:13:dialog` | 424 | static |
| `src/storage/native.c:473:13:diagnostic_line` | 408 | static |
| `src/storage/native.c:307:13:migrate` | 376 | static |
| `src/games/guesscalc_extra.c:54:6:gc_blackbox_complete` | 352 | static |
| `src/games/boards.c:41:6:nb_slither_complete` | 316 | static |
| `src/ui/draw.c:149:6:ng_wrap` | 304 | static |
| `src/ui/draw.c:128:6:ng_center` | 300 | static |
| `src/games/guesscalc.c:560:13:action_cards.part.0` | 288 | static |
| `src/games/strategyquick_extra.c:86:5:sq_reversi_pick` | 280 | static |
| `src/ui/draw.c:120:6:ng_small_fit` | 276 | static |

Callgraph status: **PARTIAL STATIC ANALYSIS**. Known definitions: 416; unresolved edges: 248.
The following sums include only known direct project continuations and
checked recursive-component limits. They are conditional contributions,
**not a whole-program bound and not observed stack peaks**.

| Direct-project entry/component | Known path bytes | Path/component sequence |
|---|---:|---|
| `src/games/guesscalc.c:action_equation` | 3356 | action_equation → gc_equation → gc_expression → SCC(atom + expression + term) → combine → normalize |
| `src/games/guesscalc.c:action_cards` | 3332 | action_cards → action_cards.part.0 → gc_expression → SCC(atom + expression + term) → combine → normalize |
| `src/games/guesscalc.c:valid_equation` | 3328 | valid_equation → gc_equation → gc_expression → SCC(atom + expression + term) → combine → normalize |
| `src/games/guesscalc.c:init_operators` | 3276 | init_operators → operators_value → gc_expression → SCC(atom + expression + term) → combine → normalize |
| `src/games/guesscalc.c:valid_operators` | 3272 | valid_operators → operators_value → gc_expression → SCC(atom + expression + term) → combine → normalize |
| `src/games/guesscalc.c:action_operators` | 3252 | action_operators → operators_value → gc_expression → SCC(atom + expression + term) → combine → normalize |
| `src/games/guesscalc.c:valid_cards` | 3196 | valid_cards → gc_expression → SCC(atom + expression + term) → combine → normalize |
| `main` | 1888 | main → ng_app_event → dispatch → start → gc_target_init → target_deck_v5 → target_deck → target_rand |
| `src/storage/native.c:dispatch` | 1108 | dispatch → migrate_state → ng_state_save_io → read_state_slot → ng_single_decode → game → ng_level_generation_policy_version → ng_bank_count_version → ng_bank_count → ng_difficulty_count → ng_has_hell |
| `src/games/guesscalc.c:valid_baseball` | 708 | valid_baseball → gc_baseball → gc_feedback |

The parser accepts atom nesting depth12; the rejected thirteenth atom still
has a frame, as do its expression/term callers. When the matching guard is
verified, the SCC uses a conservative13-frame limit for each member. Other
unbounded recursion/dynamic frames are reported without an invented bound.
Indirect game dispatch, hooks, callbacks, interrupt/OS frames and library/
assembly internals remain unresolved. Tail-call reuse is not inferred;
summing direct caller/callee frames may overestimate that path. The JSON
lists all unresolved edges and any omitted recursive components.

## C. Host observations

**Not measured by this static report.** Consult the separate instrumented
host workload evidence. Host sizeof/stack ABI and allocator peaks cannot
be relabeled as physical SH/fx-CG50 measurements.

## D. Device observations and runtime accounting

**Device stack/heap/arena peaks: NOT MEASURED — HARDWARE TEST REQUIRED.**
The installed gint fx-CG implementation requests177,408 B framebuffer
payload plus96 B alignment/margins from `_ostk`. That allocation is already
included in `_ostk` used/peak statistics; do not add it a second time.
`_uram` is the unused linked user-RAM arena; `_ostk` has350 KiB capacity in
the inspected SDK. Runtime capacity/used/free must come from public APIs.
`gint_stack_top` is the **low boundary** of the reserved16 KiB stack;
the high boundary is `mmu_uram()+mmu_uram_size()`. P1/P2 aliases describe
the same memory and must not be added together. Use range-checked samples;
do not overwrite active stack memory to obtain a watermark.
Allocator capacity−used−free describes current metadata/headers, not an
exact requested-payload history. Arena lifetime peaks are not reset by
resetting application counters. Different component peaks are not assumed
simultaneous. See the method document for workloads and coverage limits.

## Artifact identity

G3A `NUMGAME.g3a`: **664,788 B**, SHA256 `af46892f48be8ff4ca9099f6ef0deef0dde858c36c85dbb7a65fd83142586e4b`.
ELF binary payload equals G3A payload: **True**.
1,000,000-byte target: True; 1,200,000-byte hard limit: True.
Container checksum/identity validation remains a separate check; neither proves device execution.

```sh
source tools/env.sh
python3 tools/memory_report.py --build-dir <fresh-native-build> --g3a dist/NUMGAME.g3a
python3 tools/stack_callgraph.py --self-test
python3 tools/memory_report.py --self-test
```
