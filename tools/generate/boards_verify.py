#!/usr/bin/env python3
"""Independent public-clue counters. No import from the generator/runtime code.
Shikaku branches on first uncovered cell; Slitherlink branches on face colours,
not line edges. Outside face colour is 0. Exhaustive 2x2 edge enumeration checks
the face model before any pack is accepted.
"""
from __future__ import annotations
import argparse,hashlib,itertools,json,os,re,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
class Limit(Exception):pass

def key_dihedral(a,n,renumber=False):
    results=[]
    for flip in (False,True):
        for turns in range(4):
            out=[0]*(n*n)
            for r in range(n):
                for c in range(n):
                    y,x=r,n-1-c if flip else c
                    for _ in range(turns):y,x=x,n-1-y
                    out[y*n+x]=a[r*n+c]
            if renumber:
                names={};out=[names.setdefault(v,len(names)) for v in out]
            results.append(tuple(out))
    return min(results)

def shikaku_count(clues,n,budget=2000000):
    masks=[];at=[[] for _ in clues]
    for top in range(n):
        for bottom in range(top,n):
            for left in range(n):
                for right in range(left,n):
                    region=[r*n+c for r in range(top,bottom+1) for c in range(left,right+1)]
                    marked=[clues[p] for p in region if clues[p]]
                    if len(marked)!=1 or marked[0]!=len(region):continue
                    mask=sum(1<<p for p in region);masks.append(mask)
                    for p in region:at[p].append(mask)
    full=(1<<(n*n))-1;nodes=0
    def visit(used):
        nonlocal nodes
        nodes+=1
        if nodes>budget:raise Limit
        if used==full:return 1
        # Cell-driven exact cover independently enumerates every rectangle above.
        p=next(i for i in range(n*n) if not used>>i&1);total=0
        for rectangle in at[p]:
            if rectangle&used:continue
            total+=visit(used|rectangle)
            if total>=2:return 2
        return total
    return visit(0),nodes

def shikaku_witness(record):
    n=record['size'];board=[0]*(n*n);clues=record['clues']
    for tag,(r,c,h,w) in enumerate(record['rectangles'],1):
        assert h>0 and w>0 and 0<=r<r+h<=n and 0<=c<c+w<=n
        region=[y*n+x for y in range(r,r+h) for x in range(c,c+w)]
        assert sum(clues[p]!=0 for p in region)==1
        assert sum(clues[p] for p in region)==len(region)
        for p in region:assert board[p]==0;board[p]=tag
    assert all(board)
    return key_dihedral(board,n,True)

def singleton_unresolved(clues,n):
    """Independent rectangle enumeration; force unique clue/cell owners only."""
    choices=[]
    for top in range(n):
        for left in range(n):
            for bottom in range(top,n):
                for right in range(left,n):
                    cells=[r*n+c for r in range(top,bottom+1) for c in range(left,right+1)]
                    marked=[p for p in cells if clues[p]]
                    if len(marked)==1 and clues[marked[0]]==len(cells):choices.append((marked[0],frozenset(cells)))
    unused=set(range(n*n));unassigned={p for p,v in enumerate(clues) if v}
    while unassigned:
        valid=[option for option in choices if option[0] in unassigned and option[1]<=unused]
        forced=None
        for p in sorted(unassigned):
            options=[o for o in valid if o[0]==p]
            if len(options)==1:forced=options[0];break
        if forced is None:
            for p in sorted(unused):
                options=[o for o in valid if p in o[1]]
                if len(options)==1:forced=options[0];break
        if forced is None:break
        clue,cells=forced;unassigned.remove(clue);unused-=cells
    return len(unassigned)

def local_unknown_edges(clues,n):
    ends=edge_list(n);state=[-1]*len(ends);vertices=[[] for _ in range((n+1)**2)];cells=[]
    for i,(a,b) in enumerate(ends):vertices[a].append(i);vertices[b].append(i)
    for r in range(n):
        for c in range(n):cells.append([r*n+c,(r+1)*n+c,n*(n+1)+r*(n+1)+c,n*(n+1)+r*(n+1)+c+1])
    while True:
        before=state[:]
        for indices,target in [(es,k) for es,k in zip(cells,clues) if k>=0]+[(es,-2) for es in vertices]:
            on=sum(state[e]==1 for e in indices);unknown=[e for e in indices if state[e]<0];force=None
            if target>=0:
                assert on<=target<=on+len(unknown)
                if on==target:force=0
                elif on+len(unknown)==target:force=1
            else:
                assert on<=2 and not(on==1 and not unknown)
                if on==2 or (on==0 and len(unknown)==1):force=0
                elif on==1 and len(unknown)==1:force=1
            if force is not None:
                for e in unknown:state[e]=force
        if before==state:return state.count(-1)

