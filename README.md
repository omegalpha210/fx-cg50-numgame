# NUM GAME

Thirty native number, arithmetic, logic, puzzle, strategy and board games for the
**CASIO fx-CG50**, written in C with fxSDK/gint. The calculator runs offline;
Python is used only to generate and independently verify development assets.

**Experimental prerelease.** Host and native build checks have passed; physical
fx-CG50 acceptance is still pending. The package checksums are in `dist`.

**Experimental: physical calculator acceptance is pending.** The reported
persistent flashing MENU is unreproduced and its cause is unknown. A separately
fixed input-boundary bug does not establish a cure. See the
[hardware procedure](docs/HARDWARE_RETEST.md) and [diagnostic guide](docs/DIAGNOSTICS.md).

Copy [NUMGAME.g3a](dist/NUMGAME.g3a) to USB storage, disconnect safely, and open
**NUM GAME** from the calculator's main menu. Verify [SHA256SUMS.txt](dist/SHA256SUMS.txt).
[NUMGDIAG.g3a](dist/NUMGDIAG.g3a) is the separate instrumented application **NUM DIAG**.
It uses ND archives and does not overwrite the normal NG archives. No external
puzzle file or Python runtime is needed on the calculator.

![Main menu](docs/captures/00-main.png)

## Games and challenge levels

| Category | Five games |
|---|---|
| GUESS | Number Baseball, Equation Guess, Number Mind, Clue Lock, Sequence |
| CALC | Make Target, Countdown, Missing Operators, Cross Math, Prime Factor |
| LOGIC | Sudoku, Calcudoku, Kakuro, Futoshiki, Skyscrapers |
| PUZZLE | Hitori, Binary, Numbrix, Magic Square, Sum Grid |
| STRATEGY | Nim, Wythoff, Euclid, Make Fifteen, Race to Target |
| BOARD | 2048, Sliding, Lights Out, Shikaku, Slitherlink |

The MASTER implementation changes actual challenges: longer codes and equations,
verified arithmetic constraints, harder grid reasoning, tactical strategy starts,
8192 TARGET 2048, and certified difficult Sliding/Lights Out starts. HARD already
has exact CPU policies; MASTER uses the same exact CPU with verified playable
positions. It does not claim a stronger perfect AI. Magic MASTER uses a documented
4×4 panmagic rule; multiple valid answers remain allowed. Human difficulty labels
are provisional, with machine evidence and limitations recorded in the audits.

LOGIC adds a separate **HELL**, stable value4; MASTER remains value3. Each LOGIC
game embeds 50 each EASY/NORMAL/HARD/MASTER and 30 HELL records. The
[content inventory](docs/CONTENT_INVENTORY.md) records actual completed,
embedded counts, before/after quantities, modes, canonical duplicates, provenance,
licenses, generator functions, asset hashes and memory use.
External puzzle files, parsed external puzzle records and bundled external puzzles
are zero. Rules-only references, licensed fonts and runtime code are separate.

Cheap constructors remain on-device. Expensive uniqueness and rating work runs
on the host and ships verified compact banks. One grid record is decoded at a
time. A persisted bounded shuffle cycle avoids repeats within a bank and avoids
immediately repeating the last puzzle at a cycle boundary. Magic layouts, symmetry
variants, rule-based games and tactical positions are not inflated into an
"infinite unique puzzles" claim. See [family audits](docs/GAME_CATALOG.md) and
[actual in-app rules](docs/RULES.md).

## Controls

Main/category menus have six tiles. Use digits1–6, or arrows and EXE/F6 OPEN.
Game menus default to NEW GAME. Digits focus a row; UP/DOWN selects rows;
LEFT/RIGHT changes settings with clamps. EXE/F6 OPEN starts a new game from NEW
or a setting row. Only the explicit RESUME row resumes the original saved settings.
Replacing progress asks for confirmation. F4 STATS and F5 RULES do not start games.

On a LOGIC difficulty row, **F3 HELL** selects a single red HELL label. F3 again
restores the prior general level; LEFT returns to MASTER; RIGHT stays. MASTER's
RIGHT never enters HELL. F3 disappears when the difficulty row loses focus.
Short mode lists appear inline. Longer lists use **F3 MODE** on the mode row;
arrows select, EXE/F6 OK returns to entry, and EXIT cancels. Confirmation in the
mode chooser never also launches a game. MASTER is always written in full.

