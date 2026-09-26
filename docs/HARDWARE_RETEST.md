# Physical fx-CG50 retest — HARDWARE TEST REQUIRED

The previously reported persistent flashing MENU has not been reproduced or
explained on a calculator here. Host tests fixed a separate timed-transition
MENU/OFF boundary, but neither that fix nor the smaller v5 save proves the
device symptom resolved. Use the exact `dist/SHA256SUMS.txt` before testing.
Back up existing NG saves. NUM DIAG is a separate `NDSTATEA/B.dat` namespace
and must be tested with disposable ND progress.

1. Cold launch with no unfinished game: Main F1 must be blank. START GAME,
   return to Main, confirm F1 RESUME appears and opens the exact run.
2. Return to the same game's entry: RESUME and START GAME must both appear,
   with RESUME focused. Enter a different game's entry: no RESUME row or
   softkey appears. Browsing alone must not delete the prior run.
3. Start that different game: it must replace the old resume without a prompt.
   Cold launch must reopen only the new game. Repeat with the same game's
   START GAME and a changed difficulty/mode; RESUME retains its run settings.
4. Complete a game. The modal must show EXIT VIEW RESULT. EXIT must reveal the
   frozen final board; game input must not alter it. F6 NEW must use the
   completed run's game/difficulty/mode. A second EXIT returns to entry.
   Test held EXIT/EXE, completion EXE NEW and a cold launch with no RESUME.
5. Run the same bank-backed game through a full cycle if practical: no base
   puzzle repeats within that cycle, and the next cycle's first puzzle differs
   from the preceding last when the bank has more than one. Check an
   interrupted run resumes its cycle. Runtime-generated NEW should give a
   fresh seed; exact content can collide in a small space.
6. Review Cryptarithm letter column alignment, removed placeholders,
   selection box, input mapping and menu icon at actual LCD size. Check the
   recent six games' controls and displays against
   [RECENT_SIX_AUDIT.md](RECENT_SIX_AUDIT.md), especially small Nonogram
   clues, Hashi bridge/crossing feedback, Reversi CPU wait, and Net rotations.
7. Measure actual free space, `NGSTATEA.dat/B.dat` sizes, first-save and
   repeated-save latency, cold load and migration latency. A/B are two copies
   of one state. Test valid v4 recent-five and legacy archive migration after
   external backup; only the first valid unfinished run should remain. Test
   one damaged copy and an interrupted write using disposable data.
8. Repeat 1,000 game-entry/exit and MENU transitions. In NUM DIAG, inspect and
   export handles (final 0), timer peak (at most 1), arena usage and stack
   samples. Record exact MENU flash reproduction route, request/enter/return
   counters, OS version, and whether OFF/ON changes it. Do not call a rear
   RESET a verified fix.
9. Verify normal font, HELL/ENHM, inline FIRST, all button labels and text
   bounds, 2048 palette, long messages, large tiles and the six focused-game
   captures on the real LCD. Host PNGs are not LCD photographs.
10. Leave a game idle through dim and APO; restore brightness on one key and
    before MENU/OFF. Check 60-second clock, OS timeout queries, RTC wrap,
    pending CPU replay, save-failure MENU/OFF path, SHIFT+AC/ON and SHIFT+DOT.

ASan remains unverified on the available macOS runtime because execution
stalled before test output. Host UBSan/SH/package checks do not replace this
physical retest. [DIAGNOSTICS.md](DIAGNOSTICS.md) explains the instrumentation
and its coverage limits.
