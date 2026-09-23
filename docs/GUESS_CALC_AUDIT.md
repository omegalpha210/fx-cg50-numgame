# GUESS / CALC implementation audit

Game IDs 01–10 are implemented in `src/games/guesscalc.c` and the pure bounded
parser in `src/games/guesscalc_math.c`. All expose the common `NgModule` ABI,
logical-key actions, a distinct playable renderer, rules, initialization,
completion and persisted-state validation. Engines use no gint, clocks, files,
heap allocation, keyboard driver or network. Root owns application, storage,
input mapping, the integrated build and commits.

## Baseline content and difficulty (4e7da73)

All baseline content was original project generation. There is no copied problem
corpus, puzzle expansion by rotation, or digit-substitution pack expansion.
`tools/generate/guesscalc_generate.py` uses deterministic seed
`0x4e554d47414d45`. Each JSON record carries game ID, rules version, puzzle ID,
difficulty, a witness, and the relevant verification metadata. Runtime pack
selection used the current run's saved RNG. JSON is provenance/test material;
only the compact C data is compiled into the add-in.

| ID | Content | Easy / Normal / Hard | Modes / structure |
|---|---|---|---|
| 01 | Device seed generation | 16 / 12 / 10 guesses | 4/3/5 digits × unique/repeat; default 4 unique |
| 02 | 270 distinct equations | 30 per length per difficulty | 7/6/8 fixed characters; hard 7/8 use two operators |
| 03 | 90 unique code puzzles | 30 / 30 / 30 | 4 digits over 0..5; 4 over 0..7; 5 over 0..7 |
| 04 | 90 unique locks | 30 / 30 / 30 | Domains 0..99 / 0..499 / 0..999; hard modular/CRT |
| 05 | 90 distinct prefixes | 30 / 30 / 30 | Six bounded rule families; six visible terms |
| 06 | 180 distinct card/target sets | 30 per target per difficulty | Target 24/10; card ranges 1..6 / 1..9 / 3..9 |
| 07 | 90 distinct six-card/target sets | 30 / 30 / 30 | 1 / 2 / 3 distinct large cards; untimed |
| 08 | Device seed generation | 3 / 4 / 5 numbers | Fixed number order; standard precedence |
| 09 | 90 unique cross-math boards | 30 / 30 / 30 | 4 / 2 / 0 fixed values; whole-board set 1..9 |
| 10 | Device seed generation | Prime choices through 7 / 13 / 31 | Composite target 4..1,000,000 |

At baseline there were exactly **900 globally distinct records** over the seven bundled
families. Distinctness checks ignore difficulty and witness formatting and use
actual problem signatures. Sequence's 90 coefficient instances use six grammar
templates; they are not claimed to be 90 different mathematical rule families.
Content presets describe structure/ranges, not a measured human difficulty rank.

## MASTER additions and supply inventory

The append-only update contains **1,230 canonical-distinct base records**:
the original 900 plus 330 MASTER records. MASTER is stable level 3; HELL is not
accepted by any of these ten modules. No rotations, digit substitutions, card
permutations or other transforms are applied or counted as additional puzzles.
Human difficulty remains provisional; the structural criteria below are exact,
independently checked properties, rather than estimated completion times.

| ID / category | Policy before → after | Runtime entry / array | Base records before → after | E / N / H / M after |
|---|---|---|---|---|
| 01 BASEBALL / GUESS | RUNTIME_GENERATED → same | `init_baseball`; no bank | 0 → 0 | N/A: constructed code, six modes |
| 02 EQUATION / GUESS | HOST_GENERATED_BANK → same | `init_equation`; `gc_equation_pack`, `gc_master_equation` | 270 → 360 | 90 / 90 / 90 / 90 |
| 03 NUMBER MIND / GUESS | HOST_GENERATED_BANK → same | `init_mind`; `gc_mind_pack`, `gc_master_mind` | 90 → 120 | 30 / 30 / 30 / 30 |
| 04 CLUE LOCK / GUESS | HOST_GENERATED_BANK → same | `init_lock`; `gc_lock_pack`, `gc_master_lock` | 90 → 120 | 30 / 30 / 30 / 30 |
| 05 SEQUENCE / GUESS | HOST_GENERATED_BANK → same | `init_sequence`; `gc_sequence_pack`, `gc_master_sequence` | 90 → 120 | 30 / 30 / 30 / 30 |
| 06 MAKE TARGET / CALC | HOST_GENERATED_BANK → same | `init_cards`; `gc_target_pack`, `gc_master_target` | 180 → 240 | 60 / 60 / 60 / 60 |
| 07 COUNTDOWN / CALC | HOST_GENERATED_BANK → same | `init_cards`; `gc_countdown_pack`, `gc_master_countdown` | 90 → 120 | 30 / 30 / 30 / 30 |
| 08 OPERATORS / CALC | RUNTIME_GENERATED → HYBRID | `init_operators`; E/N/H construction, `gc_master_operators` | 0 → 30 | 0 / 0 / 0 / 30 |
| 09 CROSS MATH / CALC | HOST_GENERATED_BANK → same | `init_cross`; `gc_cross_pack`, `gc_master_cross` | 90 → 120 | 30 / 30 / 30 / 30 |
| 10 PRIME FACTOR / CALC | RUNTIME_GENERATED → same | `init_factor`; no bank | 0 → 0 | N/A: bounded prime products |

