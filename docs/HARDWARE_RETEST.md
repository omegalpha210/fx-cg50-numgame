# Physical fx-CG50 retest — HARDWARE TEST REQUIRED

The previously reported persistent flashing MENU has not been reproduced or
explained on a calculator here. Host tests fixed a separate timed-transition
MENU/OFF boundary, but neither that fix nor the smaller v5 save proves the
device symptom resolved. Use the exact `dist/SHA256SUMS.txt` before testing.
Back up existing NG saves. NUM DIAG is a separate `NDSTATEA/B.dat` namespace
and must be tested with disposable ND progress.

For beta.4, check these revised difficulty starts on the physical LCD:

1. Start several new Countdown HARD puzzles and check that reaching the target
   requires at least four cards. Compare with an old saved HARD run, which must
   reopen and INIT its original puzzle unchanged.
2. Compare new Sequence EASY arithmetic/geometric/simple alternating puzzles
   with NORMAL quadratic/Fibonacci/interleaved puzzles. Check full prompt and
   input readability in both banks.
3. Play new Prime Factor MASTER starts, including the largest target, and
   check every target digit fits the game panel and factors enter promptly.
4. Open Sliding EASY, NORMAL, HARD and MASTER in both 3×3 and 4×4 modes.
   Check tile contrast, current shortest-distance message and old saved-board
   RESUME/INIT. The host-certified distance bands do not replace play testing.
5. Try Make Target at targets 1, 24, 247 and 1000 in all four difficulties.
   Check that each deck has a reachable solution, expression input, target
   editor, cold RESUME, INIT and NEW after changing the target behave normally.
6. Recheck the reported MENU flashing, BFile save latency, dim/APO and
   SHIFT+AC/ON while switching among these new content banks.

The earlier beta.3 UI and lifecycle checks remain relevant:

1. The saved game's tile alone shows a legible blue RESUME badge; the selected
   border, icon and name remain distinguishable. Main F1 resumes that same run.
2. Starting another game moves the badge, completing a game removes it, and
   cold load or recovery shows it only for a validated unfinished run.
3. Entry text reads NEW GAME. The same-game entry focuses RESUME; a different
   game has NEW GAME first. Settings on RESUME remain those of the saved run.
4. Nonogram completes with all correct black cells and unmarked empty cells,
   then again with all empty cells X, then with a mixture. One extra black cell
   must prevent completion. Hold the final EXE and check it cannot dismiss the
   result or start another game. Cold resume must preserve individual X marks.
5. New Magic Square FREE E/N/H/M starts show blank 3×3/4×4/5×5/6×6 squares
   and common sums 15/34/65/111. Test 2-digit values through 36, cursor
   contrast, centered numerals, full 6×6 bounds and unobstructed F-keys.
   An old FREE save must resume at its old size and INIT the same puzzle.
6. On Make Target TARGET, LEFT opens the editor at the first digit and RIGHT
   at the end. Test insertion, cursor movement, DEL, invalid/empty draft,
   EXIT cancel, EXE commit without start, then EXE/F6 start. F6 during edit
   must do nothing. Check 1 and 1000 boundaries and persistence after cold load.
7. Compare representative EASY/NORMAL/HARD/MASTER runs using
   [DIFFICULTY_AUDIT.md](DIFFICULTY_AUDIT.md): LIGHTS 4×4 revised levels,
   MAGIC FREE sizes, CPU MASTER tactical starts, 2048 CLASSIC without a
   difficulty row, TARGET goals, and F3 HELL/ENHM for each LOGIC game.
8. Recheck the reported MENU flashing, real BFile latency and allocation,
   dim/APO, and SHIFT+AC/ON. These remain unverified on hardware.

1. Cold launch with no unfinished game: Main F1 must be blank. NEW GAME,
   return to Main, confirm F1 RESUME appears and opens the exact run.
2. Return to the same game's entry: RESUME and NEW GAME must both appear,
   with RESUME focused. Enter a different game's entry: no RESUME row or
   softkey appears. Browsing alone must not delete the prior run.
3. Start that different game: it must replace the old resume without a prompt.
   Cold launch must reopen only the new game. Repeat with the same game's
   NEW GAME and a changed difficulty/mode; RESUME retains its run settings.
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
