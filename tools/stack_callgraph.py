#!/usr/bin/env python3
"""Conditional project stack paths from GCC -fcallgraph-info=su VCG files.

Never computes a whole-program/device stack peak. Unresolved library, indirect
and callback edges remain explicit; recursion requires a checked source policy.
No third-party graph parser or copied analysis implementation is used.
"""
from __future__ import annotations
import argparse,hashlib,json,random,re
from pathlib import Path

QUOTED=r'"(?:\\.|[^"\\])*"'
BLOCK=re.compile(r'\b(node|edge)\s*:\s*\{((?:[^"{}]|'+QUOTED+r')*)\}',re.S)
ATTRIBUTE=re.compile(r'(\w+)\s*:\s*('+QUOTED+r')')

def public_source(value):
    """Keep repository-relative source names, never workstation/SDK prefixes."""
    value=value.replace('\\','/')
    match=re.search(r'(?:^|/)((?:src|include|tests|tools)/[^\n]+)$',value)
    if match:return match[1]
    if value.startswith('/') or re.match(r'^[A-Za-z]:/',value):return '<external>/'+value.rsplit('/',1)[-1]
    return value.removeprefix('./')

def public_id(value):
    if '/' in value:return public_source(value)
    return value

def parse_text(text,nodes=None,edges=None):
    nodes={} if nodes is None else nodes;edges=set() if edges is None else edges
    for kind,body in BLOCK.findall(text):
        attrs={k:json.loads(v) for k,v in ATTRIBUTE.findall(body)}
        if kind=='edge':
            if 'sourcename' in attrs and 'targetname' in attrs:edges.add((public_id(attrs['sourcename']),public_id(attrs['targetname'])))
            continue
        if 'title' not in attrs:continue
        key=public_id(attrs['title']);label=attrs.get('label','');frame=re.search(r'(\d+) bytes \(([^)]+)\)',label)
        if frame:
            definition=dict(function=key,bytes=int(frame[1]),kind=frame[2])
            if key in nodes and nodes[key]!=definition:raise ValueError('Conflicting compiler definitions for '+key)
            nodes[key]=definition
    return nodes,edges

def strongly_connected(nodes,edges):
    adjacency={n:set() for n in nodes}
    for a,b in edges:
        if a in nodes and b in nodes:adjacency[a].add(b)
    count=0;indices={};low={};stack=[];active=set();components=[]
    def visit(node):
        nonlocal count
        indices[node]=low[node]=count;count+=1;stack.append(node);active.add(node)
        for child in sorted(adjacency[node]):
            if child not in indices:visit(child);low[node]=min(low[node],low[child])
            elif child in active:low[node]=min(low[node],indices[child])
        if low[node]==indices[node]:
            component=[]
            while True:
                child=stack.pop();active.remove(child);component.append(child)
                if child==node:break
            components.append(sorted(component))
    for node in sorted(nodes):
        if node not in indices:visit(node)
    return components

def parser_policy(nodes,source_root):
    """Manual source reasoning, conditionally tied to the checked source hash.

    Accepted atom depth is12. The rejected thirteenth atom still allocates a
    frame; a parenthesized depth12 atom can also call expression/term a13th time.
    Thus each of the three mutually recursive functions has at most13 frames.
    This assumes .ci files were built from the supplied matching source tree.
    """
    source=Path(source_root)/'src/games/guesscalc_math.c';bounds={};evidence=[]
    if not source.exists():return bounds,evidence
    text=source.read_text();compact=re.sub(r'\s+','',text)
    guard='if(++p->depth>12){p->ok=false;--p->depth;returnr;}'
    if guard not in compact:return bounds,evidence
    names=[n for n in nodes if n in {'src/games/guesscalc_math.c:'+f for f in ('atom','term','expression')}]
    for name in names:bounds[name]=13
    if names:evidence.append(dict(source='src/games/guesscalc_math.c',sha256=hashlib.sha256(text.encode()).hexdigest(),accepted_atom_depth=12,max_simultaneous_frames_per_member=13,members=names,assumption='Manual bound: source matches this build; includes rejected depth13 atom and its expression/term callers.'))
    return bounds,evidence

