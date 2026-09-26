# Strategy and quick game audit

Current owned visible IDs are 21–28, 37 and 38. IDs 29/30 remain implemented
only for legacy archive inspection; they are not visible menu entries. New
strategy runs have YOU FIRST and CPU FIRST only. Old LOCAL 2P states can be
decoded for inspection when pack revision is at most 2, but their actions are
disabled and the app migration excludes these runs. The new MASTER content is original, generated on the host, and verified
with independent rule models. MASTER uses the same exact CPU as HARD; no stronger
than perfect-play claim is made. Human difficulty has not been calibrated.

## Before and after supply

Baseline is commit `4e7da73`. IDs 21–26 were rule-based games without a puzzle
bank; IDs 27/28 generated solvable starts at runtime. All had only E/N/H behavior.
The 6,382-byte AI policy table is state analysis, not a puzzle corpus.

| ID | E/N/H retained behavior | MASTER content / distinct original starts |
|---|---|---|
|21 Nim|Random 3/3/4 piles, maxima 7/15/31|Four-pile tactical starts, 30 YOU FIRST + 30 CPU FIRST|
|22 Wythoff|Two random piles, maxima 10/24/40|Long exact-play starts, 30 YOU FIRST + 12 CPU FIRST|
|23 Euclid|Random positive pairs, maxima 12/40/99|Coprime multi-step starts, 30 + 30|
|24 Make Fifteen|Empty 1..9 card board; CPU difficulty varies|Verified reachable midgames, 30 + 5 after Lo Shu symmetry removal|
|25 Race|Targets 21/31/23, add maxima 3/3/4|Varied target/add-limit tactical starts, 30 + 30; equivalent remaining-distance positions removed|
|26 2048|CLASSIC mode 0 unchanged; new TARGET mode 1 uses goals 512/1024/2048|TARGET 8192; no puzzle bank|
|27 Sliding|3×3/4×4 legal 80/160/240-step shuffles|3×3: exactly two distance-31 boards; 4×4: 30 starts with Manhattan lower bound 48–52|
|28 Lights|4×4/5×5 runtime legal press construction|30 D4-distinct starts per size; certified minimum 6 on 4×4 and 12–14 on 5×5|

CPU FIRST starts are losing for the CPU to move, so YOU can force a win; YOU
FIRST starts are winning for YOU. Former LOCAL 2P bank sharing never counted
as another set of originals. All 257 strategy starts were
verified. The small Wythoff and Make Fifteen CPU-first banks are reported as
12 and 5, not padded by equivalent copies. The latter removes Lo Shu D4
symmetries; Euclid removes common-factor scaling; Nim sorts pile order; Race
removes translated total/target pairs with the same maximum and remaining gap.

The game-level supply classification is HYBRID for 21–25 and 27/28: rule-based
or runtime E/N/H, host bank for MASTER. 2048 remains a rule-based game without a
puzzle bank. There are 349 new bank records: 257 strategy + 32 Sliding + 60 Lights.
Each selected record has one runtime variant. Do not count mode sharing or a
shuffle path as additional bases.

Supply entry points are `sq_strategy_init()` and `sq_quick_init()`. MASTER
selection uses `sq_bank_count()` and common `ng_bank_pick()`, preserving the
selected difficulty/mode. No runtime search/generator constructs MASTER starts.

## Provenance and reproduction

External puzzle files: **0**. External puzzle records bundled: **0**.
Evidence includes the actual generator inputs (only deterministic seed and
standard-library algorithms), Git introduction of the policy generator in
`577606f`, and byte-identical in-memory regeneration of the baseline policy
header. Its SHA256 is
`21520069458881ee29026926ca68e3e561f4fcd19af3229c40ead788d75b747c`.
The new generator has no external corpus or network input. Original source and
problem data are covered by the repository MIT license. Rules-only references
and separately licensed fonts/runtime are documented in ASSET_PROVENANCE.md.

- `tools/generate/strategyquick_tables.py` → `assets/strategyquick/tables.h`
  and metadata.json: bounded exact policy, unchanged.
- `tools/generate/strategyquick_master.py` → master.json, master_strategy.h,
  master_quick.h: original deterministic seed `202609230128`.
