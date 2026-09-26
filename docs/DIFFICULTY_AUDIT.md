# Difficulty audit — 36 visible games, beta.6

The [current 150-row CSV](DIFFICULTY_AUDIT.csv) covers every visible game and
EASY/NORMAL/HARD/MASTER level, plus HELL in the six LOGIC games. Its 19 columns
include measured metrics, `before`, `after`, finding ID and decision.
`before` for the four beta.6 changes is the beta.5 state; other unchanged
rows retain their earlier audit baseline. The [beta.3](DIFFICULTY_AUDIT_BETA3.csv)
and [beta.4](DIFFICULTY_AUDIT_BETA4.csv) CSVs and Git history preserve earlier
snapshots. The beta.6 measures below come from the published
[Make Target](../assets/guesscalc_target_beta6.json),
[Countdown](../assets/guesscalc_countdown_beta6.json), and
[Prime Factor](../assets/guesscalc_prime_beta6.json) records; the family
audits ([cards](MAKE_TARGET_COUNTDOWN_BETA6_AUDIT.md),
[factors](PRIME_FACTOR_BETA6_AUDIT.md),
[Baseball](BASEBALL_BETA6_AUDIT.md)) supply independent rule verification.
The integrated generator
checks exactly 36 game IDs, 150 rows and each game's expected level set.

## Beta.6 content and rule measurements

| Game | Current per level | Measured selection/difficulty fact |
|---|---:|---|
| Number Baseball | runtime secret | Total attempts 20/30/40/50, including the warned final chance. 4/5/6/7 digits and repeat/leading-zero rules are unchanged. |
| Make Target | 1,000 | Five target pools of 200 each; RANDOM reuses their union. Each level has 1,000 distinct target/card multisets, so 4,000 active bases overall. Cards remain 1–999 and counts are 4/4/5/6. The exact-minimum source reports zero fraction-required EASY/NORMAL/HARD and 1,000 of 1,000 MASTER. |
| Countdown | 200 | Distinct target/card multisets in every level. Minimum exact card-use histograms are EASY 2:13, 3:80, 4:87, 5:20; NORMAL 2:7, 3:67, 4:120, 5:6; HARD 4:127, 5:73; MASTER 6:200. All 200 MASTER records require division; ten HARD records do. The beta.6 source does not report canonical solution counts, so that measure is explicitly unremeasured in the CSV. |
| Prime Factor | 128 / 256 / 256 / 256 | EASY is 128 three-digit composites. The other levels have exactly 128 targets in each neighboring digit band: NORMAL 3/4, HARD 4/5, MASTER 5/6. All are composite, with maximum prime at most 97 and disjoint target identities across levels. |

Make Target's public bases are not all different *reasoning templates*. Among
the 1,000 records per level, distinct easiest-expression templates are
22/6/60/127; the most common template occurs 284/385/644/678 times.
Especially in MASTER, numeric/card diversity exceeds structural diversity.
These counts are reported so the larger bank is not mistaken for 4,000
unrelated solution ideas. The exact score, card-digit distributions and
canonical solution-count ranges are in the CSV. The beta.6 generator and
independent verifier reports describe how those minima were checked; the
CSV itself does not rerun an exhaustive solver.

Prime Factor's maximum-prime and trial-division-proxy distributions overlap
between some neighboring groups; the level definition deliberately combines
digit length with factor structure. The proxy is not measured human solving
time. Earlier saved runs retain the prior revision and are separate from
the current bank figures. The [36-game inventory](CONTENT_INVENTORY.md)
shows current-setting reachability versus archived records.

## Historical beta.4/beta.5 findings

## Resolved content findings

