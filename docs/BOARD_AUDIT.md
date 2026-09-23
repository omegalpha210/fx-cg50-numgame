# Shikaku and Slitherlink audit

Stable IDs 31/32 implement Shikaku and Slitherlink. The current pack has
**120 originals per game: 30 EASY, 30 NORMAL, 30 HARD, 30 MASTER**, 240 total.
MASTER is implemented; HELL is unsupported and rejected. The older 180 records
and IDs 0..89 per game are unchanged from commit `4e7da73`; MASTER appends
90..119. A JSON record comparison against that commit passed for all 180.

## Supply, provenance and counts

| Game | EASY | NORMAL | HARD | MASTER | Native public-clue payload |
|---|---|---|---|---|---|
|Shikaku31|30 at5×5|30 at6×6|30 at8×8|30 at8×8|120×65 =7,800 B|
|Slitherlink32|30 at5×5|30 at6×6|30 at8×8|30 at8×8|120×65 =7,800 B|

Supply is HOST_GENERATED_BANK. `boards_init()` calls `ng_bank_pick(g,30)`;
`nb_bank_count(id,difficulty,mode)` advertises the exact bucket size. Each base
has one runtime variant. No solver or unbounded generation runs on the device.
The common saved shuffle state supplies recent/nonrepeat selection, and each
module binds the stable ID, difficulty, size and public clues on validation.

External puzzle files: **0**. External puzzle records bundled: **0**.
This conclusion is based on the generator source and input trace, introduction
in Git commit `591018a`, plus deterministic reproduction of all 180 original
records using seed202609223132. The baseline JSON SHA256 was
`2f426769c6a3252ba1f64bb7d80fd925502b897f6d24483c46ad5d82d531bb8c`;
the original C public pack SHA256 was
`bae258a1653fe5f6f8592e5be5972d85e13c35db705a9fd849e1c981c2128f5a`.
MASTER uses a separate seed202610223132, preserving the original RNG stream.
Original generated data/source is covered by the root MIT license.

