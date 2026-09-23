#!/usr/bin/env python3
"""Original seeded HASHI/NONOGRAM construction; no runtime search or corpus.

Generation ratings are provisional instrument bands, not human calibration.
Independent public-clue counters live in tests/test_grids_extra.py.
"""
import argparse,itertools,json,pathlib,random,time
from functools import lru_cache
ROOT=pathlib.Path(__file__).resolve().parents[3]
OUT=pathlib.Path(__file__).resolve().parent
COUNT=30

def geometry(n,pos):
    edges=[]
    for i,a in enumerate(pos):
        right=[(b,j) for j,b in enumerate(pos) if b//n==a//n and b>a]
        down=[(b,j) for j,b in enumerate(pos) if b%n==a%n and b>a]
        for candidates in (right,down):
            if candidates:edges.append((i,min(candidates)[1]))
    cross=[]
    for k,(a,b) in enumerate(edges):
        for l,(c,d) in enumerate(edges[:k]):
            aa,bb,cc,dd=(pos[v] for v in (a,b,c,d))
            if aa//n==bb//n and cc%n==dd%n and aa%n<cc%n<bb%n and cc//n<aa//n<dd//n:cross.append((k,l))
            if aa%n==bb%n and cc//n==dd//n and cc%n<aa%n<dd%n and aa//n<cc//n<bb//n:cross.append((k,l))
    return edges,cross

def connected(count,edges,active,omit=-1):
    seen={0}
    for _ in range(count):
        old=len(seen)
        for k,(a,b) in enumerate(edges):
            if k!=omit and active[k] and (a in seen or b in seen):seen.update((a,b))
        if len(seen)==old:break
    return len(seen)==count

def hashi_solve(p):
    edges,cross=geometry(p['n'],p['pos']);inc=[[k for k,e in enumerate(edges) if i in e] for i in range(len(p['pos']))]
    stats={'search_nodes':0,'connectivity_deductions':0}
    def propagate(dom,advanced):
        dom=[set(x) for x in dom]
        while True:
            before=[set(x) for x in dom]
            for i,indices in enumerate(inc):
                lo=sum(min(dom[k]) for k in indices);hi=sum(max(dom[k]) for k in indices);clue=p['clues'][i]
                if not lo<=clue<=hi:return None
                for k in indices:
                    low,high=min(dom[k]),max(dom[k]);dom[k]={v for v in dom[k] if lo-low+v<=clue<=hi-high+v}
                    if not dom[k]:return None
            for a,b in cross:
                if 0 not in dom[a]:dom[b]&={0}
                if 0 not in dom[b]:dom[a]&={0}
                if not dom[a] or not dom[b]:return None
            active=[max(x)>0 for x in dom]
            if not connected(len(inc),edges,active):return None
            if advanced:
                for k,values in enumerate(dom):
                    if len(values)>1 and 0 in values and not connected(len(inc),edges,active,k):
                        values.remove(0);stats['connectivity_deductions']+=1
            if dom==before:return dom
    initial=[{0,1,2} for _ in edges];basic=propagate(initial,False);advanced=propagate(initial,True)
    if basic is None or advanced is None:return None
    stats['basic_unresolved']=sum(len(x)>1 for x in basic)
    stats['advanced_unresolved']=sum(len(x)>1 for x in advanced)
    found=[]
    def visit(dom):
        stats['search_nodes']+=1
        if stats['search_nodes']>20000:raise RuntimeError('budget')
        dom=propagate(dom,True)
        if dom is None:return
        todo=[(len(x),i) for i,x in enumerate(dom) if len(x)>1]
        if not todo:
            values=[next(iter(x)) for x in dom]
            if connected(len(inc),edges,values):found.append(values)
            return
        _,i=min(todo)
        for v in sorted(dom[i]):
            copy=[set(x) for x in dom];copy[i]={v};visit(copy)
            if len(found)>=2:return
    visit(initial)
    return stats,found

def hashi_candidate(rng,d):
    n=(5,7,7,9,9)[d];m=(6,9,12,16,21)[d]
    pos=sorted(rng.sample([r*n+c for r in range(0,n,2) for c in range(0,n,2)],m))
    edges,cross=geometry(n,pos)
    if not connected(m,edges,[1]*len(edges)):return None
    crossmap={k:set() for k in range(len(edges))}
    for a,b in cross:crossmap[a].add(b);crossmap[b].add(a)
    parent=list(range(m));values=[0]*len(edges)
    def find(i):
        while parent[i]!=i:i=parent[i]
        return i
    order=list(range(len(edges)));rng.shuffle(order)
    for k in order:
        a,b=edges[k]
        if find(a)!=find(b) and not any(values[j] for j in crossmap[k]):
            parent[find(a)]=find(b);values[k]=rng.randint(1,2)
    if not connected(m,edges,values):return None
    for k in order:
        if not values[k] and rng.random()<.18 and not any(values[j] for j in crossmap[k]):values[k]=rng.randint(1,2)
    clues=[sum(values[k] for k,e in enumerate(edges) if i in e) for i in range(m)]
    return dict(id=35,difficulty=d,n=n,pos=pos,clues=clues,solution=values)

def runs(bits):
    out=[];length=0
    for bit in bits:
        if bit:length+=1
        elif length:out.append(length);length=0
    if length:out.append(length)
    return tuple(out)

@lru_cache(None)
def patterns(n,clue):return tuple(v for v in range(1<<n) if runs([(v>>i)&1 for i in range(n)])==clue)

def nono_solve(p):
    n=p['n'];initial=[set(patterns(n,tuple(c))) for c in p['clues']];stats={'search_nodes':0,'line_rounds':0}
    def propagate(dom):
        dom=[set(x) for x in dom]
        while True:
            changed=False;stats['line_rounds']+=1
            for r in range(n):
                for c in range(n):
                    a={v>>c&1 for v in dom[r]};b={v>>r&1 for v in dom[n+c]};common=a&b
                    if not common:return None
                    nr={v for v in dom[r] if v>>c&1 in common};nc={v for v in dom[n+c] if v>>r&1 in common}
                    if nr!=dom[r] or nc!=dom[n+c]:changed=True
                    dom[r]=nr;dom[n+c]=nc
            if not changed:return dom
    basic=propagate(initial)
    if basic is None:return None
    stats['basic_unresolved']=sum(len({v>>c&1 for v in basic[r]})>1 for r in range(n) for c in range(n))
    stats['basic_rounds']=stats['line_rounds'];found=[]
    def visit(dom):
        stats['search_nodes']+=1
        if stats['search_nodes']>10000:raise RuntimeError('budget')
        dom=propagate(dom)
        if dom is None:return
        choices=[(len(v),i) for i,v in enumerate(dom) if len(v)>1]
        if not choices:found.append([next(iter(v)) for v in dom[:n]]);return
        _,i=min(choices)
        for v in sorted(dom[i]):
            nxt=[set(x) for x in dom];nxt[i]={v};visit(nxt)
            if len(found)>=2:return
    visit(initial);return stats,found

def canonical(p):
    n=p['n'];cells=([0]*(n*n))
    if p['id']==35:
        for i,v in zip(p['pos'],p['clues']):cells[i]=v
    else:cells=[(p['solution'][r]>>c)&1 for r in range(n) for c in range(n)]
    variants=[]
    for flip in (0,1):
        for rotate in range(4):
            output=[0]*(n*n)
            for r in range(n):
                for c in range(n):
                    y,x=r,(n-1-c if flip else c)
                    for _ in range(rotate):y,x=x,n-1-y
                    output[y*n+x]=cells[r*n+c]
            variants.append(tuple(output))
            if p['id']==36:variants.append(tuple(1-v for v in output))
    return min(variants)

def generate(gid,d):
    path=OUT/f'{gid}-{d}.json'
    if path.exists():return
    started=time.monotonic();accepted=[];seen=set()
    for old in OUT.glob(f'{gid}-[0-4].json'):
        seen.update(canonical(p) for p in json.loads(old.read_text()))
    for attempt in range(1000000):
        seed=gid*10000000+d*1000000+attempt;rng=random.Random(seed)
        if gid==35:p=hashi_candidate(rng,d)
        else:
            n=(5,6,7,9)[d];density=rng.uniform(.38,.65);rows=[sum((rng.random()<density)<<c for c in range(n)) for _ in range(n)]
            clues=[list(runs([(v>>c)&1 for c in range(n)])) for v in rows]+[list(runs([(v>>c)&1 for v in rows])) for c in range(n)]
            p=dict(id=36,difficulty=d,n=n,clues=clues,solution=rows)
        if p is None:continue
        key=canonical(p)
        if key in seen:continue
        try:result=hashi_solve(p) if gid==35 else nono_solve(p)
        except RuntimeError:continue
        if result is None:continue
        metrics,solutions=result
        if len(solutions)!=1:continue
        if gid==35:
            if d==0 and metrics['basic_unresolved']:continue
            if d==2 and metrics['basic_unresolved']<2:continue
            if d==3 and (metrics['basic_unresolved']<4 or metrics['connectivity_deductions']<1):continue
            if d==4 and (metrics['advanced_unresolved']<4 or metrics['search_nodes']<7):continue
        elif d==2 and metrics['basic_rounds']<4:continue
        elif d==3 and (metrics['basic_unresolved']<4 or metrics['search_nodes']<3):continue
        assert solutions==[p['solution']]
        p.update(seed=seed,puzzle_id=d*COUNT+len(accepted),solution_count=1,rating=metrics,rating_label='provisional structural and inference band')
        accepted.append(p);seen.add(key)
        print(gid,d,len(accepted),attempt+1,metrics,flush=True)
        if len(accepted)==COUNT:
            path.write_text(json.dumps(accepted,separators=(',',':'))+'\n')
            (OUT/f'{gid}-{d}-generation.json').write_text(json.dumps(dict(candidates=attempt+1,accepted=COUNT,seconds=round(time.monotonic()-started,3)),indent=2)+'\n');return
    raise RuntimeError((gid,d,'insufficient accepted records',len(accepted)))

def emit():
    arrays=[]
    for gid,levels in ((35,5),(36,4)):
        lines=[]
        for d in range(levels):
            puzzles=json.loads((OUT/f'{gid}-{d}.json').read_text());assert len(puzzles)==COUNT
            for p in puzzles:
                def arr(v):return '{'+','.join(map(str,v))+'}'
                if gid==35:
                    values=[0]*len(p['pos']);edges,_=geometry(p['n'],p['pos'])
                    for (a,b),v in zip(edges,p['solution']):values[a]+=v*(1 if p['pos'][a]//p['n']==p['pos'][b]//p['n'] else 3)
                    lines.append('{'+','.join([str(p['n']),str(len(p['pos'])),arr(p['pos']),arr(p['clues']),arr(values)])+'},')
                else:
                    packed=[sum(v<<(4*k) for k,v in enumerate(clue)) for clue in p['clues']]
                    lines.append('{'+','.join([str(p['n']),arr(packed),arr(p['solution'])])+'},')
        typename='GridsHashiPuzzle' if gid==35 else 'GridsNonoPuzzle';name='grids_hashi_pack' if gid==35 else 'grids_nono_pack'
        arrays.append(f'const {typename} {name}[{levels*COUNT}]={{\n'+ '\n'.join(lines)+'\n};\n')
    (ROOT/'src/games/grids_extra_pack.c').write_text('/* Original generated puzzles; assets/grids/extra/generate.py. */\n#include "grids_extra.h"\n'+''.join(arrays))

if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('--game',type=int,choices=(35,36));ap.add_argument('--difficulty',type=int);ap.add_argument('--emit',action='store_true');a=ap.parse_args()
    if a.game:
        for d in (range(5 if a.game==35 else 4) if a.difficulty is None else (a.difficulty,)):generate(a.game,d)
    if a.emit:emit()