| Finding | Before beta.4 | New games in beta.4 | Objective decision |
|---|---|---|---|
| GC-D01 COUNTDOWN HARD | Of 30 starts, minimum exact cards were 1:2, 2:2, 3:11, 4:12, 5:3. The minimum/median/maximum were 1/3.5/5. | Thirty revision-4 HARD starts: 4:19, 5:11, or 4/4/5. No 1–3-card exact shortcut remains. | RESOLVED / CONTENT REVISED |
| GC-D02 SEQUENCE EASY/NORMAL | EASY: 25 alternating, 5 arithmetic. NORMAL: 25 alternating, 2 quadratic, 3 Fibonacci-like. | EASY: 12 arithmetic, 9 geometric, 9 simple shared-step alternating. NORMAL: 9 quadratic, 9 Fibonacci-like, 8 unequal/interleaved, 4 offset/geometric. | RESOLVED / BANK REBALANCED |
| GC-D03 PRIME FACTOR MASTER | The old 90-target mathematical domain had maximum prime 7–13, Ω=6, and targets 1,260–715,715. Its prime domain overlapped HARD. | The new 140-target shape has four distinct primes, Ω=7–8, two repeated prime factors, two primes ≥11, maximum prime 37–73 and targets 43,956–488,808. HARD maximum prime is 31; exact target overlap is zero. | RESOLVED / MASTER REVISED |
| Sliding E/N/H | Old sampled 3×3 exact shortest distances overlapped: EASY 10–28, NORMAL 12–28, HARD 14–28. | Both sizes have 128 bank starts per level with independently certified shortest distances 8–14 / 15–20 / 21–26. | RESOLVED / EXACT DISTANCE BANDS |
| GC-D05 MAKE TARGET | A difficult construction witness could conceal a much simpler legal answer. In a fixed 96-record-per-level old sample, division-required counts were 0/46/30/12 and fractional-required counts were 0/0/0/0. | Each level has 2,000 exhaustively graded records, two for each target 1–1000. Division-required counts are 0/2000/2000/2000 and fractional-required counts 0/0/0/2000. | RESOLVED / EXHAUSTIVE COMPLEXITY GRADING |

COUNTDOWN's exact subset solver enforces positive integer intermediates and
exact division. HARD's canonical exact-solution count changed from
17/555/6,858 to 16/168/2,064 (minimum/median/maximum). EASY and NORMAL banks
remain unchanged; MASTER still has all 30 starts requiring six cards and
division. The original HARD IDs and their saved progress retain their old
rules. [Full distributions](../assets/guesscalc_difficulty_audit_beta4.json)
include minimum operation count and division/subtraction metrics.

SEQUENCE's 60 new EASY/NORMAL records use six visible terms. Within the stated
grammar, there were zero competing-next-term ambiguity rejects; exact
EASY/NORMAL prefix overlap is zero. One cross-level affine-equivalent shape
remains documented, so these are family-distribution findings, not a claim
that every sequence is unique under every mathematical transformation.
HARD/MASTER banks are unchanged.

PRIME FACTOR MASTER uses `2^(2 or 3) × 3^3 × p × q`, where `p` is one of ten
primes 37–73 and `q` is one of seven primes 11–31. The largest target remains
below one million and was checked against the renderer and factor validator.
The old MASTER range overlaps the new one numerically; the separation claimed
here is in factor structure and prime domain, not monotonic target magnitude.
The 4,096-seed native sample and the exact 140-target mathematical domain are
reported separately in [GUESS/CALC](GUESS_CALC_AUDIT.md).

Sliding 3×3 has 181,440 reachable states and diameter 31. Its retained MASTER
bank contains two distinct distance-31 boards that are one reflection class.
New 3×3 and 4×4 E/N/H histograms per size are EASY 8,9:19 each and 10–14:18
each; NORMAL 15,16:22 each and 17–20:21 each; HARD 21,22:22 each and
23–26:21 each. Old 4×4 E/N/H optima were not exhaustively measured, so their
old Manhattan bounds are not described as exact distances. The retained
4×4 MASTER 30-board bank has Manhattan lower bounds 48–52, proving it lies
above the new HARD maximum 26; its exact optimum has not been computed.
New boards have zero exact or goal-preserving reflection duplicates. See the
[independent proof report](../assets/strategyquick/sliding-beta4-audit.json).

## MAKE TARGET exact grading

The actual rules require every card exactly once, 4/4/5/6 cards by level,
binary `+ - * /`, parentheses, rational/negative/zero intermediate values and
optional unary minus. Concatenation, powers and factorial are unavailable.
The native parser bounds reduced numerator and denominator at 1,000,000,000;
the host solver applies that same legal bound. The host-only C++17 solver uses
exact signed rational arithmetic and subset dynamic programming over card
occurrences. It combines every disjoint subset in both noncommutative
directions, rejects division by zero and canonicalizes commutative reversal,
equal-card relabeling and redundant double unary negation. It preserves a
lexicographically easiest raw tuple, a separate unary-aware graded expression
and the canonical exact-solution count. There is no arbitrary magnitude cutoff,
random pruning or production-device search table. Each retained beta.4 and
beta.5 bank stores 8,000 `uint16` candidate indices (16,000 bytes per
revision). Beta.6 instead stores its 4,000 compact card records and bounded
answer strings in flash; all large exact-search reports remain host-only.
The [beta.4 CSV](MAKE_TARGET_COMPLEXITY.csv) and
[current beta.5 CSV](MAKE_TARGET_COMPLEXITY_BETA5.csv) retain all per-record
expressions and scores.

