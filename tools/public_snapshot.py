#!/usr/bin/env python3
"""Create and audit an allowlisted local source snapshot without Git history.

No remote operations, tracked-file changes, redaction, or executable rewriting.
Findings show repository-relative file names and types, never matched values.
"""
from __future__ import annotations
import argparse,hashlib,json,os,posixpath,re,shutil,struct,subprocess,tempfile,zlib
from pathlib import Path,PurePosixPath
from urllib.parse import unquote,urlsplit

ROOT=Path(__file__).resolve().parents[1]
POLICY_VERSION=1
MANIFEST='PUBLIC_MANIFEST.json'
ROOT_FILES={'.gitignore','CMakeLists.txt','LICENSE','README.md','README_KO.md','THIRD_PARTY_NOTICES.md'}
PUBLIC_DOCS={
    'ACCEPTANCE.md','AI_AUDIT.md','ASSET_PROVENANCE.md','BOARD_AUDIT.md',
    'CAPABILITIES.md','CONTENT_INVENTORY.md','CONTENT_INVENTORY.csv',
    'CONTROLS.md','DIAGNOSTICS.md','DIAGNOSTIC_GUIDE.md','GAME_CATALOG.md',
    'GENERATION_AUDIT.md','GRID_AUDIT.md','GRID_MASTER_AUDIT.md',
    'GRID_LOGIC_REVIEW.md',
    'GUESS_CALC_AUDIT.md','HARDWARE_RETEST.md','MEMORY_AUDIT.md',
    'MEMORY_METHOD.md','PERFORMANCE_AUDIT.md','PERFORMANCE_36.md','PERFORMANCE.md',
    'host-benchmark-normal.json','host-benchmark-diagnostic.json',
    'host-benchmark-36-normal.json','host-benchmark-36-diagnostic.json',
    'host-benchmark-v5-normal.json','host-benchmark-v5-diagnostic.json',
    'host-benchmark-build-evidence.json',
    'POWER_TIMER_AUDIT.md','PUBLIC_SOURCE_AUDIT.md','RULES.md',
    'RESUME_POLICY.md','CONTENT_CYCLE.md','RECENT_SIX_AUDIT.md',
    'RUNTIME_AUDIT.md','STORAGE_ARCHIVE_AUDIT.md','STORAGE_FORMAT.md',
    'STRATEGY_QUICK_AUDIT.md','UI_CONVENTIONS.md','UI_LAYOUT_KO.md',
    'UI_REFRESH_KO.md','acceptance-matrix.csv','asset-manifest.json',
    'build-metrics.json','diagnostic-build-metrics.json','game-registry.json',
    'performance-metrics.json',
}
LICENSE_FILES={
    'DIFFEQ-LICENSE.txt','GCC-RUNTIME-EXCEPTION.txt','GPL-3.0.txt',
    'OpenLibm-LICENSE.md','SOKOBAN-LICENSE.txt','fxSDK-LICENSE.txt',
    'fxlibc-Grisu2b-LICENSE.txt','fxlibc-LICENSE.txt','gint-README.md',
}
ASSET_JSON_ROOT={'guesscalc_packs.json','guesscalc_master.json','guesscalc_cryptarithm.json'}
ASSET_HEADER_ROOT={'guesscalc_packs.h','guesscalc_master.h','guesscalc_cryptarithm.h'}
ASSET_JSON_SUB={
    'boards':{'puzzles.json','verification.json'},
    'strategyquick':{'metadata.json','master.json','master_verification.json','master-sh-metrics.json','extra-native-frames.json','extra-asan-attempt.json'},
}
DIST_FILES={'NUMGAME.g3a','NUMGDIAG.g3a','SHA256SUMS.txt'}
TEXT_SUFFIXES={'.c','.h','.S','.py','.sh','.md','.txt','.json','.csv','.cmake'}
FORBIDDEN_PARTS={'.git','.local','.worktrees','__pycache__','.DS_Store','node_modules'}
PATTERNS={
    'absolute_personal_home':re.compile(rb'(?:/(?:Users|home)/[^\s/"\x27]+|[A-Za-z]:[\\/]Users[\\/][^\s\\/"\x27]+)'),
    'absolute_private_temp':re.compile(rb'/private/(?:var|tmp)/'),
    'access_token':re.compile(rb'(?:github_pat_[A-Za-z0-9_]{20,}|gh[pousr]_[A-Za-z0-9]{20,}|sk-[A-Za-z0-9]{24,}|xox[baprs]-[A-Za-z0-9-]{20,})'),
    'cloud_access_key':re.compile(rb'\b(?:AKIA|ASIA)[A-Z0-9]{16}\b'),
    'private_key':re.compile(rb'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----'),
    'url_embedded_password':re.compile(rb'\b(?:https?|ftp)://[^\s/:]+:[^\s/@]+@'),
}
INTERNAL_REFERENCE=re.compile(r'(?:REQUIREMENTS(?:_[A-Z_]+)?\.txt|(?:docs/)?(?:HANDOFF|WORK_MASTERY|WORK_RUNTIME)\.md|AGENTS\.md|[^\s`]*asan-sample\.txt)')

