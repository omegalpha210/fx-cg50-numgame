# Game catalog and acceptance matrix

Stable IDs1–28,31,32; legacy IDs29/30 are archived, never reused. All 30 have an engine, playable UI,
verified content, per-game save/resume, host verification and a linked SH
target. Every row remains **HARDWARE PENDING**. Counts below never include
difficulty, size, CPU or local-two-player variants as extra games.

| ID | Game | Modes | CPU / local 2P | Undo | Hint / reveal | Status |
|---:|---|---|---|---|---|---|
| 01 | NUMBER BASEBALL | MID UNIQUE / SHORT UNIQUE / LONG UNIQUE / MID REPEAT / SHORT REPEAT / LONG REPEAT | — | — | — | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 02 | EQUATION GUESS | STANDARD / SHORT / LONG | — | — | — | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 03 | NUMBER MIND | STANDARD | — | — | HINT | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 04 | CLUE LOCK | STANDARD | — | — | HINT | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 05 | SEQUENCE DETECTIVE | STANDARD | — | — | HINT | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 06 | MAKE TARGET | TARGET 24 / TARGET 10 | — | — | HINT; ANSWER | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 07 | COUNTDOWN | STANDARD | — | — | HINT | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 08 | MISSING OPERATORS | STANDARD | — | Yes | —; REVEAL | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 09 | CROSS MATH | STANDARD | — | Yes | —; REVEAL | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 10 | PRIME FACTOR | STANDARD | — | — | HINT; ANSWER | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 11 | SUDOKU | CLASSIC | — | Yes | REVEAL | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 12 | CALCUDOKU | CLASSIC | — | Yes | REVEAL | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 13 | KAKURO | CLASSIC | — | Yes | REVEAL | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 14 | FUTOSHIKI | CLASSIC | — | Yes | REVEAL | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 15 | SKYSCRAPERS | CLASSIC | — | Yes | REVEAL | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 16 | HITORI | CLASSIC | — | Yes | REVEAL | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 17 | BINARY PUZZLE | CLASSIC | — | Yes | REVEAL | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 18 | NUMBRIX | CLASSIC | — | Yes | REVEAL | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 19 | MAGIC SQUARE | PARTIAL / FREE | — | Yes | — | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 20 | SUM GRID | CLASSIC | — | Yes | REVEAL | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 21 | NIM | CPU / YOU FIRST / CPU / CPU FIRST / LOCAL 2P | Yes / Yes | Yes | — | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 22 | WYTHOFF | CPU / YOU FIRST / CPU / CPU FIRST / LOCAL 2P | Yes / Yes | Yes | — | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 23 | EUCLID | CPU / YOU FIRST / CPU / CPU FIRST / LOCAL 2P | Yes / Yes | Yes | — | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 24 | MAKE FIFTEEN | CPU / YOU FIRST / CPU / CPU FIRST / LOCAL 2P | Yes / Yes | Yes | — | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 25 | RACE TO TARGET | CPU / YOU FIRST / CPU / CPU FIRST / LOCAL 2P | Yes / Yes | Yes | — | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 26 | 2048 | CLASSIC / TARGET | — | Yes | — | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 27 | SLIDING PUZZLE | 3 x 3 / 4 x 4 | — | Yes | — | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 28 | LIGHTS OUT | 4 x 4 / 5 x 5 | — | Yes | HINT | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 31 | SHIKAKU | STANDARD | — | Yes | — | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 32 | SLITHERLINK | STANDARD | — | Yes | — | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |

All rows passed a real app-key/renderer workflow through terminal result,
checkpoint and cold-load result. Storage separately covers 213 visible
game × mode × difficulty configurations (including the identical classic
2048 difficulty slots for compatibility). Engine family suites exercise
additional seeds, illegal inputs, alternate answers and failure outcomes.
The CSV [status matrix](acceptance-matrix.csv) records each stage.

Assistance includes undo, hint/reveal/answer and same-seed INIT. No hidden
guess undo is offered. Statistics are separated by game/mode/difficulty.
The game-menu record viewer additionally separates normal and assisted.
Before/after content quantities and transformation caveats are in
[CONTENT_INVENTORY.md](CONTENT_INVENTORY.md).
