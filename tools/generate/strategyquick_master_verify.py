#!/usr/bin/env python3
"""Independent MASTER audit: no generator or runtime imports.

Strategy: iterative DAGs / Lo Shu tic-tac-toe minimax; native test additionally
checks Nim by an exhaustive 1,048,576-state normal-play DAG, not xor.
Lights: matrix RREF + complete affine nullspace, unlike generator row chasing.
Sliding: exhaustive reverse BFS and legal path/parity/admissible lower bound.
"""
from pathlib import Path
from functools import lru_cache
from collections import deque
import hashlib,json,os,re,time
ROOT=Path(__file__).resolve().parents[2]

def d4(values,n):
    keys=[]
    for reflected in (0,1):
        for rotation in range(4):
            out=[0]*(n*n)
            for i,v in enumerate(values):
                y,x=divmod(i,n)
                if reflected:x=n-1-x
                for _ in range(rotation):y,x=x,n-1-y
                out[y*n+x]=v
            keys.append(tuple(out))
    return min(keys)

LINES=((0,1,2),(3,4,5),(6,7,8),(0,3,6),(1,4,7),(2,5,8),(0,4,8),(2,4,6))
def line(grid,p):return any(all(grid[i]==p for i in indices) for indices in LINES)
@lru_cache(None)
def ttt(grid,player):
    if line(grid,3-player):return -1
    empty=[i for i,x in enumerate(grid) if not x]
    if not empty:return 0
    result=-1
    for i in empty:
        child=list(grid);child[i]=player;result=max(result,-ttt(tuple(child),3-player))
    return result

