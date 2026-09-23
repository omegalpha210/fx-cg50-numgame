#!/usr/bin/env python3
"""Original Shikaku tilings and Slitherlink loops; bounded host-only generation."""
from __future__ import annotations
import argparse, json, random, time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]

def transforms(values,n):
    a=list(values)
    for _ in range(4):
        yield tuple(a)
        yield tuple(a[r*n+n-1-c] for r in range(n) for c in range(n))
        a=[a[(n-1-c)*n+r] for r in range(n) for c in range(n)]
def canonical(values,n): return min(transforms(values,n))
def tiling_key(rects,n):
    labels=[0]*(n*n)
    for k,(r,c,h,w) in enumerate(rects,1):
        for y in range(r,r+h):
            for x in range(c,c+w):labels[y*n+x]=k
    keys=[]
    for t in transforms(labels,n):
        ren={}; keys.append(tuple(ren.setdefault(v,len(ren)) for v in t))
    return min(keys)

class Budget(Exception): pass
def shikaku_unresolved(options,n):
    """Clue-single and cell-single rectangle propagation, no guessing."""
    left=list(range(len(options)));used=0
    while left:
        possible={k:[m for m in options[k] if not m&used] for k in left}
        forced=next(((k,ms[0]) for k,ms in possible.items() if len(ms)==1),None)
        if forced is None:
            for p in range(n*n):
                if used>>p&1:continue
                choices=[(k,m) for k,ms in possible.items() for m in ms if m>>p&1]
                if len(choices)==1:forced=choices[0];break
        if forced is None:break
        k,mask=forced;left.remove(k);used|=mask
    return len(left)

class ShikakuCounter:
    """Generator counter: clue-first rectangle exact cover, bitset/MRV."""
    def __init__(self,clues,n):
        self.n=n; self.full=(1<<(n*n))-1; self.options=[]
        positions=[i for i,v in enumerate(clues) if v]
        cluebits=sum(1<<p for p in positions)
        for p in positions:
            area=clues[p];r,c=divmod(p,n); opts=[]
            for h in range(1,n+1):
                if area%h:continue
                w=area//h
                if w>n:continue
                for top in range(max(0,r-h+1),min(r,n-h)+1):
                    for left in range(max(0,c-w+1),min(c,n-w)+1):
                        mask=sum(((1<<w)-1)<<(y*n+left) for y in range(top,top+h))
                        if mask&cluebits==1<<p:opts.append(mask)
            self.options.append(opts)
        self.initial=sum(map(len,self.options));self.nodes=0
    def count(self,limit=2,budget=100000):
        self.nodes=0
        def search(left,used):
            self.nodes+=1
            if self.nodes>budget:raise Budget
            if not left:return int(used==self.full)
            opts=None;choice=-1
            for k in left:
                possible=[m for m in self.options[k] if not m&used]
                if not possible:return 0
                if opts is None or len(possible)<len(opts):choice=k;opts=possible
            total=0;nextleft=[k for k in left if k!=choice]
            for m in opts:
                total+=search(nextleft,used|m)
                if total>=limit:return limit
            return total
        return search(list(range(len(self.options))),0)

def tiling(rng,n,d):
    rects=[(0,0,n,n)];target=rng.randint((6,8,12,10)[d],(9,13,20,15)[d])
    for _ in range(target-1):
        candidates=[]
        for i,(r,c,h,w) in enumerate(rects):
            for cut in range(1,h):
                if cut*w>=2 and (h-cut)*w>=2:candidates.append((i,(r,c,cut,w),(r+cut,c,h-cut,w)))
            for cut in range(1,w):
                if cut*h>=2 and (w-cut)*h>=2:candidates.append((i,(r,c,h,cut),(r,c+cut,h,w-cut)))
        if not candidates:break
        # Prefer splitting larger rectangles while preserving non-strip variety.
        weights=[(a[2]*a[3]+b[2]*b[3])**1.4 for _,a,b in candidates]
        i,a,b=rng.choices(candidates,weights)[0];rects[i]=a;rects.append(b)
    return rects

