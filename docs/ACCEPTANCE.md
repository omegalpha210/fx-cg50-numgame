# Current 36-game host and package acceptance

The visible catalog has 36 complete games, stable IDs 1–28 and 31–38. Legacy
29/30 remain decodable but are not selection tiles. This milestone changes
storage and UI without adding a game. The persistent flashing MENU previously
reported on a calculator has **not** been reproduced or resolved on hardware.

| Check | Executed evidence | Result |
|---|---|---|
| C11 host/UBSan | 18 CTest targets, strict `-Wall -Wextra -Werror` | PASS |
| Native main and BFile | Release and diagnostic mock variants, v4→v5 migration with valid five/one, corrupt newest/slot, A-only/B-only, terminal-only and failed-write retry | PASS |
| Lifecycle | 30 retained games completed through actual input and renderer; completion→frozen result→entry; cold completion has no RESUME | PASS |
| Recent six | Black Box, Cryptarithm, Hashi, Nonogram, Reversi and Net independent rules, UI and actual-app paths; see focused audit | PASS on host |
| One-resume storage | A/B CRC and readback, partial-write recovery, 36 sequential game switches, cold F1 and local RESUME | PASS |
| Puzzle cycles | N=1/2/5 coprime permutations; 30-item actual app cycle, cold middle and exhaustion boundary | PASS |
| Lifecycle stress | 1,000 native game/MENU switches; exactly two v5 files; zero remaining handles; one timer peak | PASS in mock |
| Renderer | 193 actual 396×224 host frames, including Main F1/no F1, same/different entries, Cryptarithm icon/layout and frozen result | PASS for host bounds |
| SH normal | Fresh strict C11/fxSDK/gint build, `NUMGAME.g3a` 409,560 bytes, 16 container checks | PASS |
| SH diagnostic | Fresh strict build, `NUMGDIAG.g3a` 435,512 bytes, 15 container checks | PASS |
| Host memory | Previous 36-game app struct 15,904 bytes; current 15,920 bytes; session remains 12,936 bytes. Current maximum v5 two-file save 18,732 logical bytes | MEASURED ON HOST |
| ASan | Available macOS runtime timed out before useful execution | **UNVERIFIED** |
| Physical fx-CG50 | MENU flashing, LCD legibility, actual BFile latency/space, power-cut and arena/stack peaks | **HARDWARE TEST REQUIRED** |

The [storage format](STORAGE_FORMAT.md) gives exact before/after byte counts;
[current host benchmark](host-benchmark-v5-normal.json) gives POSIX timing, not
calculator flash timing. [RECENT_SIX_AUDIT.md](RECENT_SIX_AUDIT.md) links the
focused family evidence. [HARDWARE_RETEST.md](HARDWARE_RETEST.md) is the
remaining physical checklist. Source/content verification can be reproduced
with `bash tools/test.sh`, `bash tools/verify_content.sh`, and both variants of
`bash tools/clean_build.sh` using the installed SDK. The public source is a
clean allowlisted snapshot with no private local Git history.