class SnapshotError(ValueError):
    """Fixed, safe descriptions that never interpolate inspected content."""

def public_png(name):
    if name in {'assets/icon-sel.png','assets/icon-uns.png','assets/font/font5x7.png','assets/font/font8x9.png','assets/strategyquick/master-contact.png'}:return True
    if re.fullmatch(r'assets/grids/(?:(?:1[1-9]|20)|contact(?:-2x)?)\.png',name):return True
    if re.fullmatch(r'assets/grids/master/(?:capture-\d+|contact)\.png',name):return True
    if re.fullmatch(r'assets/grids/expanded/(?:capture-\d+-mode\d+|contact)\.png',name):return True
    if re.fullmatch(r'assets/grids/extra/(?:capture-\d+-\d+|contact)\.png',name):return True
    if re.fullmatch(r'assets/grids/extra/review-20260926/(?:contact|hashi-(?:empty|single|double|crossing)|nonogram-zero-marks)\.png',name):return True
    if re.fullmatch(r'assets/strategyquick/extra-\d+-master\.png',name):return True
    if re.fullmatch(r'assets/strategyquick/extra-(?:37-(?:cpu|result|result-modal)|38-result)\.png',name):return True
    if re.fullmatch(r'assets/boards/captures/(?:(?:31|32)-[0-3]-(?:complete|corner|start|edge)|(?:completed-)?contact)\.png',name):return True
    if re.fullmatch(r'docs/captures/(?:\d{2}-[a-z0-9-]+|category-[1-6]-2x|[a-z-]+-native|menus-2x)\.png',name):return True
    return False

def allowed(name,include_dist=False):
    p=PurePosixPath(name);parts=p.parts
    if not parts or p.is_absolute() or '..' in parts:return False
    if any(part in FORBIDDEN_PARTS or part=='build' or part.startswith('build-') for part in parts[:-1]) or parts[-1] in FORBIDDEN_PARTS:return False
    if name in ROOT_FILES:return True
    if parts[0] in {'src','include'}:return p.suffix in {'.c','.h','.S'}
    if parts[0]=='tests':return p.suffix in {'.c','.h','.py','.sh','.md'} or p.name=='CMakeLists.txt'
    if parts[0]=='tools':return p.suffix in {'.c','.h','.py','.sh'} or name=='tools/toolchain-lock.json'
    if parts[0]=='assets':
        if p.suffix=='.png':return public_png(name)
        if len(parts)==2:return p.name in ASSET_JSON_ROOT|ASSET_HEADER_ROOT
        if parts[1]=='grids':return bool(re.fullmatch(r'assets/grids/(?:(?:1[1-9]|20)|audit)\.json',name) or re.fullmatch(r'assets/grids/master/(?:(?:1[1-9]|20)(?:-generation)?|audit|legacy-sha256)\.json',name) or re.fullmatch(r'assets/grids/expanded/(?:(?:1[1-5]-[0-4]|1[6-9]-3|20-3)(?:-generation)?|audit|capture-selection|memory|seed-replay)\.json',name) or re.fullmatch(r'assets/grids/extra/(?:(?:35-[0-4]|36-[0-3])(?:-generation)?|audit|capture-selection|memory)\.json',name) or re.fullmatch(r'assets/grids/extra/review-20260926/(?:audit\.json|independent-validation\.txt|host-(?:ubsan|asan)\.txt)',name) or name=='assets/grids/extra/generate.py')
        if parts[1] in ASSET_JSON_SUB:
            return len(parts)==3 and (p.name in ASSET_JSON_SUB[parts[1]] or (parts[1]=='strategyquick' and p.name in {'tables.h','master_quick.h','master_strategy.h'}))
        return False
    if parts[0]=='docs':
        if len(parts)==2:return p.name in PUBLIC_DOCS
        if parts[1]=='third_party':return len(parts)==3 and p.name in LICENSE_FILES
        if parts[1]=='captures':return public_png(name) or name=='docs/captures/README.md'
    return bool(include_dist and len(parts)==2 and parts[0]=='dist' and p.name in DIST_FILES)

