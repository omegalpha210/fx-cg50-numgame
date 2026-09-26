# Make Target card readability — beta.4 to beta.5

This is the preserved beta.4→beta.5 comparison, not the current NEW pool.
Beta.6 uses revision 6 with targets 10/24/50/100/200/RANDOM and 200
distinct decks per fixed target and difficulty. See the
[current content audit](MAKE_TARGET_COUNTDOWN_BETA6_AUDIT.md) and
[current-setting inventory](CONTENT_INVENTORY.md). In beta.5, fresh games
used content revision 5. Every input card was 1–999; the target
could be 1–1000. The 8,000 records comprised two decks per target and
level: 2,000 each for EASY, NORMAL, HARD and MASTER. Earlier unfinished
revision-4 games keep their original cards, puzzle ID, progress and parser
rules. The v5 save wire format is unchanged.

The native application stores only an 8,000-entry `uint16` index table for
each of the retained beta.4 and new beta.5 banks. The host generator first
constructs candidates within the bound, exhaustively solves every legal
expression using exact rationals, applies the prior minimum-solution grade,
then selects the bank. It does not clamp large cards at runtime. Candidates
must obey the actual 96-character, nesting-12 and reduced-value ≤1e9 parser
limits. MASTER additionally requires every exact solution to use a fractional
intermediate; all 2,000 retained new MASTER scores meet or exceed the beta.4
minimum score 4,472,963. Structural grade remains primary; card size is not
used as a difficulty score. These are structural measures, not calibrated
human solving times.

Card-value quantiles count individual cards, not decks. The p90 and p95 use
the nearest-rank convention. Each row shows **beta.4 → beta.5**.

| Level | Decks | Card min / median / p90 / p95 / max |
|---|---:|---|
| EASY | 2,000 → 2,000 | 1 / 9 / 86 / 138 / 491 → 1 / 9 / 88 / 140 / 498 |
| NORMAL | 2,000 → 2,000 | 2 / 119.5 / 13,882 / 18,878 / 31,765 → 1 / 13 / 736 / 900 / 999 |
| HARD | 2,000 → 2,000 | 1 / 694.5 / 11,077 / 16,667 / 32,424 → 1 / 21 / 695 / 873 / 999 |
| MASTER | 2,000 → 2,000 | 9 / 498 / 14,340 / 19,551 / 32,748 → 1 / 115 / 792 / 904 / 999 |

| Level | 1 digit | 2 digits | 3 digits | 4+ digits |
|---|---:|---:|---:|---:|
| EASY | 4,215 → 4,217 | 3,131 → 3,104 | 654 → 679 | 0 → 0 |
| NORMAL | 1,501 → 3,326 | 2,433 → 2,181 | 576 → 2,493 | 3,490 → 0 |
| HARD | 14 → 3,295 | 4,090 → 3,744 | 1,367 → 2,961 | 4,529 → 0 |
| MASTER | 1 → 177 | 1,700 → 5,525 | 4,819 → 6,298 | 5,480 → 0 |

Across 38,000 card occurrences, four-or-more-digit values fell from 13,499
to **zero**. The old global maximum 32,748 fell to **999**. EASY was already
bounded and changed only slightly; the large improvement is in NORMAL,
HARD and MASTER.

| Level | Exact score min / median / max | Fraction-required decks | Division-required decks | Canonical solution-count min / median / max |
|---|---|---:|---:|---|
| EASY | 69 / 8,296 / 8,296 → 69 / 8,296 / 8,296 | 0 → 0 | 0 → 0 | 324 / 432 / 10,512 → 324 / 432 / 12,000 |
| NORMAL | 270,404 / 270,409 / 270,410 → 270,404 / 270,408 / 270,410 | 0 → 0 | 2,000 → 2,000 | 96 / 144 / 4,320 → 96 / 480 / 5,168 |
| HARD | 270,434 / 278,631 / 278,632 → 270,432 / 278,631 / 278,632 | 0 → 0 | 2,000 → 2,000 | 432 / 576 / 27,648 → 288 / 576 / 92,736 |
| MASTER | 4,472,963 / 5,014,693 / 5,014,693 → 4,472,963 / 4,744,356 / 9,208,997 | 2,000 → 2,000 | 2,000 → 2,000 | 2,304 / 2,304 / 52,992 → 2,304 / 4,608 / 419,712 |

The scalar score includes a small inverse-solution-count rarity term after
the exact structural grade. HARD's scalar minimum falls by two even though
its accepted structural-grade band and division/fraction requirements are
unchanged. MASTER's median score changes within its fractional-required
class; a higher scalar maximum is not a claim that every human player will
find it harder.

The reproducible [native parity and distribution report](../assets/guesscalc_target_beta5_audit.json)
compares both complete banks. The [beta.5 per-puzzle CSV](MAKE_TARGET_COMPLEXITY_BETA5.csv)
and [host JSON](../assets/guesscalc_target_beta5.json) retain exact tuples,
solution counts, witnesses and chosen indices. The generator's full-bank
re-solve and independent expression-tree samples are part of
`bash tools/verify_content.sh`. Native parity checks 8,000 generated decks
and 24,000 witness/easiest/graded expression parses.

The actual renderer's [six-card fixture sheet](captures/cards-three-digit-native.png)
contains 1, 9, 10, 99, 100 and 999. Cards have equal 55px widths and 7px
gaps in a centered six-card row; `999` is 46px wide at the displayed scale
and does not meet its border. Host image inspection and framebuffer bounds
do not establish physical LCD readability, which remains
**HARDWARE TEST REQUIRED**.