Equation has 30 records per mode per level, in mode order STANDARD/SHORT/LONG.
Target has 30 per target per level, in mode order 24/10. All other bank-backed
mode/level slices have 30. Canonical distinct counts equal the base counts in
the table. Card signatures sort the cards; Number Mind signatures sort the
clues; witness text, metadata and difficulty labels do not manufacture new bases.
Sequence signatures use the six public terms, not the stored seventh answer.
Transforms applied: **0 for every ID**. External puzzle files: **0 for every
ID**. External parsed puzzle records: **0 for every ID**. External puzzles
bundled: **0 for every ID**. Runtime-generated games have no fictional bank count.

Common dispatch is `ng_new`/`ng_new_supply` → module `init_*`. Banks are immutable
C arrays read directly from flash, copying only active fields into one `NgGame`.
There is no runtime file asset, whole-bank copy, decompressor or puzzle solver.
`gc_bank_count(id, difficulty, mode)` exposes the correct 30-record slice to the
common supply manager. Each initializer calls `ng_bank_pick`, which supports a
saved permutation ordinal and preserves the old RNG selection when supply seed
is zero. The common application owns persistent shuffle cycles and four recent
IDs; these modules do not allocate a separate history or run-time search cache.

| ID | Meaningful MASTER change and exact acceptance evidence |
|---|---|
| 01 | MID/SHORT/LONG use 6/5/7 digits (previously 4/3/5), with unique/repeat variants. 16 attempts accommodate the larger domain. Leading zero and exact-first duplicate feedback remain unchanged. |
| 02 | 9/8/10 characters, respectively 3/2/3 operators and 12 attempts. Every expression mixes additive and multiplicative operators; evaluating left-to-right changes its result. All 90 equalities independently evaluated. Feedback colors retain their position-based precedence. |
| 03 | Six digits over 0..7, repeats allowed: 262,144-code domain. Each of 30 puzzles has 8–13 clues and exactly one solution under exhaustive independent enumeration. At most 78 active board cells. |
| 04 | Domain 0..9999 with three different moduli and decimal digit sum. All 30 have one solution; deleting any one of the four public constraints leaves at least two candidates. |
| 05 | 15 two-term affine recurrences and 15 independent alternating arithmetic lanes. The former uses first two values -3..6 and three coefficients in {-2,-1,1,2}; the latter starts -6..9 with distinct nonzero steps -5..5. Every prefix has exactly one next answer across all eight allowed bounded families and does not fit any of the old six families. Old levels keep their old six-family grammar. |
| 06 | Four cards in 1..24, target24/10. All 60 exact witnesses independently checked. Exhaustive signed-integer tree search, including unary negatives, finds no integer-only solution; exhaustive two-pair trees find no depth-two solution. Thus a fraction and binary depth three are necessary. Alternative legal expressions still win. |
| 07 | Same six-card deck and positive-integer intermediate rules, with three large cards. Every exact target needs all six cards and at least one division, verified over every legal subset and integer operation tree. Partial/approximate submissions and the original 10/7/5/0 scoring remain legal. |
| 08 | Six ordered numbers and five operator slots. All 1,024 assignments per puzzle checked: 16 puzzles have one solution, 7 have two, and 7 have three. Every solution uses at least three operator kinds, including additive and multiplicative precedence groups. Every valid solution is accepted. |
| 09 | Same 3×3 global 1..9 rules, no fixed values. Both row-only and column-only counts of legal whole-board permutations exceed 168, the independently recomputed maximum of `min(row_count,column_count)` over all old Hard puzzles. New minima range176–216. All 30 full puzzles still have exactly one solution in exhaustive 9! search. This rates coupling, not measured human solve time. |
| 10 | Four distinct primes chosen from2,3,5,7,11,13; exactly two appear squared. Five bounded shuffle steps and two choices construct a composite no larger than715715. Validation re-factors this shape; 3,000 deterministic seeds tested. There is no large-prime guessing or unbounded rejection loop. |