The raw easiest tuple is `(fractional steps, division steps,
noncommutative steps, tree depth, maximum denominator)`. For selection, the
graded expression minimizes `(fractional, division, noncommutative + unary,
negative, depth, denominator, expression)` and receives
`graded = 65536*fractional + 8192*division + 256*(noncommutative+unary) +
16*negative + depth`. The published scalar is `32*graded + rarity`, where
`rarity = max(0, 16 - floor(log2(canonical solution count)))`. Rarity can
break nearby ties but cannot override a structural grade. Both objectives and
expressions remain in host JSON; CSV retains the raw tuple and graded
expression/features.

| Level | Acceptance on the exact minimum | Beta.4 score min–max | Records |
|---|---|---:|---:|
| EASY | No division or fractional intermediate; graded score ≤259 | 69–8,296 | 2,000 |
| NORMAL | Graded score exactly 8,450; exact integer division | 270,404–270,410 | 2,000 |
| HARD | Five cards, no fractional intermediate, one division; graded score 8,451–8,715 | 270,434–278,632 | 2,000 |
| MASTER | Six cards; every legal exact solution needs at least one fractional intermediate | 4,472,963–5,014,693 | 2,000 |

The table and target-24 examples below describe the retained beta.4 bank,
which remains playable through saved games. Beta.5 regenerates only new Make
Target games, keeps two decks per target/level and enforces every card in
1..999. It does not change the exact solver, parser or save format.

For example, target 24's beta.4 graded answers include EASY
`((3*4)*(9-7))` from cards 3,9,7,4; NORMAL
`((1431+777)/(6+86))`; HARD `(((22*886)+644)/(43+796))`; and MASTER
`(240/((828/((30024/387)-84))+139))`. The old MASTER sample had a
construction witness `(28*8-14)/9+2/3` but its easiest graded alternative
`(((14*3)+(2*9))-(28+8))` uses no fractional intermediate. This is why the
new bank is selected by exhaustive minimum rather than construction witness.

For beta.4, the solver was cross-checked with an independent full expression-tree
enumerator on eight fixed four-card sets, including repeated cards and
`3,3,8,8 → 24`: 2,614 exact rational values and 1,698,872 canonical trees
agreed on reachability, count, raw and graded minima. An additional 20
EASY/NORMAL bank samples across targets 1, 2, 10, 24, 99, 250, 500, 777,
999 and 1000 compared 7,627,392 canonical trees and 24,247 rational-value
groups. The entire new bank was separately re-solved. All 8,000 native
payloads and 24,000 witness/easiest-expression parser checks were compared
with host data.
These are exact arithmetic and structural checks, not measured human effort.

## Accepted findings and compatibility

GC-D04's beta.4/beta.5 Baseball allowances have been superseded by the
20/30/40/50 total-attempt rule. EQUATION GUESS MASTER still has 12 attempts
versus 12/10/8 at lower levels; its attempt rules did not change here.
GC-D06 remains **ACCEPTED / OVERLAP DOCUMENTED**: CRYPTARITHM's independent
column-search node ranges overlap, but level medians and structure rise; its
unique-solution bank is unchanged.

Fresh Make Target uses revision 6; Countdown and Prime Factor use revision 5;
Number Baseball uses revision 4. Sequence and Sliding remain revision 4.
Old unfinished runs preserve their puzzle IDs, board/progress, rule revision
and INIT behavior. The active single-resume v5 wire layout did not change.
Fixed Make Target uses a 200-record no-repeat cycle; RANDOM uses the shared
1,000-record union. The other games' content was not regraded for beta.6.
The fixed 2048 CLASSIC level remains hidden; LOGIC HELL remains its own
source bank. No human solving-time calibration was made.

Reproduce the independent content checks with `bash tools/verify_content.sh`;
host regression and package checks are listed in [ACCEPTANCE.md](ACCEPTANCE.md).
Physical LCD readability, flash timing, dim/APO and the reported MENU flashing
remain **HARDWARE TEST REQUIRED**.
