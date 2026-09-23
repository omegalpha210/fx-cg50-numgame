# Host performance audit

> Historical 30-game baseline measurement. The current 36-game catalog, controls, and compact storage are documented in [README](../README.md), [GAME_CATALOG](GAME_CATALOG.md), and [STORAGE_FORMAT](STORAGE_FORMAT.md). The figures below have not been remeasured for the current build.

**Physical fx-CG50 latency, stack peak, heap/arena peak, and timeout/fallback frequency: NOT MEASURED — HARDWARE TEST REQUIRED.** The following numbers are measured host observations. No host-to-SH conversion or device target achievement is inferred.

Two completed benchmark JSON inputs cover **30 visible games**, **213 mode/level configurations**, and **13,632 initial-load samples per build** (64 per configuration). Normal means diagnostics disabled, not an optimized native release. The largest measured normal load sample is **3.498000 ms** (02 EQUATION GUESS); the diagnostic maximum is **3.213000 ms** (20 SUM GRID). These are maxima of the sampled workload, not global worst-case bounds.

The completed RAM fixture reports zero failures in both builds; remaining benchmark handle counters are zero and the modeled timer count is one. Coverage of those counters is described below.

## Inputs, build and measurement scope

| Input | SHA256 |
| --- | --- |
| Normal host JSON | 3e1444c7ccb2e7ee6cbe742f63ece1719be2a94520152ff11af7e53fe2761e0c |
| Diagnostic host JSON | 2485b6095f9b9f73e42d8e7c49b978498c559486eb55b14985be7851ab53c685 |

| Build evidence | Normal | Diagnostic |
| --- | --- | --- |
| CMAKE_BUILD_TYPE | (empty/default) | (empty/default) |
| NG_ASAN | OFF | OFF |
| NG_DIAGNOSTIC | OFF | ON |
| NG_SANITIZE | ON | ON |
| adjacent_binary_sha256 | 963c53cf5942ecf8eef623ece5d82d266f4b5380b804099684e20bfcba86b062 | 5f5c91e990fff362c35fc0a2f0aa2115e2e1c664ad4b4ed8ecddc680fac6c0f2 |
| association | Adjacent build files observed at report generation; timing JSON does not embed build identity. | Adjacent build files observed at report generation; timing JSON does not embed build identity. |
| benchmark_flags | C_DEFINES = ; C_FLAGS = -std=gnu11 -arch arm64 -Wall -Wextra -Werror -g -UNDEBUG -fsanitize=undefined -fno-sanitize-recover=all -fno-omit-frame-pointer | C_DEFINES = -DNG_DIAGNOSTIC; C_FLAGS = -std=gnu11 -arch arm64 -Wall -Wextra -Werror -g -UNDEBUG -fsanitize=undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -finstrument-functions |
| core_flags | C_DEFINES = ; C_FLAGS = -std=gnu11 -arch arm64 -Wall -Wextra -Werror -g -UNDEBUG -fsanitize=undefined -fno-sanitize-recover=all -fno-omit-frame-pointer | C_DEFINES = -DNG_DIAGNOSTIC; C_FLAGS = -std=gnu11 -arch arm64 -Wall -Wextra -Werror -g -UNDEBUG -fsanitize=undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -finstrument-functions |

Normal: debug symbols present, assertions explicitly enabled (-UNDEBUG), UBSan enabled, abort on sanitizer failure, no optimization flag, no function-entry instrumentation flag. Diagnostic: debug symbols present, assertions explicitly enabled (-UNDEBUG), UBSan enabled, abort on sanitizer failure, no optimization flag, function-entry instrumentation present. The table reports CMAKE_BUILD_TYPE separately; empty/default is not a configured CMake Debug or Release build. ASan settings are stated only when captured. Diagnostic enables extra project function-entry sampling and counters. If the timing JSON is moved without build metadata, flags/binary identity are NOT CAPTURED; pass --build-evidence with captured metadata to reproduce that part.

Timing uses `clock_gettime(CLOCK_MONOTONIC)` around synchronous operations in one warm process. The host renderer writes every rectangle to a 396×224 RGB565 memory buffer; it does not refresh an LCD. Native gint/OS calls, keyboard latency, MENU/OFF, physical flash, OS scheduling on the calculator and hardware timer delivery are outside the timing scope. The JSON does not record host CPU model, clock frequency, compiler version, source revision or run timestamp; do not invent them. Adjacent binary hashes are report-time association, not proof of the binary that produced an older JSON.

