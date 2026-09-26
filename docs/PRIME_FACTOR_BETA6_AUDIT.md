# Prime Factor revision-5 content audit (beta.6)

Prime Factor now has **896 distinct, prevalidated composite targets**. The
entry's level selects 128 three-digit EASY targets, or 256 targets in each
other level. NORMAL, HARD and MASTER each split exactly 128/128 across their
two permitted adjacent digit lengths. This is a fixed finite target table,
not unbounded rejection sampling. It adds about 3.5 KiB of native `uint32_t`
target data; the detailed host JSON is audit material and is not linked into
the calculator.

## Exact classification

`Ω` is the number of prime factors *with multiplicity*. `max p` is the largest
prime factor. `large distinct` counts different prime factors at least 11.
Every candidate must have at least two prime factors, every prime factor at
most 97 and an exact product in the stated digit range. The following tests
apply to the factorization itself, beyond the number of digits:

| Group | Level and digits | Required factor structure | Eligible mathematical domain | Selected |
|---|---|---|---:|---:|
| E3 | EASY, 100–999 | max p ≤ 11, Ω ≥ 3 | 136 | 128 |
| N3 | NORMAL, 100–999 | 17 ≤ max p ≤ 31, Ω ≥ 3 | 139 | 128 |
| N4 | NORMAL, 1,000–9,999 | 11 ≤ max p ≤ 19, Ω ≥ 4 | 605 | 128 |
| H4 | HARD, 1,000–9,999 | 23 ≤ max p ≤ 97, Ω ≥ 4 | 1,380 | 128 |
| H5 | HARD, 10,000–99,999 | 11 ≤ max p ≤ 31, Ω ≥ 5 | 3,486 | 128 |
| M5 | MASTER, 10,000–99,999 | 37 ≤ max p ≤ 97, Ω ≥ 4, large distinct ≥ 2 | 6,385 | 128 |
| M6 | MASTER, 100,000–999,999 | 37 ≤ max p ≤ 97, 5 ≤ Ω ≤ 12, large distinct ≥ 2 | 31,374 | 128 |

The threshold gaps make same-digit assignments disjoint: EASY's three-digit
targets end at max p 11 before NORMAL begins at 17; NORMAL four-digit max p
19 is below HARD's minimum 23; HARD five-digit max p 31 is below MASTER's
minimum 37. The 896 selected target values also have zero duplicate products
across levels. The two-digit-band policy deliberately lets a longer but
factor-friendly number appear in the lower of two adjacent levels. Thus a
HARD five-digit target can have smaller prime factors than a HARD four-digit
target. This is a structural design criterion, **not** a measured human
difficulty rating.

All 95 E3 candidates whose maximum prime is at most 7 were selected first;
33 of the 41 additional max-prime-11 candidates fill the 128 slots. For each
other group, a deterministic round-robin selects among maximum-prime classes;
within a class it uses a fixed SplitMix64 ordering, with no retries or runtime
candidate rejection. The table is sorted within each 128-target group for
compact native lookup and membership validation. The host generator verifies
the full eligible domain before writing the table. A generated target is
chosen from a finite index; there is no fallback to an out-of-band number.

## Measured selected distribution

The following figures describe the **selected** table, not the much larger
mathematical eligible domains. `Repeat` gives counts by number of prime bases
whose exponent is greater than one. `Trial proxy` counts modulo operations in
a naive ascending-divisor factorization; it approximates computer search
work and is not a claim about a player's effort.

| Group | Target min–max | max p min/median/max | Ω min/median/max | Repeat 0/1/2/3 | Trial proxy min/median/max |
|---|---:|---:|---:|---:|---:|
| E3 | 100–980 | 2/7/11 | 3/5/9 | 8/82/37/1 | 4/7/13 |
| N3 | 102–992 | 17/23/31 | 3/4/6 | 54/71/3/0 | 5/7/19 |
| N4 | 1,020–9,900 | 11/15/19 | 4/5/9 | 11/79/36/2 | 6/11/22 |
| H4 | 1,060–9,964 | 23/56/97 | 4/5/8 | 20/96/12/0 | 7/11/48 |
| H5 | 10,304–99,484 | 11/19/31 | 5/6/12 | 7/55/62/4 | 9/15/33 |
| M5 | 10,370–99,932 | 37/61/97 | 4/5/8 | 45/76/7/0 | 12/25/85 |
| M6 | 105,938–998,260 | 37/61/97 | 5/6/12 | 17/76/34/1 | 13/28/87 |

By level, the digit histograms are 3-digit 128; 3/4-digit 128/128;
4/5-digit 128/128; and 5/6-digit 128/128. Median largest prime by level is
7, 19, 31 and 61 respectively. The maximum `Ω` is 12; the longest compact
`p^exponent` answer among all table values is 17 input characters, below the
existing 96-character limit. The largest target, 998,260, fits the 1,000,000
limit of the existing exact factorization parser. Host generation checks
`product(factors) == target` using unbounded integers; the native parser
checks each multiply against the target before accepting it, avoiding an
overflowing product. No target is 1 or prime.

After removing every factor 2, 3, 5 and 7, the remaining prime-factor count
(with multiplicity) has these distributions. The JSON also stores the exact
remaining product and factors for every target, along with exponents of all
repeated prime bases.

| Group | Remaining prime factors: count → target count |
|---|---|
| E3 | 0→95, 1→28, 2→5 |
| N3 | 1→103, 2→25 |
| N4 | 1→66, 2→54, 3→8 |
| H4 | 1→82, 2→45, 3→1 |
| H5 | 1→44, 2→49, 3→35 |
| M5 | 2→97, 3→31 |
| M6 | 2→53, 3→66, 4→9 |

## Runtime and save contract

The native API is `ng_prime_beta6_count`, `ng_prime_beta6_target` and
`ng_prime_beta6_valid` in
[`src/games/prime_beta6.c`](../src/games/prime_beta6.c). `ng_prime_beta6_target`
does not consume RNG or loop through candidates. New revision-5 games use
the common bank supply shuffle with counts 128/256/256/256. That supply
visits each table index once per level cycle, and its cycle-boundary rule
should avoid repeating the just-completed target. A saved run must retain its
actual target, index and supply position; RESUME must not consume a new index.
The existing revision-2 and revision-4 runtime recipes and validators remain
for older saves. The common supply/save regression is covered by the host
application tests; actual flash and cold-start behavior needs device testing.

## Reproduction and independent checks

Run `python3 tools/generate/guesscalc_prime_beta6.py --check` to re-enumerate
the 100–999,999 mathematical domain and byte-compare generated JSON/header.
Run `python3 tests/test_guesscalc_prime_beta6.py` for an independent trial
factorizer, primality/product/threshold checks on all 896 values, cross-level
disjointness, compiled strict-C native table parity and all 896 answer strings
through the actual `gc_factorization` parser. The independent check does not
import the generator's factorization or selection functions. Human play
difficulty, LCD readability and performance on a physical fx-CG50 remain
**HARDWARE TEST REQUIRED**.
