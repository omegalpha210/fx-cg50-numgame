# LOGIC and PUZZLE audit — game IDs 11–20

The current compact bank contains **1,750 records: 1,630 uniquely solved puzzles
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
rule: **every wrapped diagonal also sums to 34**, in both PARTIAL and FREE.
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
not 30 different puzzles. Stable bank IDs remain for save compatibility, but
FREE renders `FREE PAN 4x4` or `FREE 3x3/4x4`, without a puzzle-count label.

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

SH `-Os` measurements: decoder text/read-only 1,108 B, data 4 B, BSS 490 B;
engine/renderer text/read-only 14,086 B and zero data/BSS. Individual compiler
frames are init 48 B, decode 76 B, rules 508 B, render 224 B and valid 60 B.
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
