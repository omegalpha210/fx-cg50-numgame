English | [한국어](README_KO.md)

<p align="center"><img src="docs/images/icon-2x.png" width="184" height="128" alt="Original NUM GAME fx-CG50 add-in icon, enlarged by an exact 2× nearest-neighbor scale"></p>

# NUM GAME for CASIO fx-CG50

**36 native number games** in one offline `.g3a` add-in. Pick a category, set up a game, and play without a separate puzzle file. The original icons and screens below come from the app itself.

**Experimental prerelease · v0.2.0-beta.8 · [MIT License](LICENSE)**

**[Download NUM GAME](https://github.com/omegalpha210/fx-cg50-numgame/releases/download/v0.2.0-beta.8/NUMGAME.g3a)** (regular play) · [NUM DIAG](https://github.com/omegalpha210/fx-cg50-numgame/releases/download/v0.2.0-beta.8/NUMGDIAG.g3a) (instrumented troubleshooting) · [Checksums](https://github.com/omegalpha210/fx-cg50-numgame/releases/download/v0.2.0-beta.8/SHA256SUMS.txt) · [Release notes](https://github.com/omegalpha210/fx-cg50-numgame/releases/tag/v0.2.0-beta.8) · [Report an issue](https://github.com/omegalpha210/fx-cg50-numgame/issues/new)

Host tests and strict SH builds pass. Physical fx-CG50 acceptance is still pending, including the previously reported MENU flashing, LCD readability, BFile latency and memory peaks. [Hardware retest procedure](docs/HARDWARE_RETEST.md).

The current source and screenshots include a [caret spacing fix](docs/UI_CONVENTIONS.md); the downloadable beta.8 packages predate this source update.

![NUM GAME main menu with six colored category tiles](docs/images/main.png)

*Every screen on this page is a 396×224 capture from the current common-C renderer on a host. These are not photographs of the calculator or CPU-emulator output. Open an image for its full-size view, especially on a phone.*

## Choose from six categories

| Category | Six games |
|---|---|
| **GUESS** | Number Baseball · Equation Guess · Number Mind · Clue Lock · Sequence Detective · Black Box |
| **CALC** | Make Target · Countdown · Missing Operators · Cross Math · Prime Factor · Cryptarithm |
| **LOGIC** | Sudoku · Calcudoku · Kakuro · Futoshiki · Skyscrapers · Hashi |
| **PUZZLE** | Hitori · Binary Puzzle · Numbrix · Magic Square · Sum Grid · Nonogram |
| **STRATEGY** | Nim · Wythoff · Euclid · Make Fifteen · Race to Target · Reversi |
| **BOARD** | 2048 · Sliding Puzzle · Lights Out · Shikaku · Slitherlink · Net |

Each has EASY, NORMAL, HARD and MASTER. The six LOGIC games also have HELL. The labels follow game rules or checked solver metrics, not measured human difficulty. [All game rules](docs/RULES.md) · [content inventory](docs/CONTENT_INVENTORY.md) · [difficulty audit](docs/DIFFICULTY_AUDIT.md).

## Take a closer look

| GUESS: Number Baseball | CALC: Make Target |
|---|---|
| ![Baseball attempt history and input on the native game screen](docs/images/baseball.png) | ![Make Target cards and arithmetic expression input](docs/images/make-target.png) |
| Repeated digits are allowed; 4–7 digits by difficulty. | Use every card exactly once; choose 10, 24, 50, 100, 200 or RANDOM. |

| CALC: Prime Factor editor | CALC: Cryptarithm |
|---|---|
| ![Prime Factor formula with x squared inserted in the middle](docs/images/prime-editor.png) | ![Cryptarithm letter and number puzzle](docs/images/cryptarithm.png) |
| LEFT/RIGHT moves the caret; physical x² inserts `^2` here only. | Resolve letters under the displayed arithmetic constraints. |

| LOGIC: Sudoku | LOGIC: Kakuro |
|---|---|
| ![Sudoku board with selected cell and keypad](docs/images/sudoku.png) | ![Kakuro grid and clue sums](docs/images/kakuro.png) |
| Fill the grid under row, column and box rules. | Match the across and down sums. |

| PUZZLE: Nonogram | PUZZLE: Magic Square |
|---|---|
| ![Nonogram grid with row and column clues](docs/images/nonogram.png) | ![Magic Square number grid](docs/images/magic-square.png) |
| Mark the black cells described by the clues. | Balance the required rows, columns and diagonals. |

| STRATEGY: Reversi | BOARD: 2048 |
|---|---|
| ![Reversi board against the CPU](docs/images/reversi.png) | ![2048 colored tile grid](docs/images/2048.png) |
| Play the CPU; choose who moves first. | CLASSIC has fixed rules; optional target modes add a finish condition. |

| BOARD: Shikaku | BOARD: Slitherlink |
|---|---|
| ![Shikaku rectangle partition puzzle](docs/images/shikaku.png) | ![Slitherlink edge and clue puzzle](docs/images/slitherlink.png) |
| Partition the grid into clue-sized rectangles. | Draw one closed loop satisfying the clues. |

[More actual renderer captures](docs/captures/README.md) · [Curated gallery](docs/GALLERY.md).

## From menu to a finished run

**Category → Game → Difficulty and applicable settings → NEW GAME → Play.** Use arrows or 1–6 on the menus, then EXE or F6 OPEN. On the game-entry screen, UP/DOWN selects NEW GAME, difficulty or a relevant setting; LEFT/RIGHT changes the selected setting. F5 opens RULES. The same game's unfinished run appears as RESUME, and Main F1 opens it directly. F3 toggles LOGIC HELL when applicable. Strategy FIRST selects YOU or CPU.

In Equation Guess, Make Target, Countdown and Prime Factor, LEFT/RIGHT moves within a draft formula, DEL removes the character before the caret, and new characters insert at the caret. Long expressions scroll within the input box. The physical `^` key remains `^`; **x² inserts `^2` only in Prime Factor**, whose rules permit powers. SHIFT+DOT still enters `=` in Equation Guess. Grid navigation, card selection and UP/DOWN history scrolling retain their own controls. [Full controls](docs/CONTROLS.md).

HINT or ANSWER marks a run assisted where offered. An invalid submitted formula remains editable. Completion shows game-specific results; Prime Factor shows time (when enabled) and ASSISTED only when used, without calling its submission counter MOVES or displaying SCORE 0. EXIT from the completion dialog opens the frozen final view; EXE or F6 NEW starts another game.

![Prime Factor completion panel with time and assisted status](docs/images/prime-result.png)

## Saves, content and builds

The app keeps **one unfinished run**. Starting NEW safely replaces it; completing a game removes its RESUME entry. Two exact-length `NGSTATEA/B.dat` copies protect the one logical save (252 bytes together when empty, 3,996 bytes for a new active run). The diagnostic app uses separate ND files. Old supported saves still load, including earlier Make Target packs. [Save format](docs/STORAGE_FORMAT.md) · [resume policy](docs/RESUME_POLICY.md).

Make Target uses 4/4/5/6 cards by difficulty and every card exactly once; Countdown may leave cards unused. Make Target's same 4,000 verified beta.6 problems occupy a 63,000-byte packed bank in beta.7 and later, with unchanged cards, hints, answers, IDs and draw order. The bank and other content are independently checked, but this is not a claim of certified human difficulty or hardware validation. [Packing audit](docs/MAKE_TARGET_PACKING_AUDIT.md) · [user guide](docs/USER_GUIDE.md).

Copy the regular `.g3a` to the calculator's USB storage and disconnect safely. NUM DIAG is for explicit runtime/memory investigation and can export `NDDIAG.txt`; it is not required for play. Building from source needs the installed fxSDK/gint 2.11, SH GCC 14.1, CMake, fxgxa and Python 3. Set `NUMGAME_SDK_ROOT` if the SDK is elsewhere:

```sh
bash tools/test.sh
bash tools/verify_content.sh
bash tools/clean_build.sh
bash tools/clean_build.sh --diagnostic
python3 tools/curate_readme.py
```

Build outputs stay under `build/host/`, `build/clean/normal/` and `build/clean/diagnostic/`; `dist/` holds the user-facing G3A packages.

ASan is unverified because its runtime stalls before `main` on the available macOS host. Host tests, UBSan and SH builds cannot establish device latency or resolve the reported MENU issue. [Development and diagnostic notes](docs/DIAGNOSTICS.md) · [code cleanup audit](docs/CODE_CLEANUP_AUDIT.md) · [third-party notices](THIRD_PARTY_NOTICES.md).
