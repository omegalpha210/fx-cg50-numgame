# NEW, RESUME and puzzle selection

NEW GAME on the entry screen starts a new run with its selected difficulty and
settings. F1 INIT restarts the *same* puzzle and marks the run assisted. RESUME
restores the one unfinished run, including its displayed board, input, random
state, bank seed and cursor; it does not select or consume another puzzle. A
completed result's F6 NEW selects the next run using that result's settings.
Opening RULES, redrawing a timer, or browsing an entry screen consumes no bank
ordinal.

Bank-backed games use a seed, cursor and coprime affine stride. For a pool of
`N` ordinals, the stride visits all `N` once before a new permutation. If
`N > 1`, a cycle boundary is adjusted so the previous last and next first
puzzles differ. The seed and cursor travel with the one active run; no
permutation array or 36-game achievement ledger is saved. Switching to
another game or difficulty replaces the prior unfinished run and its cycle.
An unfinished run's cold RESUME continues the same order. Completion clears
persistent RESUME but the open app retains the cycle in RAM for result NEW.
After closing the app with no unfinished run, the next launch begins a new
cycle. Hardware timing remains **HARDWARE TEST REQUIRED**.

Make Target revision 6 exposes six TARGET settings: `10`, `24`, `50`,
`100`, `200`, and `RANDOM`. Each fixed target has 200 distinct
target/card-multiset bases per difficulty, so a fixed setting has a 200-item
cycle. RANDOM uses the union of those five pools, a 1,000-item cycle at the
selected difficulty. It is a *selection policy*, not another 4,000 copied
records. Two consecutive RANDOM puzzles may have the same numerical target
while their actual puzzle identities differ. A RANDOM result's NEW keeps the
RANDOM policy, and a fixed result's NEW keeps its fixed target. The requested
kind and fixed target are distinct from the current puzzle's resolved target
in the saved run. Different TARGET or difficulty settings start a fresh
cycle. Old arbitrary-target saved runs resume under their original revision;
after one finishes, NEW returns to the TARGET entry for an explicit new
choice instead of silently mapping to one of the six settings.

Countdown revision 5 uses 200 base records per difficulty and the same
no-repeat cycle. Prime Factor revision 5 uses 128 EASY and 256 each
NORMAL/HARD/MASTER. Their older unfinished records preserve their own
revision on RESUME and INIT. Legacy Make Target revision-4 and revision-5
two-deck-per-target banks, revision-3 generated decks and older Countdown
starts remain compiled for this purpose; they are not included in a new
beta.6 cycle.

Other games retain their previous selection rules. In particular, Magic
Square FREE starts with a blank size/rule board, not a bank layout, while
old FREE saved runs still replay their original revision. NEW 4×4 Lights Out
revision-3 starts keep their certified minimum-press grading; older saved
starts keep their earlier generator. Runtime-generated games use bounded
seeded selection, with only a best-effort short recent-fingerprint retry
where the game has no finite bank.

The [36-game inventory](CONTENT_INVENTORY.md) and its
[per-setting CSV](CONTENT_INVENTORY_SETTINGS.csv) distinguish a currently
reachable pool from all compiled historical records.
