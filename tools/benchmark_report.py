#!/usr/bin/env python3
"""Render measured host benchmark JSON without inventing device measurements.

Reads completed normal and diagnostic runs, checks their scope/sample coverage,
and emits Markdown to stdout or --output. --check compares without writing.
Timing JSON is the authoritative numerical input. Nearby CMake flags are used
only when present, or --build-evidence can supply a portable captured JSON file.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import math
import re
from pathlib import Path
import statistics
import sys

IDS = list(range(1, 29)) + [31, 32]
NAMES = ['NUMBER BASEBALL', 'EQUATION GUESS', 'NUMBER MIND', 'CLUE LOCK',
         'SEQUENCE DETECTIVE', 'MAKE TARGET', 'COUNTDOWN', 'MISSING OPERATORS',
         'CROSS MATH', 'PRIME FACTOR', 'SUDOKU', 'CALCUDOKU', 'KAKURO',
         'FUTOSHIKI', 'SKYSCRAPERS', 'HITORI', 'BINARY PUZZLE', 'NUMBRIX',
         'MAGIC SQUARE', 'SUM GRID', 'NIM', 'WYTHOFF', 'EUCLID', 'MAKE FIFTEEN',
         'RACE', '2048', 'SLIDING', 'LIGHTS OUT', 'SHIKAKU', 'SLITHERLINK']
NAMES = dict(zip(IDS, NAMES))
HEAVY_IDS = [7, 11, 12, 13, 14, 15, 3, 18, 32, 21, 22, 23, 24, 25]
LOAD_SCOPE = 'module_init + semantic_validation + full_RGB565_renderer; host warm process; all modes/levels mixed,64 samples/config'
HEAVY_SCOPE = 'pending_CPU_then_hint_if_supported + validate + render'
SAVE_SCOPE = 'POSIX archive save including existing copies+CRC+readback, excludes explicit cold-load assertion'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def unique_object(pairs):
    result={}
    for key,value in pairs:
        if key in result:raise ValueError('Duplicate JSON key: '+key)
        result[key]=value
    return result


def read_json(path):
    try:
        return json.loads(path.read_text(), object_pairs_hook=unique_object, parse_constant=lambda value: (_ for _ in ()).throw(ValueError('Non-finite JSON number: '+value)))
    except (OSError, json.JSONDecodeError, ValueError) as error:
        raise ValueError(f'Cannot read a complete valid benchmark JSON: {path.name}: {error}') from error


def integer(value, label, positive=False):
    if type(value) is not int or value < int(positive):
        raise ValueError(f'{label}: expected {"positive" if positive else "nonnegative"} integer')


def number(value, label):
    if type(value) not in (int, float) or not math.isfinite(value) or value < 0:
        raise ValueError(f'{label}: expected finite nonnegative number')


def timing(value, label, samples=None):
    integer(value['n'], label+'.n', True)
    if samples is not None and value['n'] != samples:
        raise ValueError(f'{label}: expected {samples} samples, got {value["n"]}')
    for field in ('p50_ms', 'p95_ms', 'max_ms'): number(value[field], label+'.'+field)
    if not value['p50_ms'] <= value['p95_ms'] <= value['max_ms']:
        raise ValueError(f'{label}: unordered percentile/maximum values')


def validate(data, diagnostic):
    if data.get('platform') != 'host' or data.get('hardware_peak') != 'NOT MEASURED':
        raise ValueError('Expected explicitly labelled host-only evidence')
    if data.get('instrumented') is not diagnostic:
        raise ValueError('Normal/diagnostic input instrumented flag mismatch')
    for field in ('pointer_bytes', 'game_bytes', 'session_bytes', 'app_bytes', 'rectangles'):
        integer(data[field], field, True)
    if data['load_scope'] != LOAD_SCOPE: raise ValueError('Load scope changed: review report methodology')
    games=data['games']; heavy=data['heavy']
    if len(games)!=30 or sorted(p['id'] for p in games)!=IDS:
        raise ValueError('Expected each of the 30 visible IDs exactly once')
    if len(heavy)!=len(HEAVY_IDS) or sorted(p['id'] for p in heavy)!=sorted(HEAVY_IDS):
        raise ValueError('Heavy-workload game coverage changed: review methodology')
    for p in games:
        timing(p, f'game {p["id"]}')
        modes={1:6,2:3,6:2,19:2,21:3,22:3,23:3,24:3,25:3,26:2,27:2,28:2}.get(p['id'],1)
        expected=64*modes*(5 if 11<=p['id']<=15 else 4)
        if p['n']!=expected:raise ValueError(f'ID {p["id"]}: expected {expected} load samples over all current configurations')
    for p in heavy:
        timing(p, f'heavy {p["id"]}', 128)
        if p['scope'] != HEAVY_SCOPE: raise ValueError('Heavy scope changed: review methodology')
    fixture=data['ram_fixture']; timing(fixture, 'ram_fixture', 1000)
    for field in ('failures','rss_highwater_before_bytes','rss_highwater_after_bytes','app_malloc_calls','app_heap_bytes','handles','timers'):
        integer(fixture[field], 'ram_fixture.'+field)
    if fixture['rss_highwater_after_bytes'] < fixture['rss_highwater_before_bytes']:
        raise ValueError('Process lifetime RSS high-water mark cannot decrease')
    if type(fixture['fixture_io']) is not bool: raise ValueError('fixture_io must be an explicit boolean')
    checkpoint=data['checkpoint']; timing(checkpoint, 'checkpoint', 128)
    if checkpoint['scope'] != SAVE_SCOPE: raise ValueError('Checkpoint timing scope changed')
    number(checkpoint['first_create_ms'], 'checkpoint.first_create_ms')
    for field in ('payload_bytes','record_with_header_bytes','archive_files','total_archive_bytes','handles'):
        integer(checkpoint[field], 'checkpoint.'+field)
    if checkpoint['record_with_header_bytes'] != checkpoint['payload_bytes']+32:
        raise ValueError('Checkpoint record header/payload lengths disagree')
    if diagnostic:
        d=data['diagnostics']
        for field in ('sampled_stack_bytes','samples','deepest_game','codec_peak','read_calls','write_calls','read_bytes','write_bytes'):
            integer(d[field], 'diagnostics.'+field)
        if d['deepest_game'] not in [0]+IDS: raise ValueError('Unknown diagnostic peak owner')
        if not isinstance(d['deepest_operation'], str): raise ValueError('Missing peak operation')
    if not isinstance(data['limits'], str): raise ValueError('Missing original measurement limits')
    return data


def build_evidence(path):
    """No compiler invocation, Git dependency or guesses about missing flags."""
    folder=path.parent; answer={}
    cache=folder/'CMakeCache.txt'
    if cache.exists():
        values={line.split(':',1)[0]:line.split('=',1)[1] for line in cache.read_text().splitlines()
                if not line.startswith(('#','//')) and ':' in line and '=' in line}
        for key in ('CMAKE_BUILD_TYPE','NG_SANITIZE','NG_ASAN','NG_DIAGNOSTIC'):
            if key in values: answer[key]=values[key] or '(empty/default)'
    flags=folder/'CMakeFiles/benchmark_host.dir/flags.make'
    core_flags=folder/'CMakeFiles/numgame_core.dir/flags.make'
    for file,label in ((flags,'benchmark_flags'),(core_flags,'core_flags')):
        if file.exists():
            lines=[line for line in file.read_text().splitlines() if line.startswith(('C_FLAGS =','C_DEFINES ='))]
            answer[label]='; '.join(lines)
    binary=folder/'benchmark_host'
    if binary.exists():answer['adjacent_binary_sha256']=sha(binary)
    answer['association']='Adjacent build files observed at report generation; timing JSON does not embed build identity.'
    return answer


def describe_build(label, evidence):
    flags=evidence.get('core_flags')
    if not flags:return label+': compiler flags NOT CAPTURED.'
    parts=[]
    parts.append('debug symbols present' if re.search(r'(?<!\S)-g(?:\d|line-tables-only)?(?=\s|$)',flags) else 'no -g debug-symbol flag captured')
    parts.append('assertions explicitly enabled (-UNDEBUG)' if '-UNDEBUG' in flags else 'assertion setting not established here')
    parts.append('UBSan enabled' if '-fsanitize=undefined' in flags else 'UBSan not present in captured flags')
    parts.append('abort on sanitizer failure' if '-fno-sanitize-recover=all' in flags else 'sanitizer recovery setting not established here')
    optimization=re.findall(r'(?<!\S)-O(?:[0-3sgz]|fast)(?=\s|$)',flags)
    parts.append('optimization flags '+', '.join(optimization) if optimization else 'no optimization flag')
    parts.append('function-entry instrumentation present' if '-finstrument-functions' in flags else 'no function-entry instrumentation flag')
    return label+': '+', '.join(parts)+'.'


def table(headers, rows):
    def cell(value):return str(value).replace('|','\\|').replace('\n',' ')
    return '\n'.join(['| '+' | '.join(headers)+' |','| '+' | '.join(['---']*len(headers))+' |']+
                     ['| '+' | '.join(cell(x) for x in row)+' |' for row in rows])


def triple(row):
    return ' / '.join(f'{row[k]:.6f}' for k in ('p50_ms','p95_ms','max_ms'))


def ratio(normal, diagnostic, key='p50_ms'):
    return f'{diagnostic[key]/normal[key]:.3f}×' if normal[key] else 'N/A (zero baseline)'


def ms(value):return f'{value:.6f}'


def name(gid):return f'{gid:02} {NAMES[gid]}'


def render(normal, diagnostic, normal_sha, diagnostic_sha, evidence):
    ng={g['id']:g for g in normal['games']}; dg={g['id']:g for g in diagnostic['games']}
    nh={g['id']:g for g in normal['heavy']}; dh={g['id']:g for g in diagnostic['heavy']}
    if any(ng[g]['n']!=dg[g]['n'] for g in IDS): raise ValueError('Builds have different load sample coverage')
    totals=sum(x['n'] for x in ng.values()); configurations=totals//64
    normal_max=max(ng.values(),key=lambda p:p['max_ms']); diag_max=max(dg.values(),key=lambda p:p['max_ms'])
    ratios=[dg[g]['p50_ms']/ng[g]['p50_ms'] for g in IDS if ng[g]['p50_ms']]
    nf=normal['ram_fixture'];df=diagnostic['ram_fixture'];nc=normal['checkpoint'];dc=diagnostic['checkpoint'];dd=diagnostic['diagnostics']
    overhead=(f'The median of the **30 per-game p50 ratios** is {statistics.median(ratios):.3f}×; their range is {min(ratios):.3f}×–{max(ratios):.3f}×.' if len(ratios)==30 else 'Some baseline p50 values are zero; a 30-game ratio summary is unavailable.')
    warning=[]
    for label,data in (('Normal',normal),('Diagnostic',diagnostic)):
        f=data['ram_fixture'];c=data['checkpoint']
        if f['failures']:warning.append(f'{label} RAM fixture recorded {f["failures"]} failures.')
        if f['handles'] or c['handles']:warning.append(f'{label} reports nonzero remaining handle counters.')
        if f['timers']!=1:warning.append(f'{label} modeled timer count is {f["timers"]}, expected 1.')
        if f['fixture_io']:warning.append(f'{label} fixture_io is true; it is not a RAM-only fixture.')
    out=['# Host performance audit',
         '> Historical 30-game baseline measurement. The current 36-game catalog, controls, and compact storage are documented in [README](../README.md), [GAME_CATALOG](GAME_CATALOG.md), and [STORAGE_FORMAT](STORAGE_FORMAT.md). The figures below have not been remeasured for the current build.',
         '**Physical fx-CG50 latency, stack peak, heap/arena peak, and timeout/fallback frequency: NOT MEASURED — HARDWARE TEST REQUIRED.** The following numbers are measured host observations. No host-to-SH conversion or device target achievement is inferred.',
         f'Two completed benchmark JSON inputs cover **30 visible games**, **{configurations} mode/level configurations**, and **{totals:,} initial-load samples per build** (64 per configuration). Normal means diagnostics disabled, not an optimized native release. The largest measured normal load sample is **{ms(normal_max["max_ms"])} ms** ({name(normal_max["id"])}); the diagnostic maximum is **{ms(diag_max["max_ms"])} ms** ({name(diag_max["id"])}). These are maxima of the sampled workload, not global worst-case bounds.',
         ('## Observed counter exceptions\n\n'+'\n'.join('- '+x for x in warning)) if warning else 'The completed RAM fixture reports zero failures in both builds; remaining benchmark handle counters are zero and the modeled timer count is one. Coverage of those counters is described below.',
         '## Inputs, build and measurement scope',
         table(['Input','SHA256'],[('Normal host JSON',normal_sha),('Diagnostic host JSON',diagnostic_sha)]),
         table(['Build evidence','Normal','Diagnostic'],[(key,evidence['normal'].get(key,'NOT CAPTURED'),evidence['diagnostic'].get(key,'NOT CAPTURED')) for key in sorted(set(evidence['normal'])|set(evidence['diagnostic']))]),
         describe_build('Normal',evidence['normal'])+' '+describe_build('Diagnostic',evidence['diagnostic'])+' The table reports CMAKE_BUILD_TYPE separately; empty/default is not a configured CMake Debug or Release build. ASan settings are stated only when captured. Diagnostic enables extra project function-entry sampling and counters. If the timing JSON is moved without build metadata, flags/binary identity are NOT CAPTURED; pass --build-evidence with captured metadata to reproduce that part.',
         'Timing uses `clock_gettime(CLOCK_MONOTONIC)` around synchronous operations in one warm process. The host renderer writes every rectangle to a 396×224 RGB565 memory buffer; it does not refresh an LCD. Native gint/OS calls, keyboard latency, MENU/OFF, physical flash, OS scheduling on the calculator and hardware timer delivery are outside the timing scope. The JSON does not record host CPU model, clock frequency, compiler version, source revision or run timestamp; do not invent them. Adjacent binary hashes are report-time association, not proof of the binary that produced an older JSON.',
         'The runner sorts each sample set. p50 is element `(n-1)//2` (the lower median for even n), p95 is nearest rank `ceil(0.95*n)-1`, and max is the last element. Values here retain six decimal places in milliseconds; extra printed zeros do not increase the precision of an input that was rounded earlier. Raw per-sample data and per-configuration percentiles are not stored, so they cannot be reconstructed or pooled from these summaries.',
         '## All 30 games: initialization, validation and first rendering',
         '`ng_new` (module initialization) + semantic `ng_valid` + complete common RGB565 renderer are inside each sample. Level and mode samples are mixed per game. The runner initializes deterministic seeds directly; this is not entry UI navigation or a user NEW bag-cycle benchmark. CLASSIC 2048 still appears in each raw level slot here even though the app hides its level selector. First-use samples are included in each process, but there is no separate cold-start/cache-eviction experiment.',
         table(['ID / game','Samples / configs','Normal p50 / p95 / max ms','Diagnostic p50 / p95 / max ms','Diagnostic / normal p50'],[(name(g),f'{ng[g]["n"]} / {ng[g]["n"]//64}',triple(ng[g]),triple(dg[g]),ratio(ng[g],dg[g])) for g in IDS]),
         overhead+' This is not a pooled runtime percentile. The two runs were not paired/interleaved trials with confidence intervals, so the comparison includes warm-cache, scheduling and measurement variation as well as diagnostic work; it is not a causal isolation of instrumentation cost.',
         '## Selected high-level action workloads',
         'Each row has 128 samples. Setup/initial render occurs before the timed interval. LOGIC 11–15 uses HELL; other rows use MASTER. CPU games use CPU FIRST (mode 1), execute the one pending exact CPU move, then validate/render. Hint games request one supported HINT, then validate/render. Slitherlink has neither a CPU nor hint flag, so its timed row is **initial-state validation and rendering only**. This does not benchmark a completed loop or a difficult edge edit. A reveal/first-step hint is not a runtime uniqueness search, and these selected starts do not certify a global worst case.',
         table(['ID / game','Timed action before validate/render','n each','Normal p50 / p95 / max ms','Diagnostic p50 / p95 / max ms','p50 ratio'],[(name(g),'CPU move' if 21<=g<=25 else 'none (start state)' if g==32 else 'one HINT',nh[g]['n'],triple(nh[g]),triple(dh[g]),ratio(nh[g],dh[g])) for g in HEAVY_IDS]),
         '## 1,000 isolated RAM fixtures',
         '`ng_storage_fixture` reuses the existing shared storage session/buffer. Each iteration constructs a seeded game, validates it, performs pending CPU/HINT work, validates again, fills the undo ring where allowed, records a result if applicable, encodes, computes CRC, decodes into the fixture, and re-encodes/compares CRC and length. It performs no fixture filesystem I/O and does not reference the user app session. IDs rotate across all 30 games; levels/modes are scheduled by integer round counters, not a promise of 1,000 samples for every configuration. Rendering is not in this fixture timing.',
         table(['Metric','Normal','Diagnostic'],[
             ('n',nf['n'],df['n']),('p50 / p95 / max ms',triple(nf),triple(df)),('p50 ratio','1.000×',ratio(nf,df)),
             ('Failures',nf['failures'],df['failures']),('RSS lifetime high-water before B',nf['rss_highwater_before_bytes'],df['rss_highwater_before_bytes']),
             ('RSS lifetime high-water after B',nf['rss_highwater_after_bytes'],df['rss_highwater_after_bytes']),
             ('Change in high-water B',nf['rss_highwater_after_bytes']-nf['rss_highwater_before_bytes'],df['rss_highwater_after_bytes']-df['rss_highwater_before_bytes']),
             ('Reported app malloc calls / owned heap B',f'{nf["app_malloc_calls"]} / {nf["app_heap_bytes"]}',f'{df["app_malloc_calls"]} / {df["app_heap_bytes"]}'),
             ('Remaining handles / modeled timers',f'{nf["handles"]} / {nf["timers"]}',f'{df["handles"]} / {df["timers"]}'),
             ('Fixture filesystem I/O',nf['fixture_io'],df['fixture_io'])]),
         'RSS is `getrusage(RUSAGE_SELF).ru_maxrss`, converted to bytes for the host platform. It is a process-lifetime resident high-water mark including runtime, code, pages, sanitizers and allocator effects; two high-water readings do not measure current live heap or prove no leak/continuous growth. The JSON app allocation/owned-heap zeros are source-ownership declarations printed by the runner, **not malloc-interposition measurements of libc/SDK/OS allocations**. The timer count of one is inserted through `NGD_TIMER_START` as a diagnostic model; this benchmark does not allocate or measure a real OS timer. Native adapter/hardware lifecycle evidence belongs to separate tests.',
         '## 128 maximum-undo POSIX archive saves',
         'The fixture uses a HELL Sudoku session with all allowed cell notes and the full four-entry undo ring. It serializes into the current production schema. A fresh temporary namespace creates two fixed archives; no user saves are involved. First creation is one separately reported sample, followed by a second untimed save to establish both copies. Each of 128 timed saves includes the production transaction\'s existing-copy reads/CRC, writing and readback verification. An explicit cold-load/equality assertion follows every save **outside** its timed interval. The total archive byte field is two times the fixed schema length, not a filesystem block-allocation measurement. These are POSIX adapter/filesystem-cache observations, not native BFile/flash or power-loss durability latency.',
         table(['Metric','Normal','Diagnostic'],[
             ('First archive creation ms (one sample)',ms(nc['first_create_ms']),ms(dc['first_create_ms'])),('Steady save samples',nc['n'],dc['n']),
             ('p50 / p95 / max ms',triple(nc),triple(dc)),('p50 ratio','1.000×',ratio(nc,dc)),
             ('Encoded payload B',nc['payload_bytes'],dc['payload_bytes']),('Record including 32-byte header B',nc['record_with_header_bytes'],dc['record_with_header_bytes']),
             ('Archive files / schema logical bytes',f'{nc["archive_files"]} / {nc["total_archive_bytes"]}',f'{dc["archive_files"]} / {dc["total_archive_bytes"]}'),
             ('Remaining handle counter',nc['handles'],dc['handles'])]),
         '## Host ABI and diagnostic counters',
         table(['Host field','Normal','Diagnostic'],[(field,normal[field],diagnostic[field]) for field in ('pointer_bytes','game_bytes','session_bytes','app_bytes','rectangles')]),
         table(['Diagnostic-only observation','Value'],[
             ('Sampled project host-stack excursion B',dd['sampled_stack_bytes']),('Stack samples',dd['samples']),
             ('Deepest recorded game / operation',f'{dd["deepest_game"]} / {dd["deepest_operation"]}'),('Maximum codec buffer bytes reported',dd['codec_peak']),
             ('Read calls / bytes',f'{dd["read_calls"]} / {dd["read_bytes"]}'),('Write calls / bytes',f'{dd["write_calls"]} / {dd["write_bytes"]}')]),
         'These sizes use the host ABI. Diagnostic stack excursion is based on instrumented project function-entry/explicit samples relative to a host reference; it is not continuous sampling of libc, OS, interrupts or every transient inside a function. It is not a native stack bound. Codec high-water is tracked bytes passed through the codec hooks, not process RAM. Read/write counters cover the whole diagnostic run, including archive setup and untimed verification loads; dividing them by 128 would mislabel bytes per timed save. Rectangle count is a rendering-work counter, not a frame rate. Do not add host objects, RSS, sampled stack, framebuffer or linked native sections to invent a simultaneous device total.',
         '## Device budgets and unresolved measurements',
         'Requested device targets remain unverified: preferably bank load within 0.5 s, runtime generation p95 within 2 s, and an approximately 3 s foreground generation hard budget considered as an initial policy. The host milliseconds above do not establish any of them. Native engines use bounded constructions or host-generated banks; no general elapsed-time timeout is implemented in these engines. Missing Operators E/N/H has a 128-attempt construction cap and rule-valid fixed fallback, which is distinct from an elapsed-time timeout. This benchmark does not count construction retries or fallback events. **Timeout/fallback frequency is NOT MEASURED, not zero.** It does not test deadline-triggered selection of a same-difficulty verified bank or silently downgrade HELL.',
         'Record calculator model/firmware, build/package hashes, actual operation and sample counts, foreground display-ready timing, live/peak arena accounting and stack coverage on hardware before asserting those budgets. Diagnostic overhead must be assessed separately on the device. See `docs/MEMORY_METHOD.md` and the hardware acceptance checklist.',
         'Original normal JSON limit statement: '+normal['limits'],
         'Original diagnostic JSON limit statement: '+diagnostic['limits'],
         '## Regeneration and validation',
         'Run the existing normal and diagnostic host benchmark executables from their respective build directories to create complete JSON files. Do not read a redirected file while its benchmark is still running. Then:',
         '```sh\npython3 tools/benchmark_report.py --normal docs/host-benchmark-normal.json --diagnostic docs/host-benchmark-diagnostic.json --build-evidence docs/host-benchmark-build-evidence.json --output docs/PERFORMANCE_AUDIT.md --check\n# Rerun the host workloads first to make new measurements:\n(cd build-host && ./benchmark_host > host-benchmark.json)\n(cd build-host-diagnostic && ./benchmark_host > host-benchmark.json)\npython3 tools/benchmark_report.py --output docs/PERFORMANCE_AUDIT.md\n```',
         'The three copied JSON inputs in `docs/` preserve the measured summaries and adjacent build-flag evidence for checking this published report. The default inputs for a new measurement are `build-host/host-benchmark.json` and `build-host-diagnostic/host-benchmark.json`. `--source-root` changes that default root. `--build-evidence` accepts `{ "normal": {...}, "diagnostic": {...} }` with captured flag evidence; `--write-build-evidence PATH` exports the currently observed adjacent metadata for portable reproduction. The script rejects truncated JSON, nonfinite/negative/unordered timing values, missing/duplicate game IDs, changed scopes/sample coverage or swapped instrumentation labels. `--check` compares the entire generated document without modifying it. It does not rerun workloads or assert physical performance.']
    return '\n\n'.join(out)+'\n'


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root',type=Path,default=Path(__file__).resolve().parents[1])
    parser.add_argument('--normal',type=Path)
    parser.add_argument('--diagnostic',type=Path)
    parser.add_argument('--output',type=Path)
    parser.add_argument('--build-evidence',type=Path)
    parser.add_argument('--write-build-evidence',type=Path)
    parser.add_argument('--check',action='store_true')
    args=parser.parse_args()
    if args.check and not args.output:parser.error('--check requires --output')
    if args.check and args.write_build_evidence:parser.error('--check cannot write build evidence')
    npath=args.normal or args.source_root/'build-host/host-benchmark.json'
    dpath=args.diagnostic or args.source_root/'build-host-diagnostic/host-benchmark.json'
    try:
        normal=validate(read_json(npath),False); diagnostic=validate(read_json(dpath),True)
        evidence=read_json(args.build_evidence) if args.build_evidence else {'normal':build_evidence(npath),'diagnostic':build_evidence(dpath)}
        if set(evidence)!= {'normal','diagnostic'} or any(not isinstance(v,dict) for v in evidence.values()):raise ValueError('Invalid build evidence mapping')
        result=render(normal,diagnostic,sha(npath),sha(dpath),evidence)
        if args.check:
            if not args.output.exists() or args.output.read_text()!=result:raise ValueError(f'Report differs: {args.output}')
        elif args.output:
            args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(result)
        else:sys.stdout.write(result)
        if args.write_build_evidence:
            args.write_build_evidence.parent.mkdir(parents=True,exist_ok=True)
            args.write_build_evidence.write_text(json.dumps(evidence,indent=2,sort_keys=True)+'\n')
    except (ValueError,KeyError,TypeError,OSError) as error:parser.exit(1,f'benchmark_report: {error}\n')

if __name__=='__main__':main()
