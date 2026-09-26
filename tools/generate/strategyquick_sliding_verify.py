#!/usr/bin/env python3
"""Cross-check native starts and independent C distance proofs against packs.

Does not import the generator. --sample-exe runs all C BFS/IDA*, legacy identity,
real-app resume/INIT and cycle checks before producing deterministic evidence.
"""
from __future__ import annotations
import argparse
from collections import Counter,defaultdict
import hashlib
import json
from pathlib import Path
import re
import statistics
import subprocess

ROOT=Path(__file__).resolve().parents[2]

def describe(values):
    return {'min':min(values),'median':statistics.median(values),'max':max(values),
            'histogram':dict(sorted(Counter(values).items()))}

def d4(board,n):
    variants=[]
    for swap in (False,True):
        for a in (False,True):
            for b in (False,True):
                out=[]
                for r in range(n):
                    for c in range(n):
                        y,x=(c,r) if swap else (r,c)
                        out.append(board[(n-1-y if a else y)*n+(n-1-x if b else x)])
                variants.append(tuple(out))
    return min(variants)

def goal_symmetry(board,n):
    out=[0]*(n*n)
    for p,v in enumerate(board):
        q=(p%n)*n+p//n
        out[q]=0 if not v else ((v-1)%n)*n+(v-1)//n+1
    return min(tuple(board),tuple(out))