def edge_list(n):
    return [(r*(n+1)+c,r*(n+1)+c+1) for r in range(n+1) for c in range(n)]+[(r*(n+1)+c,(r+1)*(n+1)+c) for r in range(n) for c in range(n+1)]
def reference_loop(edges,n):
    ends=edge_list(n);adj=[[] for _ in range((n+1)**2)]
    for bit,(u,v) in zip(edges,ends):
        if bit:adj[u].append(v);adj[v].append(u)
    used=[v for v,x in enumerate(adj) if x]
    if not used or any(len(adj[v])!=2 for v in used):return False
    reached=set();pending=[used[0]]
    while pending:
        v=pending.pop()
        if v in reached:continue
        reached.add(v);pending.extend(adj[v])
    return len(reached)==len(used)

def face_edges(face,n):
    # Separate face-to-edge construction, outside cells handled by a padded grid.
    padded=[[0]*(n+2) for _ in range(n+2)]
    for r in range(n):
        for c in range(n):padded[r+1][c+1]=face[r*n+c]
    horizontal=[padded[r][c+1]^padded[r+1][c+1] for r in range(n+1) for c in range(n)]
    vertical=[padded[r+1][c]^padded[r+1][c+1] for r in range(n) for c in range(n+1)]
    return horizontal+vertical

def edge_clues(edges,n):
    split=n*(n+1)
    return [edges[r*n+c]+edges[(r+1)*n+c]+edges[split+r*(n+1)+c]+edges[split+r*(n+1)+c+1] for r in range(n) for c in range(n)]

class FaceCounter:
    def __init__(self,clues,n):
        self.clues=clues;self.n=n;self.constraints=[];self.members=[[] for _ in clues]
        def add(cells,condition):
            variables=sorted(set(p for p in cells if p>=0));position={p:i for i,p in enumerate(variables)};patterns=[]
            for values in itertools.product((0,1),repeat=len(variables)):
                row=[0 if p<0 else values[position[p]] for p in cells]
                if condition(row):patterns.append(sum(v<<i for i,v in enumerate(values)))
            self.constraints.append((variables,patterns))
            for p in variables:self.members[p].append(len(self.constraints)-1)
        def cell(r,c):return r*n+c if 0<=r<n and 0<=c<n else -1
        for r in range(n):
            for c in range(n):
                clue=clues[r*n+c]
                if clue>=0:add([cell(r,c),cell(r-1,c),cell(r+1,c),cell(r,c-1),cell(r,c+1)],lambda v,k=clue:sum(v[0]!=x for x in v[1:])==k)
        for r in range(n+1):
            for c in range(n+1):
                # Cyclic faces around one vertex; degree cannot be four.
                add([cell(r-1,c-1),cell(r-1,c),cell(r,c),cell(r,c-1)],lambda v:sum(v[i]!=v[(i+1)%4] for i in range(4))<=2)
        self.nodes=0;self.reductions=0
    def count(self,budget=2000000):
        self.nodes=0;self.reductions=0
        def narrow(domains):
            todo=list(range(len(self.constraints)));queued=set(todo)
            while todo:
                index=todo.pop();queued.discard(index);variables,patterns=self.constraints[index]
                support=[0]*len(variables);found=False
                for pattern in patterns:
                    if all(domains[p]&(1<<((pattern>>i)&1)) for i,p in enumerate(variables)):
                        found=True
                        for i in range(len(variables)):support[i]|=1<<((pattern>>i)&1)
                if not found:return False
                for p,s in zip(variables,support):
                    changed=domains[p]&s
                    if changed!=domains[p]:
                        domains[p]=changed;self.reductions+=1
                        for neighbour in self.members[p]:
                            if neighbour not in queued:todo.append(neighbour);queued.add(neighbour)
            return True
        def visit(domains):
            self.nodes+=1
            if self.nodes>budget:raise Limit
            if not narrow(domains):return 0
            unknown=[p for p,d in enumerate(domains) if d==3]
            if not unknown:
                face=[int(d==2) for d in domains];edges=face_edges(face,self.n)
                return int(reference_loop(edges,self.n))
            variable=max(unknown,key=lambda p:len(self.members[p]));total=0
            for value in (1,2):
                child=domains.copy();child[variable]=value;total+=visit(child)
                if total>=2:return 2
            return total
        return visit([3]*(self.n*self.n))