- `tools/generate/strategyquick_master_verify.py`: independent public-state
  verification and actual C initializer comparison; no generator imports.
- master_verification.json records counts, hashes and measured host duration.

```sh
python3 tools/generate/strategyquick_master.py
python3 tools/generate/strategyquick_master_verify.py
cmake -S tests -B build-host -DNG_SANITIZE=ON -DNG_ASAN=OFF
cmake --build build-host --target test_strategyquick -j8
./build-host/test_strategyquick
```

The native target includes compact starts and public state, not the host
solution paths or rating logs. Completion remains based on game rules rather
than equality to a witness.

## Independent correctness and regression

Strict C11 `-Wall -Wextra -Werror` and UBSan tests passed against the actual
common registry, application, renderer and codec. The current focused checks
link all production modules directly and use no temporary adapters.

| Check | Verified coverage |
|---|---|
|Nim|1,048,575 nonterminal four-pile states in 0..31 against an independent move-DAG, including every selected exact move|
|Wythoff|1,680 nonterminal pairs in 0..40, row-major DP versus generator increasing-sum DP|
|Euclid|4,950 positive unordered pairs through 99, independent increasing-sum successor DAG|
|Make Fifteen|11,093 nonterminal disjoint ownership states; independent Lo Shu lines versus arithmetic triples; winning fourth-card triple and draw|
|Race|All 75 original preset states plus 441 MASTER-domain remaining-distance/add-limit states; independent DP versus exact residue policy|
|Original CPU lifecycle|600 complete seeded games over five games × three levels × two CPU modes × 20 seeds|
|MASTER starts|349 mode-specific no-repeat loads; 257 strategy playthroughs using exact play end in YOU wins; INIT, codec/undo snapshots and invalid identity checks|
|2048|Merge once, no-op RNG, deterministic replay, exponent/score caps; 10,000 starting spawns yielded 9,016 twos and 984 fours|
|2048 TARGET|All four terminal goals, replay/undo and codec; historical compatibility tests check mode-separated record structures; CLASSIC remains active after reaching 8192|
|Sliding|1,200 original seeded starts; independent inversion parity; MASTER full 181,440-state 3×3 BFS and all 30 4×4 legal-path witnesses/admissible Manhattan bounds|
|Lights|1,200 seeded E/N/H starts and all 65,535 nonempty 4×4 press subsets; MASTER matrix RREF plus full affine nullspace versus generator row chasing certifies every minimum|
|Actual app|64 setting requests (61 distinct new configurations): NEW, INIT, legal input, CPU, round UNDO, explicit RESUME, EXIT/MENU and codec|
|Legacy 29/30|Nine mode/difficulty codec cases plus 600 Rush timed/practice checks and 300 full 20-round Memory runs|
|Removed LOCAL 2P|20 level/game initial-state codec cases plus 161 played states, including 19 P2 wins; preserved ownership/result meaning, disabled actions and revision-3 rejection|

The retained strategy lifecycle and app counts exclude removed LOCAL 2P selection.
The executable also rejects corrupted dimensions, cursor, phase, mode, CPU
flags, terminal state, MASTER IDs/metadata, 2048 goals, timer ranges, arithmetic
answers and unterminated inputs. Checkpoint hooks use production encoded bytes;
native BFile and OS switching are covered by root's native adapter tests, not
by these host input tests.

## Compatibility, controls and assistance

CLASSIC 2048 is still mode 0: data[3] is zero, 2048 achievement allows continued
play, and only lack of legal moves ends the run. TARGET is mode 1 and stores
the goal exponent in data[3]; reaching it wins. Each difficulty has its own
target goal. The app starts new CLASSIC runs at its fixed NORMAL level while
retaining the selected TARGET level; compatible unfinished CLASSIC E/N/H runs
resume unchanged. Version 5 does not persist cumulative records or best scores.
Tests of mode-separated record structures cover historical compatibility only.
Exponents cap at 30 and scores saturate at UINT32_MAX.

Existing revisions retain their E/N/H initialization and puzzle identities.
The beta.3 Lights revision-3 change is detailed below. MASTER
Make Fifteen stores its even preplayed ply count separately from new player
moves; ownership counts and alternating turns are validated. HELL is rejected
by all these modules. Legacy 29/30 still reject difficulty above HARD.