E/N/H Operators preserves the original bounded 128-attempt construction and
verified fixed fallback with the requested 3/4/5-number count. Those old levels
have a structural length preset, not a solver-derived human difficulty rating.
MASTER always loads a rated bank record, with no lower-level fallback.

### Provenance and reproducibility

Legacy: `tools/generate/guesscalc_generate.py` →
`assets/guesscalc_packs.json` and `assets/guesscalc_packs.h`, seed
`0x4e554d47414d45`. MASTER: `tools/generate/guesscalc_master.py` →
`assets/guesscalc_master.json` and `assets/guesscalc_master.h`, seed
`0x4D41535445524743`. Generation imports ordinary Python arithmetic/collection
modules and local arithmetic helpers; it reads only the existing original
legacy JSON for exclusion/calibration. It has no downloaded corpus input or
network path. Every host search has a finite domain or explicit attempt cap;
failed generation raises an error and never emits an unverified substitute.

The actual generator was introduced with the current legacy pack in commit
`d267e6b8d1232ec2e368e27037ea9a658593f3ef`; the baseline reviewed here is
`4e7da73bef82e7e20dce2a8d80ff85d9b8f66639`. Original project data is MIT under
the repository LICENSE. External puzzle source URL/revision/license are not
applicable because no external puzzle corpus is used. Rules-only references
(including Project Euler Number Mind) and separately licensed fonts/SDK are
documented in `docs/ASSET_PROVENANCE.md`; they are not puzzle data imports.
The zero-import conclusion is supported by source/history review and fresh
byte-identical generation, rather than inferred from a README label. The final
MASTER generator replay reproduced both files byte-for-byte in208.556s on the
host; this is offline content creation, not device load time.

| Source asset | Bytes | SHA256 |
|---|---:|---|
| legacy JSON | 364708 | `e11fa9f8720b813570f686558837e24acab1afe73d4f700610c4651423bd7ef2` |
| legacy C header | 48839 | `430df5b94b2ffa37871443d84319aaa449576a55ad8944875317322d6521259a` |
| MASTER JSON | 167707 | `1d0eed741199b02f4b83325768943043348dc12db30c387d19db6175aa94624a` |
| MASTER C header | 20087 | `0b363f2051bf312ca1a0e161e39bf5eecdf77192902adffb00a4379d6469f0d2` |

The independent MASTER validator never imports either generator. It uses its
own finite-domain enumerators, AST evaluation, canonical-tuple subset recursion
and full-board permutation loops. An early MASTER candidate set passed the
fraction check but failed the independent depth-two check; the generator was
corrected to reject those candidates. The final 60 Target records satisfy both
requirements. No constraint was relaxed to make that verification pass.

### Stable IDs, state compatibility and tests

Legacy arrays, records and indices are unchanged byte-for-byte. Appended IDs:
Equation270–359, Mind/Lock/Sequence/Countdown/Cross90–119, Target180–239, and
new Operators bank0–29 (difficulty3 distinguishes it from old seeded puzzles).
All MASTER validators require pack revision2 and reject HELL, invalid bank
boundaries, damaged immutable fields and impossible terminal states. Legacy
revision1 states are still accepted and replayed by the original recipe.

`tests/test_guesscalc_legacy.h` holds 162 portable field fingerprints generated
by compiling the actual baseline `guesscalc.c`: every old mode × three levels ×
seeds1,17,UINT32_MAX. It covers all original state fields, including RNG, puzzle
ID and message; only the newly added common supply metadata is excluded. New
initialization matches every fingerprint, and each revision1 state completes.
Common old-wire decoding/migration tests remain the integration owner's task.

Owned regression evidence:

