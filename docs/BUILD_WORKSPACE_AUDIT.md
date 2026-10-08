# Build workspace inventory and migration

Inventory recorded before any build-directory edits or deletion (2026-09-28). Sizes are exact file-byte sums, not filesystem block usage. `source` is the CMake cache source path, when present. All 57 directories have zero Git-tracked files. `script/doc` indicates a live tool reference / any document reference; historical validation logs remain historical.

| Root directory | MiB | Modified | Kind | Source | ELF/MAP/SU/JSON | Script/doc | Decision |
|---|---:|---|---|---|---|---|---|
| `build` | 3.38 | 2026-09-19T03:18 | ad hoc | — | –/–/2/0 | yes/yes | KEEP root; archive old `guesscalc/`, then delete that subdirectory |
| `build-cg` | 4.50 | 2026-09-27T03:55 | normal | repo | Y/Y/29/1 | yes/yes | REBUILD → DELETE |
| `build-clean-1R7P67` | 2.91 | 2026-09-23T17:42 | normal | repo | Y/Y/24/1 | no/no | DELETE after verification |
| `build-clean-2rqBAW` | 4.61 | 2026-09-27T13:44 | diagnostic | repo | Y/Y/30/1 | no/no | DELETE after verification |
| `build-clean-47sF6P` | 3.41 | 2026-09-22T03:45 | normal | repo | Y/Y/23/1 | no/no | DELETE after verification |
| `build-clean-4YlNHV` | 3.83 | 2026-09-27T01:44 | normal | repo | Y/Y/28/1 | no/no | DELETE after verification |
| `build-clean-4ntkJM` | 3.99 | 2026-09-26T16:36 | diagnostic | repo | Y/Y/28/1 | no/no | DELETE after verification |
| `build-clean-4olqLh` | 3.60 | 2026-09-26T16:43 | normal | repo | Y/Y/28/1 | no/no | DELETE after verification |
| `build-clean-6tYFmR` | 3.41 | 2026-09-22T19:30 | normal | repo | Y/Y/23/1 | no/yes | DELETE after verification |
| `build-clean-7e3wcK` | 4.61 | 2026-09-27T13:30 | diagnostic | repo | Y/Y/30/1 | no/no | DELETE after verification |
| `build-clean-9J7X5x` | 3.23 | 2026-09-23T17:42 | diagnostic | repo | Y/Y/24/1 | no/no | DELETE after verification |
| `build-clean-ABXGzW` | 2.89 | 2026-09-19T03:51 | legacy variant unknown | repo | Y/Y/16/1 | no/no | DELETE after verification |
| `build-clean-ByUU5c` | 0.16 | 2026-09-19T03:51 | legacy variant unknown | repo | –/–/0/0 | no/no | DELETE after verification |
| `build-clean-CSSpzv` | 3.14 | 2026-09-22T03:35 | legacy variant unknown | repo | Y/Y/21/1 | no/yes | DELETE after verification |
| `build-clean-Cpkfd4` | 2.95 | 2026-09-19T04:30 | legacy variant unknown | repo | Y/Y/17/1 | no/no | DELETE after verification |
| `build-clean-CtbNxM` | 3.41 | 2026-09-22T03:54 | normal | repo | Y/Y/23/1 | no/no | DELETE after verification |
| `build-clean-F95fR4` | 4.14 | 2026-09-27T00:13 | diagnostic | repo | Y/Y/28/1 | no/no | DELETE after verification |
| `build-clean-HxzcUn` | 3.60 | 2026-09-26T16:47 | normal | repo | Y/Y/28/1 | no/no | DELETE after verification |
| `build-clean-IHyDjN` | 3.86 | 2026-09-23T20:13 | diagnostic | repo | Y/Y/28/1 | no/no | DELETE after verification |
| `build-clean-JRyEFE` | 4.19 | 2026-09-27T13:44 | normal | repo | Y/Y/30/1 | no/no | DELETE after verification |
| `build-clean-KvCuBo` | 3.14 | 2026-09-22T03:34 | legacy variant unknown | repo | Y/Y/21/1 | no/no | DELETE after verification |
| `build-clean-OGXPCK` | 3.41 | 2026-09-22T03:51 | diagnostic | repo | Y/Y/23/1 | no/no | DELETE after verification |
| `build-clean-OLwPQh` | 4.88 | 2026-09-27T04:09 | diagnostic | repo | Y/Y/29/1 | no/no | DELETE after verification |
| `build-clean-SxgA2O` | 4.18 | 2026-09-27T13:22 | normal | repo | Y/Y/30/1 | no/no | DELETE after verification |
| `build-clean-TT8rqv` | 3.41 | 2026-09-22T03:58 | diagnostic | repo | Y/Y/23/1 | no/yes | DELETE after verification |
| `build-clean-UhdJt2` | 4.18 | 2026-09-27T13:30 | normal | repo | Y/Y/30/1 | no/no | DELETE after verification |
| `build-clean-UmJyMz` | 4.46 | 2026-09-27T04:07 | normal | repo | Y/Y/29/1 | no/no | DELETE after verification |
| `build-clean-Ur0IdI` | 4.03 | 2026-09-26T22:45 | diagnostic | repo | Y/Y/28/1 | no/no | DELETE after verification |
| `build-clean-VwP98j` | 3.41 | 2026-09-22T03:51 | normal | repo | Y/Y/23/1 | no/no | DELETE after verification |
| `build-clean-WHV2SZ` | 3.99 | 2026-09-26T16:43 | diagnostic | repo | Y/Y/28/1 | no/no | DELETE after verification |
| `build-clean-XdJUWC` | 4.19 | 2026-09-27T21:48 | normal | repo | Y/Y/31/1 | no/no | DELETE after verification |
| `build-clean-b22Kr5` | 3.60 | 2026-09-26T16:36 | normal | repo | Y/Y/28/1 | no/no | DELETE after verification |
| `build-clean-bKBgAx` | 4.20 | 2026-09-27T21:53 | normal | repo | Y/Y/31/1 | no/yes | ARCHIVE EVIDENCE → DELETE |
| `build-clean-bOhY18` | 1.10 | 2026-09-22T03:13 | legacy variant unknown | repo | –/–/14/1 | no/no | DELETE after verification |
| `build-clean-bQd5Ja` | 3.54 | 2026-09-23T20:13 | normal | repo | Y/Y/28/1 | no/no | DELETE after verification |
| `build-clean-bmctMS` | 3.00 | 2026-09-19T08:26 | legacy variant unknown | repo | Y/Y/19/1 | no/yes | DELETE after verification |
| `build-clean-btnmFD` | 4.24 | 2026-09-27T01:44 | diagnostic | repo | Y/Y/28/1 | no/no | DELETE after verification |
| `build-clean-cuaKgj` | 4.88 | 2026-09-27T04:01 | diagnostic | repo | Y/Y/29/1 | no/no | DELETE after verification |
| `build-clean-d63zkZ` | 4.62 | 2026-09-27T21:53 | diagnostic | repo | Y/Y/31/1 | no/yes | ARCHIVE EVIDENCE → DELETE |
| `build-clean-eDJ0ah` | 3.62 | 2026-09-26T22:45 | normal | repo | Y/Y/28/1 | no/no | DELETE after verification |
| `build-clean-f8ebr6` | 3.09 | 2026-09-22T03:16 | legacy variant unknown | repo | Y/Y/21/1 | no/yes | DELETE after verification |
| `build-clean-gdvSH1` | 4.46 | 2026-09-27T04:24 | normal | repo | Y/Y/29/1 | no/no | DELETE after verification |
| `build-clean-h5j1xu` | 2.06 | 2026-09-27T01:43 | normal | repo | –/–/28/1 | no/no | DELETE after verification |
| `build-clean-iSClsj` | 4.61 | 2026-09-27T13:23 | diagnostic | repo | Y/Y/30/1 | no/no | DELETE after verification |
| `build-clean-iqhCfT` | 3.73 | 2026-09-27T00:13 | normal | repo | Y/Y/28/1 | no/no | DELETE after verification |
| `build-clean-jUhvz0` | 3.99 | 2026-09-26T16:47 | diagnostic | repo | Y/Y/28/1 | no/no | DELETE after verification |
| `build-clean-joI5Ag` | 3.41 | 2026-09-22T03:54 | diagnostic | repo | Y/Y/23/1 | no/no | DELETE after verification |
| `build-clean-mjPa3Z` | 4.88 | 2026-09-27T04:24 | diagnostic | repo | Y/Y/29/1 | no/no | DELETE after verification |
| `build-clean-wjrIzp` | 3.73 | 2026-09-27T00:03 | normal | repo | Y/Y/28/1 | no/no | DELETE after verification |
| `build-clean-yPceB1` | 3.09 | 2026-09-22T03:14 | legacy variant unknown | repo | Y/Y/21/1 | no/no | DELETE after verification |
| `build-grids-audit` | 4.12 | 2026-09-26T22:32 | ad hoc | — | –/–/4/2 | no/no | ARCHIVE EVIDENCE → DELETE |
| `build-host` | 227.94 | 2026-09-27T22:05 | normal | tests | –/–/3/27 | yes/yes | REBUILD / ARCHIVE NOTES → DELETE |
| `build-host-asan` | 19.10 | 2026-09-27T04:01 | normal | tests | –/–/0/1 | no/yes | ARCHIVE SOURCE → DELETE |
| `build-host-beta4-root` | 78.64 | 2026-09-26T23:58 | normal | tests | –/–/0/3 | no/no | ARCHIVE NOTES → DELETE |
| `build-host-diagnostic` | 101.20 | 2026-09-27T04:30 | diagnostic | tests | –/–/0/3 | yes/yes | ARCHIVE NOTES → DELETE |
| `build-mastery-diag-preliminary` | 3.59 | 2026-09-23T17:29 | diagnostic | repo | Y/Y/24/3 | no/no | ARCHIVE EVIDENCE → DELETE |
| `build-mastery-preliminary` | 3.21 | 2026-09-23T17:29 | normal | repo | Y/Y/24/3 | no/no | ARCHIVE EVIDENCE → DELETE |

