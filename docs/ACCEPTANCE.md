# Current 36-game host and package acceptance

The visible catalog has 36 complete games, stable IDs 1–28 and 31–38. Legacy
29/30 remain decodable but are not selection tiles. This milestone refines UI,
Nonogram completion and two new-run difficulty supplies without adding a game
or changing the v5 save layout. The persistent flashing MENU previously
reported on a calculator has **not** been reproduced or resolved on hardware.

| Check | Executed evidence | Result |
|---|---|---|
| C11 host/UBSan | 21 CTest targets, strict `-Wall -Wextra -Werror`; beta.3 target editor, grid and difficulty regressions included | PASS |
| Native main and BFile | Release and diagnostic mock variants, v4→v5 migration with valid five/one, corrupt newest/slot, A-only/B-only, terminal-only and failed-write retry | PASS |
| Lifecycle | 30 retained games completed through actual input and renderer; completion→frozen result→entry; cold completion has no RESUME | PASS |
| Recent six | Black Box, Cryptarithm, Hashi, Nonogram, Reversi and Net independent rules, UI and actual-app paths; see focused audit | PASS on host |
| One-resume storage | A/B CRC and readback, partial-write recovery, 36 sequential game switches, cold F1 and local RESUME; exactly one tile badge, corrupt-save removal | PASS |
| New behavior | Nonogram exact filled set with blank/X and HOLD safety; Magic FREE 3/4/5/6 with old FREE legacy decode/INIT; target EDIT/commit/start/cancel; Lights old/new revision replay | PASS on host |
| Difficulty audit | 36 games, 150 level rows, 2,020 grid and 240 board records, 12,022 GUESS/CALC starts, 11,101 STRATEGY/quick samples, 12,288 Lights regression starts | PASS for stated structural metrics; human ratings uncalibrated |
| Puzzle cycles | N=1/2/5 coprime permutations; 30-item actual app cycle, cold middle and exhaustion boundary | PASS |
| Lifecycle stress | 1,000 native game/MENU switches; exactly two v5 files; zero remaining handles; one timer peak | PASS in mock |
| Renderer | 199 actual 396×224 host frames plus eight grid review frames; badge, target editor, Nonogram blank/X and Magic 3–6 included | PASS for host bounds |
| SH normal | Fresh strict C11/fxSDK/gint build, `NUMGAME.g3a` 411,484 bytes, 16 container checks | PASS |
| SH diagnostic | Fresh strict build, `NUMGDIAG.g3a` 437,804 bytes, 15 container checks | PASS |
| Static memory | Native text 382,284 B (+1,924), data 512 B (unchanged), BSS 46,096 B (unchanged), largest individual frame 1,392 B (unchanged); project object BSS 45,014 B (+4). Two-copy v5 save 252–18,732 logical bytes | MEASURED; device peaks unknown |
| ASan | Available macOS runtime timed out before useful execution | **UNVERIFIED** |
| Physical fx-CG50 | MENU flashing, LCD legibility, actual BFile latency/space, power-cut and arena/stack peaks | **HARDWARE TEST REQUIRED** |

The [storage format](STORAGE_FORMAT.md) gives exact before/after byte counts;
[current host benchmark](host-benchmark-v5-normal.json) gives POSIX timing, not
calculator flash timing. [RECENT_SIX_AUDIT.md](RECENT_SIX_AUDIT.md) links the
focused family evidence; [DIFFICULTY_AUDIT.md](DIFFICULTY_AUDIT.md) records
level differences, fixes and review items. [HARDWARE_RETEST.md](HARDWARE_RETEST.md) is the
remaining physical checklist. Source/content verification can be reproduced
with `bash tools/test.sh`, `bash tools/verify_content.sh`, and both variants of
`bash tools/clean_build.sh` using the installed SDK. The public source is a
clean allowlisted snapshot with no private local Git history.