- Legacy independent verifier: 7,711 checks, 13.932s in the initial baseline run.
- MASTER independent verifier: **344,898 checks**,39.466s; includes independent
  recalculation of all30 old-Hard Cross coupling thresholds.
- Pure engine/render/parser UBSan: **8,509,037 assertions**,5,575,150 drawing
  rectangles. Covers all4 levels/all18 modes, all330 new bank records,30×1024
  actual Operators submissions, malformed states, alternative answers,600
  MASTER Baseball seeds and3,000 MASTER Factor seeds.
- Current common app/storage/UI + owned modules under UBSan: all18 MASTER modes
  pass draft save, cold RESUME, exact-puzzle INIT, completion, assisted MASTER
  stats isolation from Hard, and saved result round-trip. Eight bank families
  each pass35 NEW operations: complete30-item permutation and cycle boundary.
  Existing ten-game undo/RULES/records/switch/MENU/OFF-hook workflows still pass.
- All10 MASTER screen fixtures were rendered with the current common renderer
  and visually reviewed at396×224. Seven-digit Baseball, ten-character Equation,
  six-number Operators, all visible clues and expression inputs fit without
  overlap or clipping. The owned capture tool now emits18 fixtures total.
- Strict SH C11 `-Os -Wall -Wextra -Werror -Wframe-larger-than=2048` passes for
  both engine sources. Frame measurements below are compiler reports.
- A fresh ASan+UBSan binary was built; a bounded30s execution produced no output
  and timed out. A minimal ASan executable also timed out after5s before its
  first stderr marker. This is **not an ASan pass**. UBSan completed separately.

Reproduce content verification:

```
python3 tools/generate/guesscalc_verify.py
python3 tools/generate/guesscalc_verify_master.py
```

`test_guesscalc` is the normal integrated CMake target; `test_guesscalc_app`
exercises the shared application. During isolated worker verification only,
not-yet-integrated other-family bank-count hooks were stubbed to zero in an
ignored build file, and2048 was explicitly set to its unrelated CLASSIC mode.
No GUESS/CALC, codec, application, renderer or supply behavior was stubbed.

### Per-puzzle memory and bounds

SH GCC14 `-Os` symbols give the following **flash record sizes**, distinct from
the RAM required by one active puzzle. No bank is copied into RAM.

| ID | Record bytes old / MASTER | Flash bank bytes before → after | Individual init frame old → new, bytes |
|---|---:|---:|---:|
| 01 | 0 / 0 | 0 → 0 | 28 → 28 |
| 02 | 9 / 11 | 2430 → 3420 | 12 → 16 |
| 03 | 98 / 98 | 8820 → 11760 | 20 → 52 |
| 04 | 22 / 24 | 1980 → 2700 | 12 → 8 |
| 05 | 22 / 22 | 1980 → 2640 | 12 → 8 |
| 06 | 94 / 94 | 16920 → 22560 | 12 → 8 |
| 07 | 94 / 94 | 8460 → 11280 | 12 → 8 |
| 08 | 0 / 14 | 0 → 420 | 128 → 128 |
| 09 | 42 / 42 | 3780 → 5040 | 12 → 8 |
| 10 | 0 / 0 | 0 → 0 | 36 → 60 |

Total embedded bank data: **44,370 → 59,820 bytes**, increase15,450.
The module object, including code/rules/constant metadata:66,413 →84,232 bytes
(increase17,819); `.data`/`.bss` remain0. These are comparable standalone object
measurements, not a final linked package-size measurement.

Every game uses the common active `NgGame`, native sizeof1832 before and1848
with the current16-byte supply metadata. Module heap allocation is0 and
decompression scratch is0. No30-game state array is allocated. An init frame
in the table excludes callers, callees, compiler arithmetic helpers and libc;
it is not an observed stack high-water mark or total RAM peak.

The arithmetic call graph is `gc_expression` → `expression` → `term` → `atom`,
with parentheses re-entering `expression` and unary minus re-entering `atom`.
The depth guard allows12 levels and rejects the13th. SH individual frames are
56/84/84/52 bytes, `combine`44 and `normalize`84. A deliberately conservative
project-function-only bound for the parser is
`56 + 13*(84+84+52) + 44 + 84 = 3044` bytes, before its caller and any libgcc
integer division/gcd support. It is not a measured complete stack peak.
Baseball/Equation feedback uses a532-byte individual frame; flat Operators
generation does not introduce nested expressions. Compiler frames, recursion
bounds, host execution and actual device high-water are kept separate.