def exhaustive_small():
    loops=[]
    for bits in range(1<<12):
        edges=[bits>>i&1 for i in range(12)]
        if reference_loop(edges,2):loops.append(edge_clues(edges,2))
    assert len(loops)==13
    for clues in itertools.product((-1,0,1,2,3),repeat=4):
        brute=min(2,sum(all(k<0 or k==answer[i] for i,k in enumerate(clues)) for answer in loops))
        assert FaceCounter(clues,2).count()==brute,clues
    return dict(edge_assignments=4096,valid_loops=13,clue_patterns=625)

def embedded_records(records):
    """Parse the actual C initializer independently of the generator emitter."""
    source=(ROOT/'src/games/boards_pack.c').read_text()
    for game,name in ((31,'nb_shikaku_pack'),(32,'nb_slitherlink_pack')):
        match=re.search(r'const NbPuzzle '+name+r'\[NB_PACK_COUNT\]=\{(.*?)\n\};',source,re.S)
        assert match,name
        encoded=re.findall(r'\{(\d+),\{([0-9,]+)\}\}',match.group(1))
        assert len(encoded)==120
        expected=sorted((r for r in records if r['game_id']==game),key=lambda r:r['puzzle_id'])
        assert [r['puzzle_id'] for r in expected]==list(range(120))
        for (size,values),record in zip(encoded,expected):
            actual=list(map(int,values.split(',')));assert len(actual)==64
            clues=[255 if v==-1 else v for v in record['clues']]
            assert int(size)==record['size'] and actual==clues+[0]*(64-len(clues))
    return dict(records=240,bytes=240*65,sha256=hashlib.sha256(source.encode()).hexdigest())

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--path',type=Path,default=ROOT/'assets/boards/puzzles.json');args=ap.parse_args()
    start=time.monotonic();small=exhaustive_small();records=json.loads(args.path.read_text())['puzzles'];assert len(records)==240
    embedded=embedded_records(records)
    groups=[];all_clues={};all_structures={}
    for game in (31,32):
        for difficulty,n in enumerate((5,6,8,8)):
            group=[r for r in records if r['game_id']==game and r['difficulty']==difficulty];assert len(group)==30
            cluekeys=set();structurekeys=set();nodes=[];cluecounts=[]
            for record in group:
                assert record['size']==n and len(record['clues'])==n*n and record['rules_version']==1 and record['puzzle_id']//30==difficulty
                clues=record['clues'];key=key_dihedral(clues,n);assert key not in cluekeys;cluekeys.add(key)
                if game==31:
                    assert all(0<=v<=n*n for v in clues);structure=shikaku_witness(record);count,visited=shikaku_count(clues,n)
                else:
                    assert all(-1<=v<=3 for v in clues);assert reference_loop(record['edges'],n)
                    actual=edge_clues(record['edges'],n);assert all(k<0 or k==actual[p] for p,k in enumerate(clues))
                    assert face_edges(record['inside'],n)==record['edges'];structure=key_dihedral(record['inside'],n)
                    solver=FaceCounter(clues,n);count=solver.count();visited=solver.nodes
                assert structure not in structurekeys;structurekeys.add(structure);assert count==1,(game,difficulty,record['puzzle_id'],count)
                ck=all_clues.setdefault((game,n),set());sk=all_structures.setdefault((game,n),set());assert key not in ck and structure not in sk;ck.add(key);sk.add(structure)
                if difficulty==3:
                    if game==31:assert singleton_unresolved(clues,n)==record['master_singleton_unresolved']>=4
                    else:assert local_unknown_edges(clues,n)==record['master_root_unknown_edges']>=16 and visited>1
                nodes.append(visited);cluecounts.append(sum(v>0 if game==31 else v>=0 for v in clues))
            groupmetrics=dict(game_id=game,difficulty=difficulty,size=n,count=len(group),unique=30,canonical_clues=len(cluekeys),canonical_structures=len(structurekeys),reference_nodes_min=min(nodes),reference_nodes_max=max(nodes),clues_min=min(cluecounts),clues_max=max(cluecounts))
            groups.append(groupmetrics);print(json.dumps(groupmetrics),flush=True)
    report=dict(rules_version=1,records=len(records),unique=240,small_exhaustive=small,embedded=embedded,groups=groups,seconds=round(time.monotonic()-start,3),generator_independence='cell exact cover / face-colour constraint propagation; no generator imports')
    output_root=Path(os.environ['NUMGAME_VERIFY_OUTPUT']) if 'NUMGAME_VERIFY_OUTPUT' in os.environ else ROOT/'assets/boards'
    output_root.mkdir(parents=True,exist_ok=True)
    (output_root/'verification.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report),flush=True)
if __name__=='__main__':main()