The runner sorts each sample set. p50 is element `(n-1)//2` (the lower median for even n), p95 is nearest rank `ceil(0.95*n)-1`, and max is the last element. Values here retain six decimal places in milliseconds; extra printed zeros do not increase the precision of an input that was rounded earlier. Raw per-sample data and per-configuration percentiles are not stored, so they cannot be reconstructed or pooled from these summaries.

## All 30 games: initialization, validation and first rendering

`ng_new` (module initialization) + semantic `ng_valid` + complete common RGB565 renderer are inside each sample. Level and mode samples are mixed per game. The runner initializes deterministic seeds directly; this is not entry UI navigation or a user NEW bag-cycle benchmark. CLASSIC 2048 still appears in each raw level slot here even though the app hides its level selector. First-use samples are included in each process, but there is no separate cold-start/cache-eviction experiment.

| ID / game | Samples / configs | Normal p50 / p95 / max ms | Diagnostic p50 / p95 / max ms | Diagnostic / normal p50 |
| --- | --- | --- | --- | --- |
| 01 NUMBER BASEBALL | 1536 / 24 | 0.536000 / 0.568000 / 0.663000 | 0.630000 / 0.691000 / 2.118000 | 1.175× |
| 02 EQUATION GUESS | 768 / 12 | 0.575000 / 0.629000 / 3.498000 | 0.703000 / 0.759000 / 2.357000 | 1.223× |
| 03 NUMBER MIND | 256 / 4 | 0.599000 / 0.631000 / 0.682000 | 0.751000 / 0.808000 / 0.839000 | 1.254× |
| 04 CLUE LOCK | 256 / 4 | 0.573000 / 0.605000 / 0.674000 | 0.710000 / 0.774000 / 0.976000 | 1.239× |
| 05 SEQUENCE DETECTIVE | 256 / 4 | 0.652000 / 0.703000 / 0.736000 | 0.759000 / 0.817000 / 0.890000 | 1.164× |
| 06 MAKE TARGET | 512 / 8 | 0.599000 / 0.631000 / 0.692000 | 0.695000 / 0.750000 / 0.855000 | 1.160× |
| 07 COUNTDOWN | 256 / 4 | 0.638000 / 0.688000 / 2.059000 | 0.777000 / 0.860000 / 0.962000 | 1.218× |
| 08 MISSING OPERATORS | 256 / 4 | 0.561000 / 0.596000 / 0.659000 | 0.689000 / 0.745000 / 0.804000 | 1.228× |
| 09 CROSS MATH | 256 / 4 | 0.562000 / 0.597000 / 0.643000 | 0.682000 / 0.759000 / 0.839000 | 1.214× |
| 10 PRIME FACTOR | 256 / 4 | 0.601000 / 0.643000 / 0.699000 | 0.764000 / 0.831000 / 0.938000 | 1.271× |
| 11 SUDOKU | 320 / 5 | 0.721000 / 0.767000 / 0.848000 | 0.912000 / 0.989000 / 1.680000 | 1.265× |
| 12 CALCUDOKU | 320 / 5 | 0.692000 / 0.769000 / 2.011000 | 0.825000 / 0.984000 / 1.230000 | 1.192× |
| 13 KAKURO | 320 / 5 | 0.669000 / 0.707000 / 0.784000 | 0.803000 / 0.880000 / 1.012000 | 1.200× |
| 14 FUTOSHIKI | 320 / 5 | 0.614000 / 0.661000 / 0.733000 | 0.734000 / 0.782000 / 0.864000 | 1.195× |
| 15 SKYSCRAPERS | 320 / 5 | 0.616000 / 0.647000 / 0.697000 | 0.747000 / 0.798000 / 0.865000 | 1.213× |
| 16 HITORI | 256 / 4 | 0.677000 / 0.720000 / 0.784000 | 0.826000 / 0.878000 / 0.996000 | 1.220× |
| 17 BINARY PUZZLE | 256 / 4 | 0.666000 / 0.699000 / 0.737000 | 0.808000 / 0.856000 / 0.933000 | 1.213× |
| 18 NUMBRIX | 256 / 4 | 0.676000 / 0.717000 / 0.775000 | 0.795000 / 0.853000 / 0.942000 | 1.176× |
| 19 MAGIC SQUARE | 512 / 8 | 0.626000 / 0.678000 / 0.724000 | 0.718000 / 0.793000 / 0.863000 | 1.147× |
| 20 SUM GRID | 256 / 4 | 0.666000 / 0.722000 / 0.782000 | 0.860000 / 1.025000 / 3.213000 | 1.291× |
| 21 NIM | 768 / 12 | 0.603000 / 0.646000 / 0.700000 | 0.671000 / 0.739000 / 0.814000 | 1.113× |
| 22 WYTHOFF | 768 / 12 | 0.597000 / 0.633000 / 0.701000 | 0.660000 / 0.720000 / 0.785000 | 1.106× |
| 23 EUCLID | 768 / 12 | 0.560000 / 0.597000 / 0.859000 | 0.624000 / 0.680000 / 2.102000 | 1.114× |
| 24 MAKE FIFTEEN | 768 / 12 | 0.553000 / 0.596000 / 2.017000 | 0.618000 / 0.702000 / 2.459000 | 1.118× |
| 25 RACE | 768 / 12 | 0.478000 / 0.511000 / 1.411000 | 0.530000 / 0.588000 / 0.675000 | 1.109× |
| 26 2048 | 512 / 8 | 0.539000 / 0.576000 / 0.663000 | 0.590000 / 0.629000 / 0.738000 | 1.095× |
| 27 SLIDING | 512 / 8 | 0.590000 / 0.630000 / 0.709000 | 0.667000 / 0.730000 / 0.811000 | 1.131× |
| 28 LIGHTS OUT | 512 / 8 | 0.594000 / 0.643000 / 0.707000 | 0.664000 / 0.741000 / 0.823000 | 1.118× |
| 31 SHIKAKU | 256 / 4 | 0.629000 / 0.661000 / 0.735000 | 0.723000 / 0.759000 / 0.812000 | 1.149× |
| 32 SLITHERLINK | 256 / 4 | 0.524000 / 0.558000 / 0.610000 | 0.639000 / 0.684000 / 0.748000 | 1.219× |

