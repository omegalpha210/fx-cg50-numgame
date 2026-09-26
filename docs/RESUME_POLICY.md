# One unfinished run

NUM GAME stores only the last **unfinished** gameplay run. Main F1 RESUME is
visible only while such a run exists and opens it directly. The matching game's
entry also has a RESUME row above NEW GAME, focused by default. Other games'
entries have no RESUME row, and browsing menus never replaces the saved run.
Only the matching game tile has a small RESUME badge; no category tile or
previously completed game gains one. New-run replacement moves the badge, and
completion or an invalid/corrupt saved run removes it. Main F1 and the badge
both describe the same logical active run.

NEW GAME generates a new valid run and commits settings plus that run to the
alternate A/B file before it becomes the sole logical resume. It does not ask
the player to discard the previous game. A failed replacement leaves the last
valid A/B copy available and offers RETRY. The two physical files are recovery
copies of **one** resume. Saving can be slow on real flash; host timing is not a
device latency claim.

On win, loss or draw, the resume flag is cleared in the same settings/run
transaction. The terminal board remains in RAM. The completion dialog offers
EXE NEW and EXIT VIEW RESULT. VIEW RESULT freezes gameplay input; F6 NEW starts
another run at the completed run's difficulty/mode (including its Make Target
target), while EXIT returns to that game's entry. A cold launch after completion
has no F1 RESUME and no completed board. Keyboard barriers prevent one held
EXIT from closing both the dialog and the result view.

Nonogram now completes when the filled cells match precisely, whether the
other cells are blank or X. Blank and X remain distinct in unfinished saved
progress. Newly started Magic Square FREE and 4×4 Lights Out use supply revision
3. Existing revision-1/2 active runs keep their original board and rule
semantics through cold RESUME and INIT; NEW starts the current revision.

New Countdown, Sequence, Prime Factor, Sliding Puzzle and Make Target runs use
content revision 4. An unfinished earlier-revision run retains its saved
puzzle, settings and progress on RESUME; INIT reconstructs that revision's
original puzzle. NEW starts revision 4. Changing Make Target's committed target
starts a fresh shuffle cycle for that target. The v5 wire layout and the
one-unfinished-run policy are unchanged.

Validated v4 recent-five saves migrate the first valid unfinished run in the
saved recency list. If that entry is corrupt or completed, the next valid
unfinished entry is chosen. Older formats without recency use the stored last
game first, then the highest save generation with deterministic ties. Settings
are retained. Both v5 copies are written and verified before owned old files
are removed; an interrupted migration retains old sources for retry. The
[wire layout and sizes](STORAGE_FORMAT.md) and [hardware retest](HARDWARE_RETEST.md)
separate host evidence from physical flash behavior.