def inventory(root,include_dist=False):
    result=[];excluded={};findings=[]
    def visit(directory):
        for item in sorted(directory.iterdir(),key=lambda p:p.name):
            relative=item.relative_to(root).as_posix()
            if item.is_symlink():
                if allowed(relative,include_dist):findings.append(dict(file=relative,type='allowlisted_symlink',severity='error'))
                else:excluded['symlinks']=excluded.get('symlinks',0)+1
                continue
            if item.is_dir():
                parts=item.relative_to(root).parts
                if any(p in FORBIDDEN_PARTS or p=='build' or p.startswith('build-') for p in parts):
                    excluded['private_or_build_directories']=excluded.get('private_or_build_directories',0)+1;continue
                if len(parts)==1 and parts[0] not in {'src','include','tests','tools','assets','docs','dist'}:
                    excluded['outside_allowed_directories']=excluded.get('outside_allowed_directories',0)+1;continue
                visit(item)
            elif item.is_file():
                if allowed(relative,include_dist):result.append(relative)
                else:excluded['not_allowlisted_files']=excluded.get('not_allowlisted_files',0)+1
    visit(root)
    return result,excluded,findings

def png_text(data):
    if not data.startswith(b'\x89PNG\r\n\x1a\n'):raise SnapshotError('PNG signature')
    offset=8;text=[]
    while offset+12<=len(data):
        n=struct.unpack_from('>I',data,offset)[0];kind=data[offset+4:offset+8]
        if n>len(data)-offset-12:raise SnapshotError('PNG truncated chunk')
        payload=data[offset+8:offset+8+n];offset+=n+12
        def inflate(value):
            decoder=zlib.decompressobj();decoded=decoder.decompress(value,1024*1024+1)
            if len(decoded)>1024*1024 or decoder.unconsumed_tail or not decoder.eof or decoder.unused_data:raise SnapshotError('PNG oversized/invalid compressed metadata')
            return decoded
        if kind==b'zTXt':
            fields=payload.split(b'\0',1)
            if len(fields)!=2 or not fields[1] or fields[1][0]!=0:raise SnapshotError('PNG invalid compressed text')
            text.append(fields[0]+b'\0'+inflate(fields[1][1:]))
        if kind==b'iTXt':
            fields=payload.split(b'\0',1)
            if len(fields)!=2 or len(fields[1])<3 or fields[1][0] not in (0,1) or fields[1][1]!=0:raise SnapshotError('PNG invalid international text')
            compressed=fields[1][0];rest=fields[1][2:].split(b'\0',2)
            if len(rest)!=3:raise SnapshotError('PNG invalid international text fields')
            text.extend([fields[0],rest[0],rest[1],inflate(rest[2]) if compressed else rest[2]])
        if kind==b'eXIf':raise SnapshotError('PNG unsupported EXIF metadata')
        if kind==b'tEXt':text.append(payload)
        if kind==b'IEND':
            if offset!=len(data):raise SnapshotError('PNG trailing data')
            return b'\n'.join(text)
    raise SnapshotError('PNG missing IEND')

