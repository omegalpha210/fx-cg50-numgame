# UI conventions and renderer review

The native framebuffer is **396×224 RGB565**. The header occupies y0–23,
game content y27–184, messages y186–202 and softkeys y204–223. Normal text is
the retained DIFF EQ/gint proportional 8×9 face at native pixel size; compact
clues use 5×7. Host rendering asserts text bounds before primitive clipping.
All 36 games have original 40×32 menu pictograms and 2×3 category tiles.
[Actual menu captures](captures/menus-native.png) use the same renderer as the
SH build, through a host RGB565 backend.

Main F1 shows **RESUME** only for the one unfinished run; F2 SET and F6 OPEN
remain. Category screens have no RESUME softkey. Opening a different game's
entry does not affect the saved run and shows START GAME first. Opening the
same game's entry adds RESUME above START GAME and focuses RESUME. UP/DOWN and
digits select a row. LEFT/RIGHT changes an applicable value, while EXE/F6
OPEN starts the current settings or resumes only from the RESUME row. START
GAME commits the replacement without a confirmation dialog. The footer gives
short contextual help. F5 RULES remains available; F3 toggles separate LOGIC
HELL on its difficulty row and displays colored ENHM on return. Strategy FIRST
selects YOU or CPU inline. No unused MODE row or popup is shown.

In play, F1 INIT restarts the current puzzle, F2 UNDO appears only with an undo
state, F3 HINT/REVEAL and F4 are module-specific, F5 RULES and F6 the module's
actual action. Reversi hides HINT and MOVES during a pending CPU turn. Result
dialogs use **EXE: NEW GAME** and **EXIT: VIEW RESULT**. EXIT closes only the
dialog, leaving the completed board/result frozen; F6 NEW then starts the next
run using the completed run's game, difficulty and mode. A second EXIT returns
to that game's entry. RULES may be opened without changing the final state.
Completion clears persistent RESUME. Input release barriers prevent one held
EXIT/EXE from passing through two states.

The Cryptarithm renderer omits decorative input underscores. Its addition
rows, operator, rule and result use fixed integer column centers; the selected
letter has a visible box, and letter-to-digit assignment and arithmetic rules
remain intact. Its menu icon places each glyph on the same two-column grid.
[Aligned play](captures/34-cryptarithm-aligned-layout.png) ·
[icon](captures/34-cryptarithm-icon.png). 2048 colors advance gray, lime, cyan,
blue, yellow, orange, magenta and red, then deepen. Long text is measured and
wrapped within panels. [Entry captures](captures/entry-layout-native.png) and
[overflow captures](captures/overflow-native.png) are actual host frames,
including explicitly labeled renderer stress fixtures.

The focused review covered Black Box, Cryptarithm, Hashi, Nonogram, Reversi and
Net. Findings, rule tests and limitations are in
[RECENT_SIX_AUDIT.md](RECENT_SIX_AUDIT.md). Physical LCD contrast, clue
legibility and MENU behavior remain **HARDWARE TEST REQUIRED**.
