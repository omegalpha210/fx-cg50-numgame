"""Independent bounded solution counters for original NUM GAME grid packs.
Counters consume public clues only, never the stored solution witness.
"""
import itertools

class Budget(Exception): pass

class Meter:
    def __init__(self, limit=200000): self.nodes=0; self.limit=limit
    def step(self):
        self.nodes+=1
        if self.nodes>self.limit: raise Budget

def sudoku(p, limit=200000):
    n=p['n']; b=p['cells'][:]
    if n not in (4,9) or len(b)!=n*n or any(v<0 or v>n for v in b):return 0,0
    box=3 if n==9 else 2
    units=[b[r*n:r*n+n] for r in range(n)]+[b[c::n] for c in range(n)]
    units += [[b[(r+y)*n+c+x] for y in range(box) for x in range(box)] for r in range(0,n,box) for c in range(0,n,box)]
    if any(len([v for v in u if v])!=len({v for v in u if v}) for u in units):return 0,0
    total=0; m=Meter(limit)
    def go():
        nonlocal total
        m.step(); best=-1; choices=None
        for i,v in enumerate(b):
            if v: continue
            r,c=divmod(i,n)
            used=set(b[r*n:r*n+n])|{b[k*n+c] for k in range(n)}|{b[(r//box*box+y)*n+c//box*box+x] for y in range(box) for x in range(box)}
            cand=[v for v in range(1,n+1) if v not in used]
            if not cand:return
            if choices is None or len(cand)<len(choices):best,choices=i,cand
        if best<0:total+=1;return
        for v in choices:
            b[best]=v;go()
            if total>=2:break
        b[best]=0
    go();return total,m.nodes

def latin(p, limit=200000):
    n=p['n']; b=[0]*(n*n); given=p['cells']; a=p.get('a',[]); z=p.get('b',[]); gid=p['id'];total=0;m=Meter(limit)
    def vis(row):
        h=0;k=0
        for v in row:
            if v>h:h=v;k+=1
        return k
    rows=[]
    for r in range(n):
        rr=[]
        for row in itertools.permutations(range(1,n+1)):
            if any(given[r*n+c] and given[r*n+c]!=row[c] for c in range(n)):continue
            if gid==15 and ((a[2*n+r] and vis(row)!=a[2*n+r]) or (a[3*n+r] and vis(row[::-1])!=a[3*n+r])):continue
            if gid==14 and any(a[r*n+c] and (row[c]-row[c+1])*a[r*n+c]>=0 for c in range(n-1)):continue
            rr.append(row)
        rows.append(rr)
    cages={}
    if gid==12:
        for i,c in enumerate(a):cages.setdefault(c,[]).append(i)
    def cage_ok():
        for cid,cc in cages.items():
            op=z[2*cid]; target=z[2*cid+1]; vals=[b[i] for i in cc]; full=all(vals)
            if op==0 and vals[0] and vals[0]!=target:return False
            if op==1 and (sum(vals)>target or (full and sum(vals)!=target)):return False
            if op==2:
                prod=1
                for v in vals:
                    if v:prod*=v
                if prod>target or target%prod or (full and prod!=target):return False
            if full and op==3 and abs(vals[0]-vals[1])!=target:return False
            if full and op==4 and max(vals)!=min(vals)*target:return False
        return True
    def go(r):
        nonlocal total
        m.step()
        if r==n:
            if gid==15:
                for c in range(n):
                    col=[b[k*n+c] for k in range(n)]
                    if a[c] and vis(col)!=a[c]:return
                    if a[n+c] and vis(col[::-1])!=a[n+c]:return
            total+=1;return
        for row in rows[r]:
            if any(row[c] in b[c:r*n:n] for c in range(n)):continue
            if gid==14 and r and any(z[(r-1)*n+c] and (b[(r-1)*n+c]-row[c])*z[(r-1)*n+c]>=0 for c in range(n)):continue
            b[r*n:(r+1)*n]=row
            if gid!=12 or cage_ok():go(r+1)
            if total>=2:break
        b[r*n:(r+1)*n]=[0]*n
    go(0);return total,m.nodes

def binary(p, limit=200000):
    n=p['n']; given=p['cells']; rows=[]; b=[];total=0;m=Meter(limit)
    patterns=[x for x in itertools.product((0,1),repeat=n) if sum(x)==n//2 and all(not(x[i]==x[i+1]==x[i+2]) for i in range(n-2))]
    for r in range(n):rows.append([x for x in patterns if all(given[r*n+c]==255 or given[r*n+c]==x[c] for c in range(n))])
    def go(r):
        nonlocal total
        m.step()
        if r==n:
            cols=list(zip(*b))
            if len(set(cols))==n:total+=1
            return
        for row in rows[r]:
            if row in b:continue
            if r>=2 and any(b[-2][c]==b[-1][c]==row[c] for c in range(n)):continue
            if any(sum(x[c] for x in b)+row[c]>n//2 or sum(x[c] for x in b)+row[c]+(n-r-1)<n//2 for c in range(n)):continue
            b.append(row);go(r+1);b.pop()
            if total>=2:break
    go(0);return total,m.nodes

def numbrix(p, limit=200000):
    n=p['n']; size=n*n; clues=p['cells']; where={v:i for i,v in enumerate(clues) if v}; b=[0]*size;total=0;m=Meter(limit)
    neighbors=[[j for j in range(size) if abs(j//n-i//n)+abs(j%n-i%n)==1] for i in range(size)]
    def go(v,i):
        nonlocal total
        m.step()
        if b[i] or (clues[i] and clues[i]!=v) or (v in where and where[v]!=i):return
        for num,pos in where.items():
            if num>v:
                dist=abs(pos//n-i//n)+abs(pos%n-i%n)
                if dist>num-v or (num-v-dist)%2:return
        b[i]=v
        if v==size:total+=1
        else:
            for j in neighbors[i]:
                go(v+1,j)
                if total>=2:break
        b[i]=0
    for i in ([where[1]] if 1 in where else range(size)):
        go(1,i)
        if total>=2:break
    return total,m.nodes

def sumgrid(p, limit=200000):
    n=p['n']; cells=p['cells']; targets=p['a']; options=[];total=0;m=Meter(limit)
    for r in range(n):options.append([bits for bits in itertools.product((0,1),repeat=n) if sum(cells[r*n+c]*bits[c] for c in range(n))==targets[r]])
    sums=[0]*n
    def go(r):
        nonlocal total
        m.step()
        if r==n:
            if sums==targets[n:2*n]:total+=1
            return
        for bits in options[r]:
            vals=[cells[r*n+c]*bits[c] for c in range(n)]
            if any(sums[c]+vals[c]>targets[n+c] for c in range(n)):continue
            for c in range(n):sums[c]+=vals[c]
            go(r+1)
            for c in range(n):sums[c]-=vals[c]
            if total>=2:break
    go(0);return total,m.nodes

def hitori(p,limit=200000):
    n=p['n']; values=p['cells'];size=n*n;total=0;m=Meter(limit)
    neighbors=[[j for j in range(size) if abs(j//n-i//n)+abs(j%n-i%n)==1] for i in range(size)]
    duplicate=[(i,j) for i in range(size) for j in range(i+1,size) if values[i]==values[j] and (i//n==j//n or i%n==j%n)]
    def go(s):
        nonlocal total
        m.step()
        while True:
            changed=False
            for i,j in duplicate:
                if s[i]==0 and s[j]==0:return
                if s[i]==0 and s[j]<0:s[j]=1;changed=True
                if s[j]==0 and s[i]<0:s[i]=1;changed=True
            for i in range(size):
                if s[i]==1:
                    for j in neighbors[i]:
                        if s[j]==1:return
                        if s[j]<0:s[j]=0;changed=True
            if not changed:break
        reachable=[i for i in range(size) if s[i]!=1]
        if not reachable:return
        seen={next((i for i in reachable if s[i]==0),reachable[0])}; stack=list(seen)
        while stack:
            for j in neighbors[stack.pop()]:
                if s[j]!=1 and j not in seen:seen.add(j);stack.append(j)
        if any(s[i]==0 and i not in seen for i in range(size)):return
        if -1 not in s:
            if len(seen)==len(reachable):total+=1
            return
        scores=[sum(i in pair for pair in duplicate) if s[i]<0 else -1 for i in range(size)];i=max(range(size),key=lambda x:scores[x])
        for v in (0,1):
            t=s[:];t[i]=v;go(t)
            if total>=2:break
    go([-1]*size);return total,m.nodes

def kakuro(p,limit=200000):
    n=p['n']; cells=p['cells']; a=p['a']; z=p['b'];runs=[]; belongs={i:[] for i,v in enumerate(cells) if v==255}; total=0;m=Meter(limit)
    for i in range(n*n):
        for target,step in ((a[i],1),(z[i],n)):
            if not target:continue
            cc=[];j=i+step
            while j<n*n and cells[j]==255 and (step!=1 or j//n==i//n):cc.append(j);j+=step
            assert 2<=len(cc)<=9
            rid=len(runs);runs.append((cc,target))
            for j in cc:belongs[j].append(rid)
    assert all(len(r)==2 for r in belongs.values())
    vals={i:0 for i in belongs}
    def candidates(i):
        out=[]
        for v in range(1,10):
            ok=True
            for rid in belongs[i]:
                cc,target=runs[rid];used=[vals[j] for j in cc if vals[j]]
                if v in used:ok=False;break
                rem=len(cc)-len(used)-1;left=target-sum(used)-v;pool=[x for x in range(1,10) if x not in used and x!=v]
                if left<sum(pool[:rem]) or left>sum(pool[-rem:] if rem else []):ok=False;break
            if ok:out.append(v)
        return out
    def go():
        nonlocal total
        m.step();best=None;opts=None
        for i in vals:
            if vals[i]:continue
            cs=candidates(i)
            if not cs:return
            if opts is None or len(cs)<len(opts):best,opts=i,cs
        if best is None:total+=1;return
        for v in opts:
            vals[best]=v;go()
            if total>=2:break
        vals[best]=0
    go();return total,m.nodes

COUNTERS={11:sudoku,12:latin,13:kakuro,14:latin,15:latin,16:hitori,17:binary,18:numbrix,20:sumgrid}

def count(p,limit=200000):return COUNTERS[p['id']](p,limit)
