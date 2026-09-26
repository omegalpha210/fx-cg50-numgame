# In-app game rules

Exported from the same `NgModule.rules` strings compiled into NUMGAME.g3a.
See README for physical key mapping (`SHIFT+DOT` = `=`; DOT alone = `.`, power key = `^`).
All games use F1 INIT, F5 RULES, EXIT checkpoint, MENU checkpoint and
SHIFT+AC/ON checkpoint. NEW is separate from INIT.
Long RULES scroll one wrapped line per UP/DOWN event; the right rail
shows position and disables the arrow at each end. Short RULES have no rail.

## 01 — NUMBER BASEBALL

```text
Find the hidden digit sequence.
EASY/NORMAL/HARD/MASTER: 4/5/6/7 digits.
Repeated digits and leading zero are allowed.
S = correct digit and position.
B = correct digit, wrong position.
Exact matches are consumed before counting B.
Total attempts by level: 20/30/40/50.
The last try is included in that total.
Digits enter; DEL erases; EXE submits.
UP/DOWN scroll attempts. No guess undo.
Older saved games retain their original rules.
```

Modes: STANDARD.

## 02 — EQUATION GUESS

```text
Find the hidden equation TEXT exactly.
Matching its numeric value alone cannot win.
Use digits + - * / and one =. RHS integer.
No parentheses or leading zeros. Unary minus
on the RHS only.
Standard precedence. Green: exact. Yellow:
exists elsewhere. Grey: absent. Duplicates
consume exact matches first. 7/6/8-char modes.
Easy +-, Normal +-* , Hard +-*/ corpus.
Hard 7/8-char modes use two operators.
MASTER STANDARD/SHORT/LONG: 9/8/10 chars,
mixed precedence, 12 attempts.
SHIFT+DOT enters =.
EXE submits true equations. UP/DOWN history.
```

Modes: STANDARD / SHORT / LONG.

## 03 — NUMBER MIND

```text
Each clue gives exact-position matches only.
Digits in wrong positions give no information.
Easy: 4 digits 0..5. Normal: 4 digits 0..7.
Hard: 5 digits 0..7. MASTER: 6 digits 0..7.
Repeats and initial 0 OK.
All published clues have one solution in that
finite domain. Digits: input. EXE: CHECK.
UP/DOWN: clues. HINT lists values not excluded
by visible zero-match clues, one spot at a time.
```

Modes: STANDARD.

## 04 — CLUE LOCK

```text
Find an integer satisfying EVERY shown clue.
MOD is the remainder after division. Decimal
digit sum and contains ignore leading zeros.
Easy domain 0..99, Normal 0..499, Hard 0..999.
Hard uses two modular / CRT conditions.
MASTER: 0..9999, three MOD clues and digit
sum. All four clues contribute to uniqueness.
All clues shown initially; one domain solution.
HINT explains the first modular progression.
Digits: enter. EXE: CHECK. DEL: erase.
```

Modes: STANDARD.

## 05 — SEQUENCE DETECTIVE

```text
Find the next term in a FINITE rule grammar.
A finite prefix is not universally unique!
Allowed rules (n starts at 0):
a+bn: a=-9..9, b=-5..5, b nonzero.
a*b^n: a=1..5, b=-3,-2,2,3.
a+bn+cn*n: a=-5..5,b=-4..4,c=1..3.
Fibonacci sum: first two each 1..9.
Alternating lanes: starts -6..9, step 1..5.
x'=b*x+c: start -3..6,b=-2,-1,2,3,
c=-3..3 nonzero. Both lanes share a step.
MASTER adds x_next=a*x_last+b*x_before+c,
first two -3..6; a,b,c each -2,-1,1,2;
and alternating lanes with DIFFERENT steps,
starts -6..9; each step -5..5 nonzero.
Revision4 EASY uses AP, GP, shared step.
NORMAL: quadratic, Fibonacci, offset, and
unequal lanes: starts1..9, steps1..3.
HARD keeps the six old rules; MASTER uses
all old and new rules together. Old saves
keep their original grammar.
Packs reject competing next answers across
their level grammar. HINT reveals RULE FAMILY.
Enter signed integer, EXE checks.
```

Modes: STANDARD.

## 33 — BLACK BOX

