#!/usr/bin/env python3
"""Independent public-clue counters, exhaustive small spaces, seeds and pack.

HASHI branches on complete incident-edge tuples (not generator domain inference).
NONOGRAM enumerates whole rows against column prefixes (not line propagation).
"""
import collections,hashlib,importlib.util,itertools,json,pathlib,random,tempfile,time,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
OUT=ROOT/'assets/grids/extra'

def hashi_edges(n,pos):
    result=[]
    for axis in (0,1):
        for line in range(n):
            indices=[i for i,v in enumerate(pos) if (v//n if axis==0 else v%n)==line]
            result.extend(zip(indices,indices[1:]))
    return sorted(result,key=lambda e:(e[0],pos[e[0]]//n!=pos[e[1]]//n))

def hashi_reference(p):
    n=p['n'];pos=p['pos'];clues=p['clues'];count=len(pos)
    if not 2<=count<=25 or len(clues)!=count or pos!=sorted(set(pos)) or any(not 0<=v<n*n for v in pos) or any(not 1<=v<=8 for v in clues):return [],0
    edges=hashi_edges(n,pos);inc=[[k for k,e in enumerate(edges) if i in e] for i in range(count)]
    interior=[]
    for a,b in edges:
        start,end=pos[a],pos[b];step=1 if start//n==end//n else n
        interior.append(set(range(start+step,end,step)))
    crossings=[(i,j) for i in range(len(edges)) for j in range(i) if interior[i]&interior[j]]
    state=[-1]*len(edges);found=[];nodes=0
    def feasible():
        for i,indices in enumerate(inc):
            known=sum(state[k] for k in indices if state[k]>=0);unknown=sum(state[k]<0 for k in indices)
            if not known<=clues[i]<=known+2*unknown:return False
        if any(state[a]>0 and state[b]>0 for a,b in crossings):return False
        seen={0}
        for _ in range(count):
            for k,(a,b) in enumerate(edges):
                if state[k]!=0 and (a in seen or b in seen):seen.update((a,b))
        return len(seen)==count
    def visit():
        nonlocal nodes
        nodes+=1
        if nodes>2000000:raise AssertionError('HASHI reference budget exceeded')
        if not feasible():return
        best=None
        for i,indices in enumerate(inc):
            unknown=[k for k in indices if state[k]<0]
            if not unknown:continue
            need=clues[i]-sum(state[k] for k in indices if state[k]>=0)
            options=[v for v in itertools.product((0,1,2),repeat=len(unknown)) if sum(v)==need]
            if best is None or len(options)<len(best[1]):best=(unknown,options)
        if best is None:found.append(state[:]);return
        unknown,options=best
        for values in options:
            for k,v in zip(unknown,values):state[k]=v
            visit()
            for k in unknown:state[k]=-1
            if len(found)>=2:return
    visit();return found,nodes

def line_runs(mask,n):
    result=[];i=0
    while i<n:
        if not mask&(1<<i):i+=1;continue
        first=i
        while i<n and mask&(1<<i):i+=1
        result.append(i-first)
    return result

def nono_reference(p):
    n=p['n'];clues=p['clues']
    patterns=[[v for v in range(1<<n) if line_runs(v,n)==c] for c in clues]
    prefixes=[{(length,v&((1<<length)-1)) for v in patterns[n+c] for length in range(n+1)} for c in range(n)]
    found=[];nodes=0
    def visit(rows,columns):
        nonlocal nodes
        nodes+=1
        if nodes>2000000:raise AssertionError('NONOGRAM reference budget exceeded')
        depth=len(rows)
        if depth==n:found.append(rows);return
        for row in patterns[depth]:
            new=[columns[c]|((row>>c&1)<<depth) for c in range(n)]
            if all((depth+1,new[c]) in prefixes[c] for c in range(n)):visit(rows+[row],new)
            if len(found)>=2:return
    visit([], [0]*n);return found,nodes

def load_generator():
    spec=importlib.util.spec_from_file_location('grid_extra_generator',OUT/'generate.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module

class ExtraTests(unittest.TestCase):
    def test_native_pack_byte_reproduction(self):
        gen=load_generator()
        with tempfile.TemporaryDirectory() as directory:
            gen.ROOT=pathlib.Path(directory);(gen.ROOT/'src/games').mkdir(parents=True)
            gen.emit()
            self.assertEqual((gen.ROOT/'src/games/grids_extra_pack.c').read_bytes(),(ROOT/'src/games/grids_extra_pack.c').read_bytes())

    def test_hashi_complete_four_island_space(self):
        pos=[0,2,6,8];edges=hashi_edges(3,pos);cases=collections.defaultdict(list)
        for values in itertools.product((0,1,2),repeat=len(edges)):
            clues=[sum(values[k] for k,e in enumerate(edges) if i in e) for i in range(4)]
            if not all(clues):continue
            seen={0}
            for _ in range(4):
                for (a,b),v in zip(edges,values):
                    if v and (a in seen or b in seen):seen.update((a,b))
            if len(seen)==4:cases[tuple(clues)].append(values)
            else:cases[tuple(clues)]
        gen=load_generator()
        for clues,truth in cases.items():
            p=dict(id=35,n=3,pos=pos,clues=list(clues),solution=[255]*4)
            ref,_=hashi_reference(p);self.assertEqual(len(ref),min(2,len(truth)))
            result=gen.hashi_solve(p);actual=[] if result is None else result[1]
            self.assertEqual(len(actual),min(2,len(truth)))
            self.assertTrue(all(tuple(v) in truth for v in actual))

    def test_nonogram_complete_three_by_three_space(self):
        cases=collections.defaultdict(list)
        for rows in itertools.product(range(8),repeat=3):
            clues=tuple(tuple(line_runs(v,3)) for v in rows)+tuple(tuple(line_runs(sum((v>>c&1)<<r for r,v in enumerate(rows)),3)) for c in range(3))
            cases[clues].append(list(rows))
        gen=load_generator()
        for clues,truth in cases.items():
            p=dict(id=36,n=3,clues=[list(c) for c in clues],solution=[65535]*3)
            ref,_=nono_reference(p);self.assertEqual(len(ref),min(2,len(truth)))
            result=gen.nono_solve(p);self.assertIsNotNone(result);self.assertEqual(len(result[1]),min(2,len(truth)))
            self.assertTrue(all(v in truth for v in result[1]))

    def test_all_banks_independent_counts_seeds_ratings(self):
        started=time.monotonic();gen=load_generator();summary=[]
        for gid,levels in ((35,5),(36,4)):
            seen=set()
            for difficulty in range(levels):
                puzzles=json.loads((OUT/f'{gid}-{difficulty}.json').read_text());self.assertEqual(len(puzzles),30);reference_nodes=[]
                for ordinal,p in enumerate(puzzles):
                    self.assertEqual((p['id'],p['difficulty'],p['puzzle_id']),(gid,difficulty,difficulty*30+ordinal))
                    key=gen.canonical(p);self.assertNotIn(key,seen);seen.add(key)
                    public={k:v for k,v in p.items() if k not in ('solution','rating')}
                    ref,nodes=(hashi_reference(public) if gid==35 else nono_reference(public));self.assertEqual(ref,[p['solution']]);reference_nodes.append(nodes)
                    rating,found=(gen.hashi_solve(public) if gid==35 else gen.nono_solve(public));self.assertEqual(rating,p['rating']);self.assertEqual(found,ref)
                    rng=random.Random(p['seed'])
                    if gid==35:
                        reproduced=gen.hashi_candidate(rng,difficulty)
                        for k in ('n','pos','clues','solution'):self.assertEqual(reproduced[k],p[k])
                        if difficulty==0:self.assertEqual(rating['basic_unresolved'],0)
                        if difficulty==2:self.assertGreaterEqual(rating['basic_unresolved'],2)
                        if difficulty==3:self.assertGreaterEqual(rating['basic_unresolved'],4);self.assertGreater(rating['connectivity_deductions'],0)
                        if difficulty==4:self.assertGreaterEqual(rating['advanced_unresolved'],4);self.assertGreaterEqual(rating['search_nodes'],7)
                    else:
                        density=rng.uniform(.38,.65);rows=[sum((rng.random()<density)<<c for c in range(p['n'])) for _ in range(p['n'])]
                        self.assertEqual(rows,p['solution'])
                        if difficulty==2:self.assertGreaterEqual(rating['basic_rounds'],4)
                        if difficulty==3:self.assertGreaterEqual(rating['basic_unresolved'],4);self.assertGreaterEqual(rating['search_nodes'],3)
                summary.append(dict(game_id=gid,difficulty=difficulty,records=30,independent_reference_nodes_max=max(reference_nodes),source_sha256=hashlib.sha256((OUT/f'{gid}-{difficulty}.json').read_bytes()).hexdigest()))
        print(json.dumps(dict(complete=True,records=270,unique=270,public_only=True,seed_replay=270,seconds=round(time.monotonic()-started,3),groups=summary)),flush=True)

if __name__=='__main__':unittest.main()
