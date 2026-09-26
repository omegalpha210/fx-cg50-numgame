# Focused review: six recently added games

This review covers exactly BLACK BOX, CRYPTARITHM, HASHI, NONOGRAM, REVERSI
and NET. All use the common START/RESUME and frozen result flow. Their game
rules remain validated by module state checks and independent host cases;
physical LCD and key-feel checks remain pending.

| Game | Rule and input finding | Display change |
|---|---|---|
| BLACK BOX | Ray entry/exit, hit/deflection, corner and adjacent-ball cases, edge numbering and guess completion checked independently of the stored answer. | Ray marks and guessed objects remain distinct. |
| CRYPTARITHM | Assignment uniqueness, repeated-letter identity, leading zero, carry and arithmetic validation checked. | Decorative underscores removed; fixed integer columns, selected-letter box and two-column menu icon aligned. |
| HASHI | Island totals, at most two bridges, orthogonal edges, crossing and connected completion checked with independent small-board enumeration. | Empty candidate, single/double bridge and invalid-crossing feedback clarified. |
| NONOGRAM | Run order, required gaps, empty `0` clue, unknown/filled/X distinctions and completion checked. | Empty-line and mark rendering reviewed; small-clue physical legibility pending. |
| REVERSI | Eight-way flips, illegal no-flip move, forced pass, terminal score/draw and CPU replay checked. | CPU wait hides human-only HINT/MOVES; result names winner and difficulty. |
| NET | Reciprocal edges, boundary closure, connectivity, tile rotation/locks and documented loop policy checked. | Symmetric rotation gives feedback; locked tile and reveal controls clarified. |

[Guess/Calc evidence](GUESS_CALC_AUDIT.md), [grid evidence](GRID_AUDIT.md),
and [strategy/quick evidence](STRATEGY_QUICK_AUDIT.md) contain detailed
independent cases, counts, captures and exact limitations. The
[actual renderer captures](captures/README.md) include the six play screens,
Cryptarithm icon/layout, completion modal and frozen result. Host C11/UBSan
and strict SH compilation have passed; ASan execution and physical hardware
acceptance are not claimed.
