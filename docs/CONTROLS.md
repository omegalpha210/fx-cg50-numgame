# Screen and game controls

These tables describe the current visible catalog, IDs1–28,31–38. Retired29/30
are stored/decoded as legacy, never offered by the normal selector.

| Screen/state | Arrows | Digits/operators | EXE / F6 | DEL | EXIT | F1 / F2 / F3 / F4 / F5 |
|---|---|---|---|---|---|---|
| Main/category | Wrapped 2×3 selector | 1–6 opens tile; operators unused | OPEN tile | Unused | Category→Main | — / Main SET / Main valid visible RESUME / — / — |
| Game entry | UP/DOWN rows; LEFT/RIGHT clamp settings | Digits focus row only; operators unused | RESUME row resumes; all other rows request NEW | Unused | Category | — / — / HELL for LOGIC / — / RULES |
| Rules | UP/DOWN scroll | Unused | Close, preserving result state | Unused | Close | Only F6 OK |
| Settings | Unused | 1 toggles first help; 2 toggles time display | Unused | Unused | Save preferences→Main | HELP / TIME / DIAG / — / — |
| NUM DIAG diagnostics | Unused | Unused | Close/cancel stress | Unused | Close/cancel stress | RESET / EXPORT / PAGE / STRESS or STOP / — |
| NEW or INIT confirmation | Unused | Unused | Confirm once after release | Unused | Cancel | Only F6 YES |
| Save error | Unused | Unused | Bounded retry | Unused | Keep RAM/play | Only F6 RETRY |
| Result | Unused | Unused | NEW game | Unused | Game entry, NEW focused | NEW / — / — / — / RULES |
| Legacy pause (not publicly selectable) | Unused | Unused | Continue | Unused | Continue | F4 RESUME |

MENU and SHIFT+AC/ON reach the common dispatcher from every row above. They
attempt dirty checkpoints, clean pending I/O and invoke the supported OS action;
a failed save remains dirty and is reported, but does not trap MENU/OFF. Plain
AC/ON and ALPHA-conflicted OFF are ignored. OS return inserts a release barrier.

During play: F1 INIT restarts the same seed/run (confirmation after progress),
F2 UNDO appears only with supported history, F3 HINT/REVEAL only for modules
that supply it, F4 is listed below, F5 RULES, F6 mirrors the displayed primary
EXE action. F1/F2/F3 assistance is marked on the current run. INIT,
reveal and answer do not silently become NEW. Unsupported softkeys are blank.
All turn/edit actions ignore HOLD; only menu selectors and rules scroll repeat.