def content_findings(name,data):
    findings=[];blobs=[data]
    if name.endswith('.png'):
        try:blobs.append(png_text(data))
        except (ValueError,zlib.error):findings.append(dict(file=name,type='png_opaque_or_invalid_metadata',severity='error'))
    for kind,pattern in PATTERNS.items():
        if any(pattern.search(blob) for blob in blobs):findings.append(dict(file=name,type=kind,severity='error'))
    if PurePosixPath(name).suffix in TEXT_SUFFIXES or PurePosixPath(name).name in {'LICENSE','CMakeLists.txt','.gitignore'}:
        try:data.decode('utf-8')
        except UnicodeDecodeError:findings.append(dict(file=name,type='non_utf8_public_text',severity='error'))
    return findings

def reference_findings(name,data,selected):
    if not name.endswith('.md'):return []
    text=data.decode('utf-8',errors='replace');result=[]
    if INTERNAL_REFERENCE.search(text):result.append(dict(file=name,type='internal_document_mention_review',severity='warning'))
    missing=set()
    for target in re.findall(r'\]\(([^\s)]+)(?:\s+[^)]*)?\)',text):
        target=target.strip('<>')
        try:parsed=urlsplit(target)
        except ValueError:
            result.append(dict(file=name,type='invalid_link_syntax',severity='warning'));continue
        if parsed.scheme or parsed.netloc or not parsed.path:continue
        resolved=posixpath.normpath(posixpath.join(posixpath.dirname(name),unquote(parsed.path)))
        if resolved.startswith('../') or resolved.startswith('/') or (resolved not in selected and not any(p.startswith(resolved.rstrip('/')+'/') for p in selected)):
            missing.add(resolved)
    # Never echo the target: an unexpected absolute target could be sensitive.
    if missing:result.append(dict(file=name,type='upstream_notice_local_link_preserved' if name.startswith('docs/third_party/') else 'missing_or_excluded_local_link',severity='info' if name.startswith('docs/third_party/') else 'warning',count=len(missing)))
    return result

def required_findings(selected):
    required=ROOT_FILES- {'.gitignore'}|{
        'src/main.c','include/ng.h','tests/CMakeLists.txt','tools/clean_build.sh',
        'tools/check_embedded.py','tools/verify_content.sh','tools/toolchain-lock.json',
        'tools/generate/boards_generate.py','tools/generate/boards_verify.py',
        'tools/generate/grids_generate.py','tools/generate/grids_verify.py',
        'tools/generate/grids_master_generate.py','tools/generate/grids_master_verify.py',
        'tools/generate/guesscalc_generate.py','tools/generate/guesscalc_verify.py',
        'tools/generate/guesscalc_master.py','tools/generate/guesscalc_verify_master.py',
        'tools/generate/strategyquick_master.py','tools/generate/strategyquick_master_verify.py',
        'tools/generate/strategyquick_tables.py',
        'assets/boards/puzzles.json','assets/guesscalc_packs.json','assets/guesscalc_master.json',
        'assets/strategyquick/master.json','assets/strategyquick/tables.h',
        'assets/guesscalc_packs.h','assets/guesscalc_master.h','src/ui/font_data.h',
        'assets/guesscalc_cryptarithm.h','assets/guesscalc_cryptarithm.json',
        'assets/icon-uns.png','assets/icon-sel.png','assets/font/font5x7.png','assets/font/font8x9.png',
        'src/games/boards_pack.c','src/games/grids_pack.c','src/games/grids_pack_data.h',
        'assets/strategyquick/master_quick.h','assets/strategyquick/master_strategy.h',
        'tools/generate/grids_compact.py','tools/generate/grids_expand.py',
        'tools/generate/grids_expanded_verify.py',
        'assets/grids/extra/generate.py','assets/grids/extra/audit.json',
    }|{'docs/third_party/'+n for n in LICENSE_FILES}|{f'assets/grids/{n}.json' for n in range(11,21)}|{f'assets/grids/expanded/{n}-{d}.json' for n in range(11,21) for d in (range(5) if n<=15 else (3,))}
    return [dict(file=n,type='required_public_file_missing',severity='error') for n in sorted(required-set(selected))]

