# LOGIC and PUZZLE audit — game IDs 11–20, 35–36

The IDs 11–20 compact bank contains **1,750 records: 1,630 uniquely solved puzzles
and 120 Magic PARTIAL layouts**. All original 900 records and the first 80 MASTER
records retain their exact public clues, witnesses and stable IDs. The historical
80-record extension is documented separately in [GRID_MASTER_AUDIT.md](GRID_MASTER_AUDIT.md).

## Content and supply

| ID | Game | EASY / NORMAL / HARD / MASTER / HELL | MASTER / HELL board size |
|---|---|---|---|
| 11 | Sudoku | 50 / 50 / 50 / 50 / 30 | 9×9 / 9×9 |
| 12 | Calcudoku | 50 / 50 / 50 / 50 / 30 | 6×6 / 6×6 |
| 13 | Kakuro | 50 / 50 / 50 / 50 / 30 | 7×7 / 8×8 overall |
| 14 | Futoshiki | 50 / 50 / 50 / 50 / 30 | 6×6 / 6×6 |
| 15 | Skyscrapers | 50 / 50 / 50 / 50 / 30 | 6×6 / 6×6–7×7 |
| 16 | Hitori | 30 / 30 / 30 / 30 / — | 6×6 |
| 17 | Binary Puzzle | 30 / 30 / 30 / 30 / — | 8×8 |
| 18 | Numbrix | 30 / 30 / 30 / 30 / — | 6×6 |
| 19 | Magic Square PARTIAL | 30 / 30 / 30 / 30 / — layouts | 4×4 PAN |
| 20 | Sum Grid | 30 / 30 / 30 / 30 / — | 7×7 |

The nine unique families use original host-generated public-clue banks. There
is no downloaded puzzle corpus, runtime solver, whole-bank decompression or
runtime puzzle transformation. Runtime supply is `ng_new → module.init →
ng_bank_pick → grids_bank_id → grids_record`. The common application owns the
persistent shuffle/recent-history policy. `grids_bank_count(id,difficulty,mode)`
returns the actual ordinal count; the module maps each ordinal to a stable ID.

The original three levels remain structural difficulty labels. Sudoku keeps
43/34/27 givens; Calcudoku keeps 4/5/6 boards and at most 4/7/10 fixed cages;
Futoshiki and Skyscrapers keep 4/5/6 boards with their original constraints.
Kakuro keeps 5/6/7 overall boards. New ordinary content is independently counted,
and new HARD records also remain below the measured MASTER search boundary.
No new time limit is used to manufacture difficulty. Final structure checks
caught excessive fixed cages in an initial, unshipped Calcudoku batch; all 60
new ordinary Calcudoku records were regenerated under the original limits.

### Magic counts and rules

Magic is not counted as a unique-solution family. Its original 90 PARTIAL
layouts have 85 distinct D4-normalized clue layouts, derived from two underlying
3×3/4×4 constructions with 16 oriented witnesses. MASTER adds the explicit PAN
rule: **every wrapped diagonal also sums to 34** in PARTIAL and in retained
revision-2 FREE saves. New revision-3 FREE games use normal magic rules.
The rule is displayed on the board and explained in RULES. Every rule-valid
completion is accepted; the stored witness is not the only accepted answer.

The new 30 PARTIAL layouts have 30 different D4-normalized witness arrays.
Including value complement reduces those witnesses to 22 classes; including
torus row/column translations reduces them to **three construction classes**.
They are therefore described as 30 layouts, not 30 independent mathematical
bases. The original balanced-bit construction enumerates 384 arrays (48 D4,
24 D4+complement, three translated construction classes); this is a construction
count, not a claim that the tool classifies all conceivable square generators.

FREE removes all givens. It is one open rule challenge for each size/rule set,
not 30 different puzzles. Legacy stable bank IDs remain for save compatibility. New revision-3 FREE
uses four rules-only IDs outside the bank and renders `FREE 3x3/4x4/5x5/6x6`,
without a puzzle-count label. Older saves keep their original FREE/PAN label.

## Difficulty evidence

MASTER and HELL ratings are **provisional instrument bands**, not a calibrated
human rating. Every accepted unique advanced board stalls under basic inference,
then finishes under bounded, public-clue contradiction deductions. A trial
candidate is removed only when propagation proves a contradiction. The complete
trace records its location, excluded value, rule counts and propagation passes.
The independent verifier replays the entire trace without a witness.

