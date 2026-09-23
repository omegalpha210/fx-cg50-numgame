#!/usr/bin/env python3
"""Independent counters, witness-free inference replay, stable IDs and codec.

Default requires the complete 1750-record deliverable. --partial is an explicit
development audit and cannot emit a final PASS or overwrite final audit.json.
"""
import argparse,collections,hashlib,json,pathlib,statistics,time
from grids_expand import SPECS,HARD_MAX,MASTER_MAX,HELL_FORCING
from grids_master_logic import Logic
from grids_master_reference import master_count
from grids_puzzle_master import PuzzleLogic,panmagic_squares
from grids_verify import base_key,witness_valid
from grids_compact import encode,decode,padded,verify_native
from grids_report import write_report
ROOT=pathlib.Path(__file__).resolve().parents[2]
OUT=ROOT/'assets/grids/expanded'

def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def schema(p):
    n=p['n'];nn=n*n;gid=p['id'];assert 3<=n<=9
    assert len(p['cells'])==len(p['solution'])==nn
    assert all(type(v) is int for key in ('cells','solution','a','b') for v in p.get(key,[]))
    if gid==12:
        assert len(p['a'])==nn and len(p['b'])<=80
        cages=set(p['a']);assert cages==set(range(len(cages))) and len(p['b'])==len(cages)*2
        for k in cages:
            cells={i for i,v in enumerate(p['a']) if v==k};assert 1<=len(cells)<=4
            op,target=p['b'][2*k:2*k+2];assert 0<=op<=4 and 0<=target<=32767
            assert (op==0)==(len(cells)==1)
            if op in (3,4):assert len(cells)==2
            seen={next(iter(cells))}
            for _ in cells:seen|={j for i in tuple(seen) for j in cells if abs(i//n-j//n)+abs(i%n-j%n)==1}
            assert seen==cells
    if gid==13:assert len(p['a'])==len(p['b'])==nn and all(v in (0,255) for v in p['cells'])
    if gid==14:
        assert len(p['a'])==len(p['b'])==nn
        assert all(v in (-1,0,1) and (not v or i%n<n-1) for i,v in enumerate(p['a']))
        assert all(v in (-1,0,1) and (not v or i<nn-n) for i,v in enumerate(p['b']))
    if gid==15:assert len(p['a'])==4*n and all(0<=v<=n for v in p['a'])
    if gid==17:assert n%2==0 and all(v in (0,1,255) for v in p['cells'])
    if gid==20:assert len(p['a'])==2*n and all(0<=v<128 for v in p['a'])
    assert decode(encode(p))==padded(p)

def verify(partial=False):
    started=time.monotonic();old=[];manifest=json.loads((ROOT/'assets/grids/master/legacy-sha256.json').read_text())
    for name,sha in manifest['files'].items():assert digest(ROOT/name)==sha
    expected_master={11:'db5136d8ff5789f51a00d2282d234dbfeacba7499a91312a3e822a31467d39b6',12:'982639f6b8a5d0bc28d186f0e5b8121fc69097deec24ba28dc855cc4612fa3d1',14:'ed100b1131f3477a545f322633a6df63f2f84fc70ed32baea95047b1195fb2b6',15:'64ae45b06768b857b778807eaf2d22f8d00fbaa8a5959b61f237476db3d0b1c3'}
    for gid in range(11,21):old+=json.loads((ROOT/f'assets/grids/{gid}.json').read_text())
    for gid,sha in expected_master.items():
        path=ROOT/f'assets/grids/master/{gid}.json';assert digest(path)==sha;old+=json.loads(path.read_text())
    seen={gid:{base_key(p) for p in old if p['id']==gid} for gid in range(11,21)}
    groups=[];added=[];square_keys=set()
    for (gid,d),(first,target) in SPECS.items():
        path=OUT/f'{gid}-{d}.json'
        if partial and not path.exists():continue
        pp=json.loads(path.read_text());assert len(pp)==target
        for k,p in enumerate(pp):
            assert (p['id'],p['difficulty'],p['puzzle_id'])==(gid,d,first+k)
            assert p['generation_version'] in (2,3,4,5,6,7,8) and p['difficulty_policy_version']==2
            if gid==12 and d<3:assert sum(bool(v) for v in p['cells'])<=(4,7,10)[d]
            schema(p);witness_valid(p);key=base_key(p);assert key not in seen[gid];seen[gid].add(key)
            if gid==19:
                assert p['solution_count'] is None and p['variant']=='panmagic' and p['rules_version']==2
                s=p['solution'];n=4
                assert all(sum(s[r*n+(start+step*r)%n] for r in range(n))==34 for start in range(n) for step in (-1,1))
                fullkey=base_key(dict(p,cells=s));assert fullkey not in square_keys;square_keys.add(fullkey)
                assert tuple(s) in panmagic_squares()
                matches=sum(all(not g or g==v for g,v in zip(p['cells'],square)) for square in panmagic_squares())
                assert matches==p['logical_rating']['construction_matches']
            else:
                public={key:value for key,value in p.items() if key not in ('solution','logical_rating')}
                number,nodes=master_count(public,2000000);assert number==p['solution_count']==1 and nodes==p['counter_nodes']
                solver=Logic(public) if gid<=15 else PuzzleLogic(public)
                rating=solver.rate(p['probe_limit']);metrics,solutions=solver.search(30000);rating.update(metrics)
                if gid>15:rating['provisional_human_rating']=True;solutions=[solver.board_solution(s) for s in solutions]
                assert rating==p['logical_rating'] and solutions==[p['solution']]
                if gid<=15 and d==2:assert rating['search_nodes']<=HARD_MAX[gid]
                if d>=3:
                    assert rating['basic_unfilled']>0 and rating['after_chains_unfilled']==0
                    threshold=(HARD_MAX[gid] if d==3 else MASTER_MAX[gid]) if gid<=15 else {16:13,17:9,18:20,20:1}[gid]
                    assert rating['search_nodes']>threshold
                    if gid<=15:assert rating['forcing_eliminations']>=(HELL_FORCING[gid] if d==4 else 1)
                    if gid<=15 and d==3:assert rating['search_nodes']<=MASTER_MAX[gid]
                    if gid>15:
                        essential={16:('white_articulation_steps','white_connectivity_steps'),17:('line_distinct_steps',),18:('path_distance_parity_steps',),20:('subset_sum_crossing_steps',)}[gid]
                        evidence=[rating['advanced']]+[t['proof'] for t in rating['forcing_trace']]
                        assert any(any(proof.get(k,0)>0 for k in essential) for proof in evidence)
                    if d==4:
                        assert rating['search_nodes']<=1001
                        assert any(t['proof'].get('propagation_passes',0)>=4 and sum(k.endswith('_steps') and v>0 for k,v in t['proof'].items())>=2 for t in rating['forcing_trace'])
                    for trace in rating['forcing_trace']:
                        v=p['solution'].index(trace['cell']+1) if gid==18 else p['solution'][trace['cell']]
                        assert trace['excluded']!=v
                if gid==15:
                    n=p['n'];a=p['a']
                    assert any(a[i] and a[n+i] for i in range(n)) or any(a[2*n+i] and a[3*n+i] for i in range(n))
        ratings=[p['logical_rating'] for p in pp];summary={}
        for key in ('basic_unfilled','advanced_unfilled','after_chains_unfilled','forcing_eliminations','forcing_probes','search_nodes','search_max_depth'):
            values=[r[key] for r in ratings if key in r]
            if values:summary[key]={'min':min(values),'median':statistics.median(values),'max':max(values)}
        groups.append(dict(game_id=gid,difficulty=d,new_records=target,sha256=digest(path),metrics=summary));added+=pp
        kind='PAN layouts (multiple solutions permitted)' if gid==19 else 'unique original bases, independent count and inference replay'
        print(f'{"PARTIAL" if partial else "PASS"} {gid}/{d}: {target} {kind}',flush=True)
    def complement_key(s):return min(base_key(dict(id=19,n=4,cells=s)),base_key(dict(id=19,n=4,cells=[17-v for v in s])))
    magic=[p['solution'] for p in added if p['id']==19]
    magic_complement=len({complement_key(s) for s in magic})
    magic_torus=len({min(complement_key([s[((r+dy)%4)*4+(c+dx)%4] for r in range(4) for c in range(4)]) for dy in range(4) for dx in range(4)) for s in magic})
    result=dict(complete=not partial,legacy_records_preserved=980,new_records=len(added),total_records=980+len(added),groups=groups,original_files_unchanged=True,external_puzzle_files=0,external_puzzle_records=0,human_rating='provisional instrument bands',magic_master_d4_witness_classes=len(square_keys),magic_master_d4_complement_classes=magic_complement,magic_master_torus_complement_classes=magic_torus,seconds=round(time.monotonic()-started,3))
    if not partial:
        assert len(added)==770 and len(square_keys)==30
        allp=old+added;counts=collections.Counter((p['id'],p['difficulty']) for p in allp)
        assert sorted(p['puzzle_id'] for p in allp)==list(range(1750))
        assert all(counts[g,d]==(30 if d==4 else 50) for g in range(11,16) for d in range(5))
        assert all(counts[g,d]==30 for g in range(16,21) for d in range(4))
        result.update(unique_records=1630,magic_layouts=120,compact=verify_native())
    write_report('partial-audit.json' if partial else 'expanded-audit.json',result)
    print(json.dumps({k:v for k,v in result.items() if k!='groups'}),flush=True);return result

def verify_c_dump(path):
    from grids_compact import records
    expected=records();actual=[json.loads(line) for line in pathlib.Path(path).read_text().splitlines()]
    assert len(actual)==len(expected)
    for p,fields in zip(expected,actual):
        expanded=padded(p)
        assert fields==[p['id'],p['difficulty'],p['n']]+sum((expanded[key] for key in ('cells','solution','a','b')),[]),p['puzzle_id']
    print(f'PASS native C decoder export: {len(actual)} records, every clue/witness/zero padding field',flush=True)

def replay_seeds():
    import random
    from grids_expand import candidate
    started=time.monotonic();total=0
    for path in sorted(OUT.glob('[0-9][0-9]-[0-4].json')):
        for p in json.loads(path.read_text()):
            q=candidate(p['id'],p['difficulty'],random.Random(p['seed']),p['generation_version'])
            assert q is not None,p['puzzle_id']
            for key in ('id','n','cells','solution','a','b','variant'):assert q.get(key)==p.get(key),(p['puzzle_id'],key)
            total+=1
        print(f'PASS generation seed replay {path.name}',flush=True)
    assert total==770
    result=dict(records=total,generation_seed_replay=True,seconds=round(time.monotonic()-started,3))
    write_report('seed-replay.json',result);print(json.dumps(result),flush=True)

if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('--partial',action='store_true');ap.add_argument('--native-dump');ap.add_argument('--replay-seeds',action='store_true');args=ap.parse_args()
    if args.native_dump:verify_c_dump(args.native_dump)
    elif args.replay_seeds:replay_seeds()
    else:verify(args.partial)
