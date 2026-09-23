# UI conventions and renderer review

Native framebuffer: **396×224 RGB565**. There is one gint VRAM. Header is y0–23,
game content y27–184, messages y186–202, softkeys y204–223. Drawing primitives
clip to native bounds. No desktop-size background is downscaled into the app.

Main and each category have 2×3 native geometry tiles, 187×50 pixels, 8-pixel
horizontal spacing and 6-pixel vertical spacing. Six pale category colors share
dark text, selection borders, header margins and normal-weight typography.
Main numbers 1–6 are category shortcuts. Category 1–5 are games; 6 is statistics.
LEFT/RIGHT advance row-major with wrap; UP/DOWN wrap within a column.

Normal text uses the same proportional gint 8×9 face as DIFF EQ, at native
pixel size and spacing. Its atlas contains 8×11 raster cells (including
descenders), with one pixel of padding. Glyph rows/advances match DIFF EQ's
retained host font byte-for-byte. The former 1.5× enlargement of 5×7 is removed.
Compact clues retain native 5×7. Large cards/numbers use integer scale.
Sudoku pencil marks use original 3×5
geometry, with selected notes also shown in the right information area. Fixed
values have a small marker and dark ink, entered values are blue.

Each of the 30 game tiles has an original 40×32 pixel pictogram drawn by
`src/ui/icons.c`: cards, equations, clue grids, a lock, a factor tree, buildings,
pile/track diagrams, sliding tiles and rectangle/loop boards. Main category tiles reuse a
representative game icon. Statistics has a separate bar-chart pictogram.
The name stays to the right, shortcut in the top-right and RESUME below the name.
Icons use the category ink color and native lines/rectangles; they do not
allocate image buffers or read/change game state. Menu geometry retains its original tile bounds. GUESS/BASEBALL now share
a code-tile icon; BOARD uses rectangle geometry. [Menu/icon contact sheet](captures/menus-native.png).

Softkeys have semantic labels. F1 INIT is yellow/black, F2 UNDO magenta/black;
unused actions are blank, including UNDO with no available history. Grid F3 is explicitly **REVEAL**, while true public
clue/GF(2)/partial-operation suggestions remain HINT. F4 is game-specific.
F5 RULES and F6 the actual primary action are consistent. EXE tokens appear
once per help line, blue and normal weight where highlighted.

Entry screens show only applicable rows, numbered consecutively: saved RESUME,
NEW GAME, difficulty and multiple-choice MODE. 2048 CLASSIC hides difficulty; TARGET exposes its goal level. Both have a
mode selector. All strategy modes retain difficulty because MASTER changes
starting positions, including LOCAL 2P. UP/DOWN selects, LEFT/RIGHT changes a setting,
digits focus the visible row, and EXE/F6 OPEN starts NEW from any row except
explicit RESUME. NEW GAME is the default entry focus; existing progress invokes
the same NEW confirmation. Settings values clamp at both ends. EASY/NORMAL/HARD
are shown together in cyan/orange/red; supported MASTER is purple. Native-width
measurement keeps all four at normal font size, without a width alias. HELL is stable difficulty4 for LOGIC11–15, distinct
from MASTER3. On the difficulty row F3 HELL is red/white; it selects a single red
HELL label. F3 restores the preceding general level, LEFT returns to MASTER,
RIGHT stays; MASTER RIGHT never enters HELL. The value border/underline is separate from row focus.
F4 STATS, F5 RULES and F6 OPEN remain fixed. F3 is context-only: HELL on a
LOGIC difficulty row, MODE on a long/many-choice mode row, blank elsewhere.
Short two/three-choice modes are inline. The two-column chooser uses arrows,
EXE/F6 OK commits and returns to entry, and EXIT cancels without starting play.
A short bottom hint follows selection.
Changing new-game settings never changes the saved run. Record filters omit
CLASSIC 2048 LEVEL and single-choice MODE; historical CLASSIC three-level
records remain viewable. TARGET and HELL records have separate buckets. Confirmation, pause and save-error dialogs
show only their usable softkeys. [Entry examples](captures/entry-layout-native.png).

Text boxes use measured glyph widths. Long values reduce integer scale, then
use the compact font; bounded labels use ellipsis only as a final fallback.
Rules, messages and result paragraphs wrap at words. Result paragraphs switch
to compact text when needed to preserve the full message. Host builds assert
text bounds before framebuffer clipping. Wide Calcudoku product clues retain
their number with a compact multiplication sign inside the cage border.
[Boundary fixtures](captures/overflow-native.png).

2048 colors are 2 gray, 4 lime, 8 cyan, 16 blue, 32 yellow, 64 orange, 128 magenta,
256 red; 512–32768 use progressively darker reds, capped thereafter. Dark tiles
use white digits. The menu icon shares this palette. See [palette preview](captures/39-2048-large.png).

Guess games show scrolling attempt/clue rows, arithmetic games show cards and
expression drafts, logic games use appropriate grids and clue geometry, CPU
games show piles/cards and explicit turns, and 2048 uses colored numeric tiles.
These are distinct module views under shared chrome.

Raw key down/up events maintain held and blocked sets. Modal, result, exposure,
round, OS and power transitions insert a release barrier. HOLD repeats only
menu selectors and rules scrolling; turn games do not repeat moves on HOLD.
Ordinary AC/ON and ALPHA+SHIFT+AC/ON do not take the power-off path. Plain EXE
cannot open and also confirm the same modal from one physical press.

Rules, entry and all modals suspend active time. The legacy Memory decoder
retains its old phase data without exposing it in the public catalog. Result-to-rules returns to the result. INIT has a small
`Restart this game? / EXE: YES / EXIT: NO` confirmation when progress exists.

The host captures invoke `ng_render` and the actual game adapters through an
RGB565 rectangle backend. [Native captures](captures/README.md) include Main,
six categories, all 30 games, ten hard grids, notes, Kakuro triangles, CPU turns,
entry/resume, input error, rules, confirmation, result, save failure/recovery, clock progression, statistics and
large 2048 numbers. `contact-native.png` is 1× and `category-*-2x.png` is nearest
neighbor 2×. The current capture manifest gives the exact frame count. Large-tile and maximum-value/long-text
samples are explicitly marked as renderer stress fixtures in the capture index;
the cage-clue fixture selects a real verified puzzle.

Review found and fixed atlas-padding extraction, pile-label overlap, Futoshiki
spacing, selected cage borders, lost Memory comparison behind a result modal,
and missing explicit reveal labels. No clipping or overlap remained in the
reviewed captures. Physical LCD readability, contrast and key feel remain
**HARDWARE TEST REQUIRED**.


Operator rendering colors + magenta, − orange, * / x bright green and / cyan.
Each glyph is painted once with its original advance. Whole-source token context
survives line wrapping and suffix clipping: scientific signs and prose separators
stay in their base color. Equation feedback keeps its exact/partial/absent colors.
SHIFT+DOT maps to one equal after original physical-key held/up bookkeeping;
plain DOT remains a dot, grammar validation is unchanged, and S↔D is not aliased.

[CONTROLS.md](CONTROLS.md) lists every screen and visible game. For Shikaku,
EXIT while selecting cancels only the corner; otherwise it checkpoints to entry.
Slitherlink stores shared edges once and supports all144 edge cursor positions.
[New board screens](captures/boards-native.png) and
[MASTER screens](captures/master-native.png) use the production common chrome.
