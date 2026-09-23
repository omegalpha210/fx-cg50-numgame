# Save records v3 in fixed v2 archives (v1/v2 readers retained)

Release namespace: `\\fls0\NGARCA.dat` and `NGARCB.dat`. Diagnostic app uses
`NDARCA.dat` and `NDARCB.dat`. Each preallocated archive is 462,032 bytes.
Settings is stable ID0, records1–32 include hidden legacy29/30. Visible game IDs
are1–28,31,32. One invalid record cannot invalidate unrelated records.

The 32-byte global header has magic NGARCH02, version2, slot count33,
stride14,000, total462,032, reserved0, and CRC32 of its first28 bytes. Integer
fields are32-bit little endian at offsets8,12,16,20,24,28. Record ID `i` starts
at `32 + i*14000` and has its own header below. Initialization uses NGINIT02
until all empty record headers have been written; see
[initialization/migration limits](STORAGE_ARCHIVE_AUDIT.md).

All multibyte payload/header values are explicitly little endian. Raw C structs,
pointers, compiler padding and native endianness are never written to disk.

| Header offset | Length | Meaning |
|---:|---:|---|
| 0 | 8 | ASCII `NGSAVE01` |
| 8 | 4 | Stable record ID, 0–32 |
| 12 | 4 | Record version 3 (reader also accepts 1 and 2) |
| 16 | 4 | Unsigned generation counter; serial-number comparison handles wrap |
| 20 | 4 | Exact payload byte length |
| 24 | 4 | CRC32 of payload |
| 28 | 4 | CRC32 of bytes 0–27 |

Settings payload is 67 bytes: last game, 32 difficulties, 32 modes, first-rule
display preference and active-time display preference. Every setting is bounded
against the actual registry.

Game payload starts with undo count, then per-game statistics, bounded supply history, current `NgGame`,
and 0–4 explicitly encoded prior `NgGame` states. Statistics contain started
runs and total active milliseconds plus 8 modes × 5 difficulties × 2 assistance
buckets. Each bucket contains finished/win/loss/draw counters, best score,
minimum winning moves/time, and 2048 maximum tile exponent.

`NgGame` contains ID, difficulty/mode, status/phase, assistance, recorded flag,
turn, dimensions/cursor, history/scroll, notes mode, CPU pending, seed/RNG,
run ID, active milliseconds, moves/score/puzzle ID, supply seed/index, pack revision,
level generation policy, board, fixed flags, notes,
bounded module data, draft, feedback message and bounded attempt history.
One current game encodes 1,842 bytes; C size is 1,848 bytes on the measured host
and SH builds due to padding. A full four-undo record encodes **12,811 bytes including
its header** (12,779 payload bytes). The fixed codec
buffer capacity is 14,000 bytes. Unused RAM padding never enters the file.

Decoder checks exact length, magic/version/CRC/ID, all common bounds, every
module's semantic validator, NUL termination, pack/mode identity, terminal state,
stats relationships and safe tile exponents. Undo entries must be legal playing
pre-human states with the same game/run/seed/puzzle/mode/difficulty, supply identity,
pack revision and generation policy, and no pending
CPU. Unsupported versions are rejected locally; no other game is reset. Version1
63-byte settings and three-level statistics, and version2 four-level statistics,
remain readable. Old stats keep their original level index; missing MASTER/HELL
buckets start empty. MASTER has always meant value3; HELL is new value4.
Old 1,826-byte game records receive pack revision1 and preserve puzzle identity.
Current new games use revision2; old progress is not regenerated on RESUME.
Newly added ID preferences default to NORMAL. Legacy29/30 remains decodable
but cannot be selected or resumed from the public app catalog.

Both slots are probed. The newest valid slot is loaded. A corrupt/truncated
partner produces a recovery notice. Save writes the other slot, closes it,
reopens and fully validates the complete record and expected CRC/generation.
Only then is save reported successful. The old valid slot is retained throughout.
A full write whose close/readback fails may already exist on disk, but is never
reported as a confirmed save. A later cold load independently validates it.

No write occurs for every key. Checkpoints are EXIT, MENU, SHIFT+AC/ON, APO, game
switch, before NEW replacement and terminal result. Statistics opening is
read-only and does not consume RNG or start a game. `recorded` and per-game
run identity prevent repeated result/checkpoint/INIT accounting. INIT keeps the
same run and seed, clears undo, and marks assistance. Undo preserves spent active
time and restores RNG; CPU undo restores the pre-human round.

Native I/O occurs on the main thread inside `gint_world_switch`. Interrupts only
set wake flags. Direct BFile clamps reported reads to the actual file size and
keeps a failed-close handle for bounded cleanup. A failed checkpoint retains
RAM and displays an error; EXIT stays in the run. MENU/OFF are still invoked once
after a failed attempt so power/navigation is not trapped indefinitely.

Time uses actual active deltas, a 64-bit tick conversion and midnight wrap.
OS/off, modal and paused duration are excluded. A cold load uses the stored
active total (and legacy Memory exposure when decoded), not wall-clock elapsed downtime.
A checkpoint taken while CPU is pending preserves the flag; resume executes
the CPU event once and clears the flag after the legal move, preventing
duplicate moves after resume.

Host tests inject unsupported old/future versions with recomputed checksums,
exercise generation wrap, truncation, checksum damage, unsupported bounds, semantic
terminal corruption, invalid undo and statistics, partial/zero writes, readback
corruption, close/read failures and per-game recovery. Native signature-compatible
mocks execute the real main/BFile adapters. Physical power-loss recovery remains
**HARDWARE TEST REQUIRED**.

Migration, ownership checks, conflict preservation, file-count/byte bounds and
physical interruption limitations are specified in [STORAGE_ARCHIVE_AUDIT.md](STORAGE_ARCHIVE_AUDIT.md). Keep both archives and any retained legacy files in backups.

## Supply history and exact wire sizes

Each of 8 modes × 5 levels has a 25-byte supply entry: shuffle seed (u32),
next ordinal/count (u16 each), four recent stable puzzle IDs (u32 each), and
recent count (u8). The 1,000-byte history is stored once per session, not per undo.
Within a bank, a coprime affine permutation visits every ordinal once. A new cycle
rotates away from up to four recent IDs (or bank size minus one for tiny banks).
Bank size changes restart the cycle; saved game IDs remain stable. INIT preserves
the exact puzzle, run and pack revision without consuming the supply cycle.

The current payload length is `3569 + 1842*(1+undo_count)`. Old lengths
`1545 + 1826*(1+undo_count)` and `2057 + 1826*(1+undo_count)` uniquely identify
three- and four-level legacy payloads. Five-level payloads are exact-length
validated; format interpretation never depends on a display label.

A current no-undo record uses 5,443 of its 14,000 reserved bytes (8,557 reserved,
61.12%); a four-undo record uses 12,811 (1,189 reserved, 8.49%). The settings record
uses 99 bytes. With all 32 game IDs at maximum size plus settings and the archive
header, one archive contains 410,083 meaningful bytes and 51,949 reserved bytes
(11.24%). Some modules cannot use undo, so that is a conservative capacity example,
not a claim about a normal player's utilization. Existing archive sizes and offsets
stay unchanged to avoid moving users' records. Writes transfer the encoded record,
not the unused reserved space; initialization clears only the 33 record headers.