Baseline: **57 root build-related directories, 650,883,599 B (620.73 MiB)**. Root `build/` is the only path to retain; its old `guesscalc/` contents require preservation/cleanup before the standard subdirectories are final.

Reference audit: `.local/build-path-references.txt` records 169 matching lines before edits. Active tools use `build-host`, `build-host-diagnostic`, `build-clean-XXXXXX` and `build-cg`. `CMakeLists.txt` has no fixed build path. Old CMake caches contain absolute build/source paths and will **not** be moved or reused. Historical validation logs and past audit prose remain as evidence; current commands/generators will be updated.

Unique candidate evidence requiring review before deletion: beta.8 normal/diagnostic ELF, MAP, compiler `.su`/`.ci`, current memory metrics; preliminary mastery memory JSON/logs; beta.3 grid audit replay/ownership JSON; `build/guesscalc/sample.txt`. Other host files include renderer captures, benchmarks, test logs and binaries: compare with published `docs/captures` and `docs/host-benchmark-*.json`, retaining only non-reproducible evidence. The `dist/`, `assets/`, `docs/captures/` and `.local/` trees are outside cleanup scope.

## Current path policy

`bash tools/test.sh` uses one strict C11/UBSan host tree at `build/host/`.
`bash tools/clean_build.sh` and `bash tools/clean_build.sh --diagnostic`
freshly configure `build/clean/normal/` and `build/clean/diagnostic/`.
The normal wrapper `tools/build.sh` calls the same clean build, so it no longer
creates `build-cg/`. The clean script checks its exact destination, refuses
symlinked parents and unmarked pre-existing directories, and removes only its
own generated target before configuration. `build/host/diagnostic/` is an
optional host configuration for instrumented renderer/benchmark reproduction;
its configure command is recorded in `PERFORMANCE_AUDIT.md`.

