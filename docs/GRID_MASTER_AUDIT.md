# Retained first MASTER extension — historical audit, 2026-09-22

This document records the **earlier 80-record extension**, preserved unchanged
at stable IDs 900–979. Its counts, native representation and timing describe that
baseline, not the expanded current release. See [GRID_AUDIT.md](GRID_AUDIT.md)
for current content, compact decoding, all ten MASTER levels and LOGIC HELL.

**Historical difficulty-3 support: IDs 11, 12, 14, 15, with 20 bases each.**
The rating is **MASTER provisional**: it is based on recorded logical deductions,
contradiction chains and measured search, not a calibrated human difficulty scale.
No runtime generation or solving search was added. MENU/power/storage migration
and the common difficulty selector belong to the integration layer; these puzzle
results do not establish that the physical MENU problem is fixed.

## Content, identity and compatibility

| ID | Game | MASTER size | Stable appended puzzle IDs | Bases |
|---|---|---|---|---|
| 11 | Sudoku | 9×9 | 900–919 | 20 |
| 12 | Calcudoku | 6×6 | 920–939 | 20 |
| 14 | Futoshiki | 6×6 | 940–959 | 20 |
| 15 | Skyscrapers | 6×6 | 960–979 | 20 |

The original IDs 0–899 and all ten original JSON files remain byte-identical to
the retained baseline. `legacy-sha256.json` records their
hashes and the first 900 native initializer lines; verification checks both.
At that baseline, initialization made the same single bounded RNG call over the
same 30-record ranges. The `GridsPuzzle` representation and rules version stayed
unchanged. The native pack had **980 records: 890 unique puzzles and 90 Magic
partial layouts**. Magic layouts remain derived mathematical constructions with
transforms, permit multiple solutions and are not counted as independent unique
bases. `assets/grids/audit.json` deliberately covers the original 900;
`assets/grids/master/audit.json` records the extension and aggregate totals.

MASTER records are independently generated from saved seeds, not obtained by
rotating or relabeling a smaller pack. Canonical comparisons reject dihedral
variants against every original same-family record and prior accepted MASTER
record, plus the applicable value complement/relabel checks. Accepted seeds
reproduce the exact public clues and witnesses. No puzzle corpus, third-party
implementation or new external asset was fetched.

## Acceptance and independent evidence

Every accepted MASTER record satisfies all of these gates:

1. Public-clue uniqueness counter returns exactly one solution, stopping at two
   or its 2,000,000-node cap. Every stored witness independently satisfies all
   rules. Native completion separately validates the player's board and clues.
2. Basic propagation stalls. Pair/triple subset elimination and Sudoku locked
   candidates also leave unsolved cells. At least one candidate must be removed
   through an explicit single-assumption contradiction chain.
3. Those chains finish the board within 256 candidate probes. Each actual removal
   records a cell, excluded value and counts of the deduction rules used to
   produce its contradiction. Verification replays the public-clue solver and
   compares the whole trace and resulting solution; the rater never reads a
   stored witness. These are reproducible proof summaries, not a claim that a
   full human-readable step-by-step tutorial is supplied in the app.
4. In a separate search run using the same propagation and branch ordering for
   both levels, node count exceeds every original HARD record of that family.
   This is corroborating evidence; nodes alone do not define human difficulty.

Basic inference is peer/single elimination and hidden singles, plus exact cage
support (Calcudoku), strict inequality arc support (Futoshiki), or complete line
permutation support using both directional clues (Skyscrapers). Advanced
inference adds Hall pairs/triples and Sudoku locked candidates between
intersecting row/column/box units. Contradiction probes copy the current domains,
assume a candidate and apply those rules; only a proved contradiction permits
removal in the real board. No answer lookup chooses a removed candidate.

Search metrics below come from `grids_master_logic.py`: MRV cell branching with
advanced propagation, stopping after two solutions or 30,000 nodes. The separate
reference for Sudoku is direct MRV placement, and for Calcudoku/Futoshiki it is
whole-row enumeration from `grids_solver.py`. Skyscrapers has a separate whole-row
reference with independently enumerated legal column prefixes; it neither
imports nor uses the logical rater. Its counts were also cross-checked against
all 90 original Skyscrapers records. Reference and rating node totals use
different algorithms and must not be compared as the same metric.