| ID / Game | Arrows | Digits/operators | EXE / F6 | DEL | F4 |
|---|---|---|---|---|---|
| 01 Baseball | UP/DOWN history | Numeric code, preserving initial0 | Submit guess | Backspace | — |
| 02 Equation | UP/DOWN history | Digits,+−*/=; RHS unary− only | Submit true equation text | Backspace | — |
| 03 Number Mind | UP/DOWN clues | EASY0–5, other0–7 | Check code | Backspace | — |
| 04 Clue Lock | — | Up to3 digits; MASTER4 digits | Check all conditions | Backspace | — |
| 05 Sequence | — | Signed integer | Check next term | Backspace | — |
| 06 Make Target | — | Digits,+−*/() | Check every card once | Backspace | ANSWER copies an example; EXE still needed |
| 07 Countdown | — | Digits,+−*/() | Check legal positive-integer steps; retain best | Backspace | FINISH submitted best; prompts if none yet |
| 08 Missing Operators | LEFT/RIGHT slot | +−*/ only | Check expression | Clear slot | REVEAL selected example operator |
| 09 Cross Math | Wrapped cell selection | 1–9;0 clears | Check six equations and whole-board1–9 | Clear editable cell | REVEAL editable selected cell; fixed clue gives notice |
| 10 Prime Factor | — | Digits,* and^ | Check prime bases and product | Backspace | ANSWER copies factorization |
| 11 Sudoku | Wrapped cell selection | 1–9; NOTES toggles candidates | Check board | Clear notes first, otherwise value | NOTES |
| 12 Calcudoku | Wrapped cell selection | 1–N | Check board | Clear value | — |
| 13 Kakuro | Wrapped selection skipping black cells | 1–9 in white cells | Check board | Clear value | — |
| 14 Futoshiki | Wrapped cell selection | 1–N | Check board | Clear value | — |
| 15 Skyscrapers | Wrapped cell selection | 1–N | Check board | Clear value | — |
| 16 Hitori | Wrapped cell selection | — | Toggle shade | Unshade | CHECK board |
| 17 Binary | Wrapped cell selection | Actual0 or1 | Check board | Return to blank | — |
| 18 Numbrix | Move and cancel draft | Two-digit draft,1–N² | Enter draft; otherwise check | Backspace draft, otherwise clear cell | CHECK without committing draft |
| 19 Magic Square | Move and cancel draft | Two-digit draft,1–N² | Enter draft; otherwise check | Backspace draft, otherwise clear cell | CHECK without committing draft |
| 20 Sum Grid | Wrapped cell selection | — | KEEP/REMOVE | KEEP | CHECK all sums |
| 21 Nim | LEFT/RIGHT pile | Removal quantity,2 digits | Take from pile | Backspace | — |
| 22 Wythoff | LEFT/RIGHT A/B/BOTH | Removal quantity,2 digits | Take from selection | Backspace | — |
| 23 Euclid | — | Multiplier k,2 digits | Subtract k×SMALL from LARGE | Backspace | — |
| 24 Make Fifteen | Wrapped card selection | 1–9 takes card immediately | Take selected card | — | — |
| 25 Race | — | Increment,2 digits | Add increment | Backspace | — |
| 26 2048 | Move all tiles | — | —; F6 hidden | — | — |
| 27 Sliding | Move blank; boundary stops | — | —; F6 hidden | — | — |
| 28 Lights Out | Wrapped cell selection | 5 also presses | Toggle cell and orthogonal neighbours | — | — |
| 31 Shikaku | Cell; second corner during selection | — | First corner, then confirm rectangle | Remove entire selected rectangle | — |
| 32 Slitherlink | Wrapped selected edge of current orientation | — | Toggle line; X→line | Toggle X; line→X | H / V orientation |
| 33 Black Box | Select cell or ray port | 1 atom, 2 excluded, 0 clear | Cycle mark / fire ray; F6 CHECK | Clear mark | RAYS / CELLS |
| 34 Cryptarithm | Select letter | Assign digit | Check arithmetic | Clear digit | NEXT blank letter |
| 35 Hashi | Select island direction | 2/4/6/8 cycle bridges | Cycle last direction | Clear bridge | CHECK |
| 36 Nonogram | Select cell | 1 fill, 0 cross | Cycle mark | Clear mark | CHECK |
| 37 Reversi | Select square | 5 places disc | Place disc | — | Legal-move dots |
| 38 Net | Select tile | 5 turns clockwise | Turn clockwise | Turn anticlockwise | Lock/unlock |

Grid fixed clues cannot be edited. Numbrix/Magic retain an invalid draft after
range errors; movement cancels it. Partial Magic has fixed clues; FREE does not.
MASTER retains the same input controls. LOGIC HELL is selected only by F3
on the difficulty row; F3 again restores the prior general level, LEFT goes to
MASTER, RIGHT stays. Difficulty and inline settings changes apply only to NEW, not RESUME.
2048 TARGET uses goals512/1024/2048/8192; CLASSIC retains the original endless rules.
Magic MASTER is a4×4 panmagic challenge; RULES lists the extra wrap-diagonal sums.

Strategy21–25 and Reversi37 show a FIRST row with YOU or CPU; new local-two-player games are unavailable. CPU pending hides F6 and rejects game input; MENU/OFF remain common actions. CPU undo restores the whole human+CPU round. MASTER introduces verified tactical starts for21–25.
HARD and MASTER use the same exact CPU; MASTER does not claim a stronger AI.

Shikaku EXIT cancels an unfinished corner selection without leaving play;
otherwise EXIT checkpoints to entry. Invalid rectangles keep the selection.
Slitherlink distinguishes blank clues from0 and stores each shared edge once;
all60/84/144 edges are reachable, including the last boundary edge.

Physical SHIFT+DOT (held or latched SHIFT) produces exactly one '='. Plain DOT
is '.', and each module retains its grammar. F↔D is no longer an equals alias.
The power key produces '^'; the negative-sign key produces '-'. Key-up clears
the original physical key even when its character was transformed.

All visible games count real active thought time without input. Entry, rules,
rules, modals, OS and OFF suspend active time; simple dim does not. Timer/HUD,
CPU and save activity never reset user inactivity. Only actual DOWN/HOLD restores
brightness and resets it. Files are written only at dirty checkpoints (EXIT,
MENU/OFF/APO, switching/replacing a game, result), preference save and explicit
DIAG export. Opening RULES does not start games, consume RNG or write files.