```text
Find hidden atoms by firing beams from edges.
E/N/H/MASTER: 5/6/7/8 square; 3/4/5/6 atoms.
A beam hitting an atom is absorbed (H).
An atom diagonally ahead bends it away.
Two diagonally ahead reverse the beam.
A side atom at entry reflects immediately (R).
Direct hits take precedence over diagonals.
Exit numbers pair the entry and exit ports.
Any layout with the correct atom count and
ALL the same ray outcomes wins; equivalent
layouts are accepted, not just the hidden one.
CELLS: arrows move; EXE cycles blank/atom/X.
1 atom, 2 excluded, 0/DEL clear. F4: RAYS.
RAYS: arrows select port; EXE fires. F4: CELLS.
F6 CHECK costs 5 penalty points if wrong.
Score = new probes + penalties; lower is best.
HINT reveals one atom and marks assisted.
```

Modes: STANDARD.

## 06 — MAKE TARGET

```text
Choose 10/24/50/100/200/RANDOM at entry.
RANDOM mixes the five targets in your level.
Use EVERY card exactly once to reach target.
EASY/NORMAL/HARD/MASTER: 4/4/5/6 cards.
Use + - * / and parentheses.
Exact rational intermediate values allowed.
No concatenation, powers or extra constants.
Equal values are separate usable cards.
New: 200 verified decks per target and level;
RANDOM shares those 1,000 decks per level.
Every input card is 1..999 (at most 3 digits).
E: no division needed. N/H: division needed.
MASTER: every solution needs fractions.
Type expression; DEL erases; EXE checks.
HINT gives one first step; ANSWER copies one
example. Both mark assisted. All legal answers
are accepted, not just that example.
96 chars, nesting12, reduced values <= 1e9.
Earlier saved decks and targets stay playable.
```

Modes: STANDARD.

## 07 — COUNTDOWN

```text
Reach target 100..999 with up to six cards.
Each card at most once; unused cards allowed.
Every intermediate result MUST be a positive
integer; division must be exact. + - * / ( ).
Small deck: two copies each of 1..10.
Large: 25,50,75,100, without repeats.
Easy/Normal/Hard: 1/2/3 large cards.
New: 200 verified decks in each difficulty.
HARD: every exact answer needs >=4 cards.
MASTER: 3 large cards; exact target requires
all six cards and at least one division.
Untimed practice. EXE checks each expression.
Exact=10 points, distance 1..5=7,6..10=5,
else 0. Best submitted distance is retained.
FINISH keeps best result. No optimality claim.
HINT gives first step of one exact solution.
```

Modes: STANDARD.

## 08 — MISSING OPERATORS

```text
Fill the missing + - * / operators.
Number order is fixed; no parentheses.
Standard precedence: * / before + -.
Equal precedence is evaluated left to right.
All valid operator combinations are accepted.
Easy/Normal/Hard use 3/4/5 numbers.
MASTER: 6 numbers; 1..3 solutions, each with
at least three different operator kinds.
Arrows select slot; operation key sets it.
DEL clears. EXE checks. REVEAL fills selected
slot from one solution and marks assisted.
```

Modes: STANDARD.

## 09 — CROSS MATH

```text
Place 1..9 exactly once over the WHOLE board.
Satisfy three horizontal and three vertical
expressions, reading left/right or top/down.
* before + or -. No Latin-square rules.
Grey fixed clues cannot be edited.
Easy/Normal/Hard have 4/2/0 fixed numbers.
MASTER has no fixed values and needs tighter
row/column coupling: more candidate boards.
Each generated pack puzzle has one solution.
Arrows select; 1..9 enter; DEL/0 clear.
EXE checks all rules. REVEAL selected number
marks the game assisted.
```

Modes: STANDARD.

## 10 — PRIME FACTOR

```text
Factor the composite target into primes.
Use prime bases with optional positive powers.
Example 360: 2^3*3^2*5, or repeat factors.
Any factor order is accepted. 1 is not prime
or composite; composite bases are rejected.
Exponents 1..20; at most 16 written factors.
New targets: EASY 3 digits; NORMAL 3/4;
HARD 4/5; MASTER 5/6. Largest prime <=97.
Structure, not digit count alone, sets levels.
Older saves retain their original factors.
Type digits, * and ^. DEL erases, EXE checks.
HINT proves one prime divisor. ANSWER reveals
a factorization. Both mark assisted.
```

Modes: STANDARD.

## 34 — CRYPTARITHM