| During play | Action |
|---|---|
| Arrows / digits | Game-specific movement and input |
| EXE / F6 | Displayed primary action |
| DEL | Edit draft or clear an editable cell |
| F1 INIT | Restart the same puzzle/run, marked assisted |
| F2 UNDO | Supported history only; CPU undo covers the human+CPU round |
| F3 HINT / REVEAL | Supported assistance, explicitly labeled |
| F4 | Game-specific NOTES, CHECK, ANSWER, etc. |
| F5 RULES | Scrollable actual rules |
| EXIT | Close modal; from play checkpoint and return to entry |
| MENU | Checkpoint and invoke CASIO main menu |
| SHIFT+AC/ON | Checkpoint and supported gint power-off |

**SHIFT+DOT enters `=`**; DOT alone is `.`, the power key is `^`, and the negative
key is `-`. Ordinary AC/ON does not exit. Shikaku EXIT first cancels a pending
corner. INIT/undo/hints/reveals/answers are assisted. Stats remain separate by game,
mode, level and assistance. 2048 CLASSIC retains its original rules; TARGET goals
512/1024/2048/8192 use separate records. [Full controls](docs/CONTROLS.md).

Normal text matches the native gint8×9 face used by DIFF EQ. All30 original tile
icons use compact geometry. 2048 colors progress gray/lime/cyan/blue/yellow/orange/
magenta/red, then darker reds. [Actual renderer captures](docs/captures/README.md)
are host RGB565 output, not calculator photographs.

## Saves, timing and diagnostics

Normal progress and preferences occupy exactly two fixed archives:
`NGARCA.dat` and `NGARCB.dat`, 462,032 bytes each, **924,064 bytes total**. Saves
stream only the selected record and verify the other copy after writing.
Version3 records retain readers for original three-/four-level saves, preserving
puzzle IDs, old MASTER meaning, stats and progress. No whole archive is loaded
into RAM. Back up both archives and any retained legacy files.
[Format and sizes](docs/STORAGE_FORMAT.md) · [Recovery/migration](docs/STORAGE_ARCHIVE_AUDIT.md).

One common timer updates active time without input. Entry/rules/modals and time
outside the app do not count. Real user activity controls dim/APO separately;
OS timeouts are used when supported, otherwise60s dim/10min APO. Timer ticks do
not reset inactivity. Physical brightness is restored before OS exit.

NUM DIAG adds Memory/Runtime pages, RESET, bounded 1,000-operation RAM stress,
and explicit export to fixed `NDDIAG.txt`. Fixtures never overwrite player
progress. Project stack samples, two gint arena statistics, codec use, handles,
timers and operation timing have explicit coverage limits. **Hardware measured
peak: NOT MEASURED.** Do not add heap-owned VRAM again or call a compiler frame
sum a measured RAM peak. [Measurement method](docs/MEMORY_METHOD.md) ·
[Native sections/frames](docs/MEMORY_AUDIT.md) ·
[Host performance](docs/PERFORMANCE_AUDIT.md) · [Device steps](docs/DIAGNOSTICS.md).

## Build and verify

Reuse an installed fxSDK/gint2.11 SDK with SH GCC14.1, CMake, fxgxa and Python3.
Set `NUMGAME_SDK_ROOT` if it is outside `$HOME/.local/diffeq-sdk`. These commands
never modify the SDK. Pillow is needed only for asset/capture generation.

```sh
bash tools/test.sh
bash tools/verify_content.sh
bash tools/clean_build.sh
bash tools/clean_build.sh --diagnostic
source tools/env.sh
python3 tools/captures.py
python3 tools/memory_report.py --build-dir <fresh-native-build> --g3a dist/NUMGAME.g3a
```

Use a separate CMake host directory with `-DNG_DIAGNOSTIC=ON` for instrumentation
regressions and `benchmark_host`. `-DNG_ASAN=ON` enables ASan on a working runtime.
The available macOS ASan runtime hangs before even a minimal program reaches
main; **ASan is not verified**. UBSan and native adapter checks do not replace it
or hardware acceptance. The release package target is1,000,000 bytes; the hard
size gate is1,200,000 decimal bytes of the actual G3A, before compression.

Original code, icons and generated content use [MIT](LICENSE). Retained runtime,
font and adapted infrastructure notices are in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)
and [asset provenance](docs/ASSET_PROVENANCE.md). No commercial puzzle corpus,
proprietary CASIO font, user progress, SDK installation or private document is
required by this source tree.
