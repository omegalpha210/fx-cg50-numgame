# Single-resume save format (v5)

NUM GAME retains **one unfinished run for the entire app**. `NGSTATEA.dat` and
`NGSTATEB.dat` are two crash-recovery copies of that one logical state, not two
game slots. NUM DIAG uses `NDSTATEA.dat` and `NDSTATEB.dat` independently. No
per-game save file is created by v5. Starting another run replaces the previous
logical resume after the new transaction validates; completing a run commits an
empty resume while the final board remains visible in RAM. See
[RESUME_POLICY.md](RESUME_POLICY.md) for the UI transitions.

Each file has a 32-byte little-endian header (`NGSAVE01`, record ID 0, version
5, generation, exact payload length, payload CRC32, header CRC32). The payload
contains the length and 89-byte preferences, an active flag, the run length,
and, when active, that unfinished run. Preferences retain legacy wire fields
for migration compatibility but v5 writes the recent list and pending deletion
as zero. The run contains its current game, up to four bounded undo states,
run counter, and **only its current mode/level puzzle cycle**. It does not
serialize cumulative statistics, completed boards or every mode/level cycle.
Integer fields are encoded explicitly; raw C structure padding and pointers
are never stored.

| v5 state | Exact bytes per file | Two copies |
|---|---:|---:|
| Preferences, no unfinished run | 126 | 252 |
| Fresh unfinished run, no undo | 1,998 | 3,996 |
| One undo snapshot | 3,840 | 7,680 |
| Maximum four undo snapshots | 9,366 | 18,732 |

The run payload is `30 + 1842 × (1 + undo_count)` bytes. All six recently added
games start with a 1,998-byte v5 file; later size depends on their actual undo
count. A game without undo stays at 1,998 bytes. File lengths are exact in the
host adapter and requested exactly through BFile on the calculator; physical
filesystem block allocation may be larger.

Before v5, v4 used two 121-byte settings files plus two files **per retained
game**. A no-undo game record was 2,879 bytes; one maximum four-undo record
was 10,247 bytes. Five no-undo games therefore occupied 29,032 logical bytes
across 12 files; the five-game four-undo upper bound was 102,712 bytes. The
host benchmark directly measured a two-copy maximum-undo v4 game at 20,494
bytes versus v5's 18,732 bytes for the same run plus settings. The greater
saving comes from removing the other four logical resumes and their files.
[Previous host benchmark](host-benchmark-36-normal.json) and
[current v5 host benchmark](host-benchmark-v5-normal.json) contain the raw
measurements. Host timings are not calculator flash timings.

To load, the adapter validates both copies' header, CRC, exact length, settings,
game ID, module state and undo relationship. It chooses the newest valid
generation with wrap handling. A failed partial write leaves the other copy
available. Saving replaces only the older copy, closes and reads it back before
reporting success. On startup, validated v4 recent-five settings select the
first valid unfinished run in recency order. Old formats without that list use
last-game preference, then save generation with deterministic ties. Completed
runs are skipped. Preferences and the selected run are written and read back in
both v5 copies before old owned files are retired. A failed migration retains
the old source for retry. Exact old `NG2xxA/B` paths are private to this app;
legacy names are removed only when their record or archive ownership validates.

Checkpoint triggers include START GAME, EXIT, MENU, SHIFT+AC/ON, APO, settings
changes and completion. Game keys are not individually written. Native BFile
transactions run synchronously on the main thread inside `gint_world_switch`.
Host tests cover partial writes, a corrupt latest copy, migration variants and
1,000 game switches with exactly two v5 files, zero final handles and one peak
timer. Actual flash latency, physical block usage and power-cut behavior remain
**HARDWARE TEST REQUIRED**. Older archive details remain in
[STORAGE_ARCHIVE_AUDIT.md](STORAGE_ARCHIVE_AUDIT.md).
