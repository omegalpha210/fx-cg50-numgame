# Beta.6 host workload sample and measurement limits

The current `benchmark_host` was built with strict C11 warnings and UBSan in
normal and diagnostic variants. It ran all 36 visible games, 64
initialization/validation/full-render samples per mode and level (12,672 per
variant), 1,000 RAM-only codec fixtures, and 128 two-copy maximum-undo v5
saves per variant. Timings below are POSIX host observations made while other
verification work was active. They are neither idle-host regressions nor
fx-CG50 BFile/flash measurements.

| Host variant | Largest sampled game initialization | First active v5 save | 128-save p50 / p95 / maximum | Host RSS high-water before → after codec fixtures |
|---|---:|---:|---:|---:|
| Normal | 1.757 ms (Equation Guess) | 0.675 ms | 1.256 / 1.434 / 1.501 ms | 5,570,560 → 5,685,248 B |
| Diagnostic | 1.049 ms (Sudoku) | 1.062 ms | 2.001 / 2.141 / 5.643 ms | 5,586,944 → 5,750,784 B |

The v5 save fixture contains one HELL Sudoku run with full notes and four undo
snapshots. Each exact-length file is 9,366 bytes, or 18,732 logical bytes for
the two recovery copies. The 128 timed saves include existing-copy reads, CRC
and write readback; each explicit cold-load equality check is outside the
timed interval. Both variants reported zero codec-fixture failures, zero
remaining handles, one timer peak, and zero app-owned `malloc` calls. A
separate historical v4 two-copy record measured 20,494 logical bytes in this
same workload, but its timings are not a device-speed comparison.

The diagnostic host probe reported a 19,979-byte sampled stack span and a
10,215-byte codec buffer peak across its instrumented workload. The stack
sample belongs to the host ABI and instrumentation; it is not an SH stack
bound. Host RSS is the process high-water mark, including OS/runtime pages,
and the before/after difference is not a live application heap measurement.
Static SH placement and compiler frame coverage are in the
[memory audit](MEMORY_AUDIT.md). Device arena, heap and stack peaks have not
been measured. Calculator save latency, first file allocation and the
reported MENU flashing remain **HARDWARE TEST REQUIRED**.

Raw outputs: [normal](host-benchmark-beta6-normal.json) and
[diagnostic](host-benchmark-beta6-diagnostic.json). The
[pre-v5 36-game sample](PERFORMANCE_36.md) and
[v5 save benchmark](host-benchmark-v5-normal.json) remain historical evidence.
Running `build-host/benchmark_host` and
`build-host-diagnostic/benchmark_host` again after strict host builds produces
new host samples; CPU load, filesystem cache and OS scheduling can change
their timings.