For LOGIC, basic techniques are singles and peers, exact cage/run combination
support, inequality arcs, and visibility-line support using both directions.
Advanced techniques add Hall pairs/triples and Sudoku locked candidates. MASTER
requires at least one proved elimination and search above all retained HARD
boards. HELL additionally requires 5/5/4/5/8 proved eliminations respectively,
search above the family MASTER band, and a contradiction crossing at least four
propagation passes with at least two actual deduction techniques. HELL chains
must finish within 512 candidate probes and rating search within 1,001 nodes.
These conditions, not clue count or board size alone, determine acceptance.

| Game | Retained HARD search max | All MASTER search min / median / max | HELL search min / median / max | HELL proved removals |
|---|---:|---|---|---|
| Sudoku | 7 | 9 / 11 / 17 | 19 / 23 / 69 | 5–11 |
| Calcudoku | 7 | 8 / 13 / 33 | 34 / 43.5 / 200 | 5–19 |
| Kakuro | 5 | 7 / 7 / 15 | 18 / 19 / 33 | 4–10 |
| Futoshiki | 7 | 9 / 11 / 35 | 36 / 41 / 109 | 5–18 |
| Skyscrapers | 15 | 17 / 23 / 39 | 41 / 57 / 228 | 8–22 |

Kakuro reserves a conservative MASTER search ceiling of 17; its accepted maximum
is 15. Original records are never reassigned to a different difficulty.
PUZZLE MASTER requires a public-clue proof using the listed structural technique,
full deduction completion, and a separate measured gap from retained HARD:

| Game | Required proof feature | HARD search max | MASTER search range | Proved removals |
|---|---|---:|---|---|
| Hitori | white connectivity / articulation | 13 | 15–37 | 4–12 |
| Binary | distinct whole-row/column support | 9 | 11–39 | 3–12 |
| Numbrix | path distance and parity | 20 | 21–119 | 6–20 |
| Sum Grid | crossing row/column subset sums | 1 | 3–19 | 1–8 |

Magic MASTER uses the extra PAN rule instead of a false uniqueness/difficulty
score. Numbrix inference variables are numbered values: its trace `cell` means
value−1 and `excluded` means a zero-based position. The verifier converts that
representation back to the ordinary board before comparison.

Generation constructs a fresh board from each saved seed, then refines actual
clues or run sums when necessary. It does not start from a shipped puzzle or
count rotations/relabelings as new unique bases. The canonical keys reject D4
variants and the implemented family-specific label/complement/path-reversal
symmetries. This is the documented equivalence scope, not an exhaustive
classification under every possible Sudoku or mathematical-square symmetry.

## Compatibility, bounded state and compact representation

Old IDs 0–899 retain the original game/level/ordinal mapping; IDs 900–979 retain
the first four MASTER groups. New append ranges are 980–1099 (11), 1100–1219
(12), 1220–1359 (13), 1360–1479 (14), 1480–1599 (15), and 1600–1749 (16–20,
30 each). Ordinary groups combine old and appended IDs, so callers must use the
ordinal mapper rather than assume a single contiguous range.

Revision-1 states may name only IDs below 980. Revision-2 states can name the
expanded bank. Existing JSON hashes and all 980 decoded records are checked;
only native encoding changes. Because bank counts grew, recreating solely from
an old seed is insufficient: common INIT restores the stored stable puzzle ID.
That migration and the archive format are owned by the integration layer.

The native pack is **79,658 payload + 10,704 index = 90,362 bytes**, compared with
480,200 bytes before compression. This saves 389,838 bytes while adding 770
records. The index comprises 1,751 uint32 offsets, 1,750 uint16 IDs and 50 pairs
of uint16 bank offsets/counts. One record is decoded on demand into a shared
490-byte BSS cache plus a four-byte cached ID; these are counted once, not once
per game. No engine heap allocation is added.

`grids_record`/`grids_puzzle` pointers expire when another record is requested.
Engine call paths retain them only while accessing that same record; caller-owned
`grids_decode(id,out)` is available when data must survive another validation.
C tests exercise that lifetime boundary. Decoding checks record bounds, bit
lengths, dimensions, values and cage indices. There is no whole-bank RAM copy.

