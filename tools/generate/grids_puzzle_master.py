"""Public-clue PUZZLE inference instruments and original MASTER construction.

The independent row/path/mask counters live in grids_solver.py. No witness is
read by the inference engines. Human ratings are provisional instrument bands.
"""
import collections,functools,itertools
from grids_generate import MAKERS,remove_clues
from grids_master_logic import Logic,Contradiction

class PuzzleLogic(Logic):
    def __init__(self,p):
        self.p=p;self.n=n=p['n'];self.nn=n*n;self.gid=p['id']
        self.lines=[[r*n+c for c in range(n)] for r in range(n)]+[[r*n+c for r in range(n)] for c in range(n)]
        self.neighbors=[{j for j in range(n*n) if abs(j//n-i//n)+abs(j%n-i%n)==1} for i in range(n*n)]
        self.peers=[set(range(n*n))-{i} for i in range(n*n)]
        self.constraints=[]
        if self.gid==17:
            patterns=[v for v in itertools.product((0,1),repeat=n) if sum(v)==n//2 and all(len(set(v[i:i+3]))>1 for i in range(n-2))]
            self.constraints=[(line,patterns) for line in self.lines]
        elif self.gid==20:
            for line,target in zip(self.lines,p['a']):
                patterns=[v for v in itertools.product((0,1),repeat=n) if sum(p['cells'][i]*b for i,b in zip(line,v))==target]
                self.constraints.append((line,patterns))
        elif self.gid==16:
            self.duplicates=[(i,j) for i in range(n*n) for j in range(i+1,n*n) if p['cells'][i]==p['cells'][j] and (i//n==j//n or i%n==j%n)]

    def initial(self):
        if self.gid==17:return [{0,1} if v==255 else {v} for v in self.p['cells']]
        if self.gid==18:
            givens=[v for v in self.p['cells'] if v]
            if len(givens)!=len(set(givens)) or any(v<1 or v>self.nn for v in givens):return [set() for _ in range(self.nn)]
            where={v:i for i,v in enumerate(self.p['cells']) if v}
            return [{where[v]} if v in where else set(range(self.nn))-set(where.values()) for v in range(1,self.nn+1)]
        return [{0,1} for _ in range(self.nn)]

    def propagate(self,domains,advanced=False,stats=None):
        stats=stats if stats is not None else collections.Counter()
        if any(not v for v in domains):raise Contradiction
        def restrict(i,values,technique):
            removed=domains[i]-values
            if not removed:return False
            domains[i]-=removed;stats[technique+'_steps']+=1;stats[technique+'_eliminations']+=len(removed)
            if not domains[i]:raise Contradiction
            return True
        def connected(blocked,required):
            possible=set(range(self.nn))-blocked
            if not possible:return False
            start=next(iter(required or possible));seen={start};stack=[start]
            while stack:
                for j in self.neighbors[stack.pop()]&possible-seen:seen.add(j);stack.append(j)
            return required<=seen
        while True:
            stats['propagation_passes']+=1;changed=False
            if self.gid in (17,20):
                options=[]
                for line,patterns in self.constraints:
                    opts=[v for v in patterns if all(b in domains[i] for i,b in zip(line,v))]
                    if not opts:raise Contradiction
                    options.append(opts)
                if self.gid==17 and advanced:
                    for start in (0,self.n):
                        fixed=[opts[0] for opts in options[start:start+self.n] if len(opts)==1]
                        if len(fixed)!=len(set(fixed)):raise Contradiction
                        for k in range(start,start+self.n):
                            if len(options[k])>1:
                                filtered=[v for v in options[k] if v not in fixed]
                                if not filtered:raise Contradiction
                                if len(filtered)!=len(options[k]):stats['line_distinct_steps']+=1
                                options[k]=filtered
                for (line,_),opts in zip(self.constraints,options):
                    for pos,i in enumerate(line):changed=restrict(i,{v[pos] for v in opts},'balanced_line' if self.gid==17 else 'subset_sum_crossing') or changed
            elif self.gid==16:
                for i,j in self.duplicates:
                    if domains[i]=={0}:changed=restrict(j,{1},'duplicate_white') or changed
                    if domains[j]=={0}:changed=restrict(i,{1},'duplicate_white') or changed
                for i in range(self.nn):
                    if domains[i]=={1}:
                        for j in sorted(self.neighbors[i]):changed=restrict(j,{0},'black_adjacency') or changed
                blocked={i for i,v in enumerate(domains) if v=={1}};white={i for i,v in enumerate(domains) if v=={0}}
                if not connected(blocked,white):stats['white_connectivity_steps']+=1;raise Contradiction
                if advanced:
                    for i,v in enumerate(domains):
                        if len(v)>1 and not connected(blocked|{i},white):changed=restrict(i,{0},'white_articulation') or changed
            elif self.gid==18:
                used={next(iter(v)) for v in domains if len(v)==1}
                if len(used)!=sum(len(v)==1 for v in domains):raise Contradiction
                for i,v in enumerate(domains):
                    if len(v)>1:changed=restrict(i,v-used,'position_single') or changed
                for i in range(self.nn-1):
                    changed=restrict(i,{v for v in domains[i] if self.neighbors[v]&domains[i+1]},'consecutive_arc') or changed
                    changed=restrict(i+1,{v for v in domains[i+1] if self.neighbors[v]&domains[i]},'consecutive_arc') or changed
                if advanced:
                    anchors=[(k,next(iter(v))) for k,v in enumerate(domains) if len(v)==1]
                    for k,anchor in anchors:
                        for j in range(self.nn):
                            if j==k or len(domains[j])==1:continue
                            distance=abs(k-j)
                            changed=restrict(j,{v for v in domains[j] if (abs(v//self.n-anchor//self.n)+abs(v%self.n-anchor%self.n))<=distance and (distance-abs(v//self.n-anchor//self.n)-abs(v%self.n-anchor%self.n))%2==0},'path_distance_parity') or changed
                    for pos in range(self.nn):
                        owners=[k for k,v in enumerate(domains) if pos in v]
                        if not owners:raise Contradiction
                        if len(owners)==1:changed=restrict(owners[0],{pos},'position_hidden_single') or changed
            if not changed:return domains

    def board_solution(self,solution):
        if self.gid!=18:return solution
        board=[0]*self.nn
        for value,pos in enumerate(solution,1):board[pos]=value
        return board

@functools.lru_cache(maxsize=1)
def panmagic_squares():
    """Exhaustive balanced-bit construction: 384 actual panmagic squares.

    Each of four bit planes has two set bits on every row, column and wrapped
    diagonal. Orthogonal planes yield each number 1..16 exactly once. These are
    mathematical constructions, not downloaded records or uniqueness claims.
    """
    lines=[[r*4+c for c in range(4)] for r in range(4)]+[[r*4+c for r in range(4)] for c in range(4)]
    lines += [[r*4+(c+sign*r)%4 for r in range(4)] for c in range(4) for sign in (-1,1)]
    masks=[set(bits) for bits in itertools.combinations(range(16),8) if all(len(set(bits)&set(line))==2 for line in lines)]
    squares=set()
    for planes in itertools.permutations(masks,4):
        square=tuple(1+sum((i in plane)<<k for k,plane in enumerate(planes)) for i in range(16))
        if len(set(square))==16:squares.add(square)
    assert len(squares)==384
    return tuple(sorted(squares))

def panmagic_valid(square):
    return sorted(square)==list(range(1,17)) and all(sum(square[r*4+(c+sign*r)%4] for r in range(4))==34 for c in range(4) for sign in (-1,1)) and all(sum(square[r*4+c] for c in range(4))==34 for r in range(4)) and all(sum(square[r*4+c] for r in range(4))==34 for c in range(4))

def candidate(gid,rng):
    if gid==19:
        square=list(rng.choice(panmagic_squares()));givens=set(rng.sample(range(16),4))
        return dict(id=19,n=4,cells=[v if i in givens else 0 for i,v in enumerate(square)],solution=square,variant='panmagic')
    if gid==20:
        n=7;square=[rng.randrange(2) for _ in range(n*n)];cells=[rng.randint(1,5) for _ in square]
        targets=[sum(cells[r*n+c]*square[r*n+c] for c in range(n)) for r in range(n)]+[sum(cells[r*n+c]*square[r*n+c] for r in range(n)) for c in range(n)]
        return dict(id=20,n=n,cells=cells,solution=square,a=targets)
    p=MAKERS[gid](2,rng)
    if gid==17:remove_clues(p,rng,12,255)
    if gid==18:remove_clues(p,rng,7)
    return p

def grade(p):
    if p['id']==19:
        assert panmagic_valid(p['solution'])
        matches=sum(all(not g or g==v for g,v in zip(p['cells'],square)) for square in panmagic_squares())
        return dict(public_rule='all wrapped diagonals also sum 34',construction_squares=384,construction_matches=matches,multiple_solutions_allowed=True,provisional_human_rating=True)
    solver=PuzzleLogic(p);metrics,solutions=solver.search(30000)
    if metrics['solution_count']!=1:return None
    # Measured maximum on all thirty retained HARD records with this exact
    # instrument: Hitori 13, Binary 9, Numbrix 20, Sum Grid 1 nodes.
    if metrics['search_nodes']<={16:13,17:9,18:20,20:1}[p['id']]:return None
    assert solver.board_solution(solutions[0])==p['solution']
    rating=solver.rate(512);rating.update(metrics)
    if not rating['basic_unfilled'] or rating['after_chains_unfilled']:return None
    if not rating['forcing_eliminations'] and not any(rating['advanced'].get(k,0) for k in ('white_articulation_steps','line_distinct_steps','path_distance_parity_steps')):return None
    evidence=[rating['advanced']]+[t['proof'] for t in rating['forcing_trace']]
    essential={16:('white_articulation_steps','white_connectivity_steps'),17:('line_distinct_steps',),18:('path_distance_parity_steps',),20:('subset_sum_crossing_steps',)}[p['id']]
    if not any(any(proof.get(k,0)>0 for k in essential) for proof in evidence):return None
    rating['provisional_human_rating']=True
    return rating