def generate_shikaku(rng,n,d,seen_clues,seen_tilings):
    for attempt in range(1,20001):
        rects=tiling(rng,n,d);key=tiling_key(rects,n)
        if key in seen_tilings:continue
        clues=[0]*(n*n)
        for r,c,h,w in rects:clues[(r+rng.randrange(h))*n+c+rng.randrange(w)]=h*w
        cluekey=canonical(clues,n)
        if cluekey in seen_clues:continue
        solver=ShikakuCounter(clues,n)
        try: count=solver.count()
        except Budget:continue
        if count!=1:continue
        if d and max(map(len,solver.options))<d+2:continue
        # MASTER must contain interacting choices beyond singleton selection.
        # These are solver-work criteria, not a claimed human difficulty grade.
        unresolved=shikaku_unresolved(solver.options,n) if d==3 else 0
        if d==3 and (solver.initial<45 or solver.nodes<len(rects)+12 or unresolved<4):continue
        seen_clues.add(cluekey);seen_tilings.add(key)
        record=dict(clues=clues,rectangles=rects,generator_attempts=attempt,
                    generator_nodes=solver.nodes,initial_candidates=solver.initial)
        if d==3:record['master_singleton_unresolved']=unresolved
        return record
    raise RuntimeError('Shikaku generation budget exhausted')

def topology(n):
    ends=[];cells=[[] for _ in range(n*n)];vertices=[[] for _ in range((n+1)**2)]
    for r in range(n+1):
        for c in range(n):
            e=len(ends);ends.append((r*(n+1)+c,r*(n+1)+c+1))
            if r:cells[(r-1)*n+c].append(e)
            if r<n:cells[r*n+c].append(e)
    for r in range(n):
        for c in range(n+1):
            e=len(ends);ends.append((r*(n+1)+c,(r+1)*(n+1)+c))
            if c:cells[r*n+c-1].append(e)
            if c<n:cells[r*n+c].append(e)
    for e,(u,v) in enumerate(ends):vertices[u].append(e);vertices[v].append(e)
    return ends,cells,vertices

def boundary(inside,n):
    values=[]
    for r in range(n+1):
        for c in range(n):values.append(int((r>0 and (r-1)*n+c in inside) != (r<n and r*n+c in inside)))
    for r in range(n):
        for c in range(n+1):values.append(int((c>0 and r*n+c-1 in inside) != (c<n and r*n+c in inside)))
    return values

def simple_loop(edges,n):
    ends,_,vertices=topology(n);used=[i for i,es in enumerate(vertices) if any(edges[e] for e in es)]
    if not used or any(sum(edges[e] for e in vertices[v])!=2 for v in used):return False
    reached={used[0]};todo=list(reached)
    while todo:
        u=todo.pop()
        for e in vertices[u]:
            if edges[e]:
                a,b=ends[e];v=b if a==u else a
                if v not in reached:reached.add(v);todo.append(v)
    return len(reached)==len(used)

class EdgeCounter:
    """Generator solver: edge variables, local cardinality, degree/cycle propagation."""
    def __init__(self,clues,n):
        self.n=n;self.clues=clues;self.ends,self.cells,self.vertices=topology(n)
        self.ne=len(self.ends);self.constraints=[(es,k) for es,k in zip(self.cells,clues) if k>=0]
        self.weights=[0]*self.ne
        for es,k in self.constraints:
            for e in es:self.weights[e]+=4+(k in (0,3))*2
        self.nodes=0;self.propagations=0;self.root_unknown=0
    def count(self,limit=2,budget=100000):
        self.nodes=0;self.propagations=0
        def propagate(values):
            changed=True
            while changed:
                changed=False
                for es,k in self.constraints:
                    on=sum(values[e]==1 for e in es);unknown=[e for e in es if values[e]<0]
                    if on>k or on+len(unknown)<k:return False
                    if unknown and (on==k or on+len(unknown)==k):
                        value=int(on<k)
                        for e in unknown:values[e]=value
                        self.propagations+=len(unknown);changed=True
                for es in self.vertices:
                    on=sum(values[e]==1 for e in es);unknown=[e for e in es if values[e]<0]
                    if on>2 or on==1 and not unknown:return False
                    force=None
                    if on==2 or on==0 and len(unknown)==1:force=0
                    if on==1 and len(unknown)==1:force=1
                    if force is not None and unknown:
                        for e in unknown:values[e]=force
                        self.propagations+=len(unknown);changed=True
                # A closed component cannot coexist with another line component.
                parent=list(range(len(self.vertices)));active=set();cycle=False
                def find(v):
                    while parent[v]!=v:parent[v]=parent[parent[v]];v=parent[v]
                    return v
                for e,value in enumerate(values):
                    if value!=1:continue
                    u,v=self.ends[e];active.update((u,v));a,b=find(u),find(v)
                    if a==b:cycle=True
                    else:parent[a]=b
                if cycle:
                    if len({find(v) for v in active})!=1:return False
                    for e in range(self.ne):
                        if values[e]<0:values[e]=0;changed=True
            return True
        def search(values):
            self.nodes+=1
            if self.nodes>budget:raise Budget
            if not propagate(values):return 0
            remaining=[e for e,v in enumerate(values) if v<0]
            if self.nodes==1:self.root_unknown=len(remaining)
            if not remaining:return int(simple_loop(values,self.n))
            e=max(remaining,key=lambda e:self.weights[e]+sum(values[k]==1 for v in self.ends[e] for k in self.vertices[v]))
            total=0
            for value in (0,1):
                child=values.copy();child[e]=value;total+=search(child)
                if total>=limit:return limit
            return total
        return search([-1]*self.ne)