SH `-Os` measurements: decoder text/read-only 1,282 B, data 4 B, BSS 490 B;
engine/renderer text/read-only 14,314 B and zero data/BSS. Individual compiler
frames are init 48 B, decode 76 B, rules 512 B, render 224 B and valid 60 B.
These are not simultaneous whole-call-chain or measured device peaks. Exact
per-family payload/minimum/maximum record sizes are in
`assets/grids/expanded/memory.json`. **Hardware stack peak and latency: NOT MEASURED.**

## Rules, controls and assistance

Completion checks the actual board against public clues; it never compares the
board with `solution[]`. Tests poison every witness while preserving a legal
board and completion still succeeds. The only gameplay witness read is the
explicit, one-cell REVEAL action, which marks ASSISTED. Magic has no reveal.
Fixed clues, dimensions, cursor, unused fields, notes, drafts and terminal states
are validated independently before persisted states are accepted.

| Games | Digits | EXE / F6 | DEL | F4 |
|---|---|---|---|---|
| Sudoku | 1–9; note toggles in NOTES mode | CHECK | clear editable note/value | NOTES toggle |
| Calcudoku / Futoshiki / Skyscrapers | 1–N | CHECK | clear editable value | — |
| Kakuro | 1–9; arrows skip black cells | CHECK | clear white cell | — |
| Hitori | no digit action | SHADE/unshade | unshade | CHECK |
| Binary | 0/1; blank is distinct | CHECK | restore blank | — |
| Numbrix / Magic | bounded 1–2 digit draft | ENTER; CHECK with empty draft | edit draft, otherwise clear | CHECK |
| Sum Grid | no digit action | KEEP/REMOVE | KEEP | CHECK |

Arrows select and cancel a pending Numbrix/Magic draft. Fixed clues cannot change.
Sudoku notes use nine real bits and cannot coexist with a value. Operators are
clue text, not editable input. Every board/note edit increments `moves` for common
undo snapshots. Wrong entries are not automatically rejected by comparing them
with the witness. Common INIT, UNDO, RULES, records and MENU/power belong to the
application layer.

## Reproducible validation and renderer review

Verification is read-only by default. Setting `NUMGAME_VERIFY_OUTPUT` writes
reports only below that separate build directory, under `grids/`. Saved evidence
in `assets/grids/expanded` is an explicitly captured audit artifact, not rewritten
by routine validation.

```sh
python3 -B tools/generate/grids_verify.py
python3 -B tests/test_grids_master.py
python3 -B tests/test_grids_expanded.py
NUMGAME_VERIFY_OUTPUT=build-grid-audit python3 -B tools/generate/grids_expanded_verify.py --replay-seeds
```

`grids_verify.py` runs all original 900 counters, exhaustive easy-Hitori masks,
the retained MASTER80 audit, then all 770 expanded records. Expanded checks
include counts, stable IDs, canonical deduplication against old and new records,
public-only independent solution counters, exact logical trace replay, schema,
fixed-cage limits, witnesses and native compact equality. The separate reference
uses direct Sudoku MRV, whole-row Latin/visibility search, Hitori mask search,
Binary row enumeration, Numbrix path enumeration and Sum Grid row subsets. The
visibility reference preindexes legal column-prefix continuations while
preserving its original complete traversal order and node counts.

The compiled C decoder can be checked independently of the Python codec:

```sh
cc -std=c11 -Wall -Wextra -Werror -Iinclude -Isrc/games \
 tools/generate/grids_dump.c src/games/grids_decode.c src/games/grids_pack.c -o build-grid-dump
./build-grid-dump > build-grid-dump.jsonl
python3 -B tools/generate/grids_expanded_verify.py --native-dump build-grid-dump.jsonl
```

The existing `grids_generate.emit()` API emits both `grids_pack.c` and
`grids_pack_data.h`; redirecting `grids_generate.ROOT` to a temporary directory
leaves its input `OUT` unchanged. Both generated files must compare byte-for-byte.
The normal emitter refuses incomplete/noncontiguous IDs instead of publishing
an empty high-difficulty bank.

