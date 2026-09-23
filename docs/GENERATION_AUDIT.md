# Original content and solution audit (retained baseline)

Current before/after counts, new MASTER/HELL assets and policy by all30 IDs are
in [CONTENT_INVENTORY.md](CONTENT_INVENTORY.md) and
[CAPABILITIES.md](CAPABILITIES.md). The counts below describe the older pack.

Current inventory:900 GUESS/CALC records,980 grid records (including80 new MASTER),
and180 new board records. The original-corpus audit below remains valid for its
unchanged records; see the final follow-up section and BOARD_AUDIT.md /
GRID_MASTER_AUDIT.md for additions and human-rating limits.

All puzzle instances are generated locally by this project. Rule references are
not copied corpora. The calculator needs no Python, solver service or network.
Heavy solution counting is performed offline; cheap random games are seeded
on-device. Every native pack entry carries a stable puzzle index, while JSON
metadata preserves game ID, rules version, difficulty, construction seed and
witness/verification details.

| Game / family | Easy | Normal | Hard | Construction and acceptance |
|---|---:|---:|---:|---|
| 01 Baseball | Seeded | Seeded | Seeded | 3/4/5 digit sequence, distinct or repeated digits; 16/12/10 attempts |
| 02 Equation Guess | 30 per length | 30 per length | 30 per length | 270 true original equations across 6/7/8-character modes; hidden string matching |
| 03 Number Mind | 30 | 30 | 30 | Public exact-position clues; full finite-domain count =1 |
| 04 Clue Lock | 30 | 30 | 30 | Declared finite domain; parity, range, digit sum, distinct modulus constraints; count =1 |
| 05 Sequence | 30 | 30 | 30 | Six bounded grammar families; competing-rule next values checked |
| 06 Make Target | 30 per target | 30 per target | 30 per target | 180 card problems, targets 10 and24; exact rational witnesses; alternate valid expressions accepted |
| 07 Countdown | 30 | 30 | 30 | 90 exact-solvable targets, 6 small/large cards; positive integer intermediate witnesses |
| 08 Operators | Seeded | Seeded | Seeded | 3/4/5 ordered numbers; construct valid operators then hide; all legal solutions accepted |
| 09 Cross Math | 30 | 30 | 30 | Original 1–9 permutations/clues; every 9! placement checked independently for uniqueness |
| 10 Prime Factor | Seeded | Seeded | Seeded | Products of bounded primes; exact prime/exponent/product validation |
| 11 Sudoku | 30 | 30 | 30 | 9×9; 43/34/27 givens; independent public-clue solution count =1 |
| 12 Calcudoku | 30 | 30 | 30 | 4/5/6 grids, connected cages, bounded fixed cells; count =1 |
| 13 Kakuro | 30 | 30 | 30 | 5/6/7 overall size, connected runs and both memberships; count =1 |
| 14 Futoshiki | 30 | 30 | 30 | 4/5/6 Latin grids, givens + inequalities; count =1 |
| 15 Skyscrapers | 30 | 30 | 30 | 4/5/6, four-sided public visibility; count =1 |
| 16 Hitori | 30 | 30 | 30 | 4/5/6 independent patterns; uniqueness with nonadjacency + connectivity |
| 17 Binary | 30 | 30 | 30 | 6/6/8 grids; balance/triples/row+column uniqueness; count =1 |
| 18 Numbrix | 30 | 30 | 30 | 4/5/6 independent paths + clue removal; completion count =1 |
| 19 Magic Square | 30 layouts | 30 layouts | 30 layouts | 3/3/4; construction/dihedral variants, **not 90 independent bases**; partial/free multiple answers |
| 20 Sum Grid | 30 | 30 | 30 | 5/5/6, positive weights; unique keep/remove mask, zero targets included |
| 21–25 Strategy | Seeded | Seeded | Seeded | Bounded positions/presets; exact Hard policy tables, not puzzle-pack counts |
| 26 2048 | Classic | Classic | Classic | Same standard rule set, saved PRNG; not separate difficulty games |
| 27 Sliding | Seeded | Seeded | Seeded | 3×3/4×4; 80/160/240 legal shuffle steps, no shortest-distance claim |
| 28 Lights Out | Seeded | Seeded | Seeded | 4×4/5×5 legal generating toggles; any all-off solution accepted |
| 29 Rush | Seeded | Seeded | Seeded | Structural operator/range/two-step difficulty; exact integer answers |
| 30 Memory | Seeded | Seeded | Seeded | Starts3/4/5 digits; increasing lengths and difficulty-specific exposure |

