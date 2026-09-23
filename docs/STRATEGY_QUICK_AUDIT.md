# Strategy and quick game audit

Current visible IDs are 21–28. IDs 29/30 remain implemented only to decode and
resume legacy engine state through migration tooling; they are not visible menu
entries. The new MASTER content is original, generated on the host, and verified
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

LOCAL 2P shares the YOU FIRST tactical bank; it is not another set of originals.
CPU FIRST starts are losing for the CPU to move, so YOU can force a win; YOU
FIRST and LOCAL starts are winning for YOU/P1. All 257 strategy starts were
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
common registry, application, renderer and codec. In the isolated worker build,
only unselected GC/grid bank-count symbols were temporary link stubs; these
must not enter the integrated source. Root runs the complete combined build.

| Check | Verified coverage |
|---|---|
|Nim|1,048,575 nonterminal four-pile states in 0..31 against an independent move-DAG, including every selected exact move|
|Wythoff|1,680 nonterminal pairs in 0..40, row-major DP versus generator increasing-sum DP|
|Euclid|4,950 positive unordered pairs through 99, independent increasing-sum successor DAG|
|Make Fifteen|11,093 nonterminal disjoint ownership states; independent Lo Shu lines versus arithmetic triples; winning fourth-card triple and draw|
|Race|All 75 original preset states plus 441 MASTER-domain remaining-distance/add-limit states; independent DP versus exact residue policy|
|Original CPU lifecycle|900 complete seeded games over five games × three levels × three modes × 20 seeds|
|MASTER starts|499 mode-specific no-repeat loads; 407 strategy playthroughs using exact play end in YOU/P1 wins; INIT, codec/undo snapshots and invalid identity checks|
|2048|Merge once, no-op RNG, deterministic replay, exponent/score caps; 10,000 starting spawns yielded 9,016 twos and 984 fours|
|2048 TARGET|All four terminal goals, replay/undo, mode-separated records and codec; CLASSIC remains active after reaching 8192|
|Sliding|1,200 original seeded starts; independent inversion parity; MASTER full 181,440-state 3×3 BFS and all 30 4×4 legal-path witnesses/admissible Manhattan bounds|
|Lights|1,200 original starts and all 65,535 nonempty 4×4 press subsets; MASTER matrix RREF plus full affine nullspace versus generator row chasing certifies every minimum|
|Actual app|84 setting requests (81 distinct new configurations): NEW, INIT, legal input, CPU, round UNDO, explicit RESUME, EXIT/MENU and codec|
|Legacy 29/30|Nine mode/difficulty codec cases plus 600 Rush timed/practice checks and 300 full 20-round Memory runs|

The 499/407 test counts include LOCAL sharing and are not content counts.
The executable also rejects corrupted dimensions, cursor, phase, mode, CPU
flags, terminal state, MASTER IDs/metadata, 2048 goals, timer ranges, arithmetic
answers and unterminated inputs. Checkpoint hooks use production encoded bytes;
native BFile and OS switching are covered by root's native adapter tests, not
by these host input tests.

## Compatibility, controls and assistance

CLASSIC 2048 is still mode 0: data[3] is zero, 2048 achievement allows continued
play, and only lack of legal moves ends the run. Existing CLASSIC saves and
record buckets retain that meaning. TARGET is mode 1 and stores the goal
exponent in data[3]; reaching it wins. The four goals have separate difficulty
buckets, and mode separation prevents CLASSIC best scores being mixed with
TARGET. The app starts new CLASSIC runs in its fixed NORMAL bucket while retaining the selected TARGET level; old CLASSIC E/N/H runs resume unchanged. Exponents cap at 30 and scores saturate at UINT32_MAX.

E/N/H game initialization and existing puzzle identities are retained. MASTER
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