def strategy(records):
    w=[[False]*41 for _ in range(41)]
    for a in range(41):
        for b in range(41):w[a][b]=any(not w[x][b] for x in range(a)) or any(not w[a][y] for y in range(b)) or any(not w[a-k][b-k] for k in range(1,min(a,b)+1))
    e=[[False]*100 for _ in range(100)]
    for total in range(2,199):
        for a in range(1,100):
            b=total-a
            if a<=b<100:e[a][b]=any(not e[min(a,b-k*a)][max(a,b-k*a)] for k in range(1,b//a+1))
    counts={};seen={}
    for r in records:
        game,cpu,b=r['game_id'],r['cpu_first'],r['board'];group=(game,cpu);counts[group]=counts.get(group,0)+1
        if game==21:
            assert len(b)==4 and b==sorted(b) and all(8<=x<=31 for x in b)
            value=bool(b[0]^b[1]^b[2]^b[3]);key=tuple(b)
        elif game in (22,23):
            a,z=b;assert 1<=a<=z<=(40 if game==22 else 99);value=(w if game==22 else e)[a][z];key=tuple(b)
        elif game==24:
            assert b.count(1)==b.count(2) and b.count(1) in (1,2,3)
            grid=tuple(b[k-1] for k in (8,1,6,3,5,7,4,9,2));assert not line(grid,1) and not line(grid,2)
            value=ttt(grid,1)>0;assert ttt(grid,1)!=0;key=d4(grid,3)
            assert r['initial_plies']==sum(x!=0 for x in b)
        else:
            remaining=r['target']-b[0];maximum=r['max_add'];dp=[False]*(remaining+1)
            for n in range(1,len(dp)):dp[n]=any(not dp[n-k] for k in range(1,min(n,maximum)+1))
            value=dp[-1];key=(maximum,remaining)
        assert value==(not cpu) and r['value']==(-1 if cpu else 1)
        group_seen=seen.setdefault(group,set());assert key not in group_seen;group_seen.add(key)
    assert len(counts)==10
    for group,count in counts.items():assert count==({(22,1):12,(24,1):5}.get(group,30))
    return {f'{g}/cpu_first={c}':n for (g,c),n in counts.items()}

def gf2_solutions(board,n):
    cells=n*n;rows=[]
    for r in range(n):
        for c in range(n):
            mask=0
            for y,x in ((r,c),(r-1,c),(r+1,c),(r,c-1),(r,c+1)):
                if 0<=y<n and 0<=x<n:mask|=1<<(y*n+x)
            rows.append(mask|((board>>(r*n+c)&1)<<cells))
    pivot=[];rank=0
    for col in range(cells):
        candidate=next((r for r in range(rank,cells) if rows[r]>>col&1),None)
        if candidate is None:continue
        rows[rank],rows[candidate]=rows[candidate],rows[rank]
        for r in range(cells):
            if r!=rank and rows[r]>>col&1:rows[r]^=rows[rank]
        pivot.append(col);rank+=1
    assert all(not(rows[r]>>cells&1) for r in range(rank,cells))
    free=[i for i in range(cells) if i not in pivot];answers=[]
    for code in range(1<<len(free)):
        vector=sum(((code>>i)&1)<<col for i,col in enumerate(free))
        for row,col in zip(rows,pivot):
            if (row>>cells&1)^((row&vector).bit_count()%2):vector|=1<<col
        answers.append(vector)
    return answers

def lights(records):
    seen={0:set(),1:set()};minimums={0:[],1:[]}
    for r in records:
        n=r['size'];assert n==4+r['mode'];answers=gf2_solutions(r['board_bits'],n)
        assert len(answers)==r['solution_count']==(16 if n==4 else 4)
        minimum=min(a.bit_count() for a in answers);assert minimum==r['minimum'] and r['witness'] in answers and r['witness'].bit_count()==minimum
        assert minimum>=(6 if n==4 else 12)
        key=d4([r['board_bits']>>i&1 for i in range(n*n)],n);assert key not in seen[r['mode']];seen[r['mode']].add(key);minimums[r['mode']].append(minimum)
    assert all(len(group)==30 for group in seen.values())
    return {mode:dict(count=len(v),minimum=min(v),maximum=max(v)) for mode,v in minimums.items()}

def sliding(records):
    solved=bytes((1,2,3,4,5,6,7,8,0));depth={solved:0};queue=deque((solved,))
    neighbours=[tuple(q for q in range(9) if abs(q//3-p//3)+abs(q%3-p%3)==1) for p in range(9)]
    while queue:
        b=queue.popleft();p=b.index(0)
        for q in neighbours[p]:
            child=bytearray(b);child[p],child[q]=child[q],child[p];child=bytes(child)
            if child not in depth:depth[child]=depth[b]+1;queue.append(child)
    assert len(depth)==181440 and max(depth.values())==31
    seen=[set(),set()];lower=[]
    for r in records:
        b=r['board'];mode=r['mode'];key=tuple(b);assert key not in seen[mode];seen[mode].add(key)
        if not mode:assert r['distance']==depth[bytes(b)]==31;continue
        assert sorted(b)==list(range(16));inv=sum(x and y and x>y for i,x in enumerate(b) for y in b[i+1:]);assert (inv+4-b.index(0)//4)%2==1
        actual=list(range(1,16))+[0];p=15
        for q in r['legal_path']:
            assert abs(p//4-q//4)+abs(p%4-q%4)==1;actual[p],actual[q]=actual[q],actual[p];p=q
        assert actual==b and len(r['legal_path'])==400
        m=sum(abs(i//4-(v-1)//4)+abs(i%4-(v-1)%4) for i,v in enumerate(b) if v);assert m==r['manhattan_lower'] and m>=48;lower.append(m)
    assert len(seen[0])==2 and len(seen[1])==30
    return dict(solvable_3x3_states=len(depth),maximum_distance=31,distance31_starts=2,four_by_four=30,lower_min=min(lower),lower_max=max(lower))

def embedded(data):
    s=(ROOT/'assets/strategyquick/master_strategy.h').read_text();rows=re.findall(r'\{\{([0-9,]+)\},([0-9,]+)\}',s)
    assert len(rows)==len(data['strategy'])
    for (board,extra),r in zip(rows,data['strategy']):
        assert list(map(int,board.split(',')))==r['board']+[0]*(9-len(r['board']))
        assert list(map(int,extra.split(',')))==[r.get(k,0) for k in ('target','max_add','initial_plies')]
    q=(ROOT/'assets/strategyquick/master_quick.h').read_text()
    for name,items in (('sliding3',[r['board'] for r in data['sliding'] if not r['mode']]),('sliding4',[r['board'] for r in data['sliding'] if r['mode']]),('lights',[[r['board_bits'] for r in data['lights'] if r['mode']==mode] for mode in (0,1)]),('lights_min',[[r['minimum'] for r in data['lights'] if r['mode']==mode] for mode in (0,1)])):
        body=re.search(r'sq_master_'+name+r'\[[^;]+?=\{(.*?)\n\};',q,re.S);assert body,name
        rows=re.findall(r'\{([0-9u,]+)\}',body[1]);assert [[int(v.rstrip('u')) for v in row.split(',')] for row in rows]==items
    return dict(strategy_records=len(data['strategy']),sha_strategy=hashlib.sha256(s.encode()).hexdigest(),sha_quick=hashlib.sha256(q.encode()).hexdigest())

def main():
    start=time.monotonic();data=json.loads((ROOT/'assets/strategyquick/master.json').read_text())
    report=dict(strategy=strategy(data['strategy']),sliding=sliding(data['sliding']),lights=lights(data['lights']),embedded=embedded(data),seconds=round(time.monotonic()-start,3),hardware_difficulty='UNTESTED; solver metrics are not human ratings')
    output_root=Path(os.environ['NUMGAME_VERIFY_OUTPUT']) if 'NUMGAME_VERIFY_OUTPUT' in os.environ else ROOT/'assets/strategyquick'
    output_root.mkdir(parents=True,exist_ok=True)
    (output_root/'master_verification.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
