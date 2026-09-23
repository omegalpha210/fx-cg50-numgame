# Fixed archive storage audit

The old native namespace was NG00A/B.dat for settings and NG01A/B.dat through
NG30A/B.dat for per-game runs plus aggregate statistics. It was bounded at 62
files, not a demonstrated filename leak. These were real backups, not disposable
temporary files. The MENU symptom has not been linked to this file count.

Release now uses NGARCA.dat and NGARCB.dat. Each is 462,032 bytes: a 32-byte
ownership header and 33 bounded 14,000-byte slots (settings 0, stable IDs 1–32).
Only the selected record is streamed; there is no whole-archive RAM buffer.
Each record retains separate identity, version, generation, length, payload CRC
and header CRC. Newest selection handles generation wrap. A damaged game record
does not invalidate other records in the same archive. Each save writes the older
copy, closes, reads it back, validates semantics and CRC, then updates generation.
No save is marked successful on partial/zero write, readback failure or close error.

Normal files: 2, totaling 924,064 bytes, independent of game switches. Explicit
DIAG EXPORT adds only fixed NGDIAG.txt, bounded by 24,576 bytes. Diagnostic builds
use NDARCA/B.dat and NDDIAG.txt, a separate namespace. No automatic disk logging.

Legacy migration is explicit at startup using the not-yet-live application
session as workspace, distinct from the transaction decoder probe. Version1
three-level statistics and 30-ID settings decode to version3 five-level/32-ID
structures with zero new statistics and NORMAL defaults for new preferences.
The newest valid legacy slot is loaded; it is copied twice to the archive and
both archive copies are semantically re-read before deletion of valid owned old
files. Deletion requires correct magic, accepted version, identity, length, CRCs
and state validation. Foreign/invalid files are preserved. When existing archive
and legacy payload fingerprints disagree, both are preserved, migration reports
incomplete, and normal load prefers the archive. Neither namespace's generation
counter is assumed newer across independently used app versions.

Migration is retryable in the same process and on later startup. An interrupted
archive initialization has a valid NGINIT02 marker and no accepted records;
a later write can finish initialization. Failure while creating a new file removes
that newly-created file only after a successful close. Unknown or torn global
ownership headers are preserved and reported as errors, never blindly rewritten.
If such a header prevents archive use, validated legacy data remains a fallback.
If a physical power cut tears the first ownership marker or final publication,
manual inspection/recovery may be required; the implementation does not claim
hardware-atomic 32-byte writes. Existing valid second copies remain readable.

Transitional legacy sources are at most 62 additional bounded records. A
conservative owned-data upper bound is 64 files / 1,792,064 bytes before explicit
export; 65 files / 1,816,640 bytes including export. Conflicting or damaged
sources may intentionally remain until inspected. This bound excludes unrelated
files which NUM GAME neither owns nor deletes. No real user save was changed
during development; test storage consists only of mock files.

Verification: tests/test_native.c runs the production native adapter against a
Fugue mock, including exact-EOF behavior, preallocated archive seek/read/write,
16 interrupted-write cut points with retry, v1 newest-slot migration, divergent
progress preservation, foreign filename preservation, valid initialization-marker
restart, cross-record corruption isolation, bounded failed-close retry, readback
corruption and a 1,000-switch/MENU loop ending with two files and zero handles.
Generic transaction tests cover all game/mode/level encodings, CRCs, generation
wrap, unsupported versions, malicious valid-CRC records and error isolation.
Physical flash interruption and BFile latency remain HARDWARE TEST REQUIRED.