The median of the **30 per-game p50 ratios** is 1.194×; their range is 1.095×–1.291×. This is not a pooled runtime percentile. The two runs were not paired/interleaved trials with confidence intervals, so the comparison includes warm-cache, scheduling and measurement variation as well as diagnostic work; it is not a causal isolation of instrumentation cost.

## Selected high-level action workloads

Each row has 128 samples. Setup/initial render occurs before the timed interval. LOGIC 11–15 uses HELL; other rows use MASTER. CPU games use CPU FIRST (mode 1), execute the one pending exact CPU move, then validate/render. Hint games request one supported HINT, then validate/render. Slitherlink has neither a CPU nor hint flag, so its timed row is **initial-state validation and rendering only**. This does not benchmark a completed loop or a difficult edge edit. A reveal/first-step hint is not a runtime uniqueness search, and these selected starts do not certify a global worst case.

| ID / game | Timed action before validate/render | n each | Normal p50 / p95 / max ms | Diagnostic p50 / p95 / max ms | p50 ratio |
| --- | --- | --- | --- | --- | --- |
| 07 COUNTDOWN | one HINT | 128 | 0.630000 / 0.679000 / 0.734000 | 0.741000 / 0.787000 / 0.828000 | 1.176× |
| 11 SUDOKU | one HINT | 128 | 0.707000 / 0.764000 / 0.843000 | 0.875000 / 0.928000 / 0.966000 | 1.238× |
| 12 CALCUDOKU | one HINT | 128 | 0.690000 / 0.742000 / 0.778000 | 0.820000 / 0.865000 / 0.894000 | 1.188× |
| 13 KAKURO | one HINT | 128 | 0.674000 / 0.705000 / 0.736000 | 0.819000 / 0.856000 / 0.883000 | 1.215× |
| 14 FUTOSHIKI | one HINT | 128 | 0.610000 / 0.666000 / 0.701000 | 0.726000 / 0.768000 / 0.800000 | 1.190× |
| 15 SKYSCRAPERS | one HINT | 128 | 0.628000 / 0.666000 / 0.720000 | 0.759000 / 0.797000 / 0.861000 | 1.209× |
| 03 NUMBER MIND | one HINT | 128 | 0.613000 / 0.640000 / 0.679000 | 0.775000 / 0.824000 / 2.176000 | 1.264× |
| 18 NUMBRIX | one HINT | 128 | 0.678000 / 0.721000 / 0.762000 | 0.808000 / 1.009000 / 2.200000 | 1.192× |
| 32 SLITHERLINK | none (start state) | 128 | 0.524000 / 0.566000 / 0.613000 | 0.644000 / 0.671000 / 0.742000 | 1.229× |
| 21 NIM | CPU move | 128 | 0.619000 / 0.717000 / 2.386000 | 0.685000 / 0.738000 / 0.871000 | 1.107× |
| 22 WYTHOFF | CPU move | 128 | 0.598000 / 0.640000 / 0.682000 | 0.666000 / 0.705000 / 0.750000 | 1.114× |
| 23 EUCLID | CPU move | 128 | 0.558000 / 0.615000 / 0.668000 | 0.624000 / 0.670000 / 0.698000 | 1.118× |
| 24 MAKE FIFTEEN | CPU move | 128 | 0.558000 / 0.601000 / 0.655000 | 0.634000 / 0.663000 / 0.767000 | 1.136× |
| 25 RACE | CPU move | 128 | 0.479000 / 0.581000 / 0.696000 | 0.528000 / 0.567000 / 0.609000 | 1.102× |

