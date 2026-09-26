# Number Baseball beta.6 attempt and history audit

| Difficulty | Digits | New total attempts | Last-chance warning after failed attempt |
| --- | ---: | ---: | ---: |
| EASY | 4 | 20 | 19 |
| NORMAL | 5 | 30 | 29 |
| HARD | 6 | 40 | 39 |
| MASTER | 7 | 50 | 49 |

The attempt limit includes the final guess. An invalid-length submission leaves
the count unchanged. Each accepted guess is recorded before the count rises;
a correct guess on the final attempt wins. A wrong final guess loses and keeps
the secret and all guesses available in the result view. The warning is a
durable phase: `0` before the penultimate miss, `1` while confirmation is
pending, `2` after EXE or EXIT acknowledges it. Acknowledgement consumes no
attempt, and returning to a saved run does not create another chance. After
VIEW RESULT, EXIT commits the inactive save and clears the completed secret
and guesses from application RAM; the next NEW derives a new seed.

Revision 4 uses 50 `int32_t` slots in the already encoded `NgGame.data` array:
`data[2+i]` contains a fixed-width numeric guess in bits 0–23, strikes in
bits 24–27, and balls in bits 28–31. The run's 4–7 digit width restores
leading zeroes exactly. Validation reconstructs each guess, recomputes its
S/B against the saved secret, rejects impossible records and nonzero unused
slots, and checks the win/loss and warning state. These 200 bytes fit inside
the existing 512-byte module data area; the common 16×40-byte history array
and serialized `NgGame` length remain unchanged. Revisions 1–3 retain their
original string history, rules, and 16/12/10/16 limits.

The focused host test exercises every level's final loss and final win,
warning acknowledgement, the 50th slot, 51st-attempt rejection, bounded
scroll with a live input draft, a conditional rail with top/bottom arrow
colors, leading-zero/repeated-digit records, mutated record rejection,
save/load round trips, and legacy revision limits. Hardware
input timing and LCD legibility still require a calculator retest.

The app-path test also checks that MENU saves a pending warning, F1 RESUME
reopens it, the held submit key cannot leak through the dialog, EXE/F6/EXIT
acknowledge without spending the final attempt, and an acknowledged warning
stays dismissed after another cold RESUME.