def sha(data):return hashlib.sha256(data).hexdigest()

def records(root,names):
    result=[]
    for name in names:
        path=root/name;data=path.read_bytes();mode='0755' if path.stat().st_mode&0o111 else '0644'
        result.append(dict(file=name,bytes=len(data),sha256=sha(data),mode=mode))
    return result

def tree_digest(entries):
    return sha(''.join(f"{r['file']}\0{r['mode']}\0{r['bytes']}\0{r['sha256']}\n" for r in sorted(entries,key=lambda x:x['file'])).encode())

def package_findings(data,entries):
    actual={r['file']:r['sha256'] for r in entries};listed={};invalid=False
    for line in data.decode('utf-8',errors='replace').splitlines():
        if not line.strip():continue
        fields=line.split(maxsplit=1)
        if len(fields)!=2 or not re.fullmatch('[0-9a-fA-F]{64}',fields[0]):invalid=True;continue
        name=fields[1].removeprefix('*').removeprefix('./')
        if name not in {'NUMGAME.g3a','NUMGDIAG.g3a'} or name in listed:invalid=True;continue
        listed[name]=fields[0].lower()
    if set(listed)!={'NUMGAME.g3a','NUMGDIAG.g3a'} or any(actual.get('dist/'+n)!=v for n,v in listed.items()):invalid=True
    return [dict(file='dist/SHA256SUMS.txt',type='package_checksum_list_mismatch',severity='error')] if invalid else []

def audit(root,include_dist=False,require_files=True,strict_links=False):
    names,excluded,findings=inventory(root,include_dist);selected=set(names)
    if require_files:findings+=required_findings(selected)
    if include_dist:findings.extend(dict(file='dist/'+n,type='requested_package_missing',severity='error') for n in sorted(DIST_FILES) if 'dist/'+n not in selected)
    entries=[];checksums=None
    for name in names:
        path=root/name;data=path.read_bytes();findings+=content_findings(name,data)+reference_findings(name,data,selected)
        # Hash the bytes actually scanned, not a second read which might race
        # concurrent development changes between the scan and manifest creation.
        entries.append(dict(file=name,bytes=len(data),sha256=sha(data),mode='0755' if path.stat().st_mode&0o111 else '0644'))
        if name=='dist/SHA256SUMS.txt':checksums=data
    if checksums is not None:findings+=package_findings(checksums,entries)
    if strict_links:
        for finding in findings:
            if finding['type']=='missing_or_excluded_local_link':finding['severity']='error'
    return dict(policy_version=POLICY_VERSION,status='FAIL' if any(f['severity']=='error' for f in findings) else 'PASS WITH WARNINGS' if any(f['severity']=='warning' for f in findings) else 'PASS',file_count=len(names),file_bytes=sum(x['bytes'] for x in entries),tree_sha256=tree_digest(entries),include_dist=include_dist,excluded_counts=excluded,findings=findings,files=entries)

def history_audit(root):
    """Read blobs reachable from local refs; no history or author data exported."""
    proc=subprocess.run(['git','-C',str(root),'rev-list','--objects','--all'],capture_output=True,text=True)
    if proc.returncode:return dict(status='NO LOCAL GIT HISTORY',checked_text_blobs=0,findings=[])
    blobs={}
    for line in proc.stdout.splitlines():
        parts=line.split(' ',1)
        if len(parts)!=2:continue
        oid,name=parts
        if PurePosixPath(name).suffix in TEXT_SUFFIXES or PurePosixPath(name).name in ROOT_FILES:blobs[oid]=name
    findings=[];checked=0
    for oid,name in sorted(blobs.items()):
        content=subprocess.run(['git','-C',str(root),'cat-file','blob',oid],capture_output=True)
        if content.returncode:continue
        checked+=1
        for item in content_findings(name,content.stdout):
            item['blob']=oid;findings.append(item)
    return dict(status='PRIVATE FINDINGS: KEEP HISTORY LOCAL' if findings else 'NO MATCHES IN SCANNED BLOBS',checked_text_blobs=checked,findings=findings,scope='Reachable text blobs only; regex scanning is not proof of absence of secrets. Commit author/message metadata is not copied, and no history is exported.')

