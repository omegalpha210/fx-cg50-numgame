# Memory measurement method

The four evidence classes below are deliberately separate. Linked storage,
individual compiler frames, partial call paths and observed allocator peaks
answer different questions. Adding BSS to the largest frame does **not** produce
a runtime RAM peak. No physical fx-CG50 measurement has yet been supplied.

## A. ELF placement and artifact identity

Build a fresh native target with the installed SH compiler and matching source.
`tools/memory_report.py` reads the final `numgame` ELF using `sh-elf-objdump -h`,
cross-checks each section size against `sh-elf-size -A`, and compares allocated
sections with `numgame.map`. It reports VMA, LMA and section flags separately for
`.text`, `.rodata`, `.data`, `.bss`, `.gint.bss`, ILRAM and other sections present.
Non-allocated debug sections are not runtime storage. Load-section sums can
differ from binary payload size because of alignment gaps.

GNU `size` text/data/BSS aggregates are preserved only for comparison with older
reports. For this linker script, readonly puzzle banks are part of `text`, and
`.gint.drivers` has DATA flags but a ROM address. Interpret actual placement from
section addresses, not the aggregate label.

Pre-link object section totals and selected `nm -S` symbols identify project
BSS, the application/session, transaction probe, codec buffer, one-record grid
cache and readonly packs. Those symbols and object totals overlap linked ELF
storage; do not add them again. The list is selected attribution, not a complete
heap inventory. No whole-bank RAM cache is assumed.

An explicitly supplied G3A is compared byte-for-byte with `objcopy -O binary`
output at its payload range. A mismatched or missing explicitly named artifact
fails the tool. G3A container/checksum checks remain separate in
`tools/verify_g3a.py`; neither check demonstrates calculator execution. Reports
record ELF/map/G3A hashes without publishing raw linker maps, which may contain
local SDK paths.

## B. Conditional compiler stack analysis