Native load/generation p50/p95/max, actual SH stack/arena high-water and physical
device MENU/OFF behavior remain **HARDWARE TEST REQUIRED / NOT MEASURED** in this
family audit. Host timing cannot establish the device's0.5s/2s targets. Root's
integrated diagnostic workload supplies the broader runtime evidence.

Baseball preserves leading zeros and consumes exact positions before misplaced
matches. Equation guesses must be true arithmetic equations before consuming an
attempt, and success means the hidden character sequence, not merely an equal
value. Its duplicate feedback is also consumed exact-first. Both histories
scroll and neither offers guess undo.

Number Mind permits repeats and leading zero. Every pack has at least one
zero-match clue, so its public-clue hint excludes at least one digit from a
selected position. Clue Lock shows every constraint at the start and its two
modular divisors are always different.

Sequence predictions are unique **only within the documented finite grammar**:
arithmetic, geometric, quadratic, Fibonacci-like, alternating arithmetic lanes,
and affine recurrence. The app expressly disclaims universal sequence
uniqueness. Coefficient bounds appear in RULES. Completion shows the chosen
family and the next calculation. The independent validator considers every
family for each prefix, rather than only the generator's chosen family.

Countdown draws small cards from two copies of 1..10 and large cards without
replacement from 25/50/75/100. Every published target is 100..999 and has an exact
witness. Unused cards are accepted. Submitted distance 0 earns 10, 1..5 earns 7,
6..10 earns 5, otherwise 0. FINISH retains the best submitted valid distance;
it never claims a non-exact result is optimal. One hard regression pack admits
both `75+25` and the unused-card solution `100`.

Cross Math evaluates six equations with multiplication before addition and
subtraction, and checks the global 1..9 multiset. There is no extra Latin-square
constraint. Completion checks the rules, not the witness.

## Arithmetic and bounds

The parser is recursive descent with fixed storage: 96 input characters,
12 nesting levels, 16 numeric literals, 31 operations, literal at most 1,000,000,
and reduced numerator and denominator bounded by 1,000,000,000. Rational results
are normalized by gcd. Because each operand is bounded, temporary products are
at most 10^18 and addition/subtraction at most 2×10^18, within signed 64-bit range.
Division by zero, malformed syntax, unnecessary leading zeros and oversized
results are rejected. No evaluation of unrestricted host code is used.

MAKE TARGET validates the value and literal multiset, accepting alternative
expressions and repeated-valued cards independently. Unary minus, parentheses
and fractional intermediates work. COUNTDOWN additionally validates every
literal, unary and binary intermediate as a positive integer. It rejects a
fraction or negative intermediate even if a later operation would cancel it.

MISSING OPERATORS accepts every correct operator combination, using the same
bounded rational arithmetic and explicit standard precedence. PRIME FACTOR
checks each base for primality, positive exponents and the full bounded product;
order and repeated prime factors are allowed. 1 and composite bases are invalid.
Prime targets are not generated and are rejected by state validation. Answer
factorization performs trial division only through the remaining square root.

## Hint, reveal and undo policy

| IDs | Behavior |
|---|---|
| 01–02 | No hints or undo that erase observed feedback |
| 03 | HINT cycles positions and lists values allowed by visible zero-match clues |
| 04 | HINT explains the first modular arithmetic progression |
| 05 | HINT explicitly reveals only RULE FAMILY |
| 06 | HINT gives a first combination; ANSWER copies a complete example |
| 07 | HINT gives a first combination; F4 FINISH ends with best submitted distance |
| 08–09 | F4 is explicitly REVEAL, not a purported logical hint; common undo supported |
| 10 | HINT proves a prime divisor; ANSWER copies the full factorization |

Hints and reveals mark assisted. Reveals do not automatically submit an answer.
Number Mind's hint reads only public clues, not its secret witness. For Operators
and Cross Math each actual edit, deletion or reveal increments the move counter,
allowing common undo snapshots and INIT confirmation to behave correctly.
Repeated entry of an unchanged value does not manufacture an edit. Root retains
assisted status through undo.

## State and save validation