Final validation results are recorded in the expanded audit and test artifacts.
The retained 900 and first 80 counters passed in 59.132 and 31.377 host seconds;
all 770 saved generation seeds reproduced their clues and witnesses in 482.553
seconds. The current expanded-audit time is recorded in `audit.json`; timings
are observations of these host runs, not performance promises.
The C11/Wall/Wextra/Werror/UBSan suite covers all 1,750 boards, every supported
level/mode, stable-ID selection, fixed clues, notes, two-digit input, malformed
state, Magic PAN alternatives, revision-1 limits and cache lifetime. The seven
new small-space tests and five retained-MASTER tests include witness-poisoned
public inference, complete tiny-board truth spaces, and generation/proof replays.
An independent worker additionally checked 1,119 public boards, 72,890
propagation cases and 20,790 surviving-solution comparisons without a soundness
counterexample. A duplicate-given Numbrix instrument defect found during that
review now has a zero-solution negative regression; shipped valid puzzles were
unaffected.

Eleven actual renderer captures cover each maximum advanced board, nine Sudoku
notes and both Magic PAN modes. `assets/grids/expanded/contact.png` was inspected:
clues, inequalities, outside sums and note marks fit; no witness is exposed.
They are actual native-adapter pixels, not mockups. Common full-app rendering
and workflow capture remain integration checks.

The earlier macOS ASan attempt timed out before `main()` during runtime shadow
memory initialization. **ASan was NOT VERIFIED, not PASS.** Raw process dumps
and private environment logs are not needed for the public reproduction path.
UBSan and strict SH checks are recorded separately. Host captures do not prove
physical LCD readability, key behavior, MENU/OFF, storage recovery or device
memory/latency. **HARDWARE TEST REQUIRED** for those observations.

## Provenance

All puzzle instances and generators are original project work under its MIT
license. External puzzle files fetched/parsed/bundled: **0 / 0 / 0**. Rules-only
references are Nikoli Sudoku/Kakuro/Hitori, Simon Tatham Towers/Unruly and James
Harvey Unequal; no corpus, implementation or visual asset was copied from them.
All app-specific rules, including Magic PAN, are stated in the in-app RULES and
this audit. Human difficulty remains provisional until calibrated with players.

## Additional games: HASHI 35 and NONOGRAM 36

`ng_grids_extra[2]` adds two complete engines with separate original banks:
**HASHI 150 unique puzzles (30 in each of EASY/NORMAL/HARD/MASTER/HELL)** and
**NONOGRAM 120 unique puzzles (30 in each of EASY/NORMAL/HARD/MASTER)**.
Together with IDs 11–20 this is 2,020 grid-family records: 1,900 uniquely solved
puzzles and the 120 separately described Magic PARTIAL layouts. There are no
downloaded puzzle records or copied third-party implementations in these banks.

The runtime path is `init → ng_bank_pick → grids_extra_bank_id → const record`.
Both games have one CLASSIC mode. Stable `puzzle_id` is `difficulty*30+ordinal`
within each game; IDs are not shared across the two games. Existing IDs 11–20
and their bank files are untouched. These new games reject revision-1 states.
No runtime generation, solver, heap allocation, cache or whole-bank RAM copy is
introduced. The 270 records occupy **23,070 read-only bytes**: 150×77-byte sparse
HASHI records and 120×96-byte NONOGRAM records including alignment.

HASHI stores at most 25 islands on a 9×9 board. Each island's `board` entry
encodes right/down multiplicities as `right + 3*down`; non-island entries are
zero. The immutable numbers remain in the pack. Completion derives the nearest
visible neighbor in each direction, counts every incident bridge, rejects
crossings, and runs a bounded graph traversal requiring all islands to connect.
It cannot represent a diagonal bridge or a bridge passing through an island.
Incorrect totals/crossings may remain in an unfinished player board; CHECK
rejects them. A won persisted state must satisfy the full rules again.

NONOGRAM stores ordered row/column runs as nibble sequences. Its board values
are −1 unknown, 0 marked empty, and 1 filled. Completion independently extracts
runs of filled cells from the player's rows/columns; unknown and X both count as
unfilled. The annotations stay distinct for editing, REVEAL and persistence.
It checks run order and gaps against the public clues, including zero clues.
Neither game's completion routine reads the stored witness. C tests poison
every witness in a copied record and still accept its rule-valid player board.

