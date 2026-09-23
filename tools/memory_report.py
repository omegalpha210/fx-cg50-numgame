#!/usr/bin/env python3
"""Report native ELF placement and conditional compiler stack analysis.

Static sections, per-function frames, project call paths, host observations and
device observations are separate. This tool never invents a runtime RAM peak.
"""
from __future__ import annotations
import argparse,hashlib,json,re,subprocess,tempfile
from pathlib import Path
import stack_callgraph
ROOT=Path(__file__).resolve().parents[1]

def output(*args):return subprocess.check_output(args,text=True)
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def label(path,root):
    try:return str(Path(path).resolve().relative_to(Path(root).resolve()))
    except ValueError:return Path(path).name

def parse_sections(text):
    lines=text.splitlines();sections=[]
    pattern=re.compile(r'^\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+[0-9a-fA-F]+\s+2\*\*\d+\s*$')
    for i,line in enumerate(lines):
        m=pattern.match(line)
        if not m:continue
        flags=[f.strip() for f in lines[i+1].strip().split(',')] if i+1<len(lines) else []
        sections.append(dict(name=m[1],bytes=int(m[2],16),vma='0x'+m[3].lower(),lma='0x'+m[4].lower(),flags=flags))
    return sections

def parse_size_sections(text):
    result={}
    for line in text.splitlines():
        fields=line.split()
        if len(fields)==3 and fields[0].startswith('.') and fields[1].isdigit():result[fields[0]]=int(fields[1])
    return result

def map_summary(text):
    sections={};project={}
    for line in text.splitlines():
        m=re.match(r'^(\.[\w.]+)\s+0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)',line)
        if m:sections[m[1]]=int(m[3],16)
        m=re.match(r'^\s+(\.[\w.]+)\s+0x[0-9a-fA-F]+\s+0x([0-9a-fA-F]+)\s+.*CMakeFiles/numgame\.dir/src/.*\.(?:o|obj)\s*$',line)
        if m:project[m[1]]=project.get(m[1],0)+int(m[2],16)
    return dict(output_sections=sections,project_input_sections=project)

def collect_frames(build):
    frames=[];skipped=[]
    for path in sorted(build.rglob('*.su')):
        objects=[path.with_suffix(s) for s in ('.o','.obj')]
        if not any(p.exists() and p.stat().st_mtime_ns>=path.stat().st_mtime_ns for p in objects):
            skipped.append(path.name);continue
        for line in path.read_text().splitlines():
            fields=line.split('\t')
            if len(fields)!=3:raise ValueError('Malformed .su entry in '+path.name)
            location,size,kind=fields
            frames.append(dict(function=stack_callgraph.public_source(location),bytes=int(size),kind=kind))
    frames.sort(key=lambda f:(-f['bytes'],f['function']));return frames,skipped

def selected_symbols(text,sections):
    keep={'_app','_ng_storage_probe','_buffer','_grids_pack','_ng_diagnostics','_grids_pack_bytes','_grids_pack_offsets','_grids_bank_ids','_grids_bank_groups','_sq_wythoff_bits','_sq_euclid_bits','_sq_fifteen_values'}
    result=[]
    for line in text.splitlines():
        fields=line.split()
        if len(fields)!=4:continue
        address,size,kind,name=fields
        wanted=name in keep or name.startswith(('_sq_master_','_nb_shikaku_pack','_nb_slitherlink_pack','_gc_')) or re.match(r'^_cache(?:_id)?(?:\.\d+)?$',name)
        if not wanted or kind not in 'bBdDrR':continue
        address=int(address,16);size=int(size,16)
        section=next((s['name'] for s in sections if int(s['vma'],16)<=address<int(s['vma'],16)+s['bytes']),None)
        result.append(dict(name=name,bytes=size,type=kind,section=section))
    return sorted(result,key=lambda x:(x['bytes'],x['name']))

