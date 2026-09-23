#!/usr/bin/env python3
"""Verify new MASTER content, reasoning evidence and unchanged legacy records."""
import hashlib,json,pathlib,statistics,time
from grids_master_logic import analyze
from grids_master_reference import master_count,sky_count
from grids_verify import base_key,witness_valid
from grids_report import write_report
ROOT=pathlib.Path(__file__).resolve().parents[2]
OUT=ROOT/'assets/grids/master'
IDS=(11,12,14,15)

def summary(rr):
    keys=('basic_unfilled','advanced_unfilled','after_chains_unfilled','forcing_eliminations','forcing_probes','search_nodes','search_branches','search_max_depth')
    out={k:dict(min=min(r[k] for r in rr),median=statistics.median(r[k] for r in rr),max=max(r[k] for r in rr)) for k in keys}
    for group in ('basic','advanced'):
        out[group]={k:sum(r[group].get(k,0) for r in rr) for k in sorted(set().union(*(r[group] for r in rr)))}
    out['basic_stalled']=sum(r['basic_unfilled']>0 for r in rr)
    return out

def schema_valid(p):
    n=p['n'];nn=n*n;gid=p['id']
    assert len(p['cells'])==len(p['solution'])==nn
    assert all(type(v) is int and 0<=v<=n for v in p['cells'])
    assert p['counter_version']==(2 if gid==15 else 1)
    assert p['rating_label']=='MASTER provisional'
    if gid==12:
        a=p['a'];b=p['b'];assert len(a)==nn and len(b)<=81
        ids=sorted(set(a));assert ids==list(range(len(ids))) and len(b)==2*len(ids)
        for cid in ids:
            cc={i for i,v in enumerate(a) if v==cid};assert 1<=len(cc)<=4
            seen={next(iter(cc))}
            while True:
                connected=seen|{j for i in seen for j in cc if abs(i//n-j//n)+abs(i%n-j%n)==1}
                if connected==seen:break
                seen=connected
            assert seen==cc
            op,target=b[2*cid:2*cid+2];assert op in (0,1,2,3,4) and 0<=target<=32767
            assert (op==0)==(len(cc)==1)
            if op in (3,4):assert len(cc)==2
            for i in cc:assert p['cells'][i]==(target if op==0 else 0)
    elif gid==14:
        assert len(p['a'])==len(p['b'])==nn
        assert all(v in (-1,0,1) and (not v or i%n<n-1) for i,v in enumerate(p['a']))
        assert all(v in (-1,0,1) and (not v or i<nn-n) for i,v in enumerate(p['b']))
    elif gid==15:
        a=p['a'];assert len(a)==4*n and all(0<=v<=n for v in a)
        assert any(a[i] and a[n+i] for i in range(n)) or any(a[2*n+i] and a[3*n+i] for i in range(n))

def verify():
    start=time.monotonic();report=[];seen_ids=set();manifest=json.loads((OUT/'legacy-sha256.json').read_text())
    for path,digest in manifest['files'].items():assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==digest,(path,'legacy modified')
    from grids_compact import records,verify_native
    pack=records();verify_native()
    def arr(v):return '{'+','.join(map(str,v))+'}'
    old_records=''.join('{'+','.join([str(p['id']),str(p['difficulty']),str(p['n']),arr(p['cells']),arr(p['solution']),arr(p.get('a',[0])),arr(p.get('b',[0]))])+'},\n' for p in pack[:900])
    assert hashlib.sha256(old_records.encode()).hexdigest()==manifest['native_legacy_records_sha256']
    for group,gid in enumerate(IDS):
        baseline=json.loads((ROOT/f'assets/grids/{gid}.json').read_text());master=json.loads((OUT/f'{gid}.json').read_text());assert len(master)==20
        canonical={base_key(p) for p in baseline};hard=[];ratings=[]
        for p in baseline[60:90]:
            public={k:v for k,v in p.items() if k!='solution'};rating,solutions=analyze(public)
            assert rating['solution_count']==1 and solutions[0]==p['solution'];hard.append(rating)
        # Cross-check new Skyscrapers reference against every accepted legacy
        # puzzle, whose independent old row enumerator already counted one.
        if gid==15:
            for p in baseline:assert sky_count(p)[0]==1
        max_hard=max(r['search_nodes'] for r in hard)
        for index,p in enumerate(master):
            assert p['id']==gid and p['difficulty']==3 and p['rules_version']==1 and p['rating_version']==1
            assert p['puzzle_id']==900+group*20+index and p['puzzle_id'] not in seen_ids;seen_ids.add(p['puzzle_id'])
            assert p['n']==(9 if gid==11 else 6);schema_valid(p);witness_valid(p)
            key=base_key(p);assert key not in canonical;canonical.add(key)
            number,nodes=master_count(p);assert number==p['solution_count']==1 and nodes==p['counter_nodes']
            public={k:v for k,v in p.items() if k not in ('solution','logical_rating')};rating,solutions=analyze(public)
            assert solutions[0]==p['solution'] and rating==p['logical_rating']
            assert rating['basic_unfilled']>0 and rating['after_chains_unfilled']==0 and rating['forcing_eliminations']>=1
            assert rating['search_nodes']>max_hard and rating['solution_count']==1
            ratings.append(rating)
        report.append(dict(game_id=gid,master_count=20,legacy_hard_count=30,hard=summary(hard),master=summary(ratings),reference_nodes_max=max(p['counter_nodes'] for p in master)))
        print(f'PASS MASTER {gid}:20 distinct unique bases; public-clue logical evidence; search min{min(r["search_nodes"] for r in ratings)}>HARD max{max_hard}',flush=True)
    result=dict(rating_version=1,provisional=True,supported_game_ids=list(IDS),legacy_records=900,legacy_unique=810,master_records=80,total_records=980,total_unique=890,magic_layouts=90,legacy_bytes_unchanged=True,seconds=round(time.monotonic()-start,3),groups=report)
    write_report('master-audit.json',result);print(json.dumps(result),flush=True)
    return result
if __name__=='__main__':verify()