| Game | Arrows | Digits / EXE | DEL | F4 / F3 |
|---|---|---|---|---|
| HASHI | Select a visible neighboring island | 2/4/6/8 cycle down/left/right/up bridges through 0,1,2; EXE cycles the last direction | Clear that bridge | CHECK / assisted one-bridge REVEAL |
| NONOGRAM | Wrap among cells | 1 fills, 0 marks empty; EXE cycles unknown→filled→empty | Return to unknown | CHECK / assisted one-cell REVEAL |

Unsupported digits/operators do not alter the board. Every bridge/cell edit
increments `moves` for common undo. All unused board, fixed, notes, data, input
and history state is validated. Both engines reject impossible terminal states,
invalid dimensions, bad bank IDs and cursor positions. HASHI requires its cursor
to select a real island. Common menu layout and app routing are integration
responsibilities; these module tests make no category-slot assumptions.

### New-game difficulty and independent checks

The new ratings are provisional structural/inference bands. HASHI generation
constructs a fresh connected noncrossing bridge graph and derives island clues.
Public-clue propagation handles degree bounds, crossing exclusions and
connectivity-required edges; bounded branching counts up to two solutions.
The separate reference enumerates whole incident-edge tuples, checks remaining
degree capacity and scans occupied segment interiors and graph connectivity.
It imports none of the generator's propagation logic.

| HASHI level | Size / islands | Required inference evidence | Observed rating search nodes |
|---|---|---|---|
| EASY | 5×5 / 6 | Basic propagation completes | 1 |
| NORMAL | 7×7 / 9 | Larger connected network, independently unique | 1–3 |
| HARD | 7×7 / 12 | Basic propagation leaves at least 2 edges unresolved | 3–12 |
| MASTER | 9×9 / 16 | At least 4 unresolved edges and connectivity deductions in the replay | 3–23 |
| HELL | 9×9 / 21 | Advanced propagation leaves at least 4 edges unresolved; search ≥7 nodes | 7–31 |

These bands are not a claim that every HELL puzzle exceeds every MASTER puzzle
on one metric. Connectivity-deduction counts include deductions made during
search branches; the app does not claim to present a human tutorial for them.

NONOGRAM uses fresh seeded binary patterns, keeps only unique public-run boards,
and rates alternating row/column pattern support. Sizes are 5×5, 6×6, 7×7 and
9×9. HARD requires at least four basic propagation rounds; MASTER requires at
least four cells still unresolved by basic propagation and at least three search
nodes. Observed search ranges are 1 / 1 / 1–3 / 3–25. Its independent reference
enumerates whole rows against independently built column-prefix sets, rather
than reusing the generation propagator. Uniqueness and technique evidence,
not only clue count or board size, gate the advanced records.

All 270 accepted seeds reproduce their exact instances, and all ratings replay.
Canonical checks reject D4-equivalent HASHI island-clue grids, and D4/complement
equivalents for NONOGRAM. This is the documented equivalence scope, not an
exhaustive classification of every abstract graph isomorphism. The independent
counters operate on records with witnesses removed. Small-space tests also
enumerate all 3⁴ four-island bridge assignments and all 2⁹ binary 3×3 boards,
comparing solution counts and checking that the inference searches return only
actual solutions. Native C pack source reproduces byte-for-byte in a temporary
directory; routine validation leaves source/assets unchanged.

```sh
python3 -B tests/test_grids_extra.py
cc -std=c11 -Wall -Wextra -Werror -fsanitize=undefined \
 -fno-sanitize-recover=all -DNG_GRIDS_EXTRA_TEST_MAIN -Iinclude -Isrc/games \
 tests/test_grids_extra.c src/games/grids_extra.c src/games/grids_extra_pack.c \
 src/core/common.c src/ui/draw.c -o build-grid-extra-test
./build-grid-extra-test
```

The five Python tests pass for every bank, including independent uniqueness,
canonical checks, seed/rating replay, exhaustive small spaces and native pack
reproduction. The strict C11/UBSan suite completes all 270 puzzles using real
key actions and checks malformed states, disconnected/crossing HASHI boards,
NONOGRAM run order/unknown cells, and pixel bounds for every initial/won board.
The engine and pack pass strict SH compilation. Engine text/read-only size is
8,072 bytes, with zero data/BSS; largest individual frame is render 200 bytes,
HASHI rules 96 bytes, valid 48 bytes and init 16 bytes. These are compiler
estimates, not a measured hardware call-chain peak.

