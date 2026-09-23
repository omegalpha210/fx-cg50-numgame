"""Public-clue logical propagation and bounded search for MASTER rating.

This is an audit instrument, not a claim to model every human technique. It
never reads a solution witness. Existing row-permutation/MRV counters remain an
independent uniqueness reference. No runtime/native dependency is introduced.
"""
import collections,itertools,math,functools
from grids_solver import Budget

class Contradiction(Exception):pass

def visible(line):
    high=0;result=0
    for v in line:
        if v>high:high=v;result+=1
    return result

@functools.lru_cache(maxsize=128)
def sky_options(n,before,after):
    return tuple(v for v in itertools.permutations(range(1,n+1)) if (not before or visible(v)==before) and (not after or visible(v[::-1])==after))

@functools.lru_cache(maxsize=512)
def cage_options(n,length,pairs,op,target):
    result=[]
    for vs in itertools.product(range(1,n+1),repeat=length):
        if any(vs[i]==vs[j] for i,j in pairs):continue
        good=(op==0 and length==1 and vs[0]==target) or (op==1 and sum(vs)==target) or (op==2 and math.prod(vs)==target) or (op==3 and length==2 and abs(vs[0]-vs[1])==target) or (op==4 and length==2 and max(vs)==min(vs)*target)
        if good:result.append(vs)
    return tuple(result)

@functools.lru_cache(maxsize=128)
def kakuro_tuples(length,target):
    return tuple(v for combo in itertools.combinations(range(1,10),length) if sum(combo)==target for v in itertools.permutations(combo))

@functools.lru_cache(maxsize=256)
def tuple_supports(tuples):
    """Exact preindexed tuple support; same propagation, fewer Python loops."""
    if not tuples:return (),0
    support=[{} for _ in tuples[0]]
    for index,values in enumerate(tuples):
        bit=1<<index
        for pos,value in enumerate(values):support[pos][value]=support[pos].get(value,0)|bit
    return tuple(support),(1<<len(tuples))-1

