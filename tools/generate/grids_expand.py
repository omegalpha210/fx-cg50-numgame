#!/usr/bin/env python3
"""Append-only grid supply expansion. Failed/incomplete batches never publish."""
import argparse,collections,json,pathlib,random,time
from grids_generate import MAKERS,latin_solution,adjacent,connected_white,remove_clues
from grids_master_generate import CANDIDATES,remove_givens
from grids_master_logic import Logic
from grids_master_reference import master_count
from grids_solver import count,Budget
from grids_verify import base_key,witness_valid
ROOT=pathlib.Path(__file__).resolve().parents[2]
OUT=ROOT/'assets/grids/expanded'
SPECS={};offset=980
for gid in range(11,21):
    for difficulty in (range(5) if gid<=15 else (3,)):
        target=20 if difficulty<3 else 50 if gid==13 and difficulty==3 else 30
        SPECS[gid,difficulty]=(offset,target);offset+=target
assert offset==1750
HARD_MAX={11:7,12:7,13:5,14:7,15:15}
MASTER_MAX={11:17,12:33,13:17,14:35,15:39}
HELL_FORCING={11:5,12:5,13:4,14:5,15:8}

def candidate_kakuro(rng,difficulty,fast_unique=False,bias=10,min_white=None):
    n=7 if difficulty==3 else 8
    for attempt in range(500):
        cells=[0]*(n*n)
        for _ in range(rng.randrange(7,11) if n==8 else rng.randrange(6,9)):
            r=rng.randrange(1,n-1);c=rng.randrange(1,n-1)
            for dy in (0,1):
                for dx in (0,1):cells[(r+dy)*n+c+dx]=255
        white=[i for i,v in enumerate(cells) if v==255]
        if len(white)<(min_white or (21 if n==7 else 27)) or not connected_white(n,[v!=255 for v in cells]):continue
        runs=[];valid=True
        for i in white:
            r,c=divmod(i,n)
            for step,start in ((1,c==0 or cells[i-1]==0),(n,r==0 or cells[i-n]==0)):
                if not start:continue
                cc=[];j=i
                while j<n*n and cells[j]==255 and (step!=1 or j//n==r):cc.append(j);j+=step
                if not 2<=len(cc)<=5:valid=False;break
                runs.append((i-step,step,cc))
        if not valid:continue
        s=[0]*(n*n);bel={i:[cc for _,_,cc in runs if i in cc] for i in white}
        def fill(left):
            if not left:return True
            options={i:[v for v in range(1,10) if all(v!=s[j] for cc in bel[i] for j in cc)] for i in left}
            i=min(left,key=lambda i:len(options[i]));vs=options[i];rng.shuffle(vs);vs.sort(key=lambda v:v+rng.random()*bias)
            for v in vs:
                s[i]=v
                if fill(left-{i}):return True
            s[i]=0;return False
        if not fill(set(white)):continue
        a=[0]*(n*n);b=a[:]
        for clue,step,cc in runs:(a if step==1 else b)[clue]=sum(s[i] for i in cc)
        p=dict(id=13,n=n,cells=cells,solution=s,a=a,b=b)
        try:
            unique=Logic(p).search(1001)[0]['solution_count']==1 if fast_unique else count(p,100000)[0]==1
            if unique:return p
        except Budget:pass
    return None

def candidate_kakuro_refine(rng,difficulty):
    """Search fresh run sums by changing actual digits, never relabel a pack.

    Every trial is checked against public clues. The search objective is only
    a construction aid; final acceptance still requires the complete logical
    proof and the independent counter. No prior content is used as a seed.
    """
    p=None
    for _ in range(12):
        p=candidate_kakuro(rng,difficulty,fast_unique=True,bias=4,min_white=21 if difficulty==3 else 22)
        if p is not None:break
    if p is None:return None
    n=p['n'];white=[i for i,v in enumerate(p['cells']) if v==255];runs=[]
    for i in range(n*n):
        for side,step in (('a',1),('b',n)):
            if not p[side][i]:continue
            cc=[];j=i+step
            while j<n*n and p['cells'][j]==255 and (step!=1 or j//n==i//n):cc.append(j);j+=step
            runs.append((side,i,cc))
    def measure(p,metrics):
        rating=Logic(p).rate(512)
        if rating['after_chains_unfilled']:return None
        passes=max((v['proof'].get('propagation_passes',0) for v in rating['forcing_trace']),default=0)
        return min(rating['forcing_eliminations'],4 if difficulty==4 else 1),min(passes,4 if difficulty==4 else 1),metrics['search_nodes']
    best=measure(p,Logic(p).search(1001)[0])
    if best is None:return None
    for _ in range(600):
        i=rng.choice(white);v=rng.randrange(1,10)
        if any(v==p['solution'][j] for _,_,cc in runs if i in cc for j in cc if i!=j):continue
        q={**p,'solution':p['solution'][:],'a':p['a'][:],'b':p['b'][:]};q['solution'][i]=v
        for side,clue,cc in runs:
            if i in cc:q[side][clue]+=v-p['solution'][i]
        try:metrics,_=Logic(q).search(101)
        except Budget:continue
        if metrics['solution_count']!=1:continue
        if difficulty==3 and metrics['search_nodes']>MASTER_MAX[13]:continue
        score=measure(q,metrics)
        if score is None or score<best:continue
        p=q;best=score
        if metrics['search_nodes']>(HARD_MAX[13] if difficulty==3 else MASTER_MAX[13]):
            try:rating=grade(p,difficulty)
            except Budget:rating=None
            if rating:return p
    return p

def candidate_sudoku_refine(rng):
    solution=latin_solution(9,rng,3);p=remove_clues(dict(id=11,n=9,cells=solution[:],solution=solution),rng,20)
    def measure(p,metrics):
        rating=Logic(p).rate(512)
        if rating['after_chains_unfilled']:return None
        passes=max((v['proof'].get('propagation_passes',0) for v in rating['forcing_trace']),default=0)
        return min(rating['forcing_eliminations'],5),min(passes,4),metrics['search_nodes']
    try:best=measure(p,Logic(p).search(101)[0])
    except Budget:return None
    if best is None:return None
    for _ in range(600):
        given=[i for i,v in enumerate(p['cells']) if v];empty=[i for i,v in enumerate(p['cells']) if not v]
        i=rng.choice(given);j=rng.choice(empty);cells=p['cells'][:];cells[i]=0;cells[j]=solution[j];q=dict(p,cells=cells)
        try:metrics,_=Logic(q).search(101)
        except Budget:continue
        if metrics['solution_count']!=1:continue
        score=measure(q,metrics)
        if score is None or score<best:continue
        p=q;best=score
        if metrics['search_nodes']>MASTER_MAX[11] and grade(p,4):return p
    return p

def candidate(gid,d,rng,version=7):
    if d<3:return MAKERS[gid](d,rng)
    if gid==13:return candidate_kakuro_refine(rng,d) if version>=7 or (d==4 and version>=6) else candidate_kakuro(rng,d,fast_unique=version>=4)
    if gid==11 and d==4:
        if version>=7:return candidate_sudoku_refine(rng)
        s=latin_solution(9,rng,3);p=dict(id=11,n=9,cells=s[:],solution=s)
        return remove_givens(p,rng,20) if version==2 else remove_clues(p,rng,20)
    if gid==15 and d==4 and version>=5:return CANDIDATES[gid](rng,7)
    if gid<=15:return CANDIDATES[gid](rng)
    from grids_puzzle_master import candidate as puzzle_candidate
    return puzzle_candidate(gid,rng)

def grade(p,d):
    gid=p['id']
    if gid>15:
        from grids_puzzle_master import grade as puzzle_grade
        return puzzle_grade(p)
    solver=Logic(p);metrics,solutions=solver.search(1001 if d==4 else 30000)
    if metrics['solution_count']!=1:return None
    nodes=metrics['search_nodes']
    if d==2 and nodes>HARD_MAX[gid]:return None
    if d==3 and not HARD_MAX[gid]<nodes<=MASTER_MAX[gid]:return None
    if d==4 and not MASTER_MAX[gid]<nodes<=1001:return None
    rating=solver.rate(512 if d==4 else 256);rating.update(metrics)
    if d>=3 and (not rating['basic_unfilled'] or rating['after_chains_unfilled'] or rating['forcing_eliminations']<(HELL_FORCING[gid] if d==4 else 1)):return None
    if d==4:
        # At least one contradiction proof crosses multiple deduction rounds
        # and uses multiple actual techniques, in addition to the search gap.
        if not any(t['proof'].get('propagation_passes',0)>=4 and sum(k.endswith('_steps') and v>0 for k,v in t['proof'].items())>=2 for t in rating['forcing_trace']):return None
    assert solutions==[p['solution']]
    if gid==15:
        n=p['n'];a=p['a']
        if not (any(a[i] and a[n+i] for i in range(n)) or any(a[2*n+i] and a[3*n+i] for i in range(n))):return None
    return rating

def generate(gid,d,max_candidates=20000):
    OUT.mkdir(parents=True,exist_ok=True);first,target=SPECS[gid,d];path=OUT/f'{gid}-{d}.json';pending=OUT/f'{gid}-{d}.pending.json';start=time.monotonic()
    if path.exists():
        existing=json.loads(path.read_text());assert len(existing)==target;print(f'EXISTS {gid}/{d}: {target}',flush=True);return
    old=json.loads((ROOT/f'assets/grids/{gid}.json').read_text());mp=ROOT/f'assets/grids/master/{gid}.json'
    if mp.exists():old+=json.loads(mp.read_text())
    for other in OUT.glob(f'{gid}-[0-4].json'):old+=json.loads(other.read_text())
    result=[];next_attempt=0;reasons=collections.Counter();elapsed=0
    if pending.exists():
        saved=json.loads(pending.read_text());result=saved['records'];next_attempt=saved['next_attempt'];reasons.update(saved['rejections']);elapsed=saved['seconds']
    seen={base_key(p) for p in old+result}
    # MASTER Magic rejects D4-equivalent witness arrays. These are layouts;
    # translated/complemented construction classes are reported separately.
    magic_seen={base_key(dict(p,cells=p['solution'])) for p in old+result} if gid==19 else set()
    def checkpoint(attempt):
        state=dict(records=result,next_attempt=attempt,rejections=dict(reasons),seconds=elapsed+time.monotonic()-start)
        temp=pending.with_suffix('.tmp');temp.write_text(json.dumps(state,separators=(',',':'))+'\n');temp.replace(pending)
    for attempt in range(next_attempt,max_candidates):
        if attempt%100==0:
            checkpoint(attempt)
            print(f'PROGRESS {gid}/{d} {len(result)}/{target} candidates={attempt} rejections={dict(reasons)}',flush=True)
        seed=gid*100000000+d*1000000+attempt;rng=random.Random(seed)
        try:p=candidate(gid,d,rng)
        except (Budget,RuntimeError):reasons['construction_budget']+=1;continue
        if p is None:reasons['construction_rejected']+=1;continue
        if gid==12 and d<3 and sum(bool(v) for v in p['cells'])>(4,7,10)[d]:
            reasons['fixed_cage_limit']+=1;continue
        key=base_key(p)
        if key in seen:reasons['canonical_duplicate']+=1;continue
        magic_key=base_key(dict(p,cells=p['solution'])) if gid==19 else None
        if magic_key in magic_seen:reasons['canonical_square_duplicate']+=1;continue
        try:rating=grade(p,d)
        except Budget:reasons['rating_budget']+=1;continue
        if rating is None:reasons['rating_rejected']+=1;continue
        if gid==19:number,nodes=None,0
        else:
            try:number,nodes=master_count(p,2000000)
            except Budget:reasons['reference_budget']+=1;continue
            assert number==1
        witness_valid(p)
        p.update(difficulty=d,seed=seed,puzzle_id=first+len(result),rules_version=1,solution_count=number,counter_nodes=nodes,counter_version=2 if gid==15 else 1,logical_rating=rating,rating_version=1,difficulty_policy_version=2,generation_version=8,probe_limit=512 if d==4 else 256,rating_label=('HELL provisional' if d==4 else 'MASTER provisional' if d==3 else 'structural'))
        if gid>15:p['probe_limit']=512
        result.append(p);seen.add(key)
        if gid==19:magic_seen.add(magic_key);p['rules_version']=2
        print(f'ACCEPT {gid}/{d} {len(result)}/{target} attempt={attempt+1} nodes={rating.get("search_nodes",0)} forces={rating.get("forcing_eliminations",0)} ref={nodes} seconds={elapsed+time.monotonic()-start:.1f}',flush=True)
        checkpoint(attempt+1)
        if len(result)==target:
            temp=OUT/f'{gid}-{d}.tmp.json';temp.write_text(json.dumps(result,separators=(',',':'))+'\n');temp.replace(path)
            (OUT/f'{gid}-{d}-generation.json').write_text(json.dumps(dict(game_id=gid,difficulty=d,candidates=attempt+1,accepted=target,rejections=dict(reasons),seconds=round(elapsed+time.monotonic()-start,3)),indent=2)+'\n');pending.unlink();return
    checkpoint(max_candidates)
    raise RuntimeError(f'{gid}/{d}: {len(result)}/{target} accepted within {max_candidates}; pending evidence preserved, no published pack')

if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('--game',type=int,required=True,choices=range(11,21));ap.add_argument('--difficulty',type=int,choices=range(5));ap.add_argument('--max-candidates',type=int,default=20000);a=ap.parse_args()
    for d in (range(5) if a.difficulty is None and a.game<=15 else (3,) if a.difficulty is None else (a.difficulty,)):generate(a.game,d,a.max_candidates)