Immutable problem fields are checked against the selected pack and its difficulty
and mode, with explicit domain, index, cursor, operator and input bounds.
Won states are revalidated using the actual rules and submitted input/board.
Baseball and Equation histories have their grammar/feedback recomputed; premature
wins/losses, a win followed by more attempts, and inconsistent attempt scores
are rejected. Countdown checks saved best value, distance and score consistency.
Impossible `NG_WON` flags on initial puzzles and damaged completed states are
regressions for all ten IDs. No module performs its own file I/O.

Save/cold-load, common undo, EXIT, MENU, statistics and input barriers are root
integration responsibilities. The host helper `tests/test_guesscalc_workflow.h`
types valid solutions through a supplied key callback; it is deliberately a
**host-test-only** witness accessor and is not compiled into production.

## Historical pre-MASTER executed checks

- `tests/test_guesscalc.c`: UBSan with recovery disabled, C11
  `-Wall -Wextra -Werror`; **1,757,545 assertions PASS**, including **1,531,763
  renderer rectangles**. This count includes pixel-bound assertions, not that
  many independent puzzle cases.
- 1,080 lifecycle configurations: every game mode × three difficulties × 20
  seeds, including legal/illegal input, deterministic INIT, renderer calls,
  hints where supported and completion. All 900 bundled records are exercised
  by production rules, including every equation's arithmetic.
- 68,530 Baseball feedback pairs checked against independent frequency counts;
  rational identities over 1,600 positive pairs and 3,721 signed products;
  50,000 random bounded parser strings; explicit divide-by-zero, overflow,
  leading-zero, duplicate-card, fractional/negative Countdown cases.
- All 256 operator assignments for each of 30 five-number problems; valid
  alternatives are accepted. Different full Target solutions and the
  Countdown unused-card solution are also accepted.
- `tests/test_guesscalc_app.c`: actual common app, renderer and logical-key
  path for edit → DEL → UNDO, fresh REVEAL → INIT confirmation, REVEAL → UNDO,
  sticky assistance and unchanged-value input; **PASS under UBSan**.
- `python3 tools/generate/guesscalc_verify.py`: **7,711 independent checks PASS,
  13.322 seconds** on this host. Number Mind fully enumerates its finite code
  domains; Clue Lock fully enumerates its integer domain; Cross Math uses a
  separate exhaustive 9! permutation reference, unlike generation's pruned
  row-combination solver. Sequence independently enumerates the entire grammar.
  Card and equation witnesses are parsed independently using Python AST walking
  and exact `fractions.Fraction`, without importing the generator.
- Installed SH GCC, `-std=c11 -Os -Wall -Wextra -Werror -DFXCG50`, compiling both
  module sources: **PASS**. Full target linking/package validation belongs to
  root's integrated build report.
- Apple `cc` ASan+UBSan executable did not reach `main`: runtime initialization
  deadlocked recursively through dyld/malloc and the ASan shadow-map lock.
  `build/guesscalc/sample.txt` captured the stack; the owned hung process was
  terminated. **ASan execution is unverified, not PASS.** UBSan alone executes
  normally. No SDK reinstall or upgrade was performed.

## Historical pre-MASTER files and memory boundaries

- `assets/guesscalc_packs.h`: **48,839 bytes**;
  SHA256 `430df5b94b2ffa37871443d84319aaa449576a55ad8944875317322d6521259a`.
- `assets/guesscalc_packs.json`: **364,708 bytes**;
  SHA256 `e11fa9f8720b813570f686558837e24acab1afe73d4f700610c4651423bd7ef2`.
- Standalone SH `-Os` object measurement, before linker section elimination:
  `guesscalc.o` text/rodata **66,073**, data **0**, BSS **0** bytes;
  `guesscalc_math.o` text/rodata **2,793**, data **0**, BSS **0** bytes.
  These are object measurements, not the application's final flash/RAM total.
- Modules allocate no heap. State lives in the active common `NgGame`. The exact
  recursion cap is enforced; `.su` reports individual stack frames, not observed
  target peak stack. Production parser/helper/library combined peak remains
  **HARDWARE TEST REQUIRED**.

Actual calculator LCD contrast, key ergonomics/repeat, OS MENU, power-off,
filesystem reliability, latency and stack/heap behavior remain **HARDWARE TEST
REQUIRED**. Host framebuffer review is not hardware validation.

## 2026-09-22 retained-game audit (P3)

