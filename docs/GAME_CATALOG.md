# Game catalog and acceptance matrix

Stable IDs1–28,31–38; legacy IDs29/30 are archived, never reused. All 36 have an engine, playable UI,
verified content, one shared active-run save/resume policy, host verification and a linked SH
target. Every row remains **HARDWARE PENDING**. Counts below never include
difficulty, size, or CPU-turn variants as extra games.

| ID | Game | Modes | CPU | Undo | Hint / reveal | Status |
|---:|---|---|---|---|---|---|
| 01 | NUMBER BASEBALL | STANDARD | — | — | — | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 02 | EQUATION GUESS | STANDARD / SHORT / LONG | — | — | — | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 03 | NUMBER MIND | STANDARD | — | — | HINT | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 04 | CLUE LOCK | STANDARD | — | — | HINT | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 05 | SEQUENCE DETECTIVE | STANDARD | — | — | HINT | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 33 | BLACK BOX | STANDARD | — | Yes | HINT | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 06 | MAKE TARGET | STANDARD | — | — | HINT; ANSWER | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 07 | COUNTDOWN | STANDARD | — | — | HINT | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 08 | MISSING OPERATORS | STANDARD | — | Yes | —; REVEAL | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 09 | CROSS MATH | STANDARD | — | Yes | —; REVEAL | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 10 | PRIME FACTOR | STANDARD | — | — | HINT; ANSWER | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 34 | CRYPTARITHM | STANDARD | — | Yes | — | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 11 | SUDOKU | CLASSIC | — | Yes | REVEAL | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 12 | CALCUDOKU | CLASSIC | — | Yes | REVEAL | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 13 | KAKURO | CLASSIC | — | Yes | REVEAL | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 14 | FUTOSHIKI | CLASSIC | — | Yes | REVEAL | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 15 | SKYSCRAPERS | CLASSIC | — | Yes | REVEAL | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 35 | HASHI | CLASSIC | — | Yes | REVEAL | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 16 | HITORI | CLASSIC | — | Yes | REVEAL | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 17 | BINARY PUZZLE | CLASSIC | — | Yes | REVEAL | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 18 | NUMBRIX | CLASSIC | — | Yes | REVEAL | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 19 | MAGIC SQUARE | PARTIAL / FREE | — | Yes | — | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 20 | SUM GRID | CLASSIC | — | Yes | REVEAL | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 36 | NONOGRAM | CLASSIC | — | Yes | REVEAL | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 21 | NIM | YOU FIRST / CPU FIRST | Yes | Yes | — | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 22 | WYTHOFF | YOU FIRST / CPU FIRST | Yes | Yes | — | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 23 | EUCLID | YOU FIRST / CPU FIRST | Yes | Yes | — | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 24 | MAKE FIFTEEN | YOU FIRST / CPU FIRST | Yes | Yes | — | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 25 | RACE TO TARGET | YOU FIRST / CPU FIRST | Yes | Yes | — | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 37 | REVERSI | YOU FIRST / CPU FIRST | Yes | Yes | HINT | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 26 | 2048 | CLASSIC / TARGET | — | Yes | — | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 27 | SLIDING PUZZLE | 3 x 3 / 4 x 4 | — | Yes | — | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 28 | LIGHTS OUT | 4 x 4 / 5 x 5 | — | Yes | HINT | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 31 | SHIKAKU | STANDARD | — | Yes | — | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 32 | SLITHERLINK | STANDARD | — | Yes | — | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |
| 38 | NET | STANDARD | — | Yes | REVEAL | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |

The retained 30 games passed real app-key/renderer workflows through terminal
result, frozen result view and cold-load **no-resume**; the six new games have
focused family audits for the same transitions. Storage separately covers 198 visible
game × mode × difficulty configurations (including the identical classic
2048 difficulty slots for compatibility). Engine family suites exercise
additional seeds, illegal inputs, alternate answers and failure outcomes.
The CSV [status matrix](acceptance-matrix.csv) records each stage.

Assistance includes undo, hint/reveal/answer and same-seed INIT. No hidden
guess undo is offered. Only the app's last unfinished run is resumable.
See [RESUME_POLICY.md](RESUME_POLICY.md) and [RECENT_SIX_AUDIT.md](RECENT_SIX_AUDIT.md).
The earlier 30-game content quantities and transformation caveats are in
[CONTENT_INVENTORY.md](CONTENT_INVENTORY.md).