```text
Replace letters with decimal digits so the
vertical addition is true. Each letter keeps
one digit; different letters use different
digits. Leading letters cannot be zero.
E/N/H/MASTER: two 2/3/4/5-digit addends.
Longer carries and more letters are involved;
this is a structural level, not a human rating.
30 original, unique-solution puzzles per level.
Arrows select a letter. Digits set it.
Selected letters are boxed in the sum.
Blank mappings show only their letter.
DEL clears it. NEXT selects the next blank.
EXE or F6 checks all public arithmetic rules.
Any valid assignment is accepted. UNDO works
on digit edits and checks. No stored answer
is consulted by the native checker.
```

Modes: STANDARD.

## 11 — SUDOKU

```text
Fill 1-9 once in every row, column, 3x3 box.
Given clues cannot change. Digits enter values.
F4 NOTES toggles pencil marks; DEL clears.
EXE CHECK validates visible rules only.
F3 REVEAL shows one cell and marks ASSISTED.
Arrows select. UNDO restores edits.
```

Modes: CLASSIC.

## 12 — CALCUDOKU

```text
Use 1-N once in each row and column.
Cages satisfy their target and operator.
+ and * combine all cells. - is difference.
/ is larger divided by smaller, exactly.
- and / cages have two cells. = is fixed.
A cage may repeat values in different lines.
F3 REVEAL is assisted. EXE checks all rules.
```

Modes: CLASSIC.

## 13 — KAKURO

```text
Enter 1-9 in every white cell; zero is illegal.
Every run sums to its clue without repeats.
Upper-right clue: across. Lower-left: down.
Arrows select cells. DEL clears. EXE checks.
F3 REVEAL shows one cell, marking ASSISTED.
```

Modes: CLASSIC.

## 14 — FUTOSHIKI

```text
Use 1-N once in each row and column.
All printed inequalities must hold.
The pointed tip faces the smaller value.
Given clues stay fixed. DEL clears.
F3 REVEAL is assisted. EXE checks all rules.
```

Modes: CLASSIC.

## 15 — SKYSCRAPERS

```text
Use heights 1-N once in each row and column.
Outer clues count buildings seen from there.
A taller building hides lower ones behind it.
Missing outer clues impose no condition.
DEL clears. EXE checks. F3 REVEAL is assisted.
```

Modes: CLASSIC.

## 35 — HASHI

```text
Join visible islands with straight bridges.
Only horizontal/vertical bridges are allowed.
Each pair has zero, one or two bridges.
Island numbers give their total bridge count.
Bridges cannot cross or pass through islands.
All islands must form one connected network.
Arrows select a visible neighboring island.
2/4/6/8 cycle down/left/right/up bridges.
EXE cycles last direction; DEL clears it.
Blue dashed edge is empty; red X is a crossing.
F4 checks. F3 REVEAL marks ASSISTED.
```

Modes: CLASSIC.

## 16 — HITORI

```text
EXE shades or unshades the selected cell.
Unshaded values cannot repeat in a row/column.
Shaded cells cannot share an edge.
All unshaded cells must connect by edges.
Diagonal shaded contacts are allowed.
F4 CHECK validates all three rules.
DEL unshades. F3 REVEAL is assisted.
```

Modes: CLASSIC.

## 17 — BINARY PUZZLE

```text
Enter 0 or 1. A blank differs from zero.
Each row/column contains half zeros/half ones.
Never three identical neighbors in a line.
No two completed rows or columns may match.
Given clues stay fixed. DEL returns to blank.
EXE checks. F3 REVEAL is assisted.
```

Modes: CLASSIC.

## 18 — NUMBRIX

```text
Place every integer from 1 to N*N once.
Consecutive numbers must share an edge.
Diagonal steps are forbidden. Clues are fixed.
Type one or two digits, then EXE to enter.
Moving the cursor cancels a draft. DEL erases.
F4 CHECK validates the path.
F3 REVEAL shows one cell and marks ASSISTED.
```

Modes: CLASSIC.

## 19 — MAGIC SQUARE

```text
Use every integer 1 to N*N exactly once.
Rows, columns and both main diagonals must
have the displayed common sum.
PARTIAL MASTER: wrapped diagonals also sum 34.
FREE E/N/H/M: blank 3/4/5/6 square, normal sums.
Older FREE saves keep their original rules.
Any rule-valid solution is accepted.
Type 1-2 digits, EXE enters; F4 checks.
DEL edits draft or clears. Arrows select.
```

Modes: PARTIAL / FREE.

## 20 — SUM GRID