Scope: retained IDs 01–10 on task branch `task/guesscalc-runtime-audit`, isolated
worktree `.worktrees/guesscalc`, based on `ad9c34a`. The earlier counts above
record the original implementation audit. This follow-up uses the current
normal font, so renderer-rectangle assertion totals differ. It does not claim to
reproduce or repair the physical calculator's MENU failure.

| Finding | Demonstrated symptom / cause | Minimal change | Regression / remaining limit |
|---|---|---|---|
| GC-A01 | A nonterminated 97-byte input at a protected-page boundary produced SIGBUS in all three text parsers. `gc_expression` dereferenced before its length check; Equation RHS and Factor exponent scans lacked an upfront total-length check. | One shared bounded terminator scan, before any expression/equation/factor parse. Existing grammar and arithmetic are unchanged. | Protected-page cases change from signal -10 to safe `false`; included in the UBSan test. Common app input validation already rejects nonterminated persisted fields, so this is parser-boundary hardening, not evidence for MENU causation. |
| GC-A02 | Countdown `valid()` accepted a fabricated scored DRAW with zero submissions, and a PLAYING state whose best distance was already zero. Both are impossible under the action handler. | Require at least one submission when a best result exists and require WON for an exact best result. | Both malformed-state regressions fail before the fix and pass after it. Valid old runs keep the same encoding and behavior. |

Expanded checks now include:

- All Countdown score boundaries (distances 1, 5, 6, 10, 11), keeping a previous
  best after a worse answer, rejecting extra/reused cards and invalid integer
  intermediates without changing the score, exact completion and terminal input
  idempotence.
- All 24 card-position permutations of the 3,3,8,8 rational solution, extra-card
  and omitted-card attacks, 2,000 character-bijection feedback cases, and a true
  equivalent Equation Guess expression that correctly does not win.
- Every single-position wrong digit for all 90 Number Mind packs; a wrong next
  value for all 90 Sequence packs; an independent sieve comparison for primality
  on 0..10,000; a large square factorization; and `.`/`=` grammar rejection where
  those inputs are not supported. Only Equation Guess accepts `=`.
- Actual app handlers for all ten retained games: draft/edit, RULES with paused
  game time, read-only records, EXIT, another game, MENU/OFF hooks, cold resume,
  and byte-identical restored game state. These are simulated hooks, not real
  calculator OS transitions.

Executed evidence before the separate P2 renderer change:

- Baseline: all 9 CTest suites PASS in 6.54s; embedded JSON/native comparison
  MATCH. Independent content audit: 7,711 checks PASS in 13.626s. The 900 original
  pack records and their hashes are unchanged.
- Expanded `test_guesscalc`: UBSan PASS, 3,380,525 assertions including 3,105,347
  renderer rectangles. All 9 CTest suites PASS in 5.98s.
- Full installed-SDK SH build and all 15 package checks PASS. Isolated audit
  binary: 633,500 bytes, SHA256
  `d62c142d4ba1a85e873a8fd9c5522619c078949aa68321036c9dc576d513acb3`.
  This is an intermediate worktree binary, not the final integrated deliverable.
- ASan+UBSan was retried with an 8-second bound. It produced no test output and
  timed out before the test report; the process was terminated. **ASan remains
  UNVERIFIED, not PASS.** No SDK or sanitizer runtime was installed or upgraded.

No new normal-play rule or pack-validity defect was found. Device MENU, power,
clock, descriptor and timer behavior belong to the root P0/P1 audit and remain
subject to its findings and hardware tests.

## 2026-09-22 arithmetic renderer integration (P2)

The owned renderer adapters use root's actual shared API from `d4b391a`
(`ng_expression`, `ng_small_expression`, `ng_input_expression`). The shared
header/implementation were copied verbatim as local build prerequisites and
are excluded from the worker's commits. Temporary layout stubs used during
development are not part of the verification results below.

- Equation Guess, Make Target, Countdown and Prime Factor input boxes use the
  common expression input. Its measured-width clipping and cursor placement are
  preserved; the former extra 52-character truncation is removed.
- Missing Operators draws the card background and its colored glyph once.
  Cross Math arithmetic clues, Sequence answer calculations, operator legends
  and the Prime Factor example use the same arithmetic token renderer. `+` is
  magenta, `-` orange, `*` green and `/` cyan. Numbers and `=` retain their base
  color. Prose/family labels use their existing text path.
