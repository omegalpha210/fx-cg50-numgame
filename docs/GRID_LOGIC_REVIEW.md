# Independent review of GRID public-clue instruments

Review scope: `tools/generate/grids_master_logic.py` and
`tools/generate/grids_puzzle_master.py`, with their independent counters,
pack verifier and native completion rules as cross-checks. Reviewed in the
isolated grids worktree; no grids implementation file was edited by this
reviewer. Human difficulty remains provisional. This is not a proof of every
large puzzle or a hardware performance result.

## Concrete findings and resolution

1. **Malformed Numbrix duplicate clues were accepted by the instrument.**
   The original `where={value:position}` construction silently discarded the
   first copy. For `{'id':18,'n':2,'cells':[1,1,0,0]}`, `PuzzleLogic.search()`
   reported two solutions, while the independent `grids_solver.count()`
   reported zero. Returned boards included `[2,1,3,4]`, which violates a given.
   Generated packs additionally validate witnesses, so this did not demonstrate
   a shipped invalid record. The grids owner fixed duplicate and out-of-range
   givens by returning contradictory domains, added an empty-domain guard,
   and added zero-solution regressions for duplicate, out-of-range, negative
   and distance/parity-inconsistent givens. The reviewer reran all seven
   `tests/test_grids_expanded.py` cases successfully (0.278 seconds).

2. **PAN witness counts need explicit symmetry definitions.** The balanced-bit
   construction yields 384 distinct valid oriented 4×4 PAN squares, comprising
   48 D4 classes. Including cyclic row/column translations reduces these to
   three classes; also including complement `v -> 17-v` leaves three. The
   current 30 MASTER PARTIAL records have 30 D4 witness classes, 22 D4 plus
   complement classes, and three torus plus complement classes. They must not
   be described as 30 wholly unrelated mathematical constructions. Partial
   clue layouts, oriented witnesses and underlying square classes are different
   quantities. The owner added separate verifier fields and excludes all Magic
   records from unique-solution puzzle totals. FREE clears the givens and is
   one open PAN rule challenge, not 30 visible supplied puzzles.

3. **Numbrix trace coordinates differ from other games.** Its domain index
   (the common trace field named `cell`) represents `value-1`; the excluded
   candidate represents a board position. This is intentional in the solver,
   and `grids_expanded_verify.py` correctly inverts the witness when replaying
   traces. A report must not present those fields as ordinary cell/value pairs.

## Soundness inspection

No additional unsound deduction was found in the reviewed supported domains.
The instruments read public clues; they do not read a witness to eliminate a
candidate. Generation compares the final solved board to its witness only
outside the inference routines.

- Latin/Sudoku peer and hidden singles, Hall pairs/triples, and Sudoku locked
  candidates preserve the all-different constraints. Sudoku box construction
  is specifically for the supported 9×9 instrument, not a generic-size API.
- Calcudoku tuple filtering enforces cage arithmetic and same-unit inequality.
  Kakuro tuples use distinct 1..9 digits with the stated sum. Skyscraper tuple
  directions agree with the independent counter/native convention.
- Hitori uses 0=white and 1=black. A fixed white removes equal row/column
  candidates; black forces adjacent cells white. Connectivity traverses all
  cells still capable of white and requires every fixed white in one component.
  Removing a candidate cell and finding those required whites disconnected
  soundly forces that cell white. Reusing the earlier fixed-white set within
  a pass weakens propagation only; the next pass recomputes it. A completed
  board must contain a nonempty, connected white region.
- Numbrix domains map value to position. Singleton-position exclusion,
  consecutive orthogonal adjacency support, Manhattan distance/parity bounds
  and position ownership are necessary conditions on a Hamiltonian path.
  Inverting the final domains returns the correct board representation.
- Binary line patterns enforce balance/no-three; advanced propagation also
  rejects duplicate completed rows/columns and removes fixed line patterns
  from other lines. Basic propagation deliberately omits the latter technique.
- Sum Grid line tables enumerate all binary subsets with the stated weighted
  sum. Crossing support removes only unsupported assignments.
- Failed-literal elimination clones the current domains and removes a literal
  only after explicit contradiction. Its proof counters describe the deduction
  attempted, not a human difficulty certification.

## Independent bounded enumeration

The reviewer built a separate temporary enumerator at
`build-host/guesscalc-audit/grid_logic_soundness.py` in the guesscalc worktree.
It supplies no `solution` field to either instrument. Truth sets come from
independently enumerated bit boards, Hamiltonian paths, Latin squares, or
arithmetic assignments. For every tested restricted domain it checks both
basic and advanced propagation: every surviving true solution remains; a
reported contradiction has no surviving true solution. Search counts capped
at two and every returned board are checked independently.

| Space | Public boards | Propagation cases | Surviving-solution comparisons |
|---|---:|---:|---:|
|Hitori: all 2×2 labels 1/2 and ternary domains; sampled 3×3 labels with full 512-mask truth|80|6,848|848|
|Numbrix: all 209 distinct-given 2×2 boards; 128 sampled 3×3 public boards, full path truth|337|11,458|4,740|
|Binary: 128 public 4×4 boards, full bit-board truth|128|4,352|2,738|
|Sum Grid: all 2×2 weights 1/2, all resulting target vectors and ternary domains|254|41,656|8,704|
|Calcudoku/Futoshiki/Skyscrapers: 96 public 3×3 cases each, full Latin truth|288|7,488|2,584|
|Kakuro: 32 four-white-cell target sets, all 9^4 assignments|32|1,088|1,176|
|**Total**|**1,119**|**72,890**|**20,790**|

Result: PASS in 1.307 seconds. This enumerates each selected board's solution
space completely; sampled public boards and domain restrictions are not an
exhaustive test of every possible large input. Calcudoku samples here use sum
cages; other arithmetic operations were inspected and remain covered by the
owner's independent content verifier. Sudoku advanced rules were inspected,
not exhaustively enumerated in this small-board run. The committed owner
regressions provide a portable reproduction of the key complete spaces:

```sh
python3 tests/test_grids_expanded.py
python3 tools/generate/grids_expanded_verify.py
```

The expanded verifier must be run after all final assets are published. This
review does not assert that still-pending generation reached its target counts.

## Difficulty evidence and bounds

The reviewer recalculated retained HARD maxima from all 30 old records per
game with the exact new instruments: Sudoku 7 nodes/depth 3; Calcudoku 7/3;
Kakuro 5/2; Futoshiki 7/2; Skyscrapers 15/5; Hitori 13/6; Binary 9/3;
Numbrix 20/5; Sum Grid 1/0. This 0.624-second run agrees with the constants
used to select harder candidates. Search effort depends on this instrument,
branch ordering and clues. It is not a general ordering of human difficulty.
Tuple precomputation is outside the reported search-node budget; it is host
construction work, not native runtime solver memory. Actual selected cage/run
sizes bound these tables. PAN is a new rule constraint and permits multiple
valid answers, so its construction-match count is not a uniqueness certificate.

Reviewed fixed source SHA256 values:

- `grids_master_logic.py`: `fe3cfc18409d3b2ec62f7f9c37f24a098e5cea13d54b857e9eed4153af2b8d51`
- `grids_puzzle_master.py`: `f69eec70e357a21b3c0109a51cf850266e1e2d2ceb04060b284d28fc7978264c`
- `test_grids_expanded.py`: `c0c6f8c54ba1aac817edb064cb18cb629e8f43e92b78a032173a53586a58f493`

The generators were changing concurrently; these hashes identify the reviewed
snapshot. Re-run final independent verification after integration.