def artifact_report(elf,g3a,build):
    if g3a is None:return dict(status='NOT PROVIDED',bytes=None,sha256=None,elf_payload_match=None)
    if not g3a.is_file():raise ValueError('Explicitly supplied G3A is missing: '+g3a.name)
    raw=g3a.read_bytes()
    # Binary extraction uses ELF load addresses, including alignment gaps.
    # No source path or debug section is emitted into this byte comparison.
    with tempfile.TemporaryDirectory(prefix='memory-elf-',dir=build) as temporary:
        image=Path(temporary)/'payload.bin';subprocess.run(['sh-elf-objcopy','-O','binary',str(elf),str(image)],check=True,capture_output=True)
        payload=image.read_bytes()
    return dict(status='SUPPLIED ARTIFACT',file=g3a.name,bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest(),elf_payload_bytes=len(payload),elf_payload_match=len(raw)>=0x7004 and raw[0x7000:-4]==payload,target_1000000_pass=len(raw)<=1000000,hard_limit_1200000_pass=len(raw)<=1200000,container_validation='Separate tools/verify_g3a.py check required; payload equality is not hardware execution.')

def build_report(build,source_root,g3a=None):
    elf=build/'numgame'
    if not elf.is_file():raise ValueError('Native ELF numgame is missing in '+build.name)
    size_lines=output('sh-elf-size',str(elf)).splitlines();text,data,bss=map(int,size_lines[1].split()[:3])
    sections=parse_sections(output('sh-elf-objdump','-h',str(elf)))
    if not sections:raise ValueError('No ELF sections were parsed')
    exact=parse_size_sections(output('sh-elf-size','-A',str(elf)))
    if any(exact.get(s['name'])!=s['bytes'] for s in sections):raise ValueError('objdump and size -A section sizes disagree')
    allocated=[s for s in sections if 'ALLOC' in s['flags']]
    objects=sorted(p for p in build.rglob('*') if p.suffix in ('.o','.obj') and 'CompilerId' not in str(p) and p.is_file())
    object_sections={}
    for path in objects:
        for name,size in parse_size_sections(output('sh-elf-size','-A',str(path))).items():object_sections[name]=object_sections.get(name,0)+size
    frames,skipped=collect_frames(build);symbols=selected_symbols(output('sh-elf-nm','-S','--size-sort',str(elf)),sections)
    graph=stack_callgraph.collect(build,source_root);mapfile=build/'numgame.map';linker_map=dict(status='NOT AVAILABLE')
    if mapfile.exists():
        parsed=map_summary(mapfile.read_text());comparisons={s['name']:parsed['output_sections'].get(s['name'])==s['bytes'] for s in allocated}
        linker_map=dict(status='PARSED',file=mapfile.name,sha256=sha(mapfile),allocated_sections_match=all(comparisons.values()),section_checks=comparisons,project_input_sections=parsed['project_input_sections'])
        if not linker_map['allocated_sections_match']:raise ValueError('Linker map and ELF allocated sections disagree')
    violations=[f for f in frames if f['bytes']>2048];dynamic=[f for f in frames if 'dynamic' in f['kind'] and 'bounded' not in f['kind']]
    artifact=artifact_report(elf,g3a,build)
    return dict(schema_version=2,build_directory=label(build,source_root),elf_file=elf.name,elf_sha256=sha(elf),compiler=output('sh-elf-gcc','--version').splitlines()[0],
        native_text=text,native_data=data,native_bss=bss,berkeley_size_caution='Legacy size aggregates: text includes readonly data/ILRAM; data can include ROM metadata. These are not RAM peak categories.',
        elf_sections=sections,allocated_sections=allocated,non_allocated_section_bytes=sum(s['bytes'] for s in sections if 'ALLOC' not in s['flags']),
        initialized_load_section_bytes=sum(s['bytes'] for s in allocated if 'LOAD' in s['flags']),
        project_object_count=len(objects),project_object_sections=object_sections,project_object_bss=sum(n for k,n in object_sections.items() if k=='.bss' or k.startswith('.bss.')),project_object_caution='Pre-link component sums; do not add these to linked ELF sections or infer simultaneous peak.',
        linker_map=linker_map,symbols={s['name']:s['bytes'] for s in symbols},symbol_details=symbols,
        stack_frame_count=len(frames),largest_frames=frames[:25],frame_gate=dict(limit_bytes=2048,pass_gate=bool(frames) and not violations and not dynamic,coverage='PROJECT COMPILER REPORTS' if frames else 'NO FRAME REPORTS: NOT VERIFIED',over_limit=violations,unbounded_dynamic=dynamic),skipped_stack_reports=skipped,static_callgraph=graph,
        g3a=artifact,g3a_bytes=artifact['bytes'],sha256=artifact['sha256'],framebuffer_bytes=177408,
        platform_accounting=dict(framebuffer_payload_bytes=177408,framebuffer_margin_alignment_request_bytes=96,framebuffer_initial_request_bytes=177504,framebuffer_arena='_ostk',framebuffer_already_in_arena_stats=True,stack_reserved_bytes=16384,stack_lower_boundary='gint_stack_top',stack_upper_boundary='mmu_uram() + mmu_uram_size()',uram_arena='Unused linked user RAM up to the stack floor; query actual capacity.',ostk_arena_capacity_bytes=358400,overhead_definition='capacity-used-free is contemporaneous allocator metadata/header space, not exact original requested payload or a separate peak.',arena_peak_reset='Public peak_used_memory is a lifetime high-water mark; resetting app counters does not reset it.',source_scope='Installed gint2.11 fx-CG source assumptions; recheck when SDK/configuration changes.'),
        observations=dict(host=dict(status='NOT MEASURED BY THIS STATIC TOOL',stack_peak=None,heap_peak=None,scope='Use separate instrumented host workload report; host ABI/allocator results are not SH measurements.'),device=dict(status='NOT MEASURED / HARDWARE TEST REQUIRED',stack_peak=None,heap_peak=None,arena_peaks=None,scope='No physical fx-CG50 telemetry has been supplied.')),
        native_heap_peak='NOT MEASURED / HARDWARE TEST REQUIRED',native_stack_peak='NOT MEASURED / HARDWARE TEST REQUIRED',
        warnings=['No BSS+data+largest-frame/path sum is presented as a RAM peak.','Distinct arena/component high-water marks need not occur simultaneously; do not sum them into an observed peak.','Frame/graph files are accepted only beside an object at least as new; clean matching-source builds are still required.','ELF/map/debug section names are reported without workstation paths; raw linker maps contain SDK/local paths and should not be published unfiltered.'])