def verify_snapshot(root):
    manifest_path=root/MANIFEST
    if not manifest_path.is_file():raise SnapshotError('Snapshot manifest is missing')
    manifest=json.loads(manifest_path.read_text())
    if not isinstance(manifest,dict) or manifest.get('policy_version')!=POLICY_VERSION or manifest.get('tool')!='tools/public_snapshot.py':raise SnapshotError('Unrecognized snapshot manifest')
    if type(manifest.get('include_dist')) is not bool or not isinstance(manifest.get('files'),list):raise SnapshotError('Invalid snapshot manifest fields')
    if any(not isinstance(r,dict) or set(r)!={'file','bytes','sha256','mode'} or not isinstance(r['file'],str) or type(r['bytes']) is not int or r['bytes']<0 or not isinstance(r['sha256'],str) or not re.fullmatch('[0-9a-f]{64}',r['sha256']) or r['mode'] not in {'0644','0755'} for r in manifest['files']):raise SnapshotError('Invalid snapshot manifest entry')
    names=[r['file'] for r in manifest['files']]
    if len(set(names))!=len(names) or any(not allowed(n,manifest['include_dist']) for n in names):raise SnapshotError('Manifest contains duplicate or non-allowlisted paths')
    for name in names:
        path=root/name
        if any(p.is_symlink() for p in [path,*path.parents] if p!=root.parent):raise SnapshotError('Snapshot path contains a symlink')
    actual=records(root,names)
    if actual!=manifest['files'] or tree_digest(actual)!=manifest['tree_sha256']:raise SnapshotError('Snapshot bytes/modes do not match its manifest')
    present=set()
    for directory,subdirs,files in os.walk(root,followlinks=False):
        for name in subdirs+files:
            path=Path(directory)/name
            if path.is_symlink():raise SnapshotError('Snapshot contains an unexpected symlink')
            if name=='.git':raise SnapshotError('Snapshot contains Git history')
        present.update((Path(directory)/name).relative_to(root).as_posix() for name in files)
    if present!=set(names)|{MANIFEST}:raise SnapshotError('Snapshot contains missing or unexpected files')
    return dict(status='PASS',file_count=len(actual),tree_sha256=manifest['tree_sha256'],scope='Exact allowlisted bytes/modes and no extra files or Git history. Run builds outside this clean snapshot.')

def create_snapshot(root,destination,include_dist=False,replace=False,strict_links=False):
    base=root/'.local/public';destination=destination.absolute()
    if destination==base or base.resolve() not in destination.resolve().parents:raise SnapshotError('Destination must be a child of source .local/public')
    if any(p.is_symlink() for p in [destination,*destination.parents] if p!=root.parent):raise SnapshotError('Destination parent contains a symlink')
    report=audit(root,include_dist,strict_links=strict_links)
    if report['status']=='FAIL':return report
    if destination.exists():
        if not replace:raise SnapshotError('Destination exists; use a new name or explicitly --replace a recognized snapshot')
        verify_snapshot(destination)
    base.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.stage-',dir=base) as temporary:
        stage=Path(temporary)
        for entry in report['files']:
            source=root/entry['file'];target=stage/entry['file'];target.parent.mkdir(parents=True,exist_ok=True)
            # Copy only the previously audited regular file; detect concurrent
            # source edits and reject them instead of silently mixing snapshots.
            if source.is_symlink():raise SnapshotError('Source changed to symlink during snapshot')
            data=source.read_bytes()
            if sha(data)!=entry['sha256']:raise SnapshotError('Source changed while snapshot was being copied')
            target.write_bytes(data);target.chmod(int(entry['mode'],8))
        manifest=dict(tool='tools/public_snapshot.py',policy_version=POLICY_VERSION,include_dist=include_dist,tree_sha256=report['tree_sha256'],files=report['files'],history='NOT COPIED',claims='Byte-preserving source snapshot; build/content checks must be run separately.')
        (stage/MANIFEST).write_text(json.dumps(manifest,indent=2)+'\n')
        verify_snapshot(stage)
        destination.parent.mkdir(parents=True,exist_ok=True)
        if destination.exists():shutil.rmtree(destination)
        os.rename(stage,destination)
    report['snapshot']=destination.relative_to(root).as_posix();report['manifest_verification']=verify_snapshot(destination)
    return report