Nine initial-board captures select the largest bridge-degree totals and longest
NONOGRAM clue lists at each level. The 9×9 NONOGRAM uses 13-pixel cells with
5×7 clue digits; even five-number clues fit within y=27..184. All initial and
completed renders satisfy the content bounds. The contact sheet was visually
inspected, and renderers never read a witness. `assets/grids/extra/audit.json`,
`memory.json`, `capture-selection.json` and concise validation logs record the
results. The extra-game ASan binary built but timed out without test output
after 15 seconds: **ASan NOT VERIFIED**. Hardware readability, controls, stack
peak and latency remain **HARDWARE TEST REQUIRED**.

### Focused HASHI/NONOGRAM review, 2026-09-26

The retained completion validators agree with the new independent edge cases;
no puzzle rules or bank content changed. HASHI's interaction feedback needed a
small correction: the selected edge previously appeared only as a direction in
the sidebar, and a crossing received only the generic CHECK message. The selected
island and target now have blue outlines. An empty candidate has a dashed blue
line explicitly labelled EMPTY; actual single/double bridges have one/two solid
strokes. The first real crossing has a red X, and both bridge edits and CHECK
give a specific crossing message. Exact island totals with separated components
receive a disconnected-network message. Incorrect unfinished boards remain
editable and saveable, preserving existing undo and correction behavior.

An independent native truth model rasterizes bridge interiors and uses
union/find to test connectivity. All 59,049 assignments on a ten-edge geometry
agree with completion: 10,496 connected noncrossing boards; 26,244 crossing and
33,193 disconnected assignments (the latter two categories overlap). This adds
coverage for single/double bridges, crossings, isolated components and connected
cycles without importing the engine's nearest-neighbor or traversal helpers.
Pixel probes check dashed, single, double and crossing marks. The existing
public-clue counters still verify all 270 unique banks and exact native pack
reproduction.

NONOGRAM's renderer and input semantics required no change. Native and independent
Python fixtures now explicitly cover zero/empty lines, run ordering, mandatory
gaps, merged adjacent runs, impossible lengths, unknown cells and five-run 9×9
clues. A zero is drawn for an empty line; it is never an absent constraint.
Filled black, empty X and unknown white cells remain distinct. Small clue digits
fit in the host captures; actual calculator LCD readability remains a hardware
check.

All 270 completed module states reject arrows, digits, edits, CHECK and REVEAL
without changing any state byte. This supports the common frozen result view;
modal dismissal, F6 NEW, entry/resume placement, storage and physical MENU are
owned and tested by the integration layer. The tests select bank ordinals by
visiting a complete cycle rather than assuming a particular shuffle formula.
No shared API or save field was added.

Current evidence is in `assets/grids/extra/review-20260926/`: five focused native
renderer captures, their contact sheet, C11/UBSan output, five Python tests,
and the concise audit JSON. The crossing and NONOGRAM mark captures deliberately
show incorrect/partial player states. Reproduce those content-only captures with
the above C test binary's `--audit-capture OUTPUT_DIRECTORY` option. The current
SH object measurements are in `assets/grids/extra/memory.json`; individual frames
are not a whole-call-chain or physical-device peak.


## Beta.3: optional NONOGRAM marks and FREE Magic orders

NONOGRAM now completes immediately after a cell edit makes its filled set exact.
All public row/column runs must match; missing black cells and extra black cells
both fail. Since every shipped NONOGRAM is independently unique, checking the
public runs is equivalent to exact filled-set equality without comparing the
answer witness. Empty cells may remain blank or carry X. EXE still cycles
blank → filled → X → blank; DEL restores blank. REVEAL of an empty cell still
writes X and marks assistance. RULES and failed CHECK no longer demand that every
cell be annotated.

`tests/test_grids_extra.c` covers requested A–F on all 120 NONOGRAM banks: all-X,
all-blank and mixed empties; missing black; extra black with a missing required
cell; and extra black with every required cell present. All three successful
styles complete on the last required edit. The independent public-clue and
native-pack Python tests still pass. `tests/test_grids_beta3.c` covers G with an
actual A/B save and cold load preserving X versus unknown, and H by holding the
completing EXE 50 times: the result barrier prevents dismissal or accidental NEW.
EXIT exposes the frozen result board; F6 explicitly starts a new run.