def markdown(report):
    graph=report['static_callgraph'];lines=['# Memory and build audit','',f"Measured native build: `{report['build_directory']}`. ELF SHA256 `{report['elf_sha256']}`.",
        'This report separates ELF placement, compiler static analysis, host observations',
        'and device observations. **No total runtime RAM peak is inferred.**',
        'Method and measurement limits: [MEMORY_METHOD.md](MEMORY_METHOD.md).','',
        '## A. Static placement','',
        '| ELF allocated section | Bytes | VMA | LMA | Flags |','|---|---:|---|---|---|']
    lines += [f"| `{s['name']}` | {s['bytes']:,} | `{s['vma']}` | `{s['lma']}` | {', '.join(s['flags'])} |" for s in report['allocated_sections']]
    lines += ['',f"GNU size aggregate text/data/BSS: {report['native_text']:,}/{report['native_data']:,}/{report['native_bss']:,} B.",
        'These legacy aggregates do not mean code/RAM/peak: `text` includes readonly',
        'data and ILRAM code; `.gint.drivers` can be ROM metadata despite DATA flags.',
        f"Pre-link project object BSS: {report['project_object_bss']:,} B across {report['project_object_count']} objects.",
        'Do not add object totals to the linked sections; they describe overlapping storage.',
        f"Linker map: {report['linker_map']['status']}; allocated-section cross-check: {report['linker_map'].get('allocated_sections_match','unavailable')}.",'',
        '| Selected native symbol | Bytes | ELF section |','|---|---:|---|']
    lines += [f"| `{s['name']}` | {s['bytes']:,} | `{s['section']}` |" for s in report['symbol_details']]
    lines += ['','The app/session, transaction probe, codec buffer and optional one-record grid',
        'cache are already contained in these sections. Only the symbols present in',
        'this ELF are counted; no whole-bank RAM cache is assumed.','',
        '## B. Compiler static analysis','',
        f"Compiler: `{report['compiler']}`. Individual-frame limit: 2,048 B; pass: {report['frame_gate']['pass_gate']}.",
        f"Accepted .su functions: {report['stack_frame_count']}; accepted .ci units: {graph['compiler_callgraph_files']}.",'',
        '| Function | Individual frame bytes | Kind |','|---|---:|---|']
    lines += [f"| `{f['function']}` | {f['bytes']} | {f['kind']} |" for f in report['largest_frames'][:15]]
    lines += ['',f"Callgraph status: **{graph['status']}**. Known definitions: {graph['project_definitions']}; unresolved edges: {len(graph['unresolved_edges'])}.",
        'The following sums include only known direct project continuations and',
        'checked recursive-component limits. They are conditional contributions,',
        '**not a whole-program bound and not observed stack peaks**.','',
        '| Direct-project entry/component | Known path bytes | Path/component sequence |','|---|---:|---|']
    for path in graph['root_paths'][:10]:
        sequence=[]
        for c in path['components']:
            names=' + '.join(n.rsplit(':',1)[-1] for n in c['functions']);sequence.append(('SCC('+names+')' if c['recursive'] else names))
        lines.append(f"| `{' + '.join(path['entry'])}` | {path['known_project_path_bytes']} | {' → '.join(sequence)} |")
    if not graph['root_paths']:lines.append('| No compiler callgraph artifacts | — | Rebuild with `-fcallgraph-info=su` |')
    lines += ['', 'The parser accepts atom nesting depth12; the rejected thirteenth atom still',
        'has a frame, as do its expression/term callers. When the matching guard is',
        'verified, the SCC uses a conservative13-frame limit for each member. Other',
        'unbounded recursion/dynamic frames are reported without an invented bound.',
        'Indirect game dispatch, hooks, callbacks, interrupt/OS frames and library/',
        'assembly internals remain unresolved. Tail-call reuse is not inferred;',
        'summing direct caller/callee frames may overestimate that path. The JSON',
        'lists all unresolved edges and any omitted recursive components.','',
        '## C. Host observations','',
        '**Not measured by this static report.** Consult the separate instrumented',
        'host workload evidence. Host sizeof/stack ABI and allocator peaks cannot',
        'be relabeled as physical SH/fx-CG50 measurements.','',
        '## D. Device observations and runtime accounting','',
        '**Device stack/heap/arena peaks: NOT MEASURED — HARDWARE TEST REQUIRED.**',
        'The installed gint fx-CG implementation requests177,408 B framebuffer',
        'payload plus96 B alignment/margins from `_ostk`. That allocation is already',
        'included in `_ostk` used/peak statistics; do not add it a second time.',
        '`_uram` is the unused linked user-RAM arena; `_ostk` has350 KiB capacity in',
        'the inspected SDK. Runtime capacity/used/free must come from public APIs.',
        '`gint_stack_top` is the **low boundary** of the reserved16 KiB stack;',
        'the high boundary is `mmu_uram()+mmu_uram_size()`. P1/P2 aliases describe',
        'the same memory and must not be added together. Use range-checked samples;',
        'do not overwrite active stack memory to obtain a watermark.',
        'Allocator capacity−used−free describes current metadata/headers, not an',
        'exact requested-payload history. Arena lifetime peaks are not reset by',
        'resetting application counters. Different component peaks are not assumed',
        'simultaneous. See the method document for workloads and coverage limits.','',
        '## Artifact identity','']
    a=report['g3a']
    if a['bytes'] is not None:lines += [f"G3A `{a['file']}`: **{a['bytes']:,} B**, SHA256 `{a['sha256']}`.",f"ELF binary payload equals G3A payload: **{a['elf_payload_match']}**.",f"1,000,000-byte target: {a['target_1000000_pass']}; 1,200,000-byte hard limit: {a['hard_limit_1200000_pass']}.",'Container checksum/identity validation remains a separate check; neither proves device execution.']
    else:lines.append('No matching G3A supplied. No package size/hash claim is made.')
    lines += ['','```sh','source tools/env.sh','python3 tools/memory_report.py --build-dir <fresh-native-build> --g3a dist/NUMGAME.g3a','python3 tools/stack_callgraph.py --self-test','python3 tools/memory_report.py --self-test','```','']
    return '\n'.join(lines)

