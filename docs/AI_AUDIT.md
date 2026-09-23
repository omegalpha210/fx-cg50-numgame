# CPU audit

All five strategy games have human-first CPU, CPU-first and LOCAL 2P modes.
Easy samples legal moves. Normal selects the exact policy with probability3/4
and random policy with probability1/4. Hard selects an exact bounded policy.
Each preset uses the same legal-action validator for humans and all CPU levels.
Difficulty also sets starting ranges/presets; Race changes target/allowed increments.
A losing position is
not described as guaranteed to win; the UI does not claim unverified perfection.

| Game | Supported exact domain | Independent exhaustive comparison |
|---|---|---:|
| Nim | Normal play, 3/4 piles each0–31; nim sum | 1,048,575 nonterminal4-pile states vs move-DAG outcomes |
| Wythoff | Both piles0–40, single/equal removal; integer DP | 1,680 nonterminal ordered pairs |
| Euclid | Positive sorted1–99, legal integer multiplier, zero wins | 4,950 unordered pairs |
| Make Fifteen | Nine cards, all disjoint ownership assignments; minimax | 11,093 nonterminal states, including draw and4th-card triples |
| Race to Target | 21/{1,2,3},31/{1,2,3},23/{1,2,3,4} | 75 nonterminal totals |

Every audited Hard choice is legal and has the exact game-theoretic outcome
when a winning/drawing move exists. AI state tables are generated independently
from runtime move selection. Make Fifteen uses an independent Lo Shu reference
in tests against native arithmetic triples. Wythoff does not depend on floating
golden-ratio rounding. Additional tests complete900 seed×mode×difficulty games.

Immutable tables total **6,382 bytes** (Wythoff211, Euclid1,250, Make Fifteen4,921).
Nim/Race use bounded arithmetic/local DP. Runtime work is a bounded legal-move
scan, with no unbounded native game-tree search or heap allocation. The largest
supported human/CPU input amount is bounded by its game's legal-action validator.

The human action sets `cpu_pending`; the main event loop draws CPU TURN and
allows OS keys before one CPU event. The CPU uses the same apply function as
human/local play. Checkpoint/cold load preserves turn, pending flag and RNG.
Undo restores the pre-human state for a full human+CPU round and marks assistance.

Run `build-host/test_strategyquick` after `bash tools/test.sh`. See
[detailed audit](STRATEGY_QUICK_AUDIT.md) for all seeds and quick-game tests.
Real SH response latency and input responsiveness on a physical device remain
**HARDWARE TEST REQUIRED**; no host timing is substituted for device timing.

## Runtime integration follow-up

The exact policy tables and all retained21–28 engine sources were left unchanged
in this round after rerunning the above domains,900 full strategy games,2048
merge/no-op/RNG/bounds checks,1200 Sliding and1200 Lights seeds plus65535 press
sets. The public app matrix is now60 cases; former29/30 remain9 separately
labelled legacy engine/codec cases. CPU pending survives the common checkpoint
and is consumed once after resume; tests retain whole-round UNDO semantics.
New Shikaku/Slitherlink have no CPU. MASTER applies only to four grid puzzles,
not to new unverified CPU policies. See validation/runtime-content.txt.
