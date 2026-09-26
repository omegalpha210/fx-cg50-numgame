# Historical 36-game host performance sample (pre-v5 save)

The earlier `benchmark_host` measured all **36 visible games**, 12,672 initialization/validation/render samples per build, 1,000 RAM-only codec fixtures, and 128 maximum-undo v4 compact saves. This is a historical host POSIX/filesystem-cache sample, **not beta.6 or fx-CG50 BFile/flash timing**. On-device creation delay and the prior MENU symptom still require physical testing.

| Host build | Largest sampled initialization | First maximum-size compact save | 128-save p50 / p95 / maximum |
|---|---:|---:|---:|
| Normal | 2.533 ms (Wythoff) | 0.707 ms | 1.318 / 1.409 / 1.486 ms |
| Diagnostic | 3.456 ms (Reversi) | 2.312 ms | 2.173 / 2.275 / 2.507 ms |

The save fixture uses one HELL Sudoku state with full notes and four undo snapshots. Its encoded record is **10,247 bytes**, with two copies totaling **20,494 logical bytes**. The 128 timed writes include existing-copy reads, CRC and readback verification; the cold-load equality assertion is outside the timed interval. Both runs had zero RAM-fixture failures and zero remaining handles. The JSON retains the older field names `archive_files` and `total_archive_bytes` for compatibility; their values now describe compact save files, not the former fixed archives.

The [normal JSON](host-benchmark-36-normal.json) and [diagnostic JSON](host-benchmark-36-diagnostic.json) are the preserved raw outputs. Rerunning the present `benchmark_host` measures the current source and writes a different v5 checkpoint schema; it will not reproduce these numbers. Measurements depend on the host, load and filesystem cache; no regression threshold or hardware speedup is inferred. The [previous 30-game audit](PERFORMANCE_AUDIT.md) is also a historical baseline.