def self_test():
    sample='''private : file format elf32-sh\n 0 .text 00000020 00300000 00300000 00000100 2**4\n CONTENTS, ALLOC, LOAD, READONLY, CODE\n 1 .gint.bss 00000070 08100100 08100100 00000200 2**2\n ALLOC\n 2 .debug_info 00000010 00000000 00000000 00000300 2**0\n CONTENTS, READONLY, DEBUGGING, OCTETS\n'''
    parsed=parse_sections(sample);assert [(s['name'],s['bytes']) for s in parsed]==[('.text',32),('.gint.bss',112),('.debug_info',16)]
    assert parse_size_sections('section size addr\n.text 32 3145728\n.gint.bss 112 135266560\nTotal 144')=={'.text':32,'.gint.bss':112}
    fixture_home='/'+'Users/'+'private/sdk/'
    mapped=map_summary('.text 0x00300000 0x20\n .bss 0x08100000 0x10 CMakeFiles/numgame.dir/src/a.c.obj\n .bss 0x08100010 0x8 '+fixture_home+'lib.a(file.o)')
    assert mapped['output_sections']=={'.text':32} and mapped['project_input_sections']=={'.bss':16}
    assert label('/private/sdk/example','/other/repo')=='example'
    assert selected_symbols('08100100 00000020 b _app\n00300000 00000010 T _ng_encode',parsed)==[dict(name='_app',bytes=32,type='b',section='.gint.bss')]
    stack_callgraph.self_test();print('memory_report self-test: section flags/hex sizes, GNU size cross-check, private SDK map exclusion and symbol placement PASS')

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--build-dir',type=Path,default=Path('build-cg'));ap.add_argument('--source-root',type=Path,default=ROOT);ap.add_argument('--g3a',type=Path);ap.add_argument('--json-out',type=Path,default=ROOT/'docs/build-metrics.json');ap.add_argument('--markdown-out',type=Path,default=ROOT/'docs/MEMORY_AUDIT.md');ap.add_argument('--self-test',action='store_true');args=ap.parse_args()
    if args.self_test:self_test();return
    build=args.build_dir if args.build_dir.is_absolute() else ROOT/args.build_dir;build=build.resolve()
    report=build_report(build,args.source_root,args.g3a.resolve() if args.g3a else None)
    args.json_out.write_text(json.dumps(report,indent=2)+'\n');args.markdown_out.write_text(markdown(report))
    print(json.dumps(dict(build=report['build_directory'],allocated_sections=len(report['allocated_sections']),project_object_bss=report['project_object_bss'],frame_gate=report['frame_gate']['pass_gate'],callgraph_units=report['static_callgraph']['compiler_callgraph_files'],g3a_bytes=report['g3a_bytes'],elf_payload_match=report['g3a']['elf_payload_match'],device_peak=report['observations']['device']['status']),indent=2))
    if not report['frame_gate']['pass_gate'] or report['g3a']['elf_payload_match'] is False or report['g3a'].get('hard_limit_1200000_pass') is False:raise SystemExit(1)
if __name__=='__main__':main()