```text
EXE toggles KEEP or REMOVE for a number.
Kept numbers must sum to each row/column target.
Removed numbers remain visible with a line.
A zero target means remove the whole line.
F4 CHECK validates every sum. DEL keeps.
F3 REVEAL shows one choice, marking ASSISTED.
```

Modes: CLASSIC.

## 36 — NONOGRAM

```text
Fill cells to match all row/column run clues.
Runs appear in order, with an empty gap.
A zero clue means the whole line is empty.
Fill exactly the required cells. X is optional.
Arrows move. 1 fills; 0 marks an empty cross.
EXE cycles unknown, filled, empty. DEL clears.
F4 checks every visible run clue.
F3 REVEAL shows one cell, marking ASSISTED.
```

Modes: CLASSIC.

## 21 — NIM

```text
Take stones from exactly one pile.
The last stone wins (normal play).
Arrows select pile; type amount, EXE.
Easy: 3 piles up to 7; Normal: up to 15.
Hard: 4 piles up to 31.
CPU Easy random, Normal 75% exact,
Hard uses the exact nim-sum policy.
MASTER: verified four-pile tactical starts.
Play against CPU: YOU FIRST or CPU FIRST.
MASTER keeps the same exact Hard CPU.
Its starts allow YOU a forced win.
UNDO restores your complete CPU round.
UNDO marks assisted practice.
```

Modes: YOU FIRST / CPU FIRST.

## 22 — WYTHOFF

```text
Take from one pile, or take the same
positive amount from both piles.
Empty both piles to win.
Arrows select A / B / BOTH.
Type amount and press EXE.
Pile ranges: Easy 1..10, Normal 1..24,
Hard 1..40. Exact bounded DP on Hard.
MASTER: verified long tactical starts.
Easy random; Normal 75% exact moves.
Play against CPU: YOU FIRST or CPU FIRST.
MASTER keeps the same exact Hard CPU.
Its starts allow YOU a forced win.
UNDO restores the complete CPU round.
```

Modes: YOU FIRST / CPU FIRST.

## 23 — EUCLID

```text
Subtract k times SMALL from LARGE.
k is a positive integer; no negatives.
Making zero wins. Values are re-sorted.
Type k in the shown range; EXE to play.
Easy values 1..12; Normal 1..40;
Hard 1..99. All rules stay the same.
Hard uses exact memoized state analysis.
MASTER: coprime multi-step tactical starts.
Easy random; Normal 75% exact moves.
Play against CPU: YOU FIRST or CPU FIRST.
MASTER keeps the same exact Hard CPU.
Its starts allow YOU a forced win.
UNDO restores the complete CPU round.
```

Modes: YOU FIRST / CPU FIRST.

## 24 — MAKE FIFTEEN

```text
Take unused cards from 1 to 9.
Any three of YOUR cards summing to 15
win, even when you hold more than three.
All cards used without a winner: draw.
Arrows + EXE, or a digit to take a card.
Blue marks YOU; red marks CPU.
Easy random; Normal 75% exact moves.
Hard uses the full exact minimax table.
MASTER: verified midgame tactical starts.
Play against CPU: YOU FIRST or CPU FIRST.
MASTER keeps the same exact Hard CPU.
Its starts allow YOU a forced win.
UNDO restores the complete CPU round.
```

Modes: YOU FIRST / CPU FIRST.

## 25 — RACE TO TARGET

```text
Take turns adding to the running total.
Reach the target EXACTLY to win.
An addition beyond the target is illegal.
Type the addition, then EXE.
Easy: target 21; add 1, 2, or 3.
Normal: target 31; add 1, 2, or 3.
Hard: target 23; add 1, 2, 3, or 4.
Hard uses exact addition-game analysis.
MASTER: varied tactical target/add limits.
Easy random; Normal 75% exact moves.
Play against CPU: YOU FIRST or CPU FIRST.
MASTER keeps the same exact Hard CPU.
Its starts allow YOU a forced win.
```

Modes: YOU FIRST / CPU FIRST.

## 37 — REVERSI

```text
Place a disc to bracket an enemy line.
Flip every bracketed line in 8 directions.
Black moves first; choose YOU or CPU first.
Arrows select; EXE or 5 places a disc.
If you cannot move, you pass automatically.
Neither player can move: most discs wins.
Equal counts draw; empty cells stay empty.
F4 toggles dots marking legal placements.
Easy CPU chooses a random legal move.
Normal searches 1 ply, Hard up to 3,
MASTER up to 5, with fixed node budgets.
The bounded CPU is not perfect play.
HINT suggests a move and marks assisted.
UNDO restores your complete CPU round.
```

