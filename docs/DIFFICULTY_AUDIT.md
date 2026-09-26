# Difficulty audit — 36 visible games, beta.3

The [150-row CSV](DIFFICULTY_AUDIT.csv) has one row per visible game and
EASY/NORMAL/HARD/MASTER level, plus HELL for all six LOGIC games. It records the
actual supply or CPU policy, bank counts, board dimensions, deterministic sample
distributions, level overlap, findings and fixes. No retired game ID or mode
variant is counted as an additional game. The four independent source reports
are [GUESS/CALC](GUESS_CALC_AUDIT.md), [LOGIC/PUZZLE](GRID_AUDIT.md),
[STRATEGY/quick boards](STRATEGY_QUICK_AUDIT.md) and
[SHIKAKU/SLITHERLINK](BOARD_AUDIT.md). The combined CSV is checked for the exact
36-ID/150-row coverage and 14-column schema by
`python3 tools/generate/difficulty_audit.py --check`.

## Findings and decisions

**Two game IDs were corrected.** MAGIC SQUARE FREE (19) previously gave EASY
and NORMAL the same 3×3 order; new revision-3 runs use 3×3/4×4/5×5/6×6 and
normal magic sums 15/34/65/111. LIGHTS OUT (28) previously generated 4×4
E/N/H with increasing press counts but measured minimum-solution means
4.0000/4.5547/4.4375, so HARD was easier than NORMAL by this metric. New
revision-3 4×4 starts have independently checked exact minima 2/4/5/6 for
E/N/H/M. Both games keep revision-1/2 runs at their original board and rule
semantics on RESUME and INIT. Other game logic, banks and stable IDs remain.

**Seven evidence-based decisions remain REVIEW REQUIRED:** GUESS/CALC has six
documented findings GC-D01–06 covering Countdown's two one-card HARD wins,
Sequence EASY/NORMAL family concentration, Prime Factor MASTER domain/target
overlap, relaxed MASTER guess caps, Make Target constructive-witness limits,
and Cryptarithm rating overlap. Sliding Puzzle EASY/NORMAL/HARD shortest-path
distributions overlap substantially. There is no demonstrated selector no-op
in those games; silently changing their legacy bank meaning or inventing a new
human-calibrated grade would be unjustified. Other bank-search and clue bands
also overlap and are reported rather than presented as strict human ratings.

**One intentional fixed exception:** 2048 CLASSIC has one rule set; its entry
hides the difficulty row and starts at NORMAL. TARGET exposes 512/1024/2048/8192
goals. This is a mode-specific terminal objective, not a claim that CLASSIC
has four distinct difficulties. The six LOGIC HELL levels have their own
non-overlapping source-bank records. For IDs 11–15 the measured MASTER maximum
search-node count is below HELL minimum; HASHI has larger HELL geometry but
overlapping search-node ranges. HELL remains the separate F3 choice.

## Actual level mechanisms by game

| ID | Game | Main distinction across levels |
|---:|---|---|
| 01 | NUMBER BASEBALL | 4/5/6/7-digit repeated-digit code, differing guess caps |
| 02 | EQUATION GUESS | Operator count, precedence and equation mode banks |
| 03 | NUMBER MIND | Position/alphabet domain 1,296→262,144 |
| 04 | CLUE LOCK | Numeric domain and clue constraints |
| 05 | SEQUENCE | Different grammar-family bank mixtures; E/N review |
| 06 | MAKE TARGET | Four/four/five/six cards and constructive expression template; user target 1–1000 |
| 07 | COUNTDOWN | Large-card composition; MASTER needs all six and division |
| 08 | MISSING OPERATORS | Three to six operands; MASTER constrained bank solutions |
| 09 | CROSS MATH | Fixed-clue density and candidate-axis complexity |
| 10 | PRIME FACTOR | Factor-draw structure; MASTER domain review |
| 11 | SUDOKU | Distinct banks, clue and search-inference bands, HELL |
| 12 | CALCUDOKU | Distinct banks, cage and search-inference bands, HELL |
| 13 | KAKURO | Distinct banks, order/clues and search-inference bands, HELL |
| 14 | FUTOSHIKI | Distinct banks, inequalities and search-inference bands, HELL |
| 15 | SKYSCRAPERS | Distinct banks, clues and search-inference bands, HELL |
| 16 | HITORI | Distinct banks, board/clue/inference bands |
| 17 | BINARY PUZZLE | Distinct banks, board/clue/inference bands |
| 18 | NUMBRIX | Distinct banks, board/clue/inference bands |
| 19 | MAGIC SQUARE | PARTIAL clue/order banks; FREE normal order 3/4/5/6 |
| 20 | SUM GRID | Arithmetic range/size and, at MASTER, interacting subsets |
| 21 | NIM | Pile range and random/75%-exact/exact CPU; MASTER tactical starts |
| 22 | WYTHOFF | Pile range and CPU policy; MASTER tactical starts |
| 23 | EUCLID | Pair range and CPU policy; MASTER tactical starts |
| 24 | MAKE FIFTEEN | CPU policy and MASTER reachable midgame starts |
| 25 | RACE TO TARGET | Target/add limits and CPU policy; MASTER tactical starts |
| 26 | 2048 | TARGET goal levels; CLASSIC fixed exception |
| 27 | SLIDING PUZZLE | Shuffle/Manhattan parameters; certified MASTER starts |
| 28 | LIGHTS OUT | Certified minimum presses on 4×4; 5×5 press/rank distribution |
| 31 | SHIKAKU | 5×5/6×6/8×8 verified banks and clue/solver structure |
| 32 | SLITHERLINK | 5×5/6×6/8×8 verified banks and clue/solver structure |
| 33 | BLACK BOX | 5×5→8×8, three→six atoms and 20→32 ports |
| 34 | CRYPTARITHM | Two-addend length 2→5, letter/carry/solver distribution |
| 35 | HASHI | Distinct geometry and bank/inference bands, including HELL |
| 36 | NONOGRAM | Distinct banked clue/size/inference bands |
| 37 | REVERSI | Random to deeper bounded search and distinct decision probes |
| 38 | NET | Seeded rotated-tree order 3×3→6×6 and junction structure |

GUESS/CALC audited 12,022 deterministic initializations and matching replays;
LOGIC/PUZZLE replayed all 2,020 bank records plus four FREE configurations;
the STRATEGY/quick audit sampled 11,101 starts/decisions, including 12,288
additional LIGHTS regression starts; SHIKAKU/SLITHERLINK checked all 240
records. Exact or documented-canonical cross-level bank intersections were zero
for audited bank sets. The documented equivalence scopes are limited, and a
structural level difference does not guarantee every individual puzzle feels
harder than every puzzle below it. No human solving-time study was performed.

The entry selector clamps at EASY/MASTER rather than wrapping, passes the
selected value to NEW GAME, and preserves RESUME's own settings. HELL retains
F3/ENHM behavior. MAGIC FREE and LIGHTS version tests cover old/new save
compatibility. Host UBSan, content checks, strict SH and actual-renderer
captures are summarized in [ACCEPTANCE.md](ACCEPTANCE.md). Physical LCD,
flash timing and MENU behavior remain **HARDWARE TEST REQUIRED**.
