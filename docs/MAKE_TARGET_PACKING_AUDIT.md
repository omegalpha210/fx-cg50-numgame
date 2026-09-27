# Make Target beta.6 bank packing — beta.7

Baseline local source: `6c4fbc1c71a07459fca7859cfe2f4709673a640e`
(published beta.6 source `932f7a44ac50c165a360775830aeb63e63305a95`).
This changes only the physical encoding of the existing revision-6 Make Target
bank. It neither generates new problems nor changes content revision, puzzle
IDs, grade thresholds, the v5 save format, or the other 35 games. The
deterministic host-only [logical manifest](../assets/guesscalc_target_beta6_manifest.jsonl)
has SHA256 `1322ea82e3849b1bc38e5d831e7ddd3d02275a0bd414a86b440d2b23065c46ff`.

## Encoding and direct access

The former linked `GcBeta6CardRecord[4000]` held six `int16_t` card slots,
target, answer offset, and three hint bytes with alignment: 20 bytes per
record or 80,000 bytes. Four per-level NUL-terminated answer pools occupied
89,586 bytes; their pointer table occupied 16 bytes. **169,602 linked bytes**
were specific to the former current Make Target representation. The C header's
263,408 source-text bytes are not that ROM measurement.

The replacement is one `uint8_t[63000]` read-only symbol, split into four
1,000-record difficulty sections. Each record has a fixed stride for its
level, so there is no per-record offset/directory and arbitrary ordinal
lookup takes one multiplication and a bounded one-record decode. The packer
checks the 20 target groups (five targets × four levels, 200 each) against the
original source before omitting per-record target bytes. Ordinals and stable
IDs (`16240 + ordinal`) are unchanged.

| Level | Cards | Card bytes | Hint bytes | Answer-token bytes | Record bytes | 1,000-record section |
|---|---:|---:|---:|---:|---:|---:|
| EASY | 4 | 5 | 1 | 7 | 13 | 13,000 |
| NORMAL | 4 | 5 | 1 | 7 | 13 | 13,000 |
| HARD | 5 | 7 | 1 | 9 | 17 | 17,000 |
| MASTER | 6 | 8 | 1 | 11 | 20 | 20,000 |

Cards are written in displayed order as unsigned 10-bit values, least
significant bit first within explicit bytes. No bitfields, unaligned casts,
host endian assumption or read beyond the final partial byte are used.
Unused fifth/sixth slots are restored as zero. The hint byte uses bits 0–2
for the first card occurrence, 3–5 for the second, and 6–7 for the exact
ordered `+ - * /` operator. Repeated card values still address distinct
occurrences, and subtraction/division direction remains intact.

Each answer nibble is either a card slot 0–5 or one of six exact characters
`+ - * / ( )`. Low nibble precedes high. `0xf` is a final high-nibble pad
when the token count is odd. The original answer order, parentheses and
unary minus are preserved, not algebraically rewritten. All 4,000 original
answers consist only of these tokens and use every card occurrence once.
EASY/NORMAL answers have 13 tokens, HARD 17, MASTER 21 except two with 22;
their decoded character maximum is 31 plus NUL. The 16-alphabet literal text
codec was considered as an alternative. A per-level fixed stride with a
nibble length delta would take 79,000 data bytes: cards 25,000, hints 4,000,
literal answers 50,000, index zero. It was not linked or given a speculative
SH code-size claim. The chosen card-reference form takes cards 25,000,
hints 4,000, answer tokens 34,000, index zero: **63,000 bytes**.
The literal alternative would need at least 574,186 bytes for the normal
package even if its decoder cost zero bytes and alignment did not change
(`664,788 − 169,602 + 79,000`). Its actual `.text`, scratch and package size
were not built or measured. It has simpler literal-character decoding and a
similar bounded one-answer output buffer, but its 16,000-byte data disadvantage
already exceeds the entire measured card-reference decoder-code increase.
The selected codec is therefore smaller even under the most favorable
zero-cost assumption for the literal candidate.

`gc_target_beta6_read` and `gc_target_beta6_answer_text` decode at most one
record into caller-owned storage. The latter uses a 40-byte local answer
scratch, writes the caller buffer only after full validation, and rejects a
short buffer. The decoder checks ordinal bounds, array extent, nonzero card
bounds, unused tail bits, hint card indices, unknown/early tokens, duplicate
or missing card references and output length. It uses no heap, bank-wide RAM
buffer, disk cache or external file. Ordinary number editing/submission does
not reconstruct the answer; HINT reads only the card/hint fields and ANSWER
decodes the expression. The normal SH compiler reports 72- and
112-byte individual frames for these functions. A failed read does not clear
an existing Make Target game before an attempted new start.

## Equivalence and costs

