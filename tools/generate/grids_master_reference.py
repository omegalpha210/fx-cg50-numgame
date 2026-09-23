"""Independent public-clue row-permutation count for Skyscrapers MASTER.

The logical rater propagates cell domains and branches on a cell. This reference
instead enumerates whole row permutations, pruning only exact column-prefix
membership in separately enumerated legal column permutations. It does not read
or import the rater or any solution witness. Stops at 2 solutions / node budget.
"""
import itertools
from grids_solver import Budget,count

def visibility(v):return sum(x>max(v[:i],default=0) for i,x in enumerate(v))

def sky_count(p,limit=2000000):
    n=p['n'];a=p['a'];givens=p['cells'];perms=list(itertools.permutations(range(1,n+1)))
    rows=[];cols=[]
    for r in range(n):
        rows.append([v for v in perms if all(not givens[r*n+c] or givens[r*n+c]==v[c] for c in range(n)) and (not a[2*n+r] or visibility(v)==a[2*n+r]) and (not a[3*n+r] or visibility(v[::-1])==a[3*n+r])])
    for c in range(n):
        cols.append([v for v in perms if all(not givens[r*n+c] or givens[r*n+c]==v[r] for r in range(n)) and (not a[c] or visibility(v)==a[c]) and (not a[n+c] or visibility(v[::-1])==a[n+c])])
    # Enumerate the same legal whole rows in the same order as the original
    # reference. Preindex column-prefix continuations to avoid rescanning every
    # rejected row at each node; this preserves the exact search-node metric.
    next_values=[]
    for lines in cols:
        levels=[{} for _ in range(n)]
        for values in lines:
            for k in range(n):levels[k][values[:k]]=levels[k].get(values[:k],0)|(1<<values[k])
        next_values.append(levels)
    row_support=[]
    for lines in rows:
        support=[[0]*(n+1) for _ in range(n)]
        for index,line in enumerate(lines):
            for c,value in enumerate(line):support[c][value]|=1<<index
        row_support.append(support)
    current=[() for _ in range(n)];total=0;nodes=0
    def go(r):
        nonlocal total,nodes
        nodes+=1
        if nodes>limit:raise Budget
        if r==n:total+=1;return
        eligible=(1<<len(rows[r]))-1
        for c in range(n):
            values=next_values[c][r].get(current[c],0);allowed=0
            for value in range(1,n+1):
                if values&(1<<value):allowed|=row_support[r][c][value]
            eligible&=allowed
            if not eligible:return
        while eligible:
            bit=eligible&-eligible;eligible-=bit;row=rows[r][bit.bit_length()-1]
            nexts=[current[c]+(row[c],) for c in range(n)]
            current[:]=nexts;go(r+1)
            current[:]=[v[:-1] for v in nexts]
            if total>=2:return
    go(0);return total,nodes

def master_count(p,limit=2000000):return sky_count(p,limit) if p['id']==15 else count(p,limit)