def random_loop(rng,n):
    inside={rng.randrange(n*n)};target=rng.randint(n*n*2//5,n*n*3//4)
    for _ in range(target-1):
        near=set()
        for p in inside:
            r,c=divmod(p,n)
            for y,x in ((r-1,c),(r+1,c),(r,c-1),(r,c+1)):
                if 0<=y<n and 0<=x<n and y*n+x not in inside:near.add(y*n+x)
        choices=list(near);rng.shuffle(choices)
        for p in choices:
            trial=inside|{p}
            if simple_loop(boundary(trial,n),n):inside=trial;break
        else:break
    return inside,boundary(inside,n)

def generate_slither(rng,n,d,seen_clues,seen_regions):
    _,cells,_=topology(n)
    for attempt in range(1,10001):
        inside,edges=random_loop(rng,n);region=tuple(int(i in inside) for i in range(n*n));key=canonical(region,n)
        if key in seen_regions:continue
        clues=[sum(edges[e] for e in es) for es in cells]
        if max(clues)>3 or clues.count(3)<2:continue
        full=EdgeCounter(clues,n)
        try:count=full.count(budget=60000)
        except Budget:continue
        if count!=1:continue
        # Different board dimensions and clue-retention budgets define levels.
        minimum=(n*n*(72,62,52,39)[d]+99)//100
        positions=list(range(n*n));rng.shuffle(positions);removed=0;totalnodes=full.nodes
        for p in positions:
            if n*n-removed<=minimum:break
            old=clues[p];clues[p]=-1;solver=EdgeCounter(clues,n)
            try:count=solver.count(budget=50000)
            except Budget:count=2
            totalnodes+=solver.nodes
            if count==1:removed+=1
            else:clues[p]=old
        cluekey=canonical(clues,n)
        if cluekey in seen_clues:continue
        solver=EdgeCounter(clues,n)
        try:count=solver.count(budget=100000)
        except Budget:continue
        if count!=1:continue
        if d==3 and (solver.nodes<25 or solver.root_unknown<16):continue
        seen_clues.add(cluekey);seen_regions.add(key)
        record=dict(clues=clues,inside=list(region),edges=edges,generator_attempts=attempt,
                    generator_nodes=solver.nodes,generation_total_nodes=totalnodes,
                    clue_count=sum(v>=0 for v in clues))
        if d==3:record['master_root_unknown_edges']=solver.root_unknown
        return record
    raise RuntimeError('Slitherlink generation budget exhausted')

def emit(records):
    assert len(records)==240
    for game in (31,32):
        assert [r['puzzle_id'] for r in records if r['game_id']==game]==list(range(120))
    out=['/* Original generated puzzles; see tools/generate/boards_generate.py. */','#include "boards.h"']
    for game,name in ((31,'shikaku'),(32,'slitherlink')):
        out.append(f'const NbPuzzle nb_{name}_pack[NB_PACK_COUNT]={{')
        for rec in records:
            if rec['game_id']!=game:continue
            clues=[255 if v<0 else v for v in rec['clues']]+[0]*(64-len(rec['clues']))
            out.append(' {'+str(rec['size'])+',{'+','.join(map(str,clues))+'}},')
        out.append('};')
    (ROOT/'src/games/boards_pack.c').write_text('\n'.join(out)+'\n')
    fixtures=['/* Host-only generated witnesses; excluded from native source. */','#ifndef TEST_BOARDS_FIXTURES_H','#define TEST_BOARDS_FIXTURES_H','#include <stdint.h>','static const uint8_t boards_test_rects[120][33][4]={']
    for rec in records:
        if rec['game_id']!=31:continue
        rows=[[len(rec['rectangles']),0,0,0]]+rec['rectangles']
        fixtures.append(' {'+','.join('{'+','.join(map(str,r))+'}' for r in rows)+'},')
    fixtures.append('};');fixtures.append('static const uint8_t boards_test_edges[120][144]={')
    for rec in records:
        if rec['game_id']==32:fixtures.append(' {'+','.join(map(str,rec['edges']))+'},')
    fixtures.extend(['};','#endif'])
    (ROOT/'tests/test_boards_fixtures.h').write_text('\n'.join(fixtures)+'\n')

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--seed',type=int,default=202609223132);ap.add_argument('--game',type=int,choices=(31,32));ap.add_argument('--master',action='store_true',help='preserve all existing E/N/H records and append MASTER');args=ap.parse_args()
    rng=random.Random(args.seed);start=time.monotonic();path=ROOT/'assets/boards/puzzles.json';records=[]
    if args.game and not path.exists():ap.error('--game requires an existing complete pack; run without --game first')
    if args.game and path.exists():records=[r for r in json.loads(path.read_text())['puzzles'] if r['game_id']!=args.game]
    if args.master:records=[r for r in json.loads(path.read_text())['puzzles'] if r['difficulty']<3 or (args.game and r['game_id']!=args.game)]
    for game in (() if args.master else (31,32)):
        if args.game and args.game!=game:continue
        for d,n in enumerate((5,6,8)):
            seen=set();seen_structure=set()
            for index in range(30):
                rec=(generate_shikaku if game==31 else generate_slither)(rng,n,d,seen,seen_structure)
                rec.update(game_id=game,rules_version=1,difficulty=d,size=n,puzzle_id=d*30+index,generator_seed=args.seed)
                records.append(rec)
                print(f'{game} {d} {index+1}/30 n={n} nodes={rec["generator_nodes"]} elapsed={time.monotonic()-start:.2f}',flush=True)
                records.sort(key=lambda r:(r['game_id'],r['puzzle_id']))
                path.write_text(json.dumps({'generator':'boards_generate.py','seed':args.seed,'puzzles':records},indent=2)+'\n')
    # A separate deterministic stream preserves every original public ID/record.
    master_seed=args.seed+1000000;rng=random.Random(master_seed)
    for game in (31,32):
        if args.game and args.game!=game:continue
        records=[r for r in records if not(r['game_id']==game and r['difficulty']==3)]
        prior=[r for r in records if r['game_id']==game and r['size']==8]
        seen={canonical(r['clues'],8) for r in prior}
        structures={tiling_key(r['rectangles'],8) if game==31 else canonical(r['inside'],8) for r in prior}
        for index in range(30):
            rec=(generate_shikaku if game==31 else generate_slither)(rng,8,3,seen,structures)
            rec.update(game_id=game,rules_version=1,difficulty=3,size=8,puzzle_id=90+index,generator_seed=master_seed)
            records.append(rec);print(f'MASTER {game} {index+1}/30 nodes={rec["generator_nodes"]} elapsed={time.monotonic()-start:.2f}',flush=True)
    records.sort(key=lambda r:(r['game_id'],r['puzzle_id']))
    path.write_text(json.dumps({'generator':'boards_generate.py','seed':args.seed,'master_seed':master_seed,'puzzles':records},indent=2)+'\n')
    emit(records)
if __name__=='__main__':main()