Totals: **900 GUESS/CALC records** and **900 grid records**. The latter comprise
810 uniqueness-verified originals and90 explicitly distinguished Magic layouts.
There are 30 independent base problems per difficulty in every unique family;
grid verification canonicalizes rotation/reflection and digit relabeling where
the rules permit, so such equivalents are not counted as independent bases.
GUESS/CALC verifies global signature distinctness across difficulty/mode groups.

Uniqueness is not inferred from matching a stored witness. Native completion
uses actual row/column/cage/run/inequality/connectivity/adjacency/arithmetic rules.
The Python counters receive public clues only. All 900 grid witnesses are also
tested with deliberately wrong witness arrays while valid player boards remain
accepted. Magic, Lights, valid expressions and permitted alternate completions
are accepted by rules rather than reference equality.

Sequence does **not** claim a finite prefix determines a unique mathematical
continuation. The allowed coefficient/recurrence grammar is stated in RULES;
the verifier enumerates all competing grammar members. Completion displays the
selected rule and next calculation. The finite grammar is the uniqueness domain.

Independent checks include 1,966,080 exhaustive small Hitori masks, all 9!
Cross Math permutations per record, all code-domain candidates for Number Mind,
all bounded integers for Clue Lock, rational arithmetic via Python Fraction AST,
and native validator tests independent of generator witnesses. A one-off
cross-worker parser review exercised 60,000 differential cases without an
arithmetic mismatch; its temporary harness was not retained and is not part
of `tools/verify_content.sh`. The retained C suite includes 50,000 parser fuzz
inputs, and the Python pack verifier uses an independent Fraction AST.

[GUESS/CALC audit](GUESS_CALC_AUDIT.md), [grid audit](GRID_AUDIT.md) and
[strategy/quick audit](STRATEGY_QUICK_AUDIT.md) give exact generation policies,
domains, parser limits, counts, source references and executable commands.
`tools/verify_content.sh` reruns content/policy validation. `tools/generate/`
contains deterministic original generators; run individual generators only
when intentionally rebuilding content, then reverify and rebuild the target.

Native pack/table bytes and complete package size are measured from the final
ELF in [MEMORY_AUDIT.md](MEMORY_AUDIT.md), with machine-readable values in
`build-metrics.json`. JSON metadata and source-array text are development assets,
not additional runtime RAM.

## Runtime/BOARD/MASTER follow-up

The visible catalog is now1–28,31,32. Legacy29/30 data remains decodable, but
Rush/Memory are not public modes or counted games. No commercial puzzle corpus
was imported. Existing900 GC records and first900 grid records retain their
pack IDs and bytes.

Added180 original boards: Shikaku31 and Slitherlink32, each30 per EASY5×5,
NORMAL6×6 and HARD8×8. Independent solution counters verify uniqueness and
canonical distinctness of both clues and underlying region/loop structure.
The generator and verifier use different formulations. Native completion checks
public rules and connectivity, not a stored witness. See BOARD_AUDIT.md.

Added80 MASTER grids:20 each for Sudoku11, Calcudoku12, Futoshiki14 and
Skyscrapers15 at appended indices900–979. Basic/advanced propagation stalls,
public-clue failed-candidate contradiction chains and a same-rater search gap
against the existing HARD corpus are recorded, rather than calling any solver
node count a universal human difficulty grade. Independent uniqueness, all80
proof and accepted-seed replays, and small exhaustive Latin-domain comparisons
pass. Human rating remains provisional. See GRID_MASTER_AUDIT.md.

Current tools/verify_content.sh runs GC, legacy+MASTER grid, five MASTER Python
regressions, board independence and exact retained CPU comparisons. The full
integrated run is recorded in validation/runtime-content.txt. The offline
Sudoku counter's contradictory-full-givens bug was fixed with independent
negative and exhaustive4×4 regression checks; native puzzle rules are unchanged.