The original C header stays in the **host source only** as an independent
reference; `guesscalc.c` no longer includes it. The host native audit compares
every decoded record against that C header and the JSON source: ID, level,
target, active card count, six card slots including zeros, hint fields,
formatted HINT and exact ANSWER. All 4,000 decoded expressions also pass the
production parser/card-occurrence validator. The native-decoded manifest has
the same SHA256 as the frozen host manifest. The other 800 Countdown records
still pass their original native/parser audit. The old beta.4/beta.5 indices
and recipes remain linked for unfinished saves.

In a same-seed app replay against the unmodified beta.6 public source,
200 HARD fixed-target starts and a full 1,000-item NORMAL RANDOM cycle gave
byte-identical puzzle ID/target/card/order lines and byte-identical v5
checkpoint payloads. Two original beta.6 FIXED/RANDOM v5 payloads loaded,
validated, re-encoded byte-identically, and accepted HINT/ANSWER under the
packed runtime. Representative HINT and ANSWER PPM frames from the original
and packed host renderers also matched byte-for-byte. The payload SHA256s are
`5d53bfc386e1cf5406e8b55d8b64a5a6e73f100208cf929bd6a61c3ec6fc464d`
(fixed) and
`7b265f4d61461542425c691fd3bad4d3b35c3c0bebef602362ea9f333c195c90`
(RANDOM); the HINT/ANSWER PPM SHA256s are
`9f604470b27811edbb30b9e3a6e89242a9e8ad128ba6d8cd7e5ca59d440bb1de`
and `6389f5aed11cdcf5387279943aa109cb48cbf640906bc0c6138b209bcc441e94`.
The existing 27-target
strict C11/UBSan suite covers app lifecycle, legacy resume, Countdown and the
other games. Source generation is unchanged; `tools/pack_make_target.py`
`--write`/`--check` is the deterministic JSON → packed C step after content
generation and before native validation. A stale original reference, packed
asset or logical manifest fails `--check`.

| Measured native component | beta.6 baseline | packed build | Change |
|---|---:|---:|---:|
| Current Make Target linked data | 169,602 B | 63,000 B | −106,602 B |
| Normal `.rodata` | 489,520 B | 382,960 B | −106,560 B |
| Normal `.text` | 145,588 B | 146,740 B | +1,152 B |
| Normal BSS aggregate | 46,096 B | 46,096 B | 0 B |
| `NUMGAME.g3a` | 664,788 B | 559,380 B | −105,408 B |
| Diagnostic `.rodata` | 490,976 B | 384,416 B | −106,560 B |
| Diagnostic `.text` | 171,460 B | 172,788 B | +1,328 B |
| Diagnostic BSS aggregate | 50,016 B | 50,016 B | 0 B |
| `NUMGDIAG.g3a` | 692,116 B | 586,884 B | −105,232 B |

The 42-byte gap between the old/new Make Target symbol delta and `.rodata`
delta comes from other constants and alignment. The actual `.g3a` saving is
the `.rodata` decrease minus the added `.text`, not a ZIP estimate. No global
compiler flags or storage schema changed. There is no claim that the ROM
decrease saves the same amount of RAM; the bank is linked as read-only data.

Same-host macOS samples use `tools/benchmark_make_target_pack.c` at `-O2` for
51 runs of 4,000 nonsequential ordinals, and
`tools/benchmark_make_target_app.c` for 128 app operations per row against
the two UBSan host libraries. Values are medians; tiny values are subject to
clock granularity and should not be extrapolated to SH hardware.

| Host operation | beta.6 | packed | Unit |
|---|---:|---:|---|
| One random record lookup | 0.001 | 0.025 | µs/record |
| One answer lookup | 0.058 | 0.055 | µs/record |
| One formatted hint | 0.083 | 0.095 | µs/record |
| App entry, start and in-memory save | 0.005 | 0.007 | ms/operation |
| Repeated NEW and in-memory save | 0.005 | 0.007 | ms/operation |
| App HINT | <0.001 | 0.001 | ms/operation |
| App ANSWER | <0.001 | 0.001 | ms/operation |
| Active game validation | 0.002 | 0.002 | ms/operation |
| Cold RESUME from in-memory save | 0.001 | 0.001 | ms/operation |
| MENU with in-memory save hook | <0.001 | <0.001 | ms/operation |

The app sample exercises the same memory-backed save hook in each build; it
does not measure BFile or flash. Neither benchmark measures per-key input
latency directly, but ordinary number editing and submission do not call the
answer decoder. The maximum individual SH frame remains 1,392 bytes in
`start`; the decoder's 72- and 112-byte frames are smaller. There is no
whole-pack RAM decode or new BSS. **HARDWARE LATENCY NOT MEASURED.**

The focused macOS ASan executable stalled before producing test output and
was terminated after 20 seconds; ASan is **UNVERIFIED**, not a pass. Device
flash speed, LCD and observed stack/arena peaks remain **HARDWARE TEST
REQUIRED**. Host timing is not a physical calculator measurement.