def analyze(nodes,edges,bounds=None):
    bounds={} if bounds is None else bounds
    if any(not isinstance(v,int) or v<1 for v in bounds.values()):raise ValueError('Recursive frame limits must be positive integers')
    components=strongly_connected(nodes,edges);owner={n:i for i,c in enumerate(components) for n in c};out=[set() for _ in components];incoming=[0]*len(components)
    unresolved=[]
    for a,b in sorted(edges):
        if a not in nodes:continue
        if b not in nodes:unresolved.append(dict(caller=a,target=b,kind='indirect/callback' if 'indirect' in b.lower() else 'library/assembly/missing-unit'));continue
        if owner[a]!=owner[b]:out[owner[a]].add(owner[b])
    for links in out:
        for child in links:incoming[child]+=1
    groups=[];unbounded=[]
    for i,members in enumerate(components):
        recursive=len(members)>1 or any((n,n) in edges for n in members)
        dynamic=any('dynamic' in nodes[n]['kind'] and 'bounded' not in nodes[n]['kind'] for n in members)
        known=not dynamic and (not recursive or all(n in bounds for n in members))
        weight=sum(nodes[n]['bytes']*(bounds[n] if recursive else 1) for n in members) if known else None
        group=dict(functions=members,recursive=recursive,component_bytes=weight)
        if recursive:group['limits']={n:bounds.get(n) for n in members}
        if not known:group['unresolved_reason']='dynamic stack without a bound' if dynamic else 'recursive activation count has no checked bound';unbounded.append(group)
        groups.append(group)
    memo={}
    def path(index):
        if index in memo:return memo[index]
        if groups[index]['component_bytes'] is None:result=(0,[],[index])
        else:
            children=[path(child) for child in sorted(out[index])];best=max(children,key=lambda x:x[0],default=(0,[],[]))
            # Numeric value stops at any unknown component. It is a known
            # contribution/prefix, never a bound for the missing continuation.
            stopped=sorted({u for _,_,missing in children for u in missing})
            result=(groups[index]['component_bytes']+best[0],[index]+best[1],stopped)
        memo[index]=result;return result
    paths=[]
    for i,c in enumerate(components):
        if incoming[i]:continue
        value,chain,unknown=path(i)
        paths.append(dict(entry=c,known_project_path_bytes=value,components=[groups[k] for k in chain],reachable_unbounded_components=[groups[k]['functions'] for k in unknown]))
    paths.sort(key=lambda p:(-p['known_project_path_bytes'],p['entry']))
    return dict(status='PARTIAL STATIC ANALYSIS' if nodes else 'NO CALLGRAPH FILES',project_definitions=len(nodes),direct_project_edges=sum(a in nodes and b in nodes for a,b in edges),unresolved_edges=unresolved,recursive_components=[g for g in groups if g['recursive']],unbounded_components=unbounded,root_paths=paths,largest_known_project_path_bytes=paths[0]['known_project_path_bytes'] if paths else None,whole_program_stack_bound=None,device_observed_stack_peak=None,limitations=[
        'Known direct project paths only. Indirect module dispatch, function pointers, callbacks, libraries, assembly and interrupt/OS frames are not assigned zero-cost full continuations.',
        'An unresolved edge ends the reported known contribution; its omitted continuation may dominate the actual peak.',
        'Recursive SCC sums use independently justified per-member activation limits. They are conservative conditional component sums, not measured peaks or necessarily feasible simultaneous maxima.',
        'Compiler tail-call reuse is not inferred. Summing caller/callee frames can overestimate a direct path; absent library/internal tail-recursive paths remain unanalysed.',
        'Compiler inlining is reflected in the emitted frame. The build source and compiler options must match the analyzed artifacts.',
        'BSS, heap and the largest frame/path are never summed into an alleged device RAM peak.'])

def collect(build_dir,source_root):
    nodes={};edges=set();files=sorted(Path(build_dir).rglob('*.ci'));accepted=0;skipped=[]
    for path in files:
        objects=[path.with_suffix(s) for s in ('.o','.obj')]
        if not any(p.exists() and p.stat().st_mtime_ns>=path.stat().st_mtime_ns for p in objects):
            skipped.append(dict(file=path.name,reason='No matching object at least as new as this .ci; failed/stale compilation is excluded.'));continue
        parse_text(path.read_text(),nodes,edges);accepted+=1
    bounds,evidence=parser_policy(nodes,source_root);report=analyze(nodes,edges,bounds)
    report['compiler_callgraph_files']=accepted;report['skipped_callgraph_files']=skipped;report['recursive_bound_evidence']=evidence
    return report

def self_test():
    fixture_home='/'+'Users/'+'private/repo/'
    sample='''graph: {title: "private"\nnode: {title:"FIXTURE/src/core/a.c:local" label:"local\\nsrc/core/a.c:1:1\\n12 bytes (static)"}\nnode: {title:"leaf" label:"leaf\\nsrc/b.c:1:1\\n8 bytes (static)"}\nedge: {sourcename:"FIXTURE/src/core/a.c:local" targetname:"leaf"}\nedge: {sourcename:"leaf" targetname:"__indirect_call"}\n}'''.replace('FIXTURE/',fixture_home)
    nodes,edges=parse_text(sample);r=analyze(nodes,edges);assert r['largest_known_project_path_bytes']==20 and len(r['unresolved_edges'])==1 and '/Users/' not in json.dumps(r)
    rng=random.Random(519)
    for _ in range(100):
        n=8;nodes={str(i):dict(function=str(i),bytes=rng.randrange(1,40),kind='static') for i in range(n)};edges={(str(i),str(j)) for i in range(n) for j in range(i+1,n) if rng.randrange(3)==0}
        # Independent exhaustive enumeration of all simple paths on each DAG.
        sums=[]
        def enumerate_paths(node,total):
            total+=nodes[node]['bytes'];sums.append(total)
            for a,b in edges:
                if a==node:enumerate_paths(b,total)
        for node in nodes:enumerate_paths(node,0)
        assert analyze(nodes,edges)['largest_known_project_path_bytes']==max(sums)
    nodes={n:dict(function=n,bytes=v,kind='static') for n,v in (('root',5),('a',10),('b',20),('end',7))};edges={('root','a'),('a','b'),('b','a'),('b','end')}
    r=analyze(nodes,edges,{'a':2,'b':2});assert r['largest_known_project_path_bytes']==72 and not r['unbounded_components']
    r=analyze(nodes,edges);assert r['unbounded_components'] and r['whole_program_stack_bound'] is None
    nodes['a']['kind']='dynamic';assert analyze(nodes,edges,{'a':2,'b':2})['unbounded_components']
    print('stack_callgraph self-test: real VCG syntax, privacy,100 exhaustive DAGs, bounded/unbounded mutual recursion and dynamic frames PASS')

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--build-dir',type=Path);ap.add_argument('--source-root',type=Path,default=Path(__file__).resolve().parents[1]);ap.add_argument('--output',type=Path);ap.add_argument('--self-test',action='store_true');args=ap.parse_args()
    if args.self_test:self_test();return
    if args.build_dir is None:ap.error('--build-dir is required')
    result=json.dumps(collect(args.build_dir,args.source_root),indent=2)+'\n'
    if args.output:args.output.write_text(result)
    else:print(result,end='')
if __name__=='__main__':main()