## 1,000 isolated RAM fixtures

`ng_storage_fixture` reuses the existing shared storage session/buffer. Each iteration constructs a seeded game, validates it, performs pending CPU/HINT work, validates again, fills the undo ring where allowed, records a result if applicable, encodes, computes CRC, decodes into the fixture, and re-encodes/compares CRC and length. It performs no fixture filesystem I/O and does not reference the user app session. IDs rotate across all 30 games; levels/modes are scheduled by integer round counters, not a promise of 1,000 samples for every configuration. Rendering is not in this fixture timing.

| Metric | Normal | Diagnostic |
| --- | --- | --- |
| n | 1000 | 1000 |
| p50 / p95 / max ms | 0.849000 / 0.976000 / 1.178000 | 1.539000 / 1.655000 / 1.852000 |
| p50 ratio | 1.000× | 1.813× |
| Failures | 0 | 0 |
| RSS lifetime high-water before B | 5062656 | 5095424 |
| RSS lifetime high-water after B | 5111808 | 5177344 |
| Change in high-water B | 49152 | 81920 |
| Reported app malloc calls / owned heap B | 0 / 0 | 0 / 0 |
| Remaining handles / modeled timers | 0 / 1 | 0 / 1 |
| Fixture filesystem I/O | False | False |

RSS is `getrusage(RUSAGE_SELF).ru_maxrss`, converted to bytes for the host platform. It is a process-lifetime resident high-water mark including runtime, code, pages, sanitizers and allocator effects; two high-water readings do not measure current live heap or prove no leak/continuous growth. The JSON app allocation/owned-heap zeros are source-ownership declarations printed by the runner, **not malloc-interposition measurements of libc/SDK/OS allocations**. The timer count of one is inserted through `NGD_TIMER_START` as a diagnostic model; this benchmark does not allocate or measure a real OS timer. Native adapter/hardware lifecycle evidence belongs to separate tests.

## 128 maximum-undo POSIX archive saves

The fixture uses a HELL Sudoku session with all allowed cell notes and the full four-entry undo ring. It serializes into the current production schema. A fresh temporary namespace creates two fixed archives; no user saves are involved. First creation is one separately reported sample, followed by a second untimed save to establish both copies. Each of 128 timed saves includes the production transaction's existing-copy reads/CRC, writing and readback verification. An explicit cold-load/equality assertion follows every save **outside** its timed interval. The total archive byte field is two times the fixed schema length, not a filesystem block-allocation measurement. These are POSIX adapter/filesystem-cache observations, not native BFile/flash or power-loss durability latency.

| Metric | Normal | Diagnostic |
| --- | --- | --- |
| First archive creation ms (one sample) | 1.387000 | 1.915000 |
| Steady save samples | 128 | 128 |
| p50 / p95 / max ms | 1.584000 / 1.743000 / 1.887000 | 2.621000 / 2.720000 / 2.782000 |
| p50 ratio | 1.000× | 1.655× |
| Encoded payload B | 12779 | 12779 |
| Record including 32-byte header B | 12811 | 12811 |
| Archive files / schema logical bytes | 2 / 924064 | 2 / 924064 |
| Remaining handle counter | 0 | 0 |