Arrows select piles/cards or move the Sliding blank; digit + EXE submits
strategy amounts, card digits play that card directly. 2048 uses arrows only.
Lights uses arrows then EXE/5; HINT gives a valid GF(2) press, not a promised
minimum solution. Shifting a cursor or rendering does not consume game RNG.
Human strategy input only queues CPU work; the next CPU event performs one move.
UNDO restores the pre-human round and RNG and marks assistance. INIT/HINT also
mark practice as specified by the common app. No hidden-answer UNDO is added
to legacy Rush/Memory. Original timed phases and release barriers are retained.

## Memory and hardware limits

No engine heap allocation, I/O, clock or keyboard dependency exists. AI readonly
payload stays 6,382 B. MASTER strategy arrays occupy 3,610 B including fixed
array padding/counts; MASTER quick arrays occupy 798 B. Only one start is copied
into the existing fixed NgGame. No entire-bank RAM decompression occurs.

The former full-NgGame temporary in CPU evaluation and validation was removed.
The strict SH `-Os` compiler reports **56 B** each for sq_strategy_pick and
sq_strategy_valid, versus baseline **1,880/1,872 B**. sq_strategy_init is 28 B,
sq_quick_init 148 B, sq_lights_solution 236 B. These are individual frames,
not call-chain peaks or device RAM measurements. Full-game buffers owned by
common storage are accounted for in the root memory report.

Current UBSan checks passed. An ASan build/run attempt is logged in
`assets/strategyquick/master-asan-attempt.log`; a timeout is NOT an ASan pass.
Hardware stack/heap peak, physical timing, LCD readability, MENU/Fugue,
SHIFT+AC/ON and power-loss recovery remain **HARDWARE TEST REQUIRED**. Neither
original MENU flicker nor physical-device peak is claimed solved/measured here.


## REVERSI and NET expansion (IDs 37/38)

