# Make Target and Countdown content — beta.6

New Make Target games select 10, 24, 50, 100, 200, or RANDOM. Each fixed
target × difficulty has 200 different **target + card-multiset** base puzzles.
RANDOM selects from the same five fixed pools within the chosen difficulty:
1,000 reachable puzzles, with no extra copied RANDOM records. The total new
Make Target bank is 4,000. Earlier 1–1000 arbitrary-target revisions remain
for unfinished saved games; their records are excluded from the new count.

Countdown has 200 base puzzles at each difficulty, 800 in all. Thirty old
problems per difficulty were included once within the new 200 and 170 new
ones were added. Each new Countdown problem is distinct by target plus six
card values with multiplicity, regardless of display order.

## Make Target measured cells

| Level | Target | Base puzzles | Easiest-expression templates | Largest template | Pair product/quotient proxy | Score min–median–max |
|---|---:|---:|---:|---:|---:|---|
| EASY | 10 | 200 | 11 | 66/200 | 6/200 | 103–8,293–8,296 |
| EASY | 24 | 200 | 13 | 58/200 | 6/200 | 70–8,293–8,297 |
| EASY | 50 | 200 | 16 | 59/200 | 2/200 | 68–8,293–8,297 |
| EASY | 100 | 200 | 16 | 54/200 | 3/200 | 68–8,266–8,297 |
| EASY | 200 | 200 | 13 | 62/200 | 1/200 | 71–8,265–8,297 |
| NORMAL | 10 | 200 | 4 | 81/200 | 31/200 | 270,404–270,407–270,410 |
| NORMAL | 24 | 200 | 5 | 77/200 | 29/200 | 270,404–270,408–270,410 |
| NORMAL | 50 | 200 | 5 | 80/200 | 26/200 | 270,404–270,408–270,410 |
| NORMAL | 100 | 200 | 6 | 75/200 | 18/200 | 270,404–270,408–270,410 |
| NORMAL | 200 | 200 | 5 | 72/200 | 15/200 | 270,404–270,408–270,410 |
| HARD | 10 | 200 | 27 | 150/200 | 1/200 | 270,432–278,631–278,632 |
| HARD | 24 | 200 | 24 | 154/200 | 4/200 | 270,432–278,631–278,632 |
| HARD | 50 | 200 | 25 | 142/200 | 2/200 | 270,433–278,631–278,632 |
| HARD | 100 | 200 | 18 | 106/200 | 6/200 | 270,434–278,631–278,632 |
| HARD | 200 | 200 | 14 | 92/200 | 2/200 | 270,434–278,631–278,632 |
| MASTER | 10 | 200 | 42 | 133/200 | 10/200 | 4,472,993–4,744,356–4,744,356 |
| MASTER | 24 | 200 | 46 | 123/200 | 1/200 | 4,472,992–4,744,356–4,744,356 |
| MASTER | 50 | 200 | 36 | 131/200 | 0/200 | 4,472,963–4,744,356–4,744,356 |
| MASTER | 100 | 200 | 27 | 144/200 | 0/200 | 4,472,993–4,744,356–4,744,356 |
| MASTER | 200 | 200 | 15 | 147/200 | 1/200 | 4,474,019–4,744,356–4,744,356 |

The MASTER second pass replaced 40 of the dominant-template
records per target with a different rational construction. Every
replacement passed the same full exact minimum grade. The largest
template share changed as follows:

| MASTER target | Before | After |
|---:|---:|---:|
| 10 | 173/200 | 133/200 |
| 24 | 163/200 | 123/200 |
| 50 | 171/200 | 131/200 |
| 100 | 184/200 | 144/200 |
| 200 | 187/200 | 147/200 |

The pair proxy counts decks containing two cards whose product or
exact quotient alone equals the target; it is **not** a shortcut under
Make Target rules, which require all cards exactly once. Template counts
replace leaf numbers with `n` while keeping operators and tree shape.
Different cards can therefore have the same easiest-solution pattern.
The full JSON report also includes operator multiset, card digit
distribution, value quantiles, answer length, and normalized exact
solution-count quantiles for every cell.

To regenerate the selected bank, run
`python3 tools/generate/guesscalc_beta6_target.py --generate` followed by
`python3 tools/generate/guesscalc_beta6_refine.py --write`. The second
pass replaces only MASTER records after exact grading and is itself
deterministic. Its `--check` mode verifies each alternate construction
index in the published bank.