`build/archive/mastery/` is local ignored historical evidence, never a build
input. Its `MANIFEST.json` records original relative paths, byte sizes and
SHA256 for copied evidence. The beta.8 normal and diagnostic directories
contribute ELF, linker MAP, `.su`/`.ci`, memory metrics, published checksum
list and toolchain metadata. Preliminary mastery runs contribute ELF/MAP and
memory JSON/logs. Grid beta.3 contributes replay/ownership records; early
Guess/Calc and host audit folders contribute otherwise unique source snippets
and small logs. The two host benchmark JSON files already have byte-identical
copies in `docs/`, so no duplicate archive copy is needed. Full CMake caches,
object files, dependency files and raw renderer PPMs are omitted.

Current commands in README, memory method, performance report, gallery and
capture tools use the new paths. Older validation transcripts and past audit
commands retain their original paths as historical records, not current build
instructions. The source/test/build scripts contain no hard-coded personal
workspace absolute path. `build/` remains ignored by Git; `dist/` remains the
user-facing package directory. No game source, generated content bank or save
file is part of this migration.

## Validation and cleanup

Before deletion, the fresh `build/host/` run passed **28/28** strict C11/UBSan
tests. `tools/verify_content.sh` passed its complete legacy/current content
checks, including the original 8,000-record banks and the current 4,000-record
Make Target bank. Its logical SHA256 remained
`1322ea82e3849b1bc38e5d831e7ddd3d02275a0bd414a86b440d2b23065c46ff`.
Normal and diagnostic SH builds linked successfully with strict warnings;
G3A validation passed **16** and **15** checks respectively. The new normal
host renderer's 357 current and 57 beta.6 PPMs were byte-identical to the
corresponding old captures. A separate optional diagnostic host configuration
compiled and passed the renderer capture test; it was then removed because
the report's regeneration commands can recreate it on demand.

