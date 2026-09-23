#!/usr/bin/env python3
"""Generate original, provisional MASTER packs without changing legacy content."""
import argparse,collections,json,pathlib,random,time
from grids_generate import latin_solution,adjacent
from grids_master_logic import Logic,analyze
from grids_solver import Budget
from grids_master_reference import master_count
from grids_verify import base_key,witness_valid
ROOT=pathlib.Path(__file__).resolve().parents[2]
OUT=ROOT/'assets/grids/master'
IDS=(11,12,14,15)

def unique(p):
    try:return Logic(p).search(3000)[0]['solution_count']==1
    except Budget:return False

def remove_givens(p,rng,minimum=0):
    order=list(range(p['n']**2));rng.shuffle(order)
    for i in order:
        if sum(bool(v) for v in p['cells'])<=minimum:break
        old=p['cells'][i];p['cells'][i]=0
        if not unique(p):p['cells'][i]=old
    return p

def candidate_sudoku(rng):
    s=latin_solution(9,rng,3)
    return remove_givens(dict(id=11,n=9,cells=s[:],solution=s),rng,rng.choice((22,23,24)))

def candidate_calc(rng):
    n=6;s=latin_solution(n,rng);free=set(range(n*n));cages=[]
    while free:
        start=rng.choice(sorted(free));cc=[start];free.remove(start)
        for _ in range(rng.choice((1,2,2,3))):
            cand=sorted({j for i in cc for j in adjacent(n,i) if j in free})
            if not cand:break
            j=rng.choice(cand);cc.append(j);free.remove(j)
        options=[1,2]
        if len(cc)==2:
            options.append(3)
            if max(s[i] for i in cc)%min(s[i] for i in cc)==0:options.append(4)
        cages.append([cc,rng.choice(options)])
    def pack():
        cells=[0]*36;a=[0]*36;b=[]
        for cid,(cc,op) in enumerate(cages):
            values=[s[i] for i in cc]
            if len(cc)==1:op=0;target=values[0];cells[cc[0]]=target
            elif op==1:target=sum(values)
            elif op==2:
                target=1
                for v in values:target*=v
            elif op==3:target=abs(values[0]-values[1])
            else:target=max(values)//min(values)
            for i in cc:a[i]=cid
            b.extend((op,target))
        return dict(id=12,n=6,cells=cells,solution=s,a=a,b=b)
    for _ in range(4):
        p=pack()
        if sum(bool(v) for v in p['cells'])>3:return None
        if unique(p):return p
        options=[]
        for k,(cc,op) in enumerate(cages):
            if len(cc)<2:continue
            for i in cc:
                remaining=set(cc)-{i};seen={next(iter(remaining))}
                for _ in cc:seen|={j for v in tuple(seen) for j in adjacent(6,v) if j in remaining}
                if seen==remaining:options.append((k,i))
        if not options:break
        k,i=rng.choice(options);cages[k][0].remove(i);cages.append([[i],0])
    return None

def candidate_futo(rng):
    n=6;s=latin_solution(n,rng);a=[0]*36;b=a[:]
    for i in range(36):
        if i%n<n-1 and rng.random()<.30:a[i]=1 if s[i]<s[i+1] else -1
        if i//n<n-1 and rng.random()<.30:b[i]=1 if s[i]<s[i+n] else -1
    p=remove_givens(dict(id=14,n=n,cells=s[:],solution=s,a=a,b=b),rng)
    clues=[(arr,i) for arr in (a,b) for i,v in enumerate(arr) if v];rng.shuffle(clues)
    for arr,i in clues[:8]:
        old=arr[i];arr[i]=0
        if not unique(p):arr[i]=old
    return p

def candidate_sky(rng,n=6):
    from grids_master_logic import visible
    s=latin_solution(n,rng)
    rows=[s[r*n:r*n+n] for r in range(n)];cols=[s[c::n] for c in range(n)]
    a=[visible(x) for x in cols]+[visible(x[::-1]) for x in cols]+[visible(x) for x in rows]+[visible(x[::-1]) for x in rows]
    p=remove_givens(dict(id=15,n=n,cells=s[:],solution=s,a=a),rng)
    order=list(range(4*n));rng.shuffle(order)
    for i in order:
        old=a[i];a[i]=0
        if not unique(p):a[i]=old
    return p

CANDIDATES={11:candidate_sudoku,12:candidate_calc,14:candidate_futo,15:candidate_sky}

def qualifies(gid,rating):
    # Strictly above every legacy HARD record's measured branch-node count:
    # 7 for Sudoku/Calcudoku/Futoshiki, 15 for Skyscrapers. In addition require
    # a real failed-literal elimination, not just few clues or a large search.
    return rating['basic_unfilled']>0 and rating['after_chains_unfilled']==0 and rating['forcing_eliminations']>=1 and rating['search_nodes']>=(17 if gid==15 else 9)

def generate(gid,max_candidates):
    OUT.mkdir(parents=True,exist_ok=True);result=[];seen={base_key(p) for p in json.loads((ROOT/f'assets/grids/{gid}.json').read_text())};start=time.time();reasons=collections.Counter()
    for attempt in range(max_candidates):
        seed=gid*100000000+attempt;rng=random.Random(seed);p=CANDIDATES[gid](rng)
        if p is None:reasons['construction_rejected']+=1;continue
        try:rating,solutions=analyze(p,30000,256)
        except Budget:reasons['rating_budget']+=1;continue
        if rating['solution_count']!=1:reasons['not_unique']+=1;continue
        if not qualifies(gid,rating):reasons['below_rating_threshold']+=1;continue
        canonical=base_key(p)
        if canonical in seen:reasons['equivalent_base']+=1;continue
        try:number,nodes=master_count(p,2000000)
        except Budget:reasons['reference_budget']+=1;continue
        if number!=1:raise AssertionError('Independent uniqueness counter disagrees')
        assert solutions[0]==p['solution'];witness_valid(p)
        p.update(difficulty=3,seed=seed,puzzle_id=900+IDS.index(gid)*20+len(result),rules_version=1,solution_count=1,counter_nodes=nodes,counter_version=2 if gid==15 else 1,logical_rating=rating,rating_version=1,rating_label='MASTER provisional')
        result.append(p);seen.add(canonical)
        print(f'{gid} {len(result)}/20 attempt={attempt+1} search={rating["search_nodes"]} depth={rating["search_max_depth"]} forcing={rating["forcing_eliminations"]} ref={nodes} elapsed={time.time()-start:.1f}s',flush=True)
        if len(result)==20:
            temporary=OUT/f'{gid}.tmp.json';temporary.write_text(json.dumps(result,separators=(',',':'))+'\n');temporary.replace(OUT/f'{gid}.json')
            (OUT/f'{gid}-generation.json').write_text(json.dumps(dict(game_id=gid,candidates=attempt+1,accepted=20,rejections=dict(reasons),seconds=round(time.time()-start,3)),indent=2)+'\n');return
    raise RuntimeError(f'{gid}: accepted {len(result)} within {max_candidates} candidate budget')

if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('--game',type=int,choices=IDS,required=True);ap.add_argument('--max-candidates',type=int,default=3000);args=ap.parse_args();generate(args.game,args.max_candidates)