Modes: YOU FIRST / CPU FIRST.

## 26 — 2048

```text
Arrows slide all tiles. Equal tiles merge
once each move; merged value adds score.
A changed move spawns 2 (90%) or 4 (10%).
Blocked moves consume no random numbers.
CLASSIC: reach 2048 and continue playing.
TARGET goals E/N/H/M:512/1024/2048/8192.
TARGET ends when its goal tile is reached.
CLASSIC uses a fixed level.
No legal moves means game over.
UNDO restores tiles, score, and RNG;
it marks the run as assisted practice.
Tiles cap at 2^30 and cannot merge beyond.
Score saturates at 4294967295 safely.
Large tiles use exact 2^n notation.
```

Modes: CLASSIC / TARGET.

## 27 — SLIDING PUZZLE

```text
Put 1..8 or 1..15 in reading order,
with the blank in the bottom-right.
Arrows move the BLANK in that direction.
Choose a 3x3 or 4x4 board in MODE.
New E/N/H starts have exact distances:
EASY 8..14, NORMAL 15..20, HARD 21..26.
Each size/level has 128 certified starts.
MASTER 3x3: exact distance 31 (2 starts).
MASTER 4x4: at least 48 moves (30 starts).
4x4 MASTER distances are lower bounds.
Older runs keep their original shuffles.
Every start is solvable; UNDO reverses
your moves. Size stays in MODE.
```

Modes: 3 x 3 / 4 x 4.

## 28 — LIGHTS OUT

```text
Turn every light off. Arrows select.
EXE or 5 toggles the selected light and
its orthogonal neighbours (no wrapping).
Modes: 4x4 or 5x5. Both start solvable.
New 4x4 E/N/H minima: 2/4/5 presses.
Older 4x4 starts keep their shuffle rules.
5x5 E/N/H use 4/8/18 generating presses;
these are not minimum solution counts.
MASTER: certified minimum-press starts.
4x4 needs 6; 5x5 needs 12..14 presses.
Every valid solution is accepted.
HINT uses Gaussian elimination over GF(2).
Press the highlighted cell next.
The hint does not promise fewest moves.
HINT/UNDO mark assisted practice.
```

Modes: 4 x 4 / 5 x 5.

## 31 — SHIKAKU

```text
Divide all cells into rectangles.
Each rectangle contains exactly one clue.
Its area must equal that clue number.
Rectangles cannot overlap or leave gaps.
Arrows move the selected cell.
EXE: first corner, then opposite corner.
EXE again places the rectangle.
EXIT cancels an unfinished selection.
DEL removes the selected rectangle.
UNDO restores a created/deleted rectangle.
EASY 5x5, NORMAL 6x6, HARD 8x8.
MASTER 8x8: interacting choices.
30 original unique puzzles per level.
```

Modes: STANDARD.

## 32 — SLITHERLINK

```text
Connect neighbouring dots in ONE loop.
A clue counts the lines around its cell.
Blank cells have no number constraint.
0 and blank are different.
Every used dot must have two lines.
No branches, crossings, or separate loops.
Arrows move the selected edge.
F4 switches horizontal/vertical edges.
EXE toggles a line; DEL toggles an X.
X means excluded and is never a line.
UNDO restores the last edge edit.
EASY 5x5, NORMAL 6x6, HARD 8x8.
MASTER 8x8: interacting choices.
30 original unique puzzles per level.
```

Modes: STANDARD.

## 38 — NET

```text
Rotate tiles to connect every square.
Every wire must meet its neighbour's wire.
No wires may leave the board; no wrapping.
The whole network must form one tree:
all tiles connected, with no closed loops.
Arrows select. EXE/5 turns clockwise;
DEL turns anticlockwise. F4 locks a tile.
Locks block EXE/5/DEL until unlocked.
F3 REVEAL sets one original orientation
and locks it, marking assisted practice.
REVEAL can reset a locked tile too.
Every valid network wins, not just ours.
E/N/H/M use 3/4/5/6 by 3/4/5/6 grids.
Seeds construct a tree then rotate tiles.
Solutions need not be unique; no rating claim.
Moves count rotations, locks and reveals.
```

Modes: STANDARD.