class Logic:
    def __init__(self,p):
        self.p=p;self.n=n=p['n'];self.gid=p['id'];self.nn=n*n
        self.units=[[r*n+c for c in range(n)] for r in range(n)]+[[r*n+c for r in range(n)] for c in range(n)]
        if self.gid==11:self.units += [[(r+y)*n+c+x for y in range(3) for x in range(3)] for r in (0,3,6) for c in (0,3,6)]
        kakuro=[]
        if self.gid==13:
            self.units=[]
            for i in range(n*n):
                for target,step in ((p['a'][i],1),(p['b'][i],n)):
                    if not target:continue
                    cc=[];j=i+step
                    while j<n*n and p['cells'][j]==255 and (step!=1 or j//n==i//n):cc.append(j);j+=step
                    assert 2<=len(cc)<=9
                    self.units.append(cc);kakuro.append((cc,kakuro_tuples(len(cc),target),'run_sum_support'))
        self.peers=[set().union(*(set(u) for u in self.units if i in u))-{i} for i in range(n*n)]
        self.peer_order=[sorted(v) for v in self.peers]
        self.locked_overlaps=[]
        if self.gid==11:
            unit_sets=list(map(set,self.units))
            self.locked_overlaps=[[(other,other_set) for other,other_set in zip(self.units,unit_sets) if u is not other and len(us&other_set)>=2] for u,us in zip(self.units,unit_sets)]
        self.constraints=kakuro;self.inequalities=[]
        if self.gid==12:
            for cage in sorted(set(p['a'])):
                cc=[i for i,k in enumerate(p['a']) if k==cage];op,target=p['b'][2*cage:2*cage+2]
                pairs=tuple((i,j) for i in range(len(cc)) for j in range(i) if cc[j] in self.peers[cc[i]])
                self.constraints.append((cc,cage_options(n,len(cc),pairs,op,target),'cage_support'))
        if self.gid==14:
            for i in range(n*n):
                for sign,step in ((p['a'][i],1),(p['b'][i],n)):
                    if sign:self.inequalities.append((i,i+step) if sign>0 else (i+step,i))
        if self.gid==15:
            for k,u in enumerate(self.units):
                before=p['a'][2*n+k] if k<n else p['a'][k-n]
                after=p['a'][3*n+k] if k<n else p['a'][k]
                opts=sky_options(n,before,after)
                self.constraints.append((u,opts,'visibility_line_support'))
        self.indexed_constraints=[(cc,*tuple_supports(tuple(options)),technique) for cc,options,technique in self.constraints]

    def initial(self):
        if self.gid==13:return [set(range(1,10)) if v==255 else {0} for v in self.p['cells']]
        return [{v} if v else set(range(1,self.n+1)) for v in self.p['cells']]

    def propagate(self,domains,advanced=False,stats=None):
        stats=stats if stats is not None else collections.Counter()
        def restrict(i,values,technique):
            removed=domains[i]-values
            if not removed:return False
            domains[i]-=removed;stats[technique+'_eliminations']+=len(removed);stats[technique+'_steps']+=1
            if not domains[i]:raise Contradiction
            return True
        while True:
            stats['propagation_passes']+=1;changed=False
            for i,values in enumerate(domains):
                if len(values)==1:
                    v=next(iter(values))
                    for j in self.peer_order[i]:changed=restrict(j,domains[j]-{v},'peer_single') or changed
            for u in ([] if self.gid==13 else self.units):
                for v in range(1,self.n+1):
                    cc=[i for i in u if v in domains[i]]
                    if not cc:raise Contradiction
                    if len(cc)==1:changed=restrict(cc[0],{v},'hidden_single') or changed
            for i,j in self.inequalities:
                changed=restrict(i,{v for v in domains[i] if any(v<w for w in domains[j])},'inequality_arc') or changed
                changed=restrict(j,{v for v in domains[j] if any(w<v for w in domains[i])},'inequality_arc') or changed
            for cc,support,all_options,technique in self.indexed_constraints:
                possible=all_options
                for i,index in zip(cc,support):
                    allowed=0
                    for value in domains[i]:allowed|=index.get(value,0)
                    possible&=allowed
                    if not possible:break
                if not possible:raise Contradiction
                for i,index in zip(cc,support):changed=restrict(i,{value for value,bits in index.items() if bits&possible},technique) or changed
            if changed:continue
            if not advanced:break
            for u in self.units:
                unresolved=[i for i in u if 1<len(domains[i])<=3]
                for k in (2,3):
                    for cc in itertools.combinations(unresolved,k):
                        values=set().union(*(domains[i] for i in cc))
                        if len(values)==k:
                            for j in u:
                                if j not in cc:changed=restrict(j,domains[j]-values,'hall_subset') or changed
            if self.gid==11:
                for u,overlaps in zip(self.units,self.locked_overlaps):
                    for v in range(1,10):
                        places={i for i in u if v in domains[i]}
                        if len(places)<2:continue
                        for other,other_set in overlaps:
                            if not places<=other_set:continue
                            for j in other:
                                if j not in u:changed=restrict(j,domains[j]-{v},'locked_candidate') or changed
            if not changed:break
        return domains

    def search(self,limit=30000,advanced=True):
        nodes=0;maxdepth=0;branches=0;solutions=[]
        def go(domains,depth):
            nonlocal nodes,maxdepth,branches
            nodes+=1;maxdepth=max(maxdepth,depth)
            if nodes>limit:raise Budget
            try:self.propagate(domains,advanced)
            except Contradiction:return
            unresolved=[i for i,v in enumerate(domains) if len(v)>1]
            if not unresolved:solutions.append([next(iter(v)) for v in domains]);return
            i=min(unresolved,key=lambda k:(len(domains[k]),-len(self.peers[k]),k));branches+=1
            for v in sorted(domains[i]):
                child=[x.copy() for x in domains];child[i]={v};go(child,depth+1)
                if len(solutions)>=2:break
        go(self.initial(),0)
        return dict(solution_count=len(solutions),search_nodes=nodes,search_branches=branches,search_max_depth=maxdepth),solutions

    def rate(self,probe_limit=256):
        basic=collections.Counter();advanced=collections.Counter();domains=self.initial();self.propagate(domains,False,basic)
        left_basic=sum(len(x)>1 for x in domains);self.propagate(domains,True,advanced)
        left_advanced=sum(len(x)>1 for x in domains);probes=0;forcing=0;chain_passes=0;trace=[]
        # A failed candidate is removed only after an explicit contradiction.
        # Probe workspace is a fresh domain copy and never a witness shortcut.
        while probes<probe_limit and any(len(v)>1 for v in domains):
            found=False
            for i in sorted(range(self.nn),key=lambda i:(len(domains[i]),i)):
                if len(domains[i])<2:continue
                for v in sorted(domains[i]):
                    if probes>=probe_limit:break
                    probes+=1;child=[x.copy() for x in domains];child[i]={v};cs=collections.Counter()
                    try:self.propagate(child,True,cs)
                    except Contradiction:
                        trace.append(dict(cell=i,excluded=v,proof=dict(cs)))
                        domains[i].remove(v);forcing+=1;found=True;self.propagate(domains,True,advanced)
                    chain_passes+=cs['propagation_passes']
                    if found:break
                if found or probes>=probe_limit:break
            if not found:break
        return dict(basic_unfilled=left_basic,advanced_unfilled=left_advanced,after_chains_unfilled=sum(len(v)>1 for v in domains),basic=dict(basic),advanced=dict(advanced),forcing_probes=probes,forcing_eliminations=forcing,forcing_propagation_passes=chain_passes,forcing_trace=trace)

def analyze(p,search_limit=30000,probe_limit=256):
    solver=Logic(p);rating=solver.rate(probe_limit);metrics,solutions=solver.search(search_limit)
    rating.update(metrics);return rating,solutions
