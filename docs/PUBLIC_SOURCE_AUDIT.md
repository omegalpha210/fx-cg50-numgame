# Public source snapshot audit — beta.6

Public source is assembled with `tools/public_snapshot.py` into a separate
allowlisted directory. The working repository and its original Git history stay
local and unchanged. The tool neither initializes a repository nor creates a
remote, commit, push or release. A source snapshot is not evidence of hardware
execution or a successful fresh build.

## Included material and provenance

The policy includes the C/assembly runtime, headers, native and host tests,
build scripts, toolchain revision lock, original JSON puzzle banks, generated
C/header packs, generators and independent verifiers. It also includes the
documented public audits, native-renderer captures, runtime icons and attributed
font atlases. Generated data and compiled pack sources are both retained so that
their correspondence can be checked independently.

The current beta.6 content is the [4,000-record Make Target bank and 800-record
Countdown bank](MAKE_TARGET_COUNTDOWN_BETA6_AUDIT.md), plus the [896-target Prime
Factor table](PRIME_FACTOR_BETA6_AUDIT.md). Their JSON audit records and compact
native headers are selected together. The Make Target exact-rational C++ tool is
host-only source under `tools/generate`; it is not linked into either calculator
package. The beta.6 target generator, deterministic MASTER refinement, Countdown
generator, native C/JSON parity check and independent content tests are selected
with the new packs. Retained beta.4 and beta.5 Make Target banks, reports and
verifiers document historical revisions and support legacy saves; beta.5 is not
the new-game content count. The [content inventory](CONTENT_INVENTORY.md),
[setting-level CSV](CONTENT_INVENTORY_SETTINGS.csv), [cycle policy](CONTENT_CYCLE.md)
and [Korean user guide](USER_GUIDE.md) are included.

The [beta.6 capture guide](captures/BETA6_README.md), its 396×224 renderer frames
and contact sheets, and the source harness that produced them are selected.
These are host-renderer evidence, not physical LCD photographs. Nine small,
path-checked host benchmark JSON files retain historical baseline, 36-game,
v5 and [beta.6](PERFORMANCE_BETA6.md) normal/diagnostic summaries with
build-flag evidence; their reports can be regenerated without the original
workstation build directories.

`LICENSE`, `THIRD_PARTY_NOTICES.md` and every listed notice in `docs/third_party`
are required and copied byte-for-byte. The original font atlas bytes are also
preserved. Compressed PNG text metadata is inspected with a bounded decoder,
rather than stripped from attributed assets. Upstream notice files can contain
links relative to their original project; these are reported as information
and the notice text remains unchanged. Rule-reference sites are described in
[asset provenance](ASSET_PROVENANCE.md); no fetched puzzle corpus, commercial
maps, SDK source tree or toolchain binary is added by this tool.

`--include-dist` additionally includes only `NUMGAME.g3a`, `NUMGDIAG.g3a` and
`SHA256SUMS.txt`. Both named packages must exist and match the checksum list.
Package structure, ELF payload identity and physical execution remain separate
checks in the G3A verifier, [memory audit](MEMORY_AUDIT.md) and
[hardware procedure](HARDWARE_RETEST.md).

## Excluded material

The snapshot excludes existing Git history/metadata, private local directories,
worktrees, SDKs, build/cache directories, device save/export files, PDFs,
development logs and process samples, original requirements, agent instructions
and work handoff documents. Runtime saves are not used as test fixtures; the
checked-in synthetic C fixtures remain part of the tests. PNG/JSON selection is
restricted to known project asset paths rather than arbitrary files with those
extensions. Raw linker maps and sanitizer/process samples remain local because
they can contain workstation paths or other machine details.

An initial read-only history check examined 402 reachable text blobs and found
35 matching blobs across 25 file paths. Findings concerned personal home/temp
paths in requirements, handoff documents and development logs/samples. These
counts describe that initial local audit, not a promise about future commits.
The public snapshot carries none of the original history. Re-run the audit for
the final source state; no history rewrite or deletion is needed.

