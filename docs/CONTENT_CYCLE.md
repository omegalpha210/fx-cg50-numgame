# NEW and puzzle selection

Bank-backed games already used a seed plus cursor and a coprime affine stride
in v4. The stride visits each bank ordinal exactly once per cycle. V5 keeps
that no-repeat policy for the **current game and difficulty** and saves only
its active mode/level cycle. For example, a 30-problem bank produces all 30
base problems in permuted order, then starts a new permutation. At the boundary
the first new problem is kept distinct from the preceding last problem when
the bank has more than one entry. A one-item bank necessarily repeats. The
former recent-four avoidance could fix the first problem of a five-item bank
across several cycles; the boundary now avoids only the immediately previous
problem. Returning to another game or difficulty need not restore its old
cycle. No permanent cleared-problem bitset or achievement history is stored.

An unfinished run cold-loads its supply seed and cursor, so NEW after resuming
continues the same cycle. Completion clears persistent resume, but F6 NEW or
completion EXE NEW in the still-open app retains the current cycle. Closing
after completion begins a new cycle next launch because completed state is not
persisted. Older saves without cycle metadata avoid repeating their current
bank puzzle on the first NEW when possible.

Runtime-generated games receive a fresh xorshift seed. Where the immediately
preceding same-game snapshot fingerprint matches, generation retries at most
three more seeds; a small content space may still repeat. This is a bounded
best-effort policy, not permanent puzzle tracking. Make Target keeps the chosen
1–1000 target when NEW follows a completed run.

Host tests cover coprime permutations at N=1, 2 and 5, a complete 30-item
application cycle, save/reload in the middle of that cycle and the exhaustion
boundary. Physical content perception and no-repeat UX remain
**HARDWARE TEST REQUIRED**.
