#!/usr/bin/env python3
"""Deterministic original grid generation. No downloaded puzzle corpus.
Random construction is separate from the public-clue solution counters.
"""
import argparse,itertools,json,random,time,pathlib
from grids_solver import count,Budget
from grids_verify import base_key
ROOT=pathlib.Path(__file__).resolve().parents[2]
OUT=ROOT/'assets/grids'; OUT.mkdir(parents=True,exist_ok=True)

def adjacent(n,i):return [j for j in range(n*n) if abs(j//n-i//n)+abs(j%n-i%n)==1]

def latin_solution(n,rng,box=0):
    b=[0]*(n*n)
    def go():
        cells=[];minlen=99
        for i,v in enumerate(b):
            if v:continue
            r,c=divmod(i,n);used=set(b[r*n:r*n+n])|set(b[c::n])
            if box:used|={b[(r//box*box+y)*n+c//box*box+x] for y in range(box) for x in range(box)}
            cs=[x for x in range(1,n+1) if x not in used]
            if not cs:return False
            if len(cs)<minlen:minlen=len(cs);cells=[(i,cs)]
            elif len(cs)==minlen:cells.append((i,cs))
        if not cells:return True
        i,cs=rng.choice(cells);rng.shuffle(cs)
        for x in cs:
            b[i]=x
            if go():return True
        b[i]=0;return False
    assert go();return b

def remove_clues(p,rng,target,blank=0):
    order=list(range(len(p['cells'])));rng.shuffle(order)
    for i in order:
        if sum(v!=blank for v in p['cells'])<=target:break
        if p['id']==18 and p['cells'][i] in (1,len(p['cells'])):continue
        old=p['cells'][i];p['cells'][i]=blank
        try:unique=count(p,100000)[0]==1
        except Budget:unique=False
        if not unique:p['cells'][i]=old
    return p

def make_sudoku(d,rng):
    s=latin_solution(9,rng,3);p=dict(id=11,n=9,cells=s[:],solution=s)
    return remove_clues(p,rng,(43,34,27)[d])

def make_calc(d,rng):
    n=4+d;s=latin_solution(n,rng);free=set(range(n*n));cages=[]
    while free:
        start=rng.choice(sorted(free));cc=[start];free.remove(start)
        for _ in range(rng.choice((1,1,2,2,3))):
            cand=[j for i in cc for j in adjacent(n,i) if j in free]
            if not cand:break
            j=rng.choice(cand);cc.append(j);free.remove(j)
        cages.append(cc)
    def pack():
        a=[0]*(n*n);b=[];giv=[0]*(n*n)
        for k,cc in enumerate(cages):
            vals=[s[i] for i in cc]
            if len(cc)==1:op=0;t=vals[0];giv[cc[0]]=t
            else:
                ops=[1,2]
                if len(cc)==2:
                    ops.append(3)
                    if max(vals)%min(vals)==0:ops.append(4)
                op=rng.choice(ops)
                if op==1:t=sum(vals)
                elif op==2:
                    t=1
                    for x in vals:t*=x
                elif op==3:t=abs(vals[0]-vals[1])
                else:t=max(vals)//min(vals)
            for i in cc:a[i]=k
            b.extend((op,t))
        return dict(id=12,n=n,cells=giv,solution=s,a=a,b=b)
    p=pack()
    while True:
        try:unique=count(p,100000)[0]==1
        except Budget:unique=False
        if unique:return p
        big=[cc for cc in cages if len(cc)>1]
        if not big:raise AssertionError
        cc=rng.choice(big);cell=cc.pop();cages.append([cell]);p=pack()

def make_futo(d,rng):
    n=4+d;s=latin_solution(n,rng);a=[0]*(n*n);b=a[:]
    for i in range(n*n):
        if i%n<n-1 and rng.random()<(.65,.5,.4)[d]:a[i]=1 if s[i]<s[i+1] else -1
        if i//n<n-1 and rng.random()<(.65,.5,.4)[d]:b[i]=1 if s[i]<s[i+n] else -1
    return remove_clues(dict(id=14,n=n,cells=s[:],solution=s,a=a,b=b),rng,(3,5,7)[d])

def make_sky(d,rng):
    n=4+d;s=latin_solution(n,rng)
    def v(line):
        out=0;h=0
        for x in line:
            if x>h:out+=1;h=x
        return out
    a=[v(s[c::n]) for c in range(n)]+[v(s[c::n][::-1]) for c in range(n)]+[v(s[r*n:r*n+n]) for r in range(n)]+[v(s[r*n:r*n+n][::-1]) for r in range(n)]
    p=remove_clues(dict(id=15,n=n,cells=s[:],solution=s,a=a),rng,0)
    order=list(range(4*n));rng.shuffle(order)
    for i in order[:d*n]:
        old=a[i];a[i]=0
        try:unique=count(p,100000)[0]==1
        except Budget:unique=False
        if not unique:a[i]=old
    return p

def make_binary(d,rng):
    n=(6,6,8)[d];patterns=[x for x in itertools.product((0,1),repeat=n) if sum(x)==n//2 and all(not(x[i]==x[i+1]==x[i+2]) for i in range(n-2))];b=[]
    def go():
        if len(b)==n:return len(set(zip(*b)))==n
        opts=patterns[:];rng.shuffle(opts)
        for row in opts:
            if row in b:continue
            if len(b)>=2 and any(b[-2][c]==b[-1][c]==row[c] for c in range(n)):continue
            if any(sum(x[c] for x in b)+row[c]>n//2 or sum(x[c] for x in b)+row[c]+n-len(b)-1<n//2 for c in range(n)):continue
            b.append(row)
            if go():return True
            b.pop()
        return False
    assert go();s=list(itertools.chain.from_iterable(b));return remove_clues(dict(id=17,n=n,cells=s[:],solution=s),rng,(17,11,20)[d],255)

def make_numbrix(d,rng):
    n=4+d;path=[r*n+(c if r%2==0 else n-1-c) for r in range(n) for c in range(n)]
    for _ in range(2000):
        if rng.randrange(2):path.reverse()
        end=path[-1];opts=[j for j in adjacent(n,end) if j!=path[-2]]
        j=rng.choice(opts);k=path.index(j)
        path=path[:k+1]+path[k+1:][::-1]
    s=[0]*(n*n)
    for v,i in enumerate(path,1):s[i]=v
    return remove_clues(dict(id=18,n=n,cells=s[:],solution=s),rng,(8,10,12)[d])

def make_sum(d,rng):
    n=(5,5,6)[d]
    for _ in range(20000):
        s=[rng.randrange(2) for _ in range(n*n)];cells=[rng.randint(1,(5,9,12)[d]) for _ in s]
        a=[sum(cells[r*n+c]*s[r*n+c] for c in range(n)) for r in range(n)]+[sum(cells[r*n+c]*s[r*n+c] for r in range(n)) for c in range(n)]
        p=dict(id=20,n=n,cells=cells,solution=s,a=a)
        try:
            if count(p,100000)[0]==1:return p
        except Budget:pass
    raise RuntimeError('sum budget')

def connected_white(n,mask):
    white={i for i,v in enumerate(mask) if not v}
    if not white:return False
    stack=[next(iter(white))];seen=set(stack)
    while stack:
        for j in adjacent(n,stack.pop()):
            if j in white and j not in seen:seen.add(j);stack.append(j)
    return seen==white

def make_hitori(d,rng):
    n=4+d
    for _ in range(100000):
        black=[0]*(n*n);order=list(range(n*n));rng.shuffle(order)
        for i in order:
            if all(not black[j] for j in adjacent(n,i)):
                black[i]=1
                if not connected_white(n,black):black[i]=0
        s=latin_solution(n,rng);cells=s[:]
        for i,v in enumerate(black):
            if v:
                peers=[j for j in range(n*n) if not black[j] and (j//n==i//n or j%n==i%n)]
                cells[i]=cells[rng.choice(peers)]
        p=dict(id=16,n=n,cells=cells,solution=black)
        try:
            if count(p,40000)[0]==1:return p
        except Budget:pass
    raise RuntimeError('hitori budget')

def make_kakuro(d,rng):
    n=(5,6,7)[d]
    for attempt in range(200000):
        # Unequal compact run layouts are generated afresh; all white cells
        # belong to exactly two runs of length >=2.
        cells=[0]*(n*n)
        for _ in range((3,4,5)[d]):
            r=rng.randrange(1,n-1);c=rng.randrange(1,n-1)
            for dy in (0,1):
                for dx in (0,1):cells[(r+dy)*n+c+dx]=255
        runs=[];valid=True
        for r in range(n):
            for c in range(n):
                i=r*n+c
                if cells[i]!=255:continue
                for step,start in ((1,c==0 or cells[i-1]==0),(n,r==0 or cells[i-n]==0)):
                    if not start:continue
                    cc=[];j=i
                    while j<n*n and cells[j]==255 and (step!=1 or j//n==r):cc.append(j);j+=step
                    if len(cc)<2:valid=False;break
                    runs.append((i-step,step,cc))
        if not valid or sum(x==255 for x in cells)<(7,11,16)[d] or not connected_white(n,[x!=255 for x in cells]):continue
        white=[i for i,v in enumerate(cells) if v==255];s=[0]*(n*n);bel={i:[cc for _,_,cc in runs if i in cc] for i in white}
        def fill(k):
            if k==len(white):return True
            i=white[k];used={s[j] for cc in bel[i] for j in cc};opts=[v for v in range(1,10) if v not in used];rng.shuffle(opts)
            # Extremal sums create solvable small cross-sum clues naturally.
            opts.sort(key=lambda v: v + rng.random()*6)
            for v in opts:
                s[i]=v
                if fill(k+1):return True
            s[i]=0;return False
        if not fill(0):continue
        a=[0]*(n*n);b=a[:]
        for clue,step,cc in runs:(a if step==1 else b)[clue]=sum(s[i] for i in cc)
        p=dict(id=13,n=n,cells=cells,solution=s,a=a,b=b)
        try:
            if count(p,20000)[0]==1:return p
        except Budget:pass
    raise RuntimeError('kakuro budget')

def make_magic(d,rng):
    n=3 if d<2 else 4
    s=([8,1,6,3,5,7,4,9,2] if n==3 else [16,2,3,13,5,11,10,8,9,7,6,12,4,14,15,1])
    for _ in range(rng.randrange(4)):s=[s[(n-1-c)*n+r] for r in range(n) for c in range(n)]
    if rng.randrange(2):s=[s[r*n+n-1-c] for r in range(n) for c in range(n)]
    cells=[v if rng.random()<(0.45,0.2,0.4)[d] else 0 for v in s]
    return dict(id=19,n=n,cells=cells,solution=s)

MAKERS={11:make_sudoku,12:make_calc,13:make_kakuro,14:make_futo,15:make_sky,16:make_hitori,17:make_binary,18:make_numbrix,19:make_magic,20:make_sum}

def generate(gid,only_difficulty=None):
    result=[];seen=set();bases=set();start=time.time()
    if only_difficulty is not None:
        result=[p for p in json.loads((OUT/f'{gid}.json').read_text()) if p['difficulty']!=only_difficulty]
        if gid!=19:bases={base_key(p) for p in result}
    for d in range(3):
        if only_difficulty is not None and d!=only_difficulty:continue
        for k in range(30):
            seed=gid*1000000+d*10000+k;rng=random.Random(seed)
            while True:
                p=MAKERS[gid](d,rng)
                if gid==12 and sum(bool(v) for v in p['cells'])>(4,7,10)[d]:continue
                identity=json.dumps({key:p.get(key) for key in ('n','cells','a','b')},sort_keys=True)
                canonical=base_key(p) if gid!=19 else None
                if identity not in seen and (gid==19 or canonical not in bases):break
            seen.add(identity);bases.add(canonical);p.update(difficulty=d,seed=seed,puzzle_id=(gid-11)*90+d*30+k,rules_version=1)
            if gid!=19:p['solution_count'],p['counter_nodes']=count(p)
            else:p['solution_count']=None;p['counter_nodes']=0
            result.append(p)
            (OUT/f'{gid}.json').write_text(json.dumps(sorted(result,key=lambda p:p['puzzle_id']),separators=(',',':'))+'\n')
            print(f'{gid} difficulty={d} {k+1}/30 elapsed={time.time()-start:.1f}s nodes={p["counter_nodes"]}',flush=True)
    (OUT/f'{gid}.json').write_text(json.dumps(sorted(result,key=lambda p:p['puzzle_id']),separators=(',',':'))+'\n')

def emit():
    from grids_compact import emit as compact_emit
    compact_emit(destination=ROOT,input_root=OUT.parents[1])

if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('--game',type=int);ap.add_argument('--difficulty',type=int);ap.add_argument('--emit',action='store_true');args=ap.parse_args()
    if args.emit:emit()
    else:
        for gid in ([args.game] if args.game else range(11,21)):generate(gid,args.difficulty)
