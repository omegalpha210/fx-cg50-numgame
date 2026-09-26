# UI conventions and renderer review

The native framebuffer is **396×224 RGB565**. The header occupies y0–23,
game content y27–184, messages y186–202 and softkeys y204–223. Normal text is
the retained DIFF EQ/gint proportional 8×9 face at native pixel size; compact
clues use 5×7. Host rendering asserts text bounds before primitive clipping.
All 36 games have original 40×32 menu pictograms and 2×3 category tiles.
[Actual menu captures](captures/menus-native.png) use the same renderer as the
SH build, through a host RGB565 backend.

Main F1 shows **RESUME** only for the one unfinished run; F2 SET and F6 OPEN
remain. Exactly that run's game tile carries a small blue RESUME badge below
its icon/name. Category tiles have no badge or RESUME softkey. Opening a different game's
entry does not affect the saved run and shows NEW GAME first. Opening the
same game's entry adds RESUME above NEW GAME and focuses RESUME. UP/DOWN and
digits select a row. LEFT/RIGHT changes an applicable value, while EXE/F6
OPEN starts the current settings or resumes only from the RESUME row. NEW GAME
commits the replacement without a confirmation dialog. The footer gives
short contextual help. F5 RULES remains available; F3 toggles separate LOGIC
HELL on its difficulty row and displays colored ENHM on return. Strategy FIRST
selects YOU or CPU inline. No unused MODE row or popup is shown.

Make Target's TARGET row has six choices: 10, 24, 50, 100, 200, RANDOM.
LEFT/RIGHT moves to adjacent choices and clamps at either end; the current
choice and available arrow directions fit on one line. There is no numeric
target editor on this entry screen. EXE or F6 OPEN starts NEW with the
displayed choice. A saved arbitrary-target run from an older revision still
resumes with its original target; after its result, NEW returns to TARGET
selection so no different target is chosen silently.
[Actual beta.6 entry captures](captures/beta6/make-target-contact.png) show
all six choices and the selected arrows.

The three new-run labels have different scopes: entry **NEW GAME** uses selected
settings, playing **F1 INIT** restarts the same run/puzzle, and completed-result
**F6 NEW** starts the next run with the completed run's settings.

In play, F1 INIT restarts the current puzzle, F2 UNDO appears only with an undo
state, F3 HINT/REVEAL and F4 are module-specific, F5 RULES and F6 the module's
actual action. Reversi hides HINT and MOVES during a pending CPU turn. Result
dialogs use **EXE: NEW GAME** and **EXIT: VIEW RESULT**. EXIT closes only the
dialog, leaving the completed board/result frozen; F6 NEW then starts the next
run using the completed run's game, difficulty and mode. A second EXIT returns
to that game's entry. RULES may be opened without changing the final state.
Completion clears persistent RESUME. Input release barriers prevent one held
EXIT/EXE from passing through two states.

NUMBER BASEBALL's 20/30/40/50-attempt list uses a compact retained history
representation and a scroll rail when the visible rows do not contain every
guess. NUMBER MIND and EQUATION GUESS show the same rail on long clue/guess
lists without changing their attempt rules. The rail follows the RULES style:
gray UP/DOWN affordances disable at the ends and disappear for short lists.
The ONE LAST TRY dialog appears once before Baseball's final *included*
attempt; dismissing it keeps the warning state across save/resume.
[Native scroll captures](captures/beta6/scroll-contact.png) include top,
middle and bottom list positions with an unsubmitted draft.

The Cryptarithm renderer omits decorative input underscores. Its addition
rows, operator, rule and result use fixed integer column centers; the selected
letter has a visible box, and letter-to-digit assignment and arithmetic rules
remain intact. Its menu icon places each glyph on the same two-column grid.
[Aligned play](captures/34-cryptarithm-aligned-layout.png) ·
[icon](captures/34-cryptarithm-icon.png). 2048 colors advance gray, lime, cyan,
blue, yellow, orange, magenta and red, then deepen. Long text is measured and
wrapped within panels. [Beta.6 entry captures](captures/beta6/make-target-contact.png) and
[overflow captures](captures/overflow-native.png) are actual host frames,
including explicitly labeled renderer stress fixtures.

The focused review covered Black Box, Cryptarithm, Hashi, Nonogram, Reversi and
Net. Findings, rule tests and limitations are in
[RECENT_SIX_AUDIT.md](RECENT_SIX_AUDIT.md). Physical LCD contrast, clue
legibility and MENU behavior remain **HARDWARE TEST REQUIRED**.

Nonogram's optional X annotation does not block completion: only the filled
black-cell set must match its unique puzzle. Magic Square FREE new runs use
3×3/4×4/5×5/6×6 normal-square boards. The actual-renderer captures include
tile badge, older entry/editor states, optional-X completion and all four FREE sizes.
The 6×6 grid has 26-pixel square cells, centered two-digit numbers and
`SUM = 111` without footer overlap. These bounds checks are not real-LCD
readability measurements.
