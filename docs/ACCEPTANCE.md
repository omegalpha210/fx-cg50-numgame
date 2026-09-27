# Current 36-game host and package acceptance — beta.7

The visible catalog has 36 complete games, stable IDs 1–28 and 31–38. Legacy
29/30 remain decodable but are not selection tiles. Beta.7 changes only the
physical representation of beta.6 Make Target; its 4,000 logical problems
and all other game behavior are unchanged. The
v5 single-resume A/B save layout remains compatible with earlier unfinished
runs. The persistent flashing MENU reported on a calculator has **not** been
reproduced or resolved on hardware.

| Check | Executed evidence | Result |
|---|---|---|
| C11 host/UBSan | 27 CTest targets, strict `-Wall -Wextra -Werror`, including packed random-access bounds, migration, lifecycle, renderer, legacy revisions, Baseball warning/history and 200/1,000-item supply cycles | PASS |
| Native main and BFile mock | Release and diagnostic variants; v4→v5 migration, A/B recovery, failed-write retry, exactly two files after 1,000+ game switches | PASS on host |
| Make Target content | 4,000/4,000 original-vs-packed logical records and native parser/card validation; 4,000 distinct target/card multisets; 200 per target/level, RANDOM shares 1,000 per level; unchanged thresholds and 1–999 cards | PASS |
| Make Target replay | Same-seed 200 fixed + 1,000 RANDOM starts, both v5 save payloads, original beta.6 FIXED/RANDOM cold decode, and representative HINT/ANSWER host-rendered frames match prior public source | PASS on host |
| Make Target independent cross-check | 10 four-card records, 13,221 rational groups and 3,944,640 canonical expression trees; all decoded target/Countdown records also checked against JSON and native parser/card validator | PASS for stated scope |
| Countdown content | 800/800 independent Python exact-subset checks plus C++ recheck; 200 per level, HARD minimum four cards, MASTER exactly six and division required | PASS |
| Prime Factor | 896/896 composite targets independently factored, structurally classified, checked through the compiled native table and answer parser | PASS |
| Baseball | All four caps 20/30/40/50, cap−1 warning, final win/loss, 50 packed records, cold pending/acknowledged resume, leading zeros, invalid-input bounds and legacy attempts | PASS on host |
| Supply and compatibility | Fixed 200/RANDOM 1,000 no-repeat cycles, 255/256/999/1000 boundaries, immediate-boundary duplicate prevention, cold future order, result NEW policy and old arbitrary-target RESUME/INIT | PASS on host |
| Other game regressions | 36-game entry, completion, INIT, RULES, VIEW RESULT, recent six, Sliding exact distances, Nonogram/Magic and MENU/OFF mock paths | PASS on host |
| Content inventory | 36 games, 218 setting rows, 150 difficulty rows; 10,063 active base records and 16,224 distinct legacy-only bases are counted separately | PASS for public base identities |
| Renderer | 57 new actual 396×224 beta.6 frames and four contact sheets; production draw callback checks canvas bounds, with reviewed target/cards, Prime digits, Baseball 50 rows and long-list rails | PASS for host bounds; LCD pending |
| SH normal | Fresh strict C11/fxSDK/gint build, `NUMGAME.g3a` 559,380 bytes, 16 container checks, ELF payload match | PASS |
| SH diagnostic | Fresh strict build, `NUMGDIAG.g3a` 586,884 bytes, 15 container checks, ELF payload match | PASS |
| Static memory | Normal native text/data/BSS 530,180/512/46,096 B; `.text` 146,740 B and `.rodata` 382,960 B; largest single frame 1,392 B. Diagnostic text/data/BSS 557,684/512/50,016 B. Two-copy v5 save 252–18,732 logical bytes | MEASURED; device peaks unknown |
| Host workload | 12,672 init/validate/render samples per variant, 1,000 codec fixtures, 128 maximum-undo v5 saves; normal save p95 1.434 ms under concurrent host load | MEASURED on host only |
| ASan | Available macOS runtime stalled before completion; attempt terminated and not counted as a pass | **UNVERIFIED** |
| Physical fx-CG50 | MENU flashing, LCD legibility, actual BFile latency/space, power-cut and arena/stack peaks | **HARDWARE TEST REQUIRED** |

The [packing audit](MAKE_TARGET_PACKING_AUDIT.md) gives the complete lossless
comparison and actual size accounting. The [content audit](MAKE_TARGET_COUNTDOWN_BETA6_AUDIT.md) gives full-pack and
independent-reference scope, while the [Prime Factor](PRIME_FACTOR_BETA6_AUDIT.md)
and [Baseball](BASEBALL_BETA6_AUDIT.md) audits give exact policies. The
[inventory](CONTENT_INVENTORY.md) shows setting-level reachability; counting
all records together would hide the old same-setting repetition. [Storage
format](STORAGE_FORMAT.md) gives exact wire sizes. [Memory audit](MEMORY_AUDIT.md)
separates static placement from unmeasured device peaks, while the
[beta.6 host workload](PERFORMANCE_BETA6.md) states timing and coverage. The
[hardware checklist](HARDWARE_RETEST.md) lists physical tests. Reproduce host
and content checks with `bash tools/test.sh` and `bash tools/verify_content.sh`,
then build each SH variant with `bash tools/clean_build.sh` and its
`--diagnostic` option. These commands do not certify calculator timing or the
previously reported MENU symptom.