The native build emits `-fstack-usage` and `-fcallgraph-info=su`. GCC documents
callgraph output as per-object VCG, with `su` attaching stack-usage information.
These are compiler artifacts, not runtime traces. LTO requires a separate
collection policy because its output can be emitted during linking.
([GCC developer options](https://gcc.gnu.org/onlinedocs/gcc/Developer-Options.html))

The individual-frame gate is 2,048 bytes. Empty report coverage, an over-limit
frame or an unbounded dynamic frame fails that gate. GCC's `static` quantity
covers that function's fixed frame; `dynamic,bounded` also provides a bounded
dynamic amount. A function frame is not its complete transitive call stack.
([GCC stack-usage terminology](https://gcc.gnu.org/onlinedocs/gcc-14.1.0/gnat_ugn/Static-Stack-Usage-Analysis.html))

The scripts accept `.su`/`.ci` only beside an object at least as new as the
report. This rejects reports left by failed compilations, but cannot prove source
identity or linker reachability. Use a clean build, matching sources and the
recorded ELF hash. The compiler may include code later removed at link time.

`tools/stack_callgraph.py` parses direct calls, condenses strongly connected
components, and computes the largest known project contribution from each graph
entry. Acyclic paths sum compiler frames. For the arithmetic parser, independent
source reasoning supplies a recursion policy: accepted atom depth is at most
12, but the rejected thirteenth atom still enters a function frame. Its
expression/term callers also exist. A conservative component allowance is
therefore 13 frames of each of `atom`, `term` and `expression`. The script checks
the current guard and records the source hash; the policy remains conditional on
that source matching the build. It is not an automatic proof of arbitrary edits.
An unknown recursive component or unbounded dynamic frame has no numeric bound.

The JSON lists every unresolved edge. Function-pointer game dispatch, storage
hooks, callbacks, SDK/libc/compiler-runtime calls, assembly, interrupts and OS
frames are not assigned a fictitious complete continuation. A reported path
stops at missing knowledge, and its numeric contribution may omit the dominant
part of the real stack. The script does not connect an indirect edge to possible
callbacks. Compiler inlining is reflected in emitted frames; tail-call frame
reuse is not inferred, so direct sums can overestimate. Library/internal tail
recursion is outside coverage. Even a fully resolved project path is not a
whole-program or physical-device bound.

The parser is self-tested against real VCG syntax, 100 random DAGs checked by
independent exhaustive path enumeration, bounded/unbounded mutual recursion,
unbounded dynamic frames and private-path removal. Section parsing is checked
independently against GNU size and linker map sizes.

## C. Host observations

Host tests and instrumentation report their own platform, workload, completion
count and counters. They can establish functional behavior and host peaks for
the observed workload. Host ABI, allocator behavior and sanitizer overhead are
different from the SH target; those values must not be labeled native peaks.
The static tools above do not themselves run a workload and report host peaks
as **NOT MEASURED BY THIS STATIC TOOL**. ASan remains unverified unless a complete
successful run is recorded; a compiled ASan binary is not a passed ASan test.

Useful workloads include entry/first board, hardest puzzle load and first draw,
undo, hint, invalid input, retry/new/resume, result/rules transitions, codec
roundtrip and damaged-slot recovery, all configured difficulty/mode combinations,
and repeated isolated diagnostic loads. Report the operation, game and sample
count with any maximum. A zero counter means only what the instrumentation
actually covers; it does not prove absence of SDK allocation.

## D. Device observations and allocator accounting

**Device stack, heap and arena peaks: NOT MEASURED — HARDWARE TEST REQUIRED.**
The diagnostics target provides measurement facilities, not completed physical
evidence. Its function-entry stack samples cover instrumented project functions
and explicit checkpoints. They do not continuously sample SDK/OS/library work,
interrupts or every transient within a function. Instrumentation changes frames
and timing; report diagnostic and normal builds separately.

The installed gint 2.11 fx-CG source was inspected in `src/kernel/kernel.c`,
`src/render-cg/dvram.c` and `include/gint/kmalloc.h`. Recheck these assumptions
when changing SDK, target or configuration:

- `gint_stack_top` is the **low address boundary** of a reserved 16 KiB stack.
  The upper boundary is `mmu_uram() + mmu_uram_size()`. Normalize P1/P2 aliases,
  range-check samples, and never overwrite a live stack to paint a watermark.
- `_uram` spans unused linked user RAM below that stack floor. Its actual
  capacity depends on the linked image and must be queried, not guessed.
- The inspected fx-CG50 `_ostk` arena has 350 KiB capacity. Its default one-buffer
  VRAM allocation requests 177,408 bytes of payload plus 96 bytes of margins and
  alignment, or 177,504 bytes before allocator metadata. Aliases and the two
  default VRAM pointers do not imply two allocations.
- VRAM is **already included** in `_ostk` allocator used/peak statistics. Do not
  add its payload or request to those statistics again.
- Query the public arena and optional gint allocator statistics APIs. Statistics
  may be unavailable for a non-gint arena; absence is not zero use. Record
  capacity, current used/free, available lifetime peak, live blocks and failures.
  `capacity - used - free` is current allocator metadata/header space; it is not
  a recovered history of original requested payloads.
- Public `peak_used_memory` is a lifetime high-water mark. Resetting app counters
  does not reset arena history. Component peaks may occur at different times;
  adding those peaks does not yield an observed simultaneous total.

Use the diagnostics screen/export and the stated workloads on a physical
calculator before replacing the device status. Preserve build hash, firmware,
operation/sample count and coverage limitations alongside each observation.

## Reproduction

From the repository root after a fresh native build:

```sh
source tools/env.sh
python3 tools/memory_report.py --self-test
python3 tools/stack_callgraph.py --self-test
python3 tools/memory_report.py --build-dir build-cg --g3a dist/NUMGAME.g3a
```

Use `--source-root`, `--json-out` and `--markdown-out` when analyzing a separate
build tree. Omit `--g3a` if no corresponding package exists; the report then makes
no package size/hash claim. Keep normal and diagnostics reports separate.
