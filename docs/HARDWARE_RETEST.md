# Physical fx-CG50 retest — HARDWARE TEST REQUIRED

The reported persistent flashing MENU has not been reproduced on a physical
calculator here. Its cause and cure are unconfirmed. Host tests establish a
separate fixed boundary bug (timed transition discarded MENU/OFF), not proof of
the persistent device failure. OFF/ON resumes the supported gint process/world;
it is not equivalent to rear RESET or cold reinitialization.

Use the exact hashes in dist/SHA256SUMS.txt. Back up user archives and retained
legacy files before testing. NUMGAME.g3a is the normal app. NUMGDIAG.g3a appears
as NUM DIAG, internal @NGDIAG, and uses NDARCA/B.dat + NDDIAG.txt. It does not
migrate or overwrite NG saves. Use disposable diagnostic progress for failure
injection; never use another app's files. No SDK/reference changes are needed.

1. Record model, OS version, installed app hash, storage free bytes and cold-vs-
   resumed launch. Open Settings→F3 DIAG; inspect counters, then F2 EXPORT only
   when useful. The log is a bounded RAM ring, not a complete session history.
2. Reproduce the user's route: enter two or more games, make moves, EXIT, open
   RULES/STATS, explicitly RESUME, then MENU. Repeat across all30 visible games
   and all screen classes. Log exact order and whether CASIO MAIN MENU appears.
   If flashing occurs, record request/enter/return counts and export immediately
   from Settings if reachable. Record whether OFF/ON preserves it. Do not label
   rear RESET a fix or take it before collecting recoverable evidence.
3. Follow [diagnostic measurement steps](DIAGNOSTICS.md): RESET, run 1,000 RAM
   fixtures, EXPORT, then repeat separately with real game/MENU transitions and
   heavy operations. Check handles return to0, active timer≤1, and arena live
   usage does not grow with repeated work. Stack low-water is observed project
   sampling, not a proven worst-case bound; SDK/OS/interrupt coverage is incomplete.
   Hardware measured peak remains NOT MEASURED until this procedure is executed.
4. Leave a game60s with no keys: its clock must advance about60s and dim must
   still occur according to timeout. Verify OS30/60/180s and APO10/60min values,
   then unsupported-query fallback60s/10min if possible. Dim must change actual
   brightness, not paint black; restore exact prior brightness on one key and
   before MENU/OFF. A wake key must produce no duplicate move/submission.
5. Test APO immediately before/after a real input; save failure must not trap
   OFF. Resume pending CPU exactly once and terminal statistics exactly once.
   RULES/entry/modals and OS/OFF time must not inflate the game clock. Test RTC
   midnight wrap and long gaps; timestamps spanning a full unobserved day need
   separate evaluation because the clock source is time-of-day modulo24h.
6. Check latched and held SHIFT+DOT, plain DOT, alpha conflict and subsequent
   digits. F↔D must not insert '='. Ordinary AC/ON must not turn off. Hold EXE
   across entry/confirmation, and arrows in menus vs turn games.
7. Inspect normal font, all difficulty choices including full MASTER and separate red HELL, context F3 HELL/MODE, independent row
   focus and value selection, long modes/titles, colored operators and Equation
   feedback. Inspect9×9 notes,6×6 cage clues,8×8 Shikaku/Slitherlink, two-digit
   clues and maximum2048 tiles. Host screenshots are not LCD photographs.
8. Test NEW defaults, digit focus, LEFT/RIGHT clamps and EXE/F6 from settings.
   Cancel NEW without changing saved draft/RNG/settings. RESUME must use the
   saved difficulty/mode. Shikaku EXIT during a corner cancels only selection;
   Slitherlink reaches every edge and saves cursor143 on HARD.
9. On disposable ND data, test full storage, missing partner, individual-record
   corruption, interrupted writes and close failures. One damaged record must
   not erase unrelated games. Normal count is2 archives, each462,032bytes; one
   explicit export adds a fixed text file. Measure migration/startup latency.
10. For release legacy migration, first keep an external backup. Test valid old
    NG00–30 A/B progress, incomplete archive initialization and conflicting old/
    new records. Two archive copies must validate before owned old files are
    removed. Unknown/torn global ownership headers and conflicts are preserved;
    a torn ownership publication may require manual recovery. Never infer that
    every NG-looking filename is owned or disposable.

ASan also remains UNVERIFIED: the current macOS runtime was sampled stuck in
shadow-memory initialization before main. Rerun the host suites with NG_ASAN=ON
on a working runtime. UBSan/SH/package success does not replace either missing
ASan execution or physical acceptance.
