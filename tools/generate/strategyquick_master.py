#!/usr/bin/env python3
"""Original, deterministic host-only MASTER starts. No external puzzle input.

Strategy records encode side-to-move ownership; CPU-first swaps labels.
Sliding 3x3 has exactly two distance-31 states, not thirty claimed variants.
Lights records are distinct boards, with dihedral equivalents removed.
"""
from pathlib import Path
from functools import lru_cache
from collections import deque
import random,json,math
ROOT=Path(__file__).resolve().parents[2]
SEED=202609230128

def canonical(board,n):
    a=list(board);out=[]
    for _ in range(4):
        out.extend((tuple(a),tuple(a[r*n+n-1-c] for r in range(n) for c in range(n))))
        a=[a[(n-1-c)*n+r] for r in range(n) for c in range(n)]
    return min(out)

@lru_cache(None)
def wythoff(a,b):
    if not a and not b:return -1,0
    children=[wythoff(a-k,b) for k in range(1,a+1)]+[wythoff(a,b-k) for k in range(1,b+1)]+[wythoff(a-k,b-k) for k in range(1,min(a,b)+1)]
    win=[d for v,d in children if v<0]
    return (1,1+min(win)) if win else (-1,1+max(d for v,d in children))

@lru_cache(None)
def euclid(a,b):
    if not a:return -1,0
    children=[euclid(*sorted((a,b-k*a))) for k in range(1,b//a+1)]
    win=[d for v,d in children if v<0]
    return (1,1+min(win)) if win else (-1,1+max(d for v,d in children))

TRIPLES=[sum(1<<i for i in (a,b,c)) for a in range(9) for b in range(a+1,9) for c in range(b+1,9) if a+b+c==12]
def won(mask):return any(mask&t==t for t in TRIPLES)
@lru_cache(None)
def fifteen(mine,theirs):
    if won(mine):return 1,0
    if won(theirs):return -1,0
    if mine|theirs==511:return 0,0
    children=[fifteen(theirs,mine|1<<i) for i in range(9) if not (mine|theirs)>>i&1]
    best=max(-v for v,d in children);depths=[d for v,d in children if -v==best]
    return best,1+(max(depths) if best<0 else min(depths))

def strategies(rng):
    records=[]
    for game in range(21,26):
        for cpu in (0,1):
            candidates=[];want=-1 if cpu else 1
            if game==21:
                for a in range(8,25):
                    for b in range(a,28):
                        for c in range(b,31):
                            for d in range(c,32):
                                x=a^b^c^d;win=sum((v^x)<v for v in (a,b,c,d))
                                if (bool(x)==(want>0)) and (not x or win==1) and len({a,b,c,d})>=3:
                                    candidates.append(dict(board=[a,b,c,d],value=want,depth_lower=4))
            elif game in (22,23):
                maximum=40 if game==22 else 99;oracle=wythoff if game==22 else euclid
                for a in range(1,maximum+1):
                    for b in range(a,maximum+1):
                        if game==23 and math.gcd(a,b)!=1:continue
                        value,depth=oracle(a,b)
                        if value==want and depth>=(7 if game==22 else 5):candidates.append(dict(board=[a,b],value=value,depth=depth))
            elif game==24:
                for code in range(3**9):
                    n=code;board=[];mine=theirs=0
                    for i in range(9):
                        p=n%3;n//=3;board.append(p)
                        if p==1:mine|=1<<i
                        elif p==2:theirs|=1<<i
                    if mine.bit_count()!=theirs.bit_count() or mine.bit_count() not in (1,2,3) or won(mine) or won(theirs):continue
                    value,depth=fifteen(mine,theirs)
                    if value==want and depth>=3:candidates.append(dict(board=board,value=value,depth=depth,initial_plies=(mine|theirs).bit_count()))
            else:
                for maximum in range(2,9):
                    for target in range(24,64):
                        for total in range(3,target//2):
                            remaining=target-total;value=1 if remaining%(maximum+1) else -1
                            if value==want and remaining>=3*(maximum+1):candidates.append(dict(board=[total],target=target,max_add=maximum,value=value,depth_lower=remaining//maximum))
            # Do not inflate the bank using order/symmetry/translation copies.
            unique={}
            for r in candidates:
                if game==24:
                    grid=[r['board'][card-1] for card in (8,1,6,3,5,7,4,9,2)];key=canonical(grid,3)
                elif game==25:key=(r['max_add'],r['target']-r['board'][0])
                else:key=tuple(r['board'])
                unique.setdefault(key,r)
            candidates=list(unique.values())
            assert len(candidates)>=2,(game,cpu,len(candidates))
            rng.shuffle(candidates)
            for i,r in enumerate(candidates[:30]):r.update(game_id=game,cpu_first=cpu,puzzle_id=i);records.append(r)
            print('strategy',game,cpu,len(candidates),flush=True)
    return records

def sliding(rng):
    goal=tuple(range(1,9))+(0,);distance={goal:0};todo=deque([goal]);farthest=[]
    while todo:
        board=todo.popleft();d=distance[board];p=board.index(0)
        if d==31:farthest.append(board)
        for q in (p-3 if p>=3 else -1,p+3 if p<6 else -1,p-1 if p%3 else -1,p+1 if p%3<2 else -1):
            if q<0:continue
            child=list(board);child[p],child[q]=child[q],child[p];child=tuple(child)
            if child not in distance:distance[child]=d+1;todo.append(child)
    assert len(distance)==181440 and len(farthest)==2
    records=[dict(mode=0,puzzle_id=i,board=list(b),distance=31) for i,b in enumerate(sorted(farthest))]
    seen=set()
    for i in range(30):
        for attempt in range(10000):
            b=list(range(1,16))+[0];p=15;previous=-1;path=[]
            for _ in range(400):
                candidates=[q for q in (p-4 if p>=4 else -1,p+4 if p<12 else -1,p-1 if p%4 else -1,p+1 if p%4<3 else -1) if q>=0 and q!=previous]
                q=rng.choice(candidates);path.append(q);b[p],b[q]=b[q],b[p];previous,p=p,q
            lower=sum(abs(i//4-(v-1)//4)+abs(i%4-(v-1)%4) for i,v in enumerate(b) if v)
            if lower>=48 and tuple(b) not in seen:break
        else:raise RuntimeError('4x4 generation budget exhausted')
        seen.add(tuple(b));records.append(dict(mode=1,puzzle_id=i,board=b,manhattan_lower=lower,legal_path=path))
    print('sliding',len(records),'records',flush=True);return records

def toggle_mask(press,n):
    out=0
    for p in range(n*n):
        if press>>p&1:
            for q in (p,p-n if p>=n else -1,p+n if p<n*(n-1) else -1,p-1 if p%n else -1,p+1 if p%n<n-1 else -1):
                if q>=0:out^=1<<q
    return out

def chase(board,n):
    solutions=[];mask=(1<<n)-1
    for first in range(1<<n):
        presses=first;previous=0;row=first
        for r in range(n-1):
            nextrow=((board>>(r*n))^previous^row^(row<<1)^(row>>1))&mask
            presses|=nextrow<<((r+1)*n);previous,row=row,nextrow
        if (((board>>((n-1)*n))^previous^row^(row<<1)^(row>>1))&mask)==0:solutions.append(presses)
    return solutions

def lights(rng):
    records=[]
    for mode,n,threshold in ((0,4,6),(1,5,12)):
        seen=set();candidates=[]
        for attempt in range(100000):
            board=toggle_mask(rng.randrange(1,1<<(n*n)),n);key=canonical([(board>>p)&1 for p in range(n*n)],n)
            if key in seen:continue
            solutions=chase(board,n);minimum=min(p.bit_count() for p in solutions)
            if minimum<threshold:continue
            seen.add(key);candidates.append(dict(mode=mode,puzzle_id=len(candidates),size=n,board_bits=board,minimum=minimum,witness=min(solutions,key=int.bit_count),solution_count=len(solutions)))
            if len(candidates)==30:break
        assert len(candidates)==30;records+=candidates
        print('lights',mode,'min',min(r['minimum'] for r in candidates),'max',max(r['minimum'] for r in candidates),flush=True)
    return records

def emit(data):
    out=['/* Original verified MASTER starts; host generator strategyquick_master.py. */','#ifndef SQ_MASTER_STRATEGY_H','#define SQ_MASTER_STRATEGY_H','typedef struct { uint8_t board[9],target,max_add,initial_plies; } SqStart;','static const SqStart sq_master_starts[5][2][30]={']
    for game in range(21,26):
        out.append(' {')
        for cpu in (0,1):
            out.append('  {')
            for r in data['strategy']:
                if r['game_id']==game and r['cpu_first']==cpu:out.append('   {{'+','.join(map(str,r['board']+[0]*(9-len(r['board']))))+'},'+','.join(str(r.get(k,0)) for k in ('target','max_add','initial_plies'))+'},')
            out.append('  },')
        out.append(' },')
    out+=['};','static const uint8_t sq_master_start_count[5][2]={']
    for game in range(21,26):out.append(' {'+','.join(str(sum(r['game_id']==game and r['cpu_first']==cpu for r in data['strategy'])) for cpu in (0,1))+'},')
    out+=['};','#endif'];(ROOT/'assets/strategyquick/master_strategy.h').write_text('\n'.join(out)+'\n')
    out=['/* Original verified MASTER starts; host generator strategyquick_master.py. */','#ifndef SQ_MASTER_QUICK_H','#define SQ_MASTER_QUICK_H','static const uint8_t sq_master_sliding3[2][9]={']
    out+=[' {'+','.join(map(str,r['board']))+'},' for r in data['sliding'] if r['mode']==0];out+=['};','static const uint8_t sq_master_sliding4[30][16]={']
    out+=[' {'+','.join(map(str,r['board']))+'},' for r in data['sliding'] if r['mode']==1];out+=['};','static const uint32_t sq_master_lights[2][30]={']
    for mode in (0,1):out.append(' {'+','.join(str(r['board_bits'])+'u' for r in data['lights'] if r['mode']==mode)+'},')
    out+=['};','static const uint8_t sq_master_lights_min[2][30]={']
    for mode in (0,1):out.append(' {'+','.join(str(r['minimum']) for r in data['lights'] if r['mode']==mode)+'},')
    out+=['};','#endif'];(ROOT/'assets/strategyquick/master_quick.h').write_text('\n'.join(out)+'\n')

def main():
    rng=random.Random(SEED);data=dict(seed=SEED,rules_version=1,strategy=strategies(rng),sliding=sliding(rng),lights=lights(rng))
    (ROOT/'assets/strategyquick/master.json').write_text(json.dumps(data,indent=2)+'\n');emit(data)
if __name__=='__main__':main()