New MAGIC SQUARE FREE games use revision 3 and blank normal magic squares:

| Level | Order | Values | Common sum | Bank records |
|---|---|---|---|---|
| EASY | 3×3 | 1–9 | 15 | 0 |
| NORMAL | 4×4 | 1–16 | 34 | 0 |
| HARD | 5×5 | 1–25 | 65 | 0 |
| MASTER | 6×6 | 1–36 | 111 | 0 |

All values must occur exactly once; every row, column and both main diagonals
must have the common sum. Wrapped diagonals are not required in new FREE.
Alternative valid arrays, including rotated known squares, are accepted.
PARTIAL retains its existing clues, stable bank IDs and MASTER PAN rule.
Revision-1/2 FREE saves retain original orders 3/3/4/4 and the original MASTER
PAN rule, including during INIT. A later NEW uses the selected level's new order.
The four new rules-only IDs are `0xfffffff0 + difficulty`; they are not extra
bank puzzles. They use the existing one-record decoded cache, with no new save
fields, heap allocations or additional cache. Construction witnesses exist only
for diagnostics; completion and rendering do not read them.

Known 3×3, 4×4, 5×5 and 6×6 solutions and rotations pass. Duplicate, missing,
out-of-range, wrong row, wrong column and wrong diagonal fixtures fail. Real
key entry supports 36, and codec/cold resume/INIT preserves order, values and
pending two-digit drafts. The full 396×224 application renderer captures are in
`assets/grids/extra/beta3/`; this is the actual fx-CG50 renderer resolution. The
6×6 board uses square 26-pixel cells, readable centered two-digit text, a blue
cursor and `SUM = 111`, with no footer overlap. Initial and filled boards for
all four orders and the NONOGRAM result with every empty cell blank were inspected.
A separate mixed-mark pre-completion capture records the preserved X annotation.
Physical LCD legibility and hold timing remain **HARDWARE TEST REQUIRED**.

The owned strict C11/UBSan suites and strict SH compilation pass. Current native
object sizes and frame estimates are in the existing expanded/extra memory
JSON files. The beta.3 ASan binary again timed out after 15 seconds with no test
output: **ASan NOT VERIFIED**, not PASS. The concise `beta3/asan.json` contains
no raw process dump or private paths.

### All twelve games: measured difficulty differences

`assets/grids/extra/difficulty-beta3.json` covers 54 game/level rows and all
2,020 supplied bank records, plus the four FREE rule configurations. Its exact
14-column `csv_rows` supports the integrated difficulty table. Measurements
freshly replay public-clue inference and bounded search after removing witnesses.
It includes board/clue distributions, exact and documented-canonical counts,
cross-level intersections, source hashes and explicit interpretation caveats.
The 1,900 unique-solution records remain unique; the other 120 records are Magic
PARTIAL layouts. Cross-level exact and canonical intersections are zero under
the documented equivalence scope. Magic's within-level D4 clue-layout counts
are 29/26/30/30, so those 120 records must not be claimed as 120 independent bases.

For IDs 11–15, measured MASTER maximum search nodes are 17/33/15/35/39 and HELL
minimum nodes are 19/34/18/36/41, respectively. HASHI has increasing geometry
(6/9/12/16/21 islands), but MASTER 3–23 and HELL 7–31 node ranges overlap.
Lower bands also overlap in several games. SUM GRID E/N/H all finish by basic
propagation with one search node; their distinction is arithmetic range
(maximum values 5/9/11–12) and size (5/5/6), not a demonstrated need for harder
inference. MASTER uses 7×7 interacting subset sums and 3–19 nodes. No measured
algorithmic band is presented as a calibrated human solving-time ranking.
The identified FREE E/N no-op is fixed by the distinct board orders above.

Reproduce the full deterministic difficulty report without mutating source:

```sh
python3 -B tests/test_grids_difficulty.py --output build/difficulty-beta3.json
```

Build/link `tests/test_grids_beta3.c` with `numgame_core`; its `--capture DIR`
option reproduces the full application PPM captures. The normal unit binaries
remain `test_grids` and `test_grids_extra`; no external puzzle corpus was added.