| Game | HARD node max, all 30 | MASTER nodes min / median / max | MASTER maximum-depth range | Proven candidate removals per puzzle | Reference node max |
|---|---:|---|---|---|---:|
| Sudoku | 7 | 9 / 11 / 17 | 3–7 | 1–10 | 4,160 |
| Calcudoku | 7 | 9 / 11.5 / 33 | 3–6 | 1–7 | 12,323 |
| Futoshiki | 7 | 9 / 11 / 35 | 3–7 | 1–6 | 229,502 |
| Skyscrapers | 15 | 17 / 23 / 39 | 4–8 | 2–13 | 45,098 |

All 80 MASTER boards stall under basic and advanced propagation; all 80 finish
under the bounded contradiction-chain audit. Basic propagation leaves 31–54
Sudoku cells, 27–36 Calcudoku cells, 20–33 Futoshiki cells and 28–35 Skyscrapers
cells unresolved. The same basic rules stall on only 12/30, 4/30, 2/30 and 11/30
of the respective original HARD groups. The machine-readable audit includes
per-technique counts, branch counts, depth, probes and remaining-cell measures.
Its `advanced` counters include actual deductions after a proved removal;
separate `forcing_trace` counters describe trial branches.

Givens are descriptive, not an acceptance threshold: Sudoku has 23–26; Calcudoku
0–3; Futoshiki 2–9; Skyscrapers 0–4. Some MASTER Futoshiki boards therefore have
more givens than original HARD boards. Every MASTER Skyscrapers puzzle retains
at least one line with both directional clues, and all line and Latin constraints
participate in the inference instrument. No board was enlarged beyond the sizes
above and no shorter time limit was introduced.

## Bounded generation and storage cost

`grids_master_generate.py --game ID` has a 3,000-candidate default cap, a
3,000-node uniqueness budget during clue removal, a 30,000-node rating search,
a 256-probe chain cap and the independent 2,000,000-node reference cap. Rejected
or over-budget candidates are not included. A game file is atomically replaced
only after all 20 records pass, so an incomplete run does not publish a partial
MASTER pack. Each saved record has its construction seed, rules/rating versions,
reference count/nodes and full logical audit metadata.

| Game | Candidates examined | Accepted | Recorded generation seconds |
|---|---:|---:|---:|
| Sudoku | 226 | 20 | 63.468 |
| Calcudoku | 1,905 | 20 | 24.357 |
| Futoshiki | 174 | 20 | 21.740 |
| Skyscrapers | 351 | 20 | 156.557 |

Exact timings are host observations, not target performance promises; the
per-game `*-generation.json` files are authoritative. The native constant pack
is 480,200 bytes, exactly 39,200 bytes above the original pack (80 × 490).
The grid engine/renderer object is 13,723 bytes of text/read-only data under SH
`-Os`, with zero data/BSS. The largest compiler-reported individual grid frame
is 512 bytes (`grids_rules_complete`); renderer frame is 228 bytes. There is no
engine-owned heap, solver workspace, file handle, timer or callback allocation.
These are object/frame measurements, not a measured device stack peak.

## Retained ten-game rule and input audit

The existing C rule probes and all 980 witnesses were rerun. Every witness array
is deliberately overwritten in a temporary puzzle copy while the valid board is
retained; completion still succeeds. Incomplete or invalid edits fail. Native
state validation rejects unsupported difficulty/record combinations, changed
fixed clues, invalid dimensions/cursors, nonzero unused state and impossible
winning boards. MASTER selection visits every one of each game's 20 records
within the deterministic 500-seed test, and legacy ranges remain unchanged.

| Game | Rule boundary checked | Input/EXE/F4 |
|---|---|---|
| Sudoku | Latin rows/columns plus 3×3 boxes; box-invalid Latin square fails | 1–9; EXE checks; F4 toggles actual notes |
| Calcudoku | Latin plus each cage's exact operation; repeated cage value allowed in different lines | 1–N; EXE checks; no F4 action |
| Kakuro | Both run memberships, nonzero 1–9, sum and no repeats | 1–9; EXE checks; arrows skip black cells |
| Futoshiki | Both inequality directions; top-smaller sign matches drawn tip | 1–N; EXE checks; no F4 action |
| Skyscrapers | Independently expected top/bottom/left/right visibility | 1–N; EXE checks; missing clues impose nothing |
| Hitori | No white repeats, no adjacent black cells, connected white region | EXE toggles shade; DEL unshades; F4 checks |
| Binary | Blank distinct from zero; half counts, no triples, unique lines | 0/1; EXE checks; DEL restores blank |
| Numbrix | Full permutation with orthogonal consecutive adjacency | 1–2 digit draft + EXE enters; F4 checks |
| Magic | Full normal permutation and both diagonals; alternative squares accepted | 1–2 digit draft + EXE enters; F4 checks; PARTIAL/FREE |
| Sum Grid | Actual kept weights meet every row/column target, including zero | EXE toggles keep; DEL keeps; F4 checks |