- Equation Guess history tiles retain the positional green/yellow/grey
  feedback, including their original operator foreground colors. The entry
  hint and RULES state `SHIFT+DOT` enters `=`. Input grammar is unchanged.
- Common footer/message rendering is root-owned; it is outside this adapter
  change. The captures show the shared footer state at the tested prerequisite.

Executed verification with the production shared API:

- Pixel and per-pixel write-count comparisons cover all four operator-card
  glyphs, all four expression input paths, a 96-character input, an Equation
  feedback operator tile, and every horizontal/vertical Cross Math operator.
  `test_guesscalc`: UBSan PASS, **3,495,097 assertions**, including **3,130,547
  renderer rectangle checks**. These totals include pixel comparisons, not
  that many independent puzzle cases.
- All **9 CTest suites PASS in 6.80s**, C11 `-Wall -Wextra -Werror` with UBSan
  recovery disabled. The full installed-SDK SH build and **15 container checks
  PASS**. Intermediate worktree binary: **634,496 bytes**, SHA256
  `ffff7013805bb52fb2aaff853efe07bb816f99f554f83fbf8e0b5a33095c2fd3`.
  The final integrated package is root's deliverable, not this intermediate.
- `tools/generate/guesscalc_capture.c` generated eight deterministic fixtures
  through the production renderer: Equation entry/feedback, solved Sequence,
  rational Target, Countdown distance 10, all four Missing Operators colors,
  Cross Math and Prime Factor. All eight were visually reviewed at 396×224;
  no overlap or clipping was observed. They are host fixtures, not hardware
  photographs or claims of normal-play completion.

Reproduce the fixture executable after the host build:

```sh
clang -std=c11 -Wall -Wextra -Werror -fsanitize=undefined \
  -fno-sanitize-recover=all -Iinclude -Isrc/games \
  tools/generate/guesscalc_capture.c build-host/libnumgame_core.a \
  -o build-host/guesscalc_capture
mkdir -p build-host/guesscalc-captures
build-host/guesscalc_capture build-host/guesscalc-captures
```

ASan remains unverified for the host-runtime reason recorded above. Actual LCD
color/contrast, SHIFT+DOT physical key dispatch, MENU and power behavior remain
**HARDWARE TEST REQUIRED**.

## P2 integration follow-up: entry routing and wrapped token context

The app workflow tests now follow the common digit-focus policy: a numeric
shortcut selects a row; EXE/F6 performs OPEN. All ten retained games verify a
cold entry initially focuses NEW, numeric selection does not mutate settings,
RNG or the saved draft, and OPEN on a setting row requests NEW confirmation.
Cancelling preserves the exact saved game. Only explicitly selecting RESUME
and then opening it restores play; both EXE and F6 are exercised.

The new rendering regression exposed **21 failures among 84 wrapped prose
cases** before root's common renderer fix: a line boundary could make a
prose hyphen/slash or scientific exponent sign look like an arithmetic token.
Root changed token classification to retain the original string and offset.
The owned regression covers 84 wrapped cases (six strings, seven widths, two
font sizes), 42 clipped-input cases, and two explicit arithmetic-color cases.
It compares both framebuffer pixels and per-pixel write counts, including
input borders and cursor positions. Prose examples include `Zero-match`,
`NORMAL / ASSISTED`, `expression / zero divisor`, `1e-3` and `2E+4`.

After the common fix, both owned executables were compiled directly against
the current root `src` and `include` with C11 `-Wall -Wextra -Werror`, UBSan,
and recovery disabled. The app workflow test **PASS**; `test_guesscalc`
**PASS: 5,372,594 assertions, 3,130,547 module renderer rectangle checks**.
Only the test object included root's current `tests/support.h`; compilation
outputs stayed in the isolated worktree. No common source was changed by this
worker. These are host test changes; final integrated native/package validation
and physical SHIFT+DOT dispatch remain root-owned.

A final token-classification review found that the prose-separator exception
also suppressed `*` between variables in Sequence's real `n*n` explanation.
Root limited that exception to prose `-` and `/`. Eighteen additional cases
now check `n*n`, `3n*n` and `2n+n` at three widths with both fonts, using
explicit expected glyph colors and per-pixel write counts. The context suite
now has **146 cases**. Direct current-root linkage with the same strict UBSan
flags **PASS: 5,764,868 assertions, 3,130,547 module renderer rectangle checks**.