Nikoli's [Shikaku rules](https://www.nikoli.co.jp/en/puzzles/shikaku/) and
[Slitherlink rules](https://www.nikoli.co.jp/en/puzzles/slitherlink/) are rules-only
references. No sample diagrams, puzzle corpus, source code, artwork or logo
were copied. The external puzzle-data license field is therefore not applicable;
font/runtime licenses are separate and retained in THIRD_PARTY_NOTICES.md.

- `tools/generate/boards_generate.py`: locally generates rectangular partitions
  or simple-loop boundaries, then removes/places clues subject to uniqueness.
- `assets/boards/puzzles.json`: host public clues, witnesses, IDs and metrics.
- `src/games/boards_pack.c`: native public clues only; NbPuzzle is65 B.
- `tests/test_boards_fixtures.h`: host-only witnesses, excluded from native source.
- `tools/generate/boards_verify.py`: independent counters, native initializer
  comparison, rules, exact and D4 dedup and MASTER structural checks.
- `assets/boards/verification.json`: measured counts, hashes and host duration.

No exact or D4-equivalent clues/solutions occur within a size/game group,
including HARD versus MASTER. The verifier counts originals, not transforms.

## Meaning of MASTER

Board size remains8×8. Fewer givens alone are not used as the difficulty claim.
The generation acceptance conditions are independently checked from public clues:

- Shikaku: after both clue-single and cell-single rectangle propagation, at least
  four rectangles remain unresolved. The generator also requires at least45
  initial candidates and search work beyond selecting every rectangle once.
  This forces interacting candidates rather than only obvious singleton regions.
- Slitherlink: clue/degree/cycle propagation leaves at least16 unknown edges;
  the generator's exact edge solver visits at least25 nodes. The independent
  face-color solver must also need more than one node. Every final answer has
  one connected loop; local clue matches alone do not suffice.

| Independent verifier nodes | EASY | NORMAL | HARD | MASTER |
|---|---|---|---|---|
|Shikaku cell-first exact cover|8–17|9–54|15–76|21–125|
|Slitherlink face-color CSP|1–5|1–7|1–199|5–1,317|

MASTER Shikaku has10–15 clues and Slitherlink25–26 clues. These are measured
structural/search indicators, not absolute human grades. Their ranges overlap
HARD; no assertion is made that every MASTER is harder for every person than
every HARD. **Human difficulty and hardware response time remain untested.**

The Shikaku generator uses clue-first rectangle MRV; the independent counter
enumerates rectangles and branches on the first uncovered cell. Slitherlink's
generator reasons over edges, while its verifier branches on face colors with
an independent connected-loop graph check. Uniqueness is counted from public
clues with a cap of two, not inferred from a stored witness.

## Rule-based runtime and input

Shikaku completion requires all cells covered by nonoverlapping rectangles,
each containing exactly one clue with equal area. Arbitrary valid region labels
are accepted. It never compares a player's labels to a solution array.
`board[]` stores labels0..64; data[0..63] stores public clues. data[64] is the
pending corner: phase0 requires-1, phase1 requires a valid index. Arrows move;
EXE selects first/opposite corners; reverse corner order works. Invalid regions
leave the selection active with an explanation. EXIT in phase1 cancels only
the selection. DEL removes the entire region under the cursor. Only actual
creation/removal increments moves and adds an undo snapshot.

Slitherlink packs each shared edge once using two bits:0 blank,1 line,2 X.
State3 and nonzero unused bits are invalid. Horizontal edges precede vertical
edges. Sizes5/6/8 have60/84/144 edges and4/6/9 data words; cursor143 is valid.
Arrows wrap within an orientation; F4 selects H/V. EXE toggles line/blank and
replaces an X; DEL toggles X/blank and replaces a line. Completion checks public
clue counts, degree2 at each used vertex, a nonempty graph and single-component
connectivity. Open lines, crossings, branches and separate loops are rejected.

Both validators check public immutable clues, fixed flags, dimensions, status,
phase, selection, packed edge bounds, unused storage and unsupported fields.
No native code depends on a host witness or accesses files, keyboard or clock.

## Actual tests and captures

Strict C11 and UBSan passed with the real core, application, drawing code and
production codec. The isolated worker linked temporary count stubs only for
unselected GC/grid banks; root's full integration must omit those build files.

- All240 native records completed through actual navigation/actions and real
  module rendering; INIT repeats the same puzzle/RNG; codec and undo snapshots
  round-trip during play and after completion.
- All625 labeled/gapped2×2 Shikaku boards ×625 clue patterns:390,625 comparisons
  against independently enumerated rectangles.
- All4,096 2×2 Slitherlink edge assignments ×625 clue patterns:2,560,000
  comparisons against an independent graph model;13 loops/625 public patterns
  separately verify the host face-color uniqueness counter.
- Shikaku gap, overlap, missing/two clues, wrong area, nonrectangular labels,
  reverse corners, selection cancel/removal and terminal input cases.
- Slitherlink no/one/two loops, degree4, open/extra lines, zero versus blank,
  X states, all edge cursors and packed-word boundaries.
- Corrupted IDs, levels including HELL, public clues, metadata, dimensions,
  phase/corner, labels, fixed/notes flags and unused/packed storage rejected.
- Eight actual-app workflows (two IDs×four levels): entry level selection,
  phase/held-input barriers, UNDO, MENU encoded checkpoint/cold RESUME, NEW
  cancellation, changed setting versus saved run, cross-game resume, RESULT→
  RULES→RESULT, repeated save/OFF hooks and no double result count. The measured
  run rendered1,665 common-app frames and35,551 module frames. The malformed-state
  loop also covers MASTER. Primitive assertion totals
  are not claimed as independent rule cases.

`tests/test_boards_run.sh` still supplies a small standalone registry adapter and
no-op codec telemetry; it separately passed. The app suite omits that adapter
and links the real core. Host hooks cover encoded sessions, not BFile atomic
archive recovery or actual OS transitions. Root owns those integration checks.

`assets/strategyquick/master-contact.png` contains all ten owned MASTER games
and Shikaku pending selection from the actual app renderer. It was visually
inspected: geometry/text remain in their content bounds at8×8. The optional
owned capture runner is `tests/test_strategyquick_capture.c`. These are host
renderer images, not emulator or physical LCD photographs. Older18 board-only
captures are retained as E/N/H baseline evidence; the runner now supports all
four levels.

```sh
python3 tools/generate/boards_generate.py --master
python3 tools/generate/boards_verify.py
cmake -S tests -B build-host -DNG_SANITIZE=ON -DNG_ASAN=OFF
cmake --build build-host --target test_boards -j8
./build-host/test_boards
bash tests/test_boards_run.sh
```

## Memory and remaining hardware acceptance

The two current native arrays total15,600 B readonly, +3,900 B from baseline.
Loading copies only one record into the already allocated NgGame. There is no
heap allocation, writable module BSS, whole-bank decompression or runtime solver.
Strict SH GCC14.1 compilation uses C11, `-Wall -Wextra -Werror`, `-Os`,
`-m4-nofpu -mb`, `-Wframe-larger-than=2048` and `-fstack-usage`.
The largest individual board frame remains316 B in nb_slither_complete;
boards_init is12 B. These are individual function frames, not observed stack
peak, call-chain totals or a final linked-package memory measurement.

Current UBSan checks passed. The ASan+UBSan binary builds, but the latest owned
attempt times out after15 seconds before output; **ASan is NOT VERIFIED**.
See assets/strategyquick/master-asan-attempt.log. No cause is inferred from
this particular attempt. Physical HOLD behavior, LCD readability, timing,
MENU/Fugue, SHIFT+AC/ON, recovery under power loss and actual calculator stack/
heap peaks remain **HARDWARE TEST REQUIRED**. The original MENU flicker is not
claimed fixed by these host results.