def self_test():
    assert allowed('src/main.c') and allowed('tests/native/gint/bfile.h') and allowed('assets/boards/puzzles.json')
    assert allowed('docs/third_party/GPL-3.0.txt') and allowed('tools/generate/grids_master_verify.py')
    assert allowed('tools/benchmark_report.py') and allowed('docs/PERFORMANCE_AUDIT.md')
    assert allowed('docs/build-metrics.json') and allowed('docs/diagnostic-build-metrics.json')
    assert all(allowed('docs/'+name+'.json') for name in ('host-benchmark-normal','host-benchmark-diagnostic','host-benchmark-build-evidence'))
    assert all(allowed('assets/grids/expanded/'+name+'.json') for name in ('audit','capture-selection','memory','seed-replay'))
    for name in ('AGENTS.md','docs/HANDOFF.md','docs/FINAL_REPORT_KO.md','docs/REQUIREMENTS_KO.txt','assets/grids/asan-sample.txt','docs/validation/run.txt','test.dat','private.pdf','.git/config','src/../private.c','src/build-leak/a.c','assets/boards/a.dat'):
        assert not allowed(name),name
    assert not allowed('dist/NUMGAME.g3a') and allowed('dist/NUMGAME.g3a',True)
    assert not allowed('assets/private-photo.png') and not allowed('assets/grids/private-export.json')
    fake_entries=[dict(file='dist/'+n,sha256='a'*64) for n in ('NUMGAME.g3a','NUMGDIAG.g3a')]
    assert not package_findings(('a'*64+'  ./NUMGAME.g3a\n'+'a'*64+'  NUMGDIAG.g3a\n').encode(),fake_entries)
    assert package_findings(('a'*64+'  NUMGAME.g3a\n').encode(),fake_entries)
    home=('/'+'Users/'+'fixture/'+'a.c').encode();findings=content_findings('src/a.c',home)
    assert findings[0]['type']=='absolute_personal_home' and home.decode() not in json.dumps(findings)
    key=b'gh' + b'p_' + b'A'*32;assert content_findings('src/a.c',key)
    assert reference_findings('docs/a.md',b'[private](HANDOFF.md)',{'docs/a.md'})[0]['type']=='internal_document_mention_review'
    assert reference_findings('docs/a.md',b'[ok](../LICENSE)',{'docs/a.md','LICENSE'})==[]
    chunk=lambda k,p:struct.pack('>I',len(p))+k+p+struct.pack('>I',zlib.crc32(k+p)&0xffffffff)
    png=b'\x89PNG\r\n\x1a\n'+chunk(b'tEXt',b'Note\0'+home)+chunk(b'IEND',b'')
    assert content_findings('assets/x.png',png)
    png=b'\x89PNG\r\n\x1a\n'+chunk(b'zTXt',b'x\0\0'+zlib.compress(home))+chunk(b'IEND',b'')
    assert content_findings('assets/x.png',png)[0]['type']=='absolute_personal_home'
    png=b'\x89PNG\r\n\x1a\n'+chunk(b'zTXt',b'x\0\0'+zlib.compress(b'A'*(1024*1024+1)))+chunk(b'IEND',b'')
    assert content_findings('assets/x.png',png)[0]['type']=='png_opaque_or_invalid_metadata'
    # Fixture stays inside this repository and contains no original files.
    (ROOT/'.local').mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='snapshot-selftest-',dir=ROOT/'.local') as temporary:
        fixture=Path(temporary);(fixture/'src').mkdir();(fixture/'src/a.c').write_text('int a;\n')
        names,_,issues=inventory(fixture);assert names==['src/a.c'] and not issues
        (fixture/'src/link.c').symlink_to('a.c');assert inventory(fixture)[2][0]['type']=='allowlisted_symlink'
        (fixture/'src/link.c').unlink()
        first=audit(fixture,require_files=False);second=audit(fixture,require_files=False);assert first['tree_sha256']==second['tree_sha256']
        manifest=dict(tool='tools/public_snapshot.py',policy_version=POLICY_VERSION,include_dist=False,tree_sha256=first['tree_sha256'],files=first['files'])
        (fixture/MANIFEST).write_text(json.dumps(manifest));assert verify_snapshot(fixture)['status']=='PASS'
        (fixture/'extra.dat').write_bytes(b'private fixture')
        try:verify_snapshot(fixture)
        except ValueError:pass
        else:raise AssertionError('Unexpected private file was accepted')
        (fixture/'extra.dat').unlink()
        (fixture/'src/a.c').write_text('int b;\n')
        try:verify_snapshot(fixture)
        except ValueError:pass
        else:raise AssertionError('Tampered file was accepted')
    with tempfile.TemporaryDirectory(prefix='snapshot-roundtrip-',dir=ROOT/'.local') as temporary:
        fixture=Path(temporary)
        for required in required_findings(set()):
            path=fixture/required['file'];path.parent.mkdir(parents=True,exist_ok=True)
            path.write_bytes(b'fixture\n' if path.suffix!='.png' else b'\x89PNG\r\n\x1a\n'+chunk(b'IEND',b''))
        (fixture/'tools/clean_build.sh').chmod(0o755)
        first=create_snapshot(fixture,fixture/'.local/public/a');assert first['status']=='PASS'
        second=create_snapshot(fixture,fixture/'.local/public/b');assert first['tree_sha256']==second['tree_sha256']
        assert (fixture/'.local/public/a'/MANIFEST).read_bytes()==(fixture/'.local/public/b'/MANIFEST).read_bytes()
        assert create_snapshot(fixture,fixture/'.local/public/a',replace=True)['status']=='PASS'
        (fixture/'README.md').write_text('[missing](private.pdf)\n')
        assert audit(fixture,strict_links=True)['status']=='FAIL'
        assert not (fixture/'.git').exists()
    print('public_snapshot self-test: allowlist, privacy output, tokens, links, bounded PNG metadata, symlinks, deterministic two-copy manifest, replacement, extra-file/tamper rejection PASS')

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--source-root',type=Path,default=ROOT)
    ap.add_argument('--output',type=Path);ap.add_argument('--include-dist',action='store_true');ap.add_argument('--replace',action='store_true')
    ap.add_argument('--audit-history',action='store_true');ap.add_argument('--verify',type=Path);ap.add_argument('--report',type=Path)
    ap.add_argument('--strict-links',action='store_true',help='Fail for missing/excluded project Markdown links; preserve upstream notices unchanged.')
    ap.add_argument('--self-test',action='store_true');args=ap.parse_args()
    if args.self_test:self_test();return
    root=args.source_root.resolve()
    if args.verify:report=verify_snapshot(args.verify.resolve())
    elif args.output:
        destination=args.output if args.output.is_absolute() else root/args.output
        report=create_snapshot(root,destination,args.include_dist,args.replace,args.strict_links)
    else:report=audit(root,args.include_dist,strict_links=args.strict_links)
    if args.audit_history:report['history_audit']=history_audit(root)
    # Detailed manifest has only allowlisted paths; public console stays compact.
    if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
    compact={k:v for k,v in report.items() if k!='files'}
    print(json.dumps(compact,indent=2))
    if report['status']=='FAIL':raise SystemExit(1)

if __name__=='__main__':
    try:main()
    except (ValueError,OSError,json.JSONDecodeError) as error:
        # Built-in exceptions can contain local paths or sensitive filenames.
        # Custom ValueError messages above contain only fixed safe descriptions.
        if isinstance(error,SnapshotError):message=str(error)
        else:message=type(error).__name__+' during snapshot operation; no source was rewritten'
        print(json.dumps(dict(status='FAIL',type=message)));raise SystemExit(1)