Other number-grid DEL actions clear the current editable cell (or a pending
multi-digit draft first). Arrows cancel a multi-digit draft. Fixed clues remain
immutable. F3 explicitly reveals one cell and marks ASSISTED in all grid games
except Magic, which exposes no hint. Board/note edits increment `moves`, enabling
the common undo snapshot path. MASTER does not change any of these controls.
Root owns common F-keys, rules/stats/pause timing, MENU/OFF, archive persistence,
undo storage and four-level statistics separation/migration.

Confirmed audit defect **GRID-GEN-001**: the offline Sudoku counter previously
returned one for a fully filled board whose givens repeated in a unit. It skipped
unit validation at its terminal case. For example, start from the valid 4×4
rows `1234 / 3412 / 2143 / 4321`, then set the first value to 2: the old counter
returned `(1, 1)`. Initial bounds and row/column/box given checks now return
`(0, 0)` for that board. Regression covers valid and duplicate-filled 4×4 boards
and unsupported size 5. All original valid puzzle hashes and counts remain
unchanged; native gameplay completion already rejected such boards. No retained
native rule defect was found in this audit.

## Verification and renderer evidence

Commands (existing SDK, no toolchain reinstall):

```sh
python3 -B tools/generate/grids_verify.py
python3 -B tests/test_grids_master.py
cc -std=c11 -Wall -Wextra -Werror -fsanitize=undefined \
  -fno-sanitize-recover=all -g -DNG_GRIDS_TEST_MAIN -Iinclude -Isrc/games \
  tests/test_grids.c src/games/grids.c src/games/grids_decode.c src/games/grids_pack.c \
  src/core/common.c src/ui/draw.c -o /tmp/numgame-test-grids-master-ubsan
/tmp/numgame-test-grids-master-ubsan
bash tools/build.sh
```

Public evidence under `assets/grids/master/` is the four original JSON files,
their complete logical traces and `audit.json`, the per-game generation summaries,
and `legacy-sha256.json`. The schema checks enforce connected cages, valid
operation sizes, fixed givens, inequality edges and opposing visibility clues.
The independently reproducible five-test Python suite covers the complete 576
Latin-4 solution space for 78 clue sets, all 80 generation seeds and all 80
public-only proof replays. Its historical run passed in 23.919 seconds. The
historical original/full-MASTER counters passed in 57.303 / 41.346 seconds.
Strict C11/Wall/Wextra/Werror and UBSan passed all 980 records and 37 handler
lifecycles at that baseline. These observations are not claims about a later
integrated application build.

- `contact.png` and five `capture-*.png`: actual renderer, highest search case
  for each family (903/937/947/968) plus widest MASTER product clue (928). The
  9×9 notes, 6×6 inequalities, exterior clues and compact product fit. Numeric
  colors remain unchanged and each operator glyph is drawn once through the
  shared color policy. No hidden solution is drawn on the playing board.

The historical macOS ASan attempt timed out before `main()` in runtime shadow
memory initialization and was stopped after 12 seconds. **ASan was NOT VERIFIED,
not a PASS.** No raw host process dump is required to reproduce the public tests.

To reproduce the supplemental screenshots, build
`tools/generate/grids_capture.c` with the same grid/common/draw sources and run
its `--master` option. It writes five PPMs; PNGs contain only format-converted
actual pixels, and the contact sheet adds filenames. App-level entry/resume,
MASTER statistics/storage migration and full workflow captures are integration
checks, not asserted by this module-only capture tool.

**HARDWARE TEST REQUIRED:** physical key behavior, LCD readability/color contrast,
MENU/OFF, elapsed/idle timing, cold archive recovery, target stack and latency.