Both games have original fixed-state C engines and renderers in
`src/games/strategyquick_extra.c`. No external program code, puzzle corpus,
image, or sound was copied. Their rule descriptions were checked against the
[World Othello Federation official rules](https://www.worldothello.org/about/about-othello/othello-rules/official-rules/english)
and [Simon Tatham's Net documentation](https://www.chiark.greenend.org.uk/~sgtatham/puzzles/doc/net.html),
respectively; these are rules references, not bundled assets. Original source,
tests and renderer output use the repository MIT license.

| Game | Content supply and levels | Actual controls |
|---|---|---|
|37 REVERSI|Fixed standard 8×8 opening; CPU opponent only, YOU FIRST / CPU FIRST. No puzzle bank.|Arrows select; EXE or 5 places; F4 toggles legal-move dots; F3 suggests an assisted move.|
|38 NET|Runtime seeded tree construction, then rotations. E/N/H/M grids are 3×3/4×4/5×5/6×6. No puzzle bank.|Arrows select; EXE or 5 rotates clockwise; DEL anticlockwise; F4 locks/unlocks; F3 REVEAL restores and locks one original orientation, marking assistance.|

Reversi implements all eight flip directions, automatic forced passes, and
termination when neither player can move, including non-full boards. Counts
determine human win, loss or draw. CPU work is deferred through `cpu_pending`;
there is no synchronous opponent move inside human input. UNDO restores the
whole human/CPU round and RNG through the common app.

Reversi Easy selects a random legal move. Normal uses one ply with a 256-node
budget; Hard uses iterative deepening through three plies with 1,500 nodes;
MASTER through five plies with 6,000 nodes. Budgets cover the whole decision,
not each iteration. Only complete iterations replace the chosen move. The
heuristic values corners, mobility, unsafe corner neighbours, edges and discs.
It is bounded play, not a full-game exact solver or a calibrated skill rating.
The search does become exact in the separately tested reachable endgames with
at most five empty cells; that result is not generalized to the opening.

Net completion checks reciprocal wires, no boundary escape, connectivity from
the central source and exactly one fewer undirected edge than tiles. Together
these enforce one connected tree. Completion never compares the board with its
construction witness: the independent test finds and accepts an alternative
valid orientation for seed 52 on 3×3. The retained construction is used only
to validate immutable tile shapes and to provide the explicitly assisted
REVEAL. Generated orientation puzzles are not promised unique, rated, or
nonrepeating between different seeds. Rotations, lock changes and reveals count
as moves so the common app can undo them. Runtime construction uses bounded
randomized frontier selection over at most 36 cells; it does not solve a puzzle.

Independent coverage in `tests/test_strategyquick_extra.c`:

- Reversi bitboard oracle versus production row/column rays: 17,496 exhaustive
  single-ray states, 400 arbitrary full boards, and 86,192 placement checks.
- Independent complete endgame minimax: 96 reachable positions with at most
  five empty cells; every MASTER selection attains the exact best disc margin.
  Reference playouts include 143 passes, two early non-full endings and a draw.
- 128 level/mode/seed search replays reproduce choices and RNG. Sixteen full
  engine games produce 953 placements and 34 automatic passes; all intermediate
  states round-trip through the production codec. Budgets are checked per level.
- Net independent DSU oracle checks all 50,625 nonempty-mask 2×2 boards (four
  valid trees), plus 480 replayed/generated/completed seeded puzzles across
  four levels. Connected-cell scores are checked independently; rotations,
  locks, reveals, alternate solutions and codec round-trips are exercised.
- Both validators reject malformed dimensions, modes, phases, reserved fields,
  scores and turns; Net also rejects modified seed, RNG and immutable shapes.
- Twelve real-app configurations cover NEW, CPU input barriers, full-round
  UNDO, assisted hint/reveal, explicit RESUME, RULES and INIT. Renderer bounds
  checks cover 51,132 rectangles at two cursor positions. Host captures were visually inspected:
  [Reversi](../assets/strategyquick/extra-37-master.png) and
  [Net](../assets/strategyquick/extra-38-master.png). These are host rasters,
  not photos or hardware LCD evidence.

Reproduction after all parallel modules are integrated:

```sh
cmake -S tests -B build-host -DNG_SANITIZE=ON -DNG_ASAN=OFF
cmake --build build-host --target test_strategyquick test_strategyquick_extra -j8
./build-host/test_strategyquick
./build-host/test_strategyquick_extra
```

Both new modules add no mutable static or heap storage and retain the existing
NgGame layout. Strict SH GCC `-Os -fstack-usage -fcallgraph-info=su` compilation
passes `-Wframe-larger-than=2048`; largest individual new-game frame is 280 B
(`sq_reversi_pick`), recursive `rv_search` is 244 B and `net_render` is 216 B.
Reversi search has at most five move plies and at most one forced pass before
each move, giving a conservative bound of 11 simultaneous search activations.
This is a recursion bound, not a measured stack peak. Root call paths, library
calls, interrupts and actual hardware peaks require separate accounting.
Native frame evidence is in
[extra-native-frames.json](../assets/strategyquick/extra-native-frames.json).
The [ASan retry](../assets/strategyquick/extra-asan-attempt.json) compiled but
produced no output and timed out after 20 seconds on 2026-09-26; it is
NOT VERIFIED. That retry covers the engine/codec/renderer test build; the
current full app tests are verified under UBSan.
CPU elapsed time, device stack/heap peak and physical controls remain
**HARDWARE TEST REQUIRED**. UBSan is verified; ASan is not claimed passed.


## Focused recent-six follow-up — 2026-09-26

This review covered Reversi and Net within the six-game audit, starting from
local source `2ecaf26`. Their state layouts, generation policies, CPU budgets
and rule verdicts were preserved. No new content bank or game was added.

The production rules and validators agree: Reversi flips every bracketed ray
without cascading, requires a flip for a move, passes only when no move exists,
and ends when neither side can move. Disc counts determine win/loss/draw;
unfilled cells are not awarded to either player. Net explicitly forbids loops
and wrapping. Reciprocal ports, boundary closure, connectivity and the edge
count together enforce a tree. Alternative valid rotations still win.

The changes address specific control feedback:

- Terminal Reversi and Net panels now describe F6 NEW and EXIT to the game
  entry, instead of advertising placement/rotation actions that are frozen.
- Reversi hides its human placement/F4 sidebar instruction while the CPU is
  pending. Its current-player label, scores, legal dots and selected cell
  remain separate. There is no LOCAL 2P mode or softkey.
- Rotating Net's four-way cross gives an explicit unchanged-orientation
  message. The board, RNG, assistance and move count are unchanged, so this
  does not create an undo record. Locked rotations retain their clear warning.
- Net rules now say exactly what a lock blocks: EXE/5/DEL. Assisted REVEAL can
  reset and relock a locked tile; that existing behavior is now explicit.

New independent named edge tests supplement the existing exhaustive oracles:
12 Reversi fixtures cover simultaneous eight-direction flips, no cascade,
occupied/own-disc/edge failures, human and CPU forced passes, full-board
win/loss/draw, and a non-full early ending. These synthetic fixtures isolate
rules and are not claimed as reachable complete game histories. Frozen keys,
terminal score/turn corruption and codec round-trips are checked as well.

Net tests isolate a connected tree with an open boundary port, a connected
cycle, a reciprocal mismatch, and a disconnected cycle-plus-tree with exactly
N−1 edges. All 15 nonzero port masks are exercised clockwise and anticlockwise
(30 cases), including the symmetric cross, every lock case, assisted override,
and frozen solved-state input. These tests agree with the independent DSU
oracle; the engine completion algorithm was not rewritten.

The actual application tests use the real version-5 A/B transaction hooks.
All 12 game/level/mode configurations pass START, deferred CPU input barriers,
whole-round UNDO, hint/reveal, same-game RESUME, Main F1 cold RESUME, RULES and
INIT. Eight Reversi configurations additionally checkpoint while the CPU is
pending; a cold resume produces the identical CPU reply and preserves the
human-round undo snapshot. Six further result paths cover Reversi win/loss/draw and Net solved:
completion modal → EXIT VIEW RESULT → frozen board → EXIT entry, direct
modal EXE NEW, and result-view F6 NEW. Held EXIT/EXE/F6 cannot cross the next
screen, frozen keys and clocks do not mutate the result, completion durably
clears the resume, and a newly started run cold-loads again.

The `tests/test_strategyquick_capture.c` host harness accepts `OUTPUT_DIR recent`
after compilation to render only these two games for focused inspection. It drives gameplay through the real application
and captures the initial board, CPU wait, completion modal, frozen result and
next run. These are actual host renderer outputs, not hardware photographs.
The common renderer now hides inactive F3/F4 keys during Reversi CPU work
and labels its result with the actual difficulty and UNASSISTED/ASSISTED,
avoiding confusion between NORMAL difficulty and unassisted play. The result
title identifies the human or CPU winner. Root applied these shared changes.
Reviewed host evidence: [CPU wait](../assets/strategyquick/extra-37-cpu.png),
[completion modal](../assets/strategyquick/extra-37-result-modal.png),
[Reversi result](../assets/strategyquick/extra-37-result.png), and
[Net result](../assets/strategyquick/extra-38-result.png).

Strict C11/UBSan `strategyquick` and `strategyquick_extra` targets pass against
the integrated core. The new game source also passes strict SH GCC with the
2048-byte individual-frame gate; its largest frame remains 280 B. ASan remains
unverified as described above. Native LCD clarity of the 8×8 legal dots and
6×6 lock marks, physical controls, storage latency and MENU behavior remain
**HARDWARE TEST REQUIRED**. This audit does not claim to resolve MENU flicker.

## beta.3 difficulty audit

The audit samples the actual native engines, rather than inferring difficulty
from the menu label. `tests/test_strategyquick_difficulty.c` links the production
core and emits deterministic JSONL. Runtime groups use seeds 1..128; each bank
is loaded in full using a fixed shuffle key and every ordinal. All initial
states pass their real validators and exact replay checks. The owned report
contains IDs 21–28, 37 and 38: 76 game/mode/level groups and 11,101 rows including
CPU decisions. Its 40-row CSV uses the common integration schema. The same
harness also loads all 240 Shikaku/Slitherlink clues; their detailed grading
review and final report belong to root.

| ID / game | EASY / NORMAL / HARD mechanism | MASTER mechanism | Finding |
|---|---|---|---|
|21 NIM|3 piles ≤7 / 3 piles ≤15 / 4 piles ≤31; CPU random / mixed / exact|Verified four-pile forced-win starts, same exact CPU|Distinct starts and E/N/H policies; MASTER is not stronger AI|
|22 WYTHOFF|Pile limits 10/24/40; CPU random / mixed / exact|30 YOU-first and 12 CPU-first tactical starts; same exact CPU|Distinct challenge, not a stronger-than-exact claim|
|23 EUCLID|Value limits 12/40/99; CPU random / mixed / exact|30 starts per mode, coprime multi-step positions; same exact CPU|Distinct challenge, not a stronger-than-exact claim|
|24 MAKE FIFTEEN|Same empty board; CPU random / mixed / exact|30 YOU-first and 5 CPU-first midgames|E/N/H opening equality is intentional; CPU behavior changes|
|25 RACE TO TARGET|Target/add limit 21/3, 31/3, 23/4; CPU random / mixed / exact|30 starts per mode with varied targets/add limits; same exact CPU|Rule parameters change; numeric target alone is not a rating|
|26 2048|TARGET goals 512/1024/2048|TARGET goal 8192|CLASSIC is a fixed-rule exception, not four meaningful levels|
|27 SLIDING|80/160/240 legal shuffle moves; Manhattan thresholds 6/10/14|3×3 shortest path 31; 4×4 lower bound ≥48|E/N/H calibration **REVIEW REQUIRED**; MASTER has certified separation|
|28 LIGHTS OUT|New 4×4 exact minima 2/4/5; 5×5 generates with 4/8/18 presses|Exact minima 6 on 4×4, 12..14 on 5×5|4×4 inverse grading fixed; 5×5 ranges still overlap|
|31 SHIKAKU|5×5/6×6/8×8, 30 banks per level|30 8×8 puzzles with interacting rectangle choices|Distinct structure, human grade uncalibrated; root review|
|32 SLITHERLINK|5×5/6×6/8×8, 30 banks per level|30 8×8 puzzles with unresolved edge interactions|Distinct structure, human grade uncalibrated; root review|
|37 REVERSI|Random / 1 ply and 256 nodes / up to 3 plies and 1,500 nodes|Up to 5 plies and 6,000 nodes|Search work and choices differ; no full-game strength guarantee|
|38 NET|3×3/4×4/5×5 seeded rotated trees|6×6 seeded rotated trees|Larger structural problem; no uniqueness or human-rating claim|

For IDs 21–25, NORMAL chooses the exact-policy branch with probability 75%;
the other branch may also happen to pick an optimal move. This is not a 75%
win rate. Counterfactual probes compare the same reachable position and RNG
at all levels. These are decision probes, not imported MASTER-bank save states.
HARD and MASTER produced identical boards and RNG in all 128 positions per
game. Existing independent exhaustive table/DAG tests remain the exact-policy
oracle validation; the new comparison does not claim a second independent AI.

| Game | Decisions with better/worse alternatives | Optimal EASY | Optimal NORMAL | Optimal HARD / MASTER |
|---|---|---|---|---|
|NIM|106|6|79|106 / 106|
|WYTHOFF|111|9|87|111 / 111|
|EUCLID|89|22|69|89 / 89|
|MAKE FIFTEEN|67|31|53|67 / 67|
|RACE|102|27|84|102 / 102|

The Reversi probes use 64 reachable positions after at least eight placements.
NORMAL used 1–16 nodes and completed depth 1; HARD used 5–1,500 and completed
depth 2–3; MASTER used 9–6,000 and completed depth 3–5. Adjacent E→N, N→H and
H→M choices differed in 54, 29 and 22 positions. Equal decisions on some boards
are not a no-op setting. Bigger budgets alone do not prove stronger play on
every position or establish a human skill rating.

Sliding's independent reverse BFS covers all 181,440 reachable 3×3 positions.
E/N/H shortest-path means were 21.9219 / 22.1875 / 23.25, with ranges
10–28 / 12–28 / 14–28. Those distributions overlap substantially, particularly
EASY and NORMAL. On 4×4, mean Manhattan lower bounds were 28.3125 / 34.0938 /
36.25, versus 49.0667 in MASTER. A lower bound is not the actual shortest path.
No Sliding generation change was made from this sample alone; its E/N/H
human difficulty calibration remains **REVIEW REQUIRED**.

### LIGHTS OUT correction and compatibility

The independent first-row/row-chasing solver found a real 4×4 grading failure
in revision 2. E/N/H generated with 4/8/12 presses, but minimum-solution means
were 4.0000 / 4.5547 / 4.4375. NORMAL included a one-press puzzle; HARD could
need only two. Increasing generating presses had not produced an ordered
solution-length scale. The [baseline metrics](../assets/strategyquick/difficulty-beta3-baseline.json)
preserve the 128-seed evidence.

Revision 3 now gives 4×4 E/N/H exact minima 2/4/5, with the existing MASTER
minimum 6. The complete four-dimensional GF(2) nullspace contains 16 masks;
every solution of a selected press pattern is one of those 16 XOR variants.
The generator checks their weights, tries at most 16 candidate patterns, and
has a deterministic fallback whose exact minimum is five. The independent
row-chasing audit reconstructs the nullspace from the rules and checks every
native constant, its minimum nonzero weight eight, and the fallback.

The C regression tests independently solve 4,096 seeds for each new E/N/H
level (12,288 starts), reject changed minimum metadata, and check six preserved
revision-2 board/RNG/count digests covering 768 old starts. Sixteen version-2
and version-3 mode/level combinations pass compact codec round-trip and actual
application INIT with the original board, RNG, metadata and revision restored.
Old saves retain their puzzle; the shared core selects revision 3 for new
Lights runs. The state layout, bank sizes, MASTER identities, gameplay rules,
hint algorithm and 5×5 generator are unchanged.

The 5×5 E/N/H/M exact minimum means were 4 / 7.9844 / 10.0781 / 12.2667, with
ranges 4 / 6–8 / 6–14 / 12–14. These are increasing sample means with overlap,
not a guarantee that every higher-level instance is harder. Net's 128 samples
per level all had distinct initial boards; average degree-three-or-more
junction counts grew 1.9688 / 3.8281 / 6.1406 / 9.0391 as area grew
9 / 16 / 25 / 36 cells. This measures structural size, not human difficulty.

2048 TARGET keeps identical movement/spawn rules and changes the terminal goal.
CLASSIC has identical boards and RNG across direct-API level probes. The app
hides its level control and starts it at NORMAL while retaining the separately
selected TARGET level. This intentional fixed exception is reported explicitly.
The stale in-game claim about separate persistent records was removed to agree
with v5, which does not persist cumulative records.

Evidence: [structured report](../assets/strategyquick/difficulty-beta3.json),
[integration CSV](../assets/strategyquick/difficulty-beta3.csv), and
[strict SH object/frame comparison](../assets/strategyquick/difficulty-beta3-native.json).
The Lights change adds 252 B of `.text`, 44 B of `.rodata` and 36 B of strings
to its object (+332 B total); `.data` and `.bss` remain zero. The `sq_quick_init`
frame remains 148 B and `sq_lights_solution` remains 236 B. These are object and
individual-frame measurements, not a linked-package delta or runtime RAM peak.

```sh
cmake -S tests -B build-host -DNG_SANITIZE=ON -DNG_ASAN=OFF
cmake --build build-host --target test_strategyquick_difficulty test_strategyquick test_strategyquick_extra -j8
ctest --test-dir build-host -R '^strategyquick(_extra|_difficulty)?$' --output-on-failure
python3 tools/generate/strategyquick_difficulty.py --sample-exe build-host/test_strategyquick_difficulty --output assets/strategyquick/difficulty-beta3.json
```

The report embeds relative-source SHA256 values. Its metrics use fixed seeds
and no timestamps, private local paths or timing-based pass criteria. Hardware
latency, display usability and MENU behavior remain **HARDWARE TEST REQUIRED**;
the three owned C11/UBSan targets passed. The new ASan target compiled, but its
20-second execution attempt timed out with zero stdout/stderr bytes; ASan is
still unverified. No stronger claim follows from the host audit.