## Make Target exact difficulty and readability

| Level | Records / RANDOM pool | Card digits 1/2/3 | Card min / median / p90 / max | Exact score min / median / max | Division / fraction required |
|---|---:|---|---|---|---|
| EASY | 1,000 | 900/2,488/612 | 1 / 33 / 119 / 999 | 68 / 8,293 / 8,297 | 0 / 0 |
| NORMAL | 1,000 | 1,695/1,305/1,000 | 1 / 12 / 440 / 998 | 270,404 / 270,408 / 270,410 | 1,000 / 0 |
| HARD | 1,000 | 742/2,426/1,832 | 1 / 36 / 721 / 998 | 270,432 / 278,631 / 278,632 | 1,000 / 0 |
| MASTER | 1,000 | 90/3,347/2,563 | 2 / 64 / 708 / 999 | 4,472,963 / 4,744,356 / 4,744,356 | 1,000 / 1,000 |

All Make Target cards are 1–999. The unchanged exhaustive rational
solver evaluates every legal full-card expression and assigns the
minimum complexity across *all* solutions, rather than accepting a
hard-looking construction witness. EASY has four cards, no required
division/fractions and graded score ≤259; NORMAL has four cards and
graded score 8450; HARD has five, one required division, no fractional
intermediate and graded score 8451–8715. MASTER has six cards, every
legal exact solution needs a fractional intermediate, and the published
structural-plus-rarity score remains ≥4,472,963. These are exact solver
metrics, not measured human solving times.

The solver retains all rational values for every card multiset. For a
given subset and rational value, it keeps Pareto features on tree depth
and maximum denominator after first minimizing the additive structural
cost tuple. Every parent operation adds the same value-dependent cost to
either child candidate; parent depth and denominator are monotone maxima.
A discarded dominated child therefore cannot become the easiest parent.
Canonical solution counts are aggregated separately from witness choice.
The bound is the native parser’s reduced numerator/denominator ≤10^9;
there is no content-search timeout or arbitrary rational cutoff.

## Countdown exact minimum cards

| Level | Base puzzles | Min-card distribution | Division required | Distinct six-card decks |
|---|---:|---|---:|---:|
| EASY | 200 | 2 cards: 13, 3 cards: 80, 4 cards: 87, 5 cards: 20 | 0 | 189 |
| NORMAL | 200 | 2 cards: 7, 3 cards: 67, 4 cards: 120, 5 cards: 6 | 0 | 189 |
| HARD | 200 | 4 cards: 127, 5 cards: 73 | 10 | 175 |
| MASTER | 200 | 6 cards: 200 | 200 | 168 |

Countdown uses six cards drawn from two copies each of 1–10 and
distinct 25/50/75/100 large cards: 1/2/3/3 large cards for
EASY/NORMAL/HARD/MASTER. The target remains 100–999. The exact subset
dynamic program checks every card occurrence, positive integer
intermediates and only exact division. HARD has no 1–3-card exact
shortcut; MASTER needs six cards and has no legal exact expression
without division. The old EASY/NORMAL minimum-card mix is preserved
proportionally, without changing the game rule.

## Verification scope and limits

The reproducible [machine audit](../assets/guesscalc_beta6_content_audit.json)
summarizes [4,000 Make Target records](../assets/guesscalc_target_beta6.json)
and [800 Countdown records](../assets/guesscalc_countdown_beta6.json).
The target generator verifies construction witnesses and displayed
answers against card occurrences, 1–999 bounds, duplicate signatures,
all acceptance thresholds and answer width. Its `--verify` mode re-solves
all 4,000 with the C++ exact rational DP. The Countdown generator checks
all 800 with an independent Python positive-integer subset DP, then its
`--verify` mode rechecks shortest-card reachability with the unchanged
C++ integer solver. The [native payload audit](../assets/guesscalc_beta6_native_audit.json)
compares all 4,800 C-header records to JSON and passes every embedded
answer through the production `gc_expression` and `gc_cards` functions.
Ten EASY/NORMAL records (one per fixed target and level) received a
separate expression-tree cross-check: 13,221 exact rational-value groups
and 3,944,640 canonical trees agreed on reachability, normalized count,
raw minimum and graded minimum. The independent tree check is a sample,
not an exhaustive 4,000-record verification.
Native/device timing and perceived variety require hardware/player testing.
The previous MENU flicker report remains HARDWARE TEST REQUIRED.