## Automated checks and limits

- Scan selected bytes for absolute personal-home/private-temp paths, common
  access-token/private-key formats and credentials embedded in URLs. Reports
  print only relative file names, finding types and counts, never matched values.
  PNG plain/compressed text is checked too; decompressed text is limited to
  1 MiB, and unsupported EXIF or malformed metadata fails.
- Check required source, generated-asset and license paths. Project Markdown
  links to absent/excluded local files are warnings, or errors with
  `--strict-links`. Mentions of internal documents are review warnings. The tool
  does not silently rewrite documentation or redact source code.
- Hash exactly the bytes inspected, preserve executable/non-executable mode as
  0755/0644, reject symlinks, and recheck bytes while copying. Concurrent edits
  detected during copying fail the operation; freeze source edits for the final
  snapshot. The allowlist does not establish that a file's contents are correct.
- Emit `PUBLIC_MANIFEST.json` with sorted paths, sizes, modes and SHA256 hashes.
  The tree digest covers these entries and contains no timestamps or local
  absolute path. Two copies of an unchanged tree produce the same manifest.
  The manifest excludes itself to avoid a circular digest.
- Verify every manifest entry and reject changed modes/bytes, missing files,
  unexpected files, symlinks and Git metadata. Creating a fresh snapshot requires
  an unused destination; `--replace` only replaces a recognized, verified
  snapshot within the source repository's local public-output directory.

Regex scanning is a bounded check, not proof that every possible secret or
private detail is absent. It does not OCR images, understand every encoding,
inspect arbitrary binary metadata or determine license compatibility. Review
the selected file list, source provenance and findings before publication.
History inspection covers reachable text blobs, not every unreachable object
or commit-message/author field; none of those Git objects are exported anyway.

The tool's self-test passed for allowlist exclusions, token/path reporting,
local links, plain/compressed PNG metadata, decompression limits, symlink
rejection, two-copy deterministic manifests, protected replacement, checksum
lists, changed contents and unexpected private files. These are host checks.
The beta.6 target, Countdown and Prime validators, native parity check, supply
and Baseball host regressions, and renderer capture harness are source material
for separate execution; inclusion in the snapshot does not itself certify a
passing build or content audit.
ASan remains unverified until a complete successful run is recorded; compiler
success alone is not an ASan pass. Physical calculator checks remain
**HARDWARE TEST REQUIRED**.

## Reproduction and final publication gate

Run from the working repository. Reports belong outside the clean source
directory, so they do not become unlisted snapshot files.

```sh
python3 tools/public_snapshot.py --self-test
python3 tools/public_snapshot.py --include-dist --audit-history \
  --report .local/public-audit.json
python3 tools/public_snapshot.py --include-dist --strict-links \
  --output .local/public/source --report .local/public-source-report.json
python3 tools/public_snapshot.py --verify .local/public/source
```

Resolve project link errors and review warnings first. Check the public source
with a fresh host/native build and `bash tools/verify_content.sh` using a
disposable test copy, or direct build outputs outside the clean snapshot. That
script includes the beta.6 exact Make Target/Countdown checks, native JSON/header
parity, Prime Factor checks and retained revision checks; run the host test
suite separately.
Some existing test scripts create local caches; such outputs are correctly
rejected by a later clean-snapshot verification. For the JSON-to-native byte
check, avoid Python cache writes:

```sh
source tools/env.sh
PYTHONDONTWRITEBYTECODE=1 "$NUMGAME_PYTHON" \
  .local/public/source/tools/check_embedded.py
```

Record build/compiler identity, package hashes, content results and any
limitations separately. Regenerate and verify the clean snapshot after testing
if tests produced extra files. Compare its manifest tree digest with the tested
source manifest; do not treat a changed source tree as already tested. The final
source snapshot may then receive its own clean public history under the release
workflow. Remote operations are performed separately; this tool has no network
or publishing command.