The final deletion gate checked the exact 56-name list in the inventory table:
every target was a direct root child, a real directory (no symlink), ignored
with no Git-tracked files, and free of active tool/test/build references. All
240 copied archive entries passed SHA256/size verification; this includes both
beta.8 ELF/MAP pairs, grid replay, the early Guess/Calc sample and two host
audit C snippets. The 56 old root directories, old `build/guesscalc/` and the
new optional `build/host/diagnostic/` were then deleted. No other build, source,
asset, capture, save or distribution path was deleted. The full list of deleted
root names is the table above: every `build-*` row; `build/` itself is retained.

After cleanup, two consecutive cycles each ran a fresh normal SH build, a
fresh diagnostic SH build, **28/28** host tests with UBSan, and both memory/
stack audits. The normal ELF SHA256 stayed
`fedb70333f435599b9ba4d292c7f348b5b9a5a563a136d0f4fabca7164cc9317`;
the diagnostic ELF stayed
`a413b23597df7ad300939af1d7bfbe6e6f17a8d84e6033f808fcf2fe782880e4`.
The two cycles had the same 151 `build/` subdirectories. Root contained only
`build/` after each cycle: no `build-clean-*`, `build-host*` or other random
build directory reappeared. Both memory reports passed the 2,048 B compiler
frame gate and package-payload comparison; device peaks remain unmeasured.

| Package | Published beta.8 bytes / SHA256 | Last rebuilt bytes / SHA256 | Full byte equality | Executable payload equality |
|---|---|---|---|---|
| Normal | 558,236 / `94dff9816330da84a4ba8843d2aa3ad2959c7fc369cf7059cba9bfbf6232f649` | 558,236 / `16353e01605cf8f2b1fc205fd1833d3732266188867916d3e4121af1645b5ef0` | No | Yes |
| Diagnostic | 585,900 / `bed9b7fe9cd608b993db86e937f4d3f8c116e265b2a85a678ce728b1feefc2b7` | 585,900 / `4c0e5d7260d7593055c08b08a5583ac762fbdb4f03b3af4b4752cb3fe88b8037` | No | Yes |

Only header bytes and the final checksum byte differ; the full G3A executable
payload from offset `0x7000` to the four-byte trailer is identical. Repeating
the build changed those container metadata bytes again, while ELF hashes
remained fixed. The original published `dist/` files and checksum list were
restored byte-for-byte. They passed G3A validation and `shasum -a 256 -c`;
the regenerated memory reports match those exact published packages.

## Disk accounting

Sizes below sum file lengths, excluding directory entries; allocated bytes
sum filesystem blocks. The same method was used before and after cleanup.

| Area | Logical bytes | Logical MiB | Allocated bytes |
|---|---:|---:|---:|
| Before: 57 root build-related directories | 650,883,599 | 620.73 | 687,046,656 |
| After: all of `build/` | 212,678,182 | 202.83 | 242,151,424 |
| `build/host/` | 194,467,880 | 185.46 | 222,527,488 |
| `build/clean/normal/` | 4,398,896 | 4.20 | 4,812,800 |
| `build/clean/diagnostic/` | 4,850,056 | 4.63 | 5,242,880 |
| `build/archive/mastery/` | 8,961,350 | 8.55 | 9,568,256 |

Logical saving: **438,205,417 B (417.90 MiB; 67.32%)**. Allocated-block
saving: **444,895,232 B (424.29 MiB; 64.75%)**. The host tree is still the
largest area because it holds the current renderer PPM captures and test
executables; keeping one current host tree makes those tests repeatable.
