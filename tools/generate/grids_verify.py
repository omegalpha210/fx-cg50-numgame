#!/usr/bin/env python3
"""Audit every shipped public-clue puzzle, its witness, and solution count.
Includes small-board exhaustive reference checks independent of CSP counters.
"""
import collections,itertools,json,pathlib,time
from grids_solver import count
from grids_report import write_report
ROOT=pathlib.Path(__file__).resolve().parents[2]

def witness_valid(p):
    n=p['n'];s=p['solution'];giv=p['cells'];gid=p['id'];a=p.get('a',[]);b=p.get('b',[])
    rows=[s[r*n:r*n+n] for r in range(n)];cols=list(map(list,zip(*rows)))
    if gid in (11,12,14,15):
        assert all(sorted(line)==list(range(1,n+1)) for line in rows+cols)
        assert all(not v or v==s[i] for i,v in enumerate(giv))
    if gid==11:
        assert all(sorted(s[(r+y)*n+c+x] for y in range(3) for x in range(3))==list(range(1,10)) for r in (0,3,6) for c in (0,3,6))
    elif gid==12:
        for cage in set(a):
            vv=[s[i] for i in range(n*n) if a[i]==cage];op,t=b[2*cage:2*cage+2]
            if op==0:assert len(vv)==1 and vv[0]==t
            elif op==1:assert sum(vv)==t
            elif op==2:
                prod=1
                for v in vv:prod*=v
                assert prod==t
            elif op==3:assert len(vv)==2 and abs(vv[0]-vv[1])==t
            elif op==4:assert len(vv)==2 and max(vv)==min(vv)*t
            else:assert False
    elif gid==13:
        membership=collections.Counter()
        for i in range(n*n):
            for target,step in ((a[i],1),(b[i],n)):
                if not target:continue
                assert giv[i]==0
                cc=[];j=i+step
                while j<n*n and giv[j]==255 and (step!=1 or j//n==i//n):cc.append(j);j+=step
                vv=[s[j] for j in cc];assert 2<=len(vv)<=9 and len(set(vv))==len(vv) and min(vv)>=1 and max(vv)<=9 and sum(vv)==target
                assert sum(range(1,len(vv)+1))<=target<=sum(range(10-len(vv),10))
                membership.update(cc)
        assert all(membership[i]==2 for i,v in enumerate(giv) if v==255)
    elif gid==14:
        assert all(not v or (s[i]-s[i+1])*v<0 for i,v in enumerate(a))
        assert all(not v or (s[i]-s[i+n])*v<0 for i,v in enumerate(b))
    elif gid==15:
        def visible(vv):return sum(x>max(vv[:i],default=0) for i,x in enumerate(vv))
        observed=[visible(v) for v in cols]+[visible(v[::-1]) for v in cols]+[visible(v) for v in rows]+[visible(v[::-1]) for v in rows]
        assert all(not v or observed[i]==v for i,v in enumerate(a))
    elif gid==16:
        white={i for i in range(n*n) if not s[i]};assert white
        for i in range(n*n):
            if s[i]:assert all(not s[j] for j in (i+1,i+n) if j<n*n and (j!=i+1 or i//n==j//n))
            else:assert all(giv[i]!=giv[j] for j in white if i<j and (i//n==j//n or i%n==j%n))
        stack=[next(iter(white))];seen=set(stack)
        while stack:
            i=stack.pop()
            for j in white-seen:
                if abs(i//n-j//n)+abs(i%n-j%n)==1:seen.add(j);stack.append(j)
        assert seen==white
    elif gid==17:
        assert all(v==255 or v==s[i] for i,v in enumerate(giv))
        for lines in (rows,cols):
            assert len(set(map(tuple,lines)))==n
            assert all(sum(line)==n//2 and set(line)=={0,1} and all(not(line[i]==line[i+1]==line[i+2]) for i in range(n-2)) for line in lines)
    elif gid in (18,19):
        assert sorted(s)==list(range(1,n*n+1)) and all(not v or v==s[i] for i,v in enumerate(giv))
        if gid==18:
            pp={v:divmod(i,n) for i,v in enumerate(s)}
            assert all(abs(pp[v][0]-pp[v+1][0])+abs(pp[v][1]-pp[v+1][1])==1 for v in range(1,n*n))
        else:assert len({sum(v) for v in rows+cols}|{sum(s[i*n+i] for i in range(n)),sum(s[i*n+n-1-i] for i in range(n))})==1
    elif gid==20:
        assert all(1<=v<=12 for v in giv) and set(s)<={0,1}
        assert a==[sum(giv[r*n+c]*s[r*n+c] for c in range(n)) for r in range(n)]+[sum(giv[r*n+c]*s[r*n+c] for r in range(n)) for c in range(n)]

# Entire 4x4 Hitori mask space; no CSP propagation shared with production counter.
def hitori_small_reference(p):
    n=4;valid=0
    for bits in range(1<<16):
        if bits&(bits<<4):continue
        if bits&(bits<<1)&0xeeee:continue
        white=[i for i in range(16) if not bits>>i&1]
        if not white:continue
        if any(p['cells'][i]==p['cells'][j] and (i//n==j//n or i%n==j%n) for k,i in enumerate(white) for j in white[k+1:]):continue
        seen={white[0]}
        for _ in range(16):seen|={j for j in white for i in tuple(seen) if abs(i//n-j//n)+abs(i%n-j%n)==1}
        if len(seen)==len(white):valid+=1
        if valid>=2:return valid
    return valid

def spatial_variants(n):
    a=list(range(n*n))
    for _ in range(4):
        yield a
        yield [a[r*n+n-1-c] for r in range(n) for c in range(n)]
        a=[a[(n-1-c)*n+r] for r in range(n) for c in range(n)]

def base_key(p):
    """Reject dihedral duplicates; normalize label symmetries where applicable."""
    n=p['n'];gid=p['id'];keys=[]
    for order in spatial_variants(n):
        mapping={old:new for new,old in enumerate(order)}
        cells=[p['cells'][i] for i in order];a=p.get('a',[]);b=p.get('b',[])
        if gid in (11,16):
            labels={0:0};normalized=[]
            for v in cells:
                if v not in labels:labels[v]=len(labels)
                normalized.append(labels[v])
            keys.append((tuple(normalized),))
        elif gid==17:
            keys.extend(((tuple(cells),),(tuple(1-v if v!=255 else 255 for v in cells),)))
        elif gid==18:
            keys.extend(((tuple(cells),),(tuple(n*n+1-v if v else 0 for v in cells),)))
        else:
            constraints=[]
            if gid==12:
                for cage in set(a):constraints.append((tuple(sorted(mapping[i] for i,c in enumerate(a) if c==cage)),b[cage*2],b[cage*2+1]))
            elif gid==13:
                for i in range(n*n):
                    for target,step in ((a[i],1),(b[i],n)):
                        if not target:continue
                        cc=[];j=i+step
                        while j<n*n and p['cells'][j]==255 and (step!=1 or i//n==j//n):cc.append(mapping[j]);j+=step
                        constraints.append((tuple(sorted(cc)),target))
            elif gid==14:
                for i in range(n*n):
                    for sign,step in ((a[i],1),(b[i],n)):
                        if sign:constraints.append((mapping[i if sign>0 else i+step],mapping[i+step if sign>0 else i]))
                # Order reversal preserves all public inequality semantics.
                keys.append((tuple(n+1-v if v else 0 for v in cells),tuple(sorted((j,i) for i,j in constraints))))
            elif gid==15:
                lines=[[c+r*n for r in range(n)] for c in range(n)]
                lines+= [line[::-1] for line in lines]
                rows=[[r*n+c for c in range(n)] for r in range(n)]
                lines+=rows+[line[::-1] for line in rows]
                constraints=[(tuple(mapping[i] for i in line),target) for line,target in zip(lines,a) if target]
            elif gid==20:
                lines=[[r*n+c for c in range(n)] for r in range(n)]+[[r*n+c for r in range(n)] for c in range(n)]
                constraints=[(tuple(sorted(mapping[i] for i in line)),target) for line,target in zip(lines,a)]
            keys.append((tuple(cells),tuple(sorted(constraints))))
    return min(keys)

def main():
    start=time.time();report=[];total=0
    for gid in range(11,21):
        pp=json.loads((ROOT/f'assets/grids/{gid}.json').read_text());assert len(pp)==90
        identities=set()
        for p in pp:
            witness_valid(p)
            key=json.dumps({k:p.get(k) for k in ('n','cells','a','b')},sort_keys=True);assert key not in identities;identities.add(key)
            assert p['rules_version']==1 and p['puzzle_id']==(gid-11)*90+p['difficulty']*30+p['seed']%10000
            if gid!=19:
                number,nodes=count(p);assert number==p['solution_count']==1;assert nodes==p['counter_nodes']
            if gid==16 and p['difficulty']==0:assert hitori_small_reference(p)==1
            total+=1
        for d in range(3):
            group=[p for p in pp if p['difficulty']==d];assert len(group)==30
            if gid!=19:assert len({base_key(p) for p in group})==30
            report.append(dict(game_id=gid,difficulty=d,count=len(group),size=group[0]['n'],max_counter_nodes=max(p['counter_nodes'] for p in group),unique=gid!=19))
        print(f'PASS {gid}: 90 public-clue records + witnesses'+(' (multiple solutions accepted)' if gid==19 else ', unique'),flush=True)
    out=dict(rules_version=1,records=total,unique_records=810,magic_partial_instances=90,hitori_exhaustive_masks=30*65536,seconds=round(time.time()-start,3),groups=report)
    write_report('legacy-audit.json',out)
    print(json.dumps(out),flush=True)
    from grids_master_verify import verify
    verify()
    if (ROOT/'assets/grids/expanded').exists():
        from grids_expanded_verify import verify as verify_expanded
        verify_expanded()
if __name__=='__main__':main()