## Host ABI and diagnostic counters

| Host field | Normal | Diagnostic |
| --- | --- | --- |
| pointer_bytes | 8 | 8 |
| game_bytes | 1848 | 1848 |
| session_bytes | 12936 | 12936 |
| app_bytes | 15736 | 15736 |
| rectangles | 50272084 | 50272084 |

| Diagnostic-only observation | Value |
| --- | --- |
| Sampled project host-stack excursion B | 18139 |
| Stack samples | 193158784 |
| Deepest recorded game / operation | 28 / validate |
| Maximum codec buffer bytes reported | 12779 |
| Read calls / bytes | 2443 / 9930785 |
| Write calls / bytes | 200 / 1667670 |

These sizes use the host ABI. Diagnostic stack excursion is based on instrumented project function-entry/explicit samples relative to a host reference; it is not continuous sampling of libc, OS, interrupts or every transient inside a function. It is not a native stack bound. Codec high-water is tracked bytes passed through the codec hooks, not process RAM. Read/write counters cover the whole diagnostic run, including archive setup and untimed verification loads; dividing them by 128 would mislabel bytes per timed save. Rectangle count is a rendering-work counter, not a frame rate. Do not add host objects, RSS, sampled stack, framebuffer or linked native sections to invent a simultaneous device total.

## Device budgets and unresolved measurements

Requested device targets remain unverified: preferably bank load within 0.5 s, runtime generation p95 within 2 s, and an approximately 3 s foreground generation hard budget considered as an initial policy. The host milliseconds above do not establish any of them. Native engines use bounded constructions or host-generated banks; no general elapsed-time timeout is implemented in these engines. Missing Operators E/N/H has a 128-attempt construction cap and rule-valid fixed fallback, which is distinct from an elapsed-time timeout. This benchmark does not count construction retries or fallback events. **Timeout/fallback frequency is NOT MEASURED, not zero.** It does not test deadline-triggered selection of a same-difficulty verified bank or silently downgrade HELL.

Record calculator model/firmware, build/package hashes, actual operation and sample counts, foreground display-ready timing, live/peak arena accounting and stack coverage on hardware before asserting those budgets. Diagnostic overhead must be assessed separately on the device. See `docs/MEMORY_METHOD.md` and the hardware acceptance checklist.

Original normal JSON limit statement: No host->SH timing/ABI conversion; RSS highwater includes runtime/pages and is not live heap. App allocation zero is source ownership, not OS allocation coverage. No wall-clock timeout implemented in engines; bounded construction/host banks avoid device search. Hardware latency/fallback budgets unverified.

Original diagnostic JSON limit statement: No host->SH timing/ABI conversion; RSS highwater includes runtime/pages and is not live heap. App allocation zero is source ownership, not OS allocation coverage. No wall-clock timeout implemented in engines; bounded construction/host banks avoid device search. Hardware latency/fallback budgets unverified.

## Regeneration and validation

Run the existing normal and diagnostic host benchmark executables from their respective build directories to create complete JSON files. Do not read a redirected file while its benchmark is still running. Then:

```sh
python3 tools/benchmark_report.py --normal docs/host-benchmark-normal.json --diagnostic docs/host-benchmark-diagnostic.json --build-evidence docs/host-benchmark-build-evidence.json --output docs/PERFORMANCE_AUDIT.md --check
# Rerun the host workloads first to make new measurements:
(cd build-host && ./benchmark_host > host-benchmark.json)
(cd build-host-diagnostic && ./benchmark_host > host-benchmark.json)
python3 tools/benchmark_report.py --output docs/PERFORMANCE_AUDIT.md
```

The three copied JSON inputs in `docs/` preserve the measured summaries and adjacent build-flag evidence for checking this published report. The default inputs for a new measurement are `build-host/host-benchmark.json` and `build-host-diagnostic/host-benchmark.json`. `--source-root` changes that default root. `--build-evidence` accepts `{ "normal": {...}, "diagnostic": {...} }` with captured flag evidence; `--write-build-evidence PATH` exports the currently observed adjacent metadata for portable reproduction. The script rejects truncated JSON, nonfinite/negative/unordered timing values, missing/duplicate game IDs, changed scopes/sample coverage or swapped instrumentation labels. `--check` compares the entire generated document without modifying it. It does not rerun workloads or assert physical performance.
