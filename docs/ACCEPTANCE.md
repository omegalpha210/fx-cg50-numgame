# Current 36-game host and package acceptance

The visible catalog has 36 complete games, stable IDs 1–28 and 31–38. Legacy
29/30 remain decodable but are not selection tiles. Beta.5 gives new MAKE TARGET
games 1–999 cards under the same exact minimum-solution grading, while older
active runs retain their card decks and the v5 save layout. Conditional RULES
scroll indicators and focused renderer alignment changes are described in the
[UI audit](UI_ALIGNMENT_AUDIT.md). The persistent flashing MENU previously
reported on a calculator has **not** been reproduced or resolved on hardware.

| Check | Executed evidence | Result |
|---|---|---|
| C11 host/UBSan | 23 CTest targets, strict `-Wall -Wextra -Werror`; old/new revision, Make Target target-cycle, exact Sliding and beta.3 regressions included | PASS |
| Native main and BFile | Release and diagnostic mock variants, v4→v5 migration with valid five/one, corrupt newest/slot, A-only/B-only, terminal-only and failed-write retry | PASS |
| Lifecycle | 30 retained games completed through actual input and renderer; completion→frozen result→entry; cold completion has no RESUME | PASS |
| Recent six | Black Box, Cryptarithm, Hashi, Nonogram, Reversi and Net independent rules, UI and actual-app paths; see focused audit | PASS on host |
| One-resume storage | A/B CRC and readback, partial-write recovery, 36 sequential game switches, cold F1 and local RESUME; exactly one tile badge, corrupt-save removal | PASS |
| New behavior | Nonogram exact filled set with blank/X and HOLD safety; Magic FREE 3/4/5/6 with old FREE legacy decode/INIT; target EDIT/commit/start/cancel; Lights old/new revision replay | PASS on host |
| Difficulty audit | 36 games, 150 level rows, 19 columns with before/after decisions; beta.4 content findings retained; beta.5 Make Target 8,000 revised 1–999-card decks | PASS for stated structural metrics; human ratings uncalibrated |
| Make Target independent reference | Eight fixed four-card sets plus 20 beta.4 and 20 beta.5 bank samples. All 8,000 beta.5 records re-solved exactly; native deck parity and expression parser checks cover every record. | PASS |
| Sliding independent reference | Complete 3×3 BFS 181,440 reachable states; 384 new 4×4 E/N/H exact shortest proofs; 1,156 production completions; 32 old/new cold resume and INIT cases | PASS |
| Puzzle cycles | N=1/2/5 coprime permutations; 30-item actual app cycle, cold middle and exhaustion boundary | PASS |
| Lifecycle stress | 1,000 native game/MENU switches; exactly two v5 files; zero remaining handles; one timer peak | PASS in mock |
| Renderer | 354 actual 396×224 host captures, including all 36 gameplay states, 108 RULES positions, six three-digit cards, five Prime Factor values and four Sequence levels | PASS for host bounds and focused pixel checks |
| SH normal | Fresh strict C11/fxSDK/gint build, `NUMGAME.g3a` 458,392 bytes, 16 container checks | PASS |
| SH diagnostic | Fresh strict build, `NUMGDIAG.g3a` 485,336 bytes, 15 container checks | PASS |
| Static memory | Normal native text 429,192 B (+18,368 from beta.4), data 512 B and BSS 46,096 B (unchanged), `.text` 143,140 B (+2,288), `.rodata` 285,572 B (+16,080), largest individual frame 1,396 B (unchanged). Two-copy v5 save 252–18,732 logical bytes | MEASURED; device peaks unknown |
| ASan | Available macOS runtime timed out before useful execution | **UNVERIFIED** |
| Physical fx-CG50 | MENU flashing, LCD legibility, actual BFile latency/space, power-cut and arena/stack peaks | **HARDWARE TEST REQUIRED** |

The [storage format](STORAGE_FORMAT.md) gives exact before/after byte counts;
[current host benchmark](host-benchmark-v5-normal.json) gives POSIX timing, not
calculator flash timing. [RECENT_SIX_AUDIT.md](RECENT_SIX_AUDIT.md) links the
focused family evidence; [DIFFICULTY_AUDIT.md](DIFFICULTY_AUDIT.md) records
level differences, fixes and accepted decisions. [HARDWARE_RETEST.md](HARDWARE_RETEST.md) is the
remaining physical checklist. Source/content verification can be reproduced
with `bash tools/test.sh`, `bash tools/verify_content.sh`, and both variants of
`bash tools/clean_build.sh` using the installed SDK. The public source is a
clean allowlisted snapshot with no private local Git history.