def validate_witness(record):
    board=record['board'][:];n=3+record['mode'];p=board.index(0)
    assert len(record['witness'])==record['distance']
    for key in record['witness']:
        q=p+{'U':-n,'R':1,'D':n,'L':-1}[key]
        assert 0<=q<n*n and abs(p//n-q//n)+abs(p%n-q%n)==1
        board[p],board[q]=board[q],board[p];p=q
    assert board==list(range(1,n*n))+[0]

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--sample-exe',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    raw=subprocess.check_output([str(args.sample_exe.resolve()),'--emit']);samples=[json.loads(line) for line in raw.splitlines()]
    assert len(samples)==1600
    pack=json.loads((ROOT/'assets/strategyquick/sliding_bands.json').read_text());records=pack['records'];assert len(records)==768
    original_master=json.loads((ROOT/'assets/strategyquick/master.json').read_text())['sliding']
    master_lookup={(r['mode'],r['puzzle_id']):r for r in original_master}
    lookup={(r['mode'],r['difficulty'],r['index']):r for r in records};assert len(lookup)==768
    text=(ROOT/'assets/strategyquick/sliding_bands.h').read_text();native_bytes=0
    for mode,width in ((0,6),(1,9)):
        match=re.search(rf'sq_sliding_banded{mode+3}\[3\]\[128\]\[{width}\]=\{{(.*?)\n\}};',text,re.S);assert match
        packed=[list(map(int,row.split(','))) for row in re.findall(r'\{([0-9,]+)\}',match.group(1))];assert len(packed)==384
        for i,values in enumerate(packed):
            assert len(values)==width;native_bytes+=width
            r=lookup[mode,i//128,i%128];board=[(values[p//2]>>(4*(p%2)))&15 for p in range((mode+3)**2)]
            assert r['board']==board and r['distance']==values[-1];validate_witness(r)
    assert native_bytes==5760
    groups=defaultdict(list)
    for s in samples:
        groups[s['revision'],s['mode'],s['difficulty']].append(s)
        if s['revision']==4 and s['difficulty']<3:
            r=lookup[s['mode'],s['difficulty'],s['puzzle_id']]
            assert s['board']==r['board'] and s['exact']==r['distance']==s['stored_metric']
        if s['difficulty']==3:
            master=master_lookup[s['mode'],s['puzzle_id']];assert master['board']==s['board']
            if s['mode']:
                board=list(range(1,16))+[0];p=15
                for q in master['legal_path']:
                    assert abs(p//4-q//4)+abs(p%4-q%4)==1
                    board[p],board[q]=board[q],board[p];p=q
                assert board==s['board'] and len(master['legal_path'])==400
    summaries=[]
    for (revision,mode,level),rows in sorted(groups.items()):
        board=[tuple(r['board']) for r in rows];n=3+mode
        exact=[r['exact'] for r in rows if r['exact'] is not None]
        summaries.append({'revision':revision,'mode':mode,'difficulty':level,'states':len(rows),
            'scope':'entire bank' if revision==4 or level==3 else 'seeds 1..128 runtime sample',
            'optimal_distance':describe(exact) if exact else None,
            'manhattan_lower_bound':describe([r['lower_bound'] for r in rows]),
            'exact_duplicates':len(board)-len(set(board)),
            'raw_position_D4_duplicates':len(board)-len({d4(b,n) for b in board}),
            'goal_preserving_reflection_duplicates':len(board)-len({goal_symmetry(b,n) for b in board})})
    for mode in (0,1):
        new=[r for r in records if r['mode']==mode]
        assert len({tuple(r['board']) for r in new})==384
        assert len({d4(r['board'],3+mode) for r in new})==384
        assert len({goal_symmetry(r['board'],3+mode) for r in new})==384
        for level,(lo,hi) in enumerate(((8,14),(15,20),(21,26))):
            d=[s['exact'] for s in groups[4,mode,level]];assert min(d)==lo and max(d)==hi
        master=groups[4,mode,3]
        assert all((s['exact']==31 if not mode else s['lower_bound']>=48) for s in master)
        assert {tuple(s['board']) for s in master}=={tuple(s['board']) for s in groups[3,mode,3]}
    cross=[]
    for mode in (0,1):
        boards=[s['board'] for s in samples if s['revision']==4 and s['mode']==mode]
        cross.append({'mode':mode,'all_current_states':len(boards),'exact_duplicates':len(boards)-len({tuple(b) for b in boards}),
            'raw_position_D4_duplicates':len(boards)-len({d4(b,mode+3) for b in boards}),
            'goal_preserving_reflection_duplicates':len(boards)-len({goal_symmetry(b,mode+3) for b in boards})})
    work=[s['reference_nodes'] for s in samples if s['revision']==4 and s['mode']==1 and s['difficulty']<3]
    result={'format':1,'milestone':'beta.4','decision':'RESOLVED / EXACT DISTANCE BANDS','game_id':27,
        'native_pack_bytes':native_bytes,'new_states':768,'retained_master_states':32,'new_bucket_size':128,
        'distance_bands':{'EASY':[8,14],'NORMAL':[15,20],'HARD':[21,26],'MASTER_3x3':[31,31],'MASTER_4x4_lower_bound':[48,None]},
        'certified_adjacent_shortest_distance_overlap':0,
        'methods':{'3x3':'Generator byte-map BFS versus independent native-test factorial-rank BFS; 181440 states, diameter31',
          '4x4_ENH':'Generator incremental-Manhattan ordered IDA* versus independent fixed-order recomputed-Manhattan C IDA*; every lower threshold exhausted',
          '4x4_MASTER':'Original 30 boards retained; Manhattan>=48 and previously verified legal 400-move construction path. Exact optimum NOT computed.',
          'symmetry':'Raw-position D4 counts are geometric only. Goal-preserving transpose plus tile relabeling is separately measured.'},
        'candidate_work':{k:v for k,v in pack.items() if k.startswith('four_by_four')},
        'reference_IDA_nodes':describe(work),'reference_IDA_total_nodes':sum(work),
        'generator_budget_per_candidate':2000000,'reference_budget_per_board':2000000,
        'compatibility':{'legacy_identity_starts':3072,'baseline_commit':'14aa932','actual_resume_INIT_codec_cases':32,'cycle_groups':8,'cycles_per_group':2,'additional_boundary_start':True,'midcycle_cold_resume':True},
        'production_completions':1156,'groups':summaries,'cross_level_duplicates':cross,'samples_sha256':hashlib.sha256(raw).hexdigest(),
        'hardware':'HARDWARE TEST REQUIRED; no human difficulty or device timing claim'}
    paths=['src/games/strategyquick.h','src/games/strategyquick_quick.c','src/games/strategyquick_strategy.c','src/games/strategyquick_registry.c',
           'assets/strategyquick/sliding_bands.h','assets/strategyquick/sliding_bands.json','assets/strategyquick/master_quick.h','assets/strategyquick/master.json',
           'tests/test_strategyquick_sliding.c','tools/generate/strategyquick_sliding.py','tools/generate/strategyquick_sliding_verify.py']
    result['source_sha256']={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths}
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,indent=2)+'\n')
    print('PASS: 768 exact new starts +32 unchanged MASTER; both sizes have zero adjacent distance-range overlap; native pack matches independent proofs')

if __name__=='__main__':main()
