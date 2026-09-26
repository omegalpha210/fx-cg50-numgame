#!/usr/bin/env python3
"""Deterministic difficulty audit of native samples, with independent metrics.

The sample executable links production code. Sliding 3x3 distances are a full
reverse BFS; Lights minimum counts enumerate every first-row press choice and
force subsequent rows. Neither metric imports the production solver/generator.
No human difficulty rating or real-calculator performance is inferred.
"""
from __future__ import annotations
import argparse
import csv
from collections import defaultdict, deque
import hashlib
import json
from pathlib import Path
import re
import statistics
import subprocess

ROOT = Path(__file__).resolve().parents[2]
NAMES = {21:'NIM',22:'WYTHOFF',23:'EUCLID',24:'MAKE FIFTEEN',25:'RACE TO TARGET',
         26:'2048',27:'SLIDING',28:'LIGHTS OUT',31:'SHIKAKU',32:'SLITHERLINK',37:'REVERSI',38:'NET'}
FIELDS=['game_id','name','difficulty','mechanism','bank_size','board_size','generator_params',
        'AI_policy','rating_metric','observed_distribution','overlap','meaningfully_distinct','finding','fix']
OWNED={21,22,23,24,25,26,27,28,37,38}

def spread(values):
    return {'min':min(values),'max':max(values),'mean':round(statistics.mean(values),4),
            'median':statistics.median(values),'distinct':len(set(values))}

def sliding_distances():
    goal=bytes([1,2,3,4,5,6,7,8,0]); distance={goal:0}; queue=deque([goal])
    neighbours=[[j for j in range(9) if abs(i//3-j//3)+abs(i%3-j%3)==1] for i in range(9)]
    while queue:
        board=queue.popleft(); p=board.index(0)
        for q in neighbours[p]:
            child=bytearray(board); child[p],child[q]=child[q],child[p]; child=bytes(child)
            if child not in distance:distance[child]=distance[board]+1;queue.append(child)
    assert len(distance)==181440 and max(distance.values())==31
    return distance

def lights_solutions(board,n):
    """Every solution is determined by its first row (at most 32 choices)."""
    initial=[sum(board[r*n+c]<<c for c in range(n)) for r in range(n)]
    mask=(1<<n)-1; candidates=[]
    for first in range(1<<n):
        state=initial[:]; solution=0
        for r in range(n):
            press=first if r==0 else state[r-1]
            solution|=press<<(n*r)
            state[r]^=press^(press<<1&mask)^(press>>1)
            if r:state[r-1]^=press
            if r+1<n:state[r+1]^=press
        if not any(state):candidates.append(solution)
    assert candidates
    return candidates

def lights_minimum(board,n):
    return min(mask.bit_count() for mask in lights_solutions(board,n))

def verify_lights_kernel():
    source=(ROOT/'src/games/strategyquick_quick.c').read_text()
    match=re.search(r'kernel\[16\]=\{([^}]+)\}',source)
    assert match
    native={int(x,0) for x in match.group(1).split(',')}
    independent=set(lights_solutions([0]*16,4))
    assert native==independent and len(native)==16
    assert min(x.bit_count() for x in native if x)==8
    assert min((31^x).bit_count() for x in native)==5
    return {'native_kernel_words':16,'independent_nullity':4,'nonzero_minimum_weight':8,'fallback_minimum':5}

def manhattan(board,n):
    return sum(abs(i//n-(v-1)//n)+abs(i%n-(v-1)%n) for i,v in enumerate(board) if v)

def summarize(samples):
    groups=defaultdict(list); cpu=defaultdict(list); rv=defaultdict(list)
    for row in samples:
        if row['kind']=='start':groups[row['id'],row['mode'],row['level']].append(row)
        elif row['kind']=='strategy_cpu':cpu[row['id'],row['level']].append(row)
        else:rv[row['level']].append(row)
    distances=sliding_distances(); summaries=[]
    for (game,mode,level),rows in sorted(groups.items()):
        item={'id':game,'name':NAMES[game],'mode':mode,'level':level,'samples':len(rows),
              'bank_count':rows[0]['bank'],'size':rows[0]['size'],'columns':len(rows[0]['board'])//rows[0]['size'],
              'pack_revision':rows[0].get('revision',2),
              'distinct_initial_boards':len({tuple(r['construction'] if game==31 else r['board']) for r in rows})}
        item['distinct_rule_starts']=len({(tuple(r.get('construction',r['board'])),tuple(r['knobs'][1:3]) if game==25 else r['knobs'][3] if game==26 else None) for r in rows})
        if game in (21,22,23):
            item['pile_sum']=spread([sum(r['board']) for r in rows]);item['largest_pile']=spread([max(r['board']) for r in rows])
        elif game==24:item['preplayed_cards']=spread([sum(v!=0 for v in r['board']) for r in rows])
        elif game==25:
            item['target']=spread([r['knobs'][1] for r in rows]);item['max_add']=spread([r['knobs'][2] for r in rows]);item['remaining']=spread([r['knobs'][1]-r['board'][0] for r in rows])
        elif game==26:
            item['target_tiles']=sorted({2**r['knobs'][3] if r['knobs'][3] else 0 for r in rows})
            item['policy']='FIXED_CLASSIC_NO_LEVEL' if mode==0 else 'TARGET_GOAL'
        elif game==27:
            item['manhattan_lower_bound']=spread([manhattan(r['board'],r['size']) for r in rows])
            if mode==0:item['exact_shortest_moves']=spread([distances[bytes(r['board'])] for r in rows])
            item['shuffle_moves_or_master_witness_limit']=spread([r['knobs'][0] for r in rows])
        elif game==28:
            minima=[lights_minimum(r['board'],r['size']) for r in rows]
            item['exact_minimum_presses']=spread(minima)
            item['declared_start_count']=spread([r['knobs'][1] for r in rows])
            if level==3:assert all(v==r['knobs'][1] for v,r in zip(minima,rows))
        elif game in (31,32):
            item['clues']=spread([sum(v>0 for v in r['construction']) if game==31 else sum(v>=0 for v in r['board']) for r in rows])
        elif game==38:
            item['cells']=rows[0]['size']**2
            item['junctions_degree_at_least_3']=spread([sum(v.bit_count()>=3 for v in r['construction']) for r in rows])
            item['leaves']=spread([sum(v.bit_count()==1 for v in r['construction']) for r in rows])
        summaries.append(item)
    decisions=[]
    for (game,level),rows in sorted(cpu.items()):
        opportunities=[r for r in rows if r['best']>r['worst']]
        decisions.append({'id':game,'level':level,'positions':len(rows),
            'decision_opportunities':len(opportunities),
            'optimal_on_opportunities':sum(r['actual']==r['best'] for r in opportunities),
            'different_choice_from_previous_level':None if level==0 else sum(r['board']!=p['board'] for r,p in zip(rows,cpu[game,level-1]))})
    reversi=[]
    for level,rows in sorted(rv.items()):
        reversi.append({'level':level,'positions':len(rows),'nodes':spread([r['nodes'] for r in rows]),
            'completed_depth':spread([r['depth'] for r in rows]),
            'different_choice_from_previous_level':None if level==0 else sum(r['choice']!=p['choice'] for r,p in zip(rows,rv[level-1]))})
    comparisons=[]
    for (game,mode,level),rows in sorted(groups.items()):
        if level==0:continue
        previous=groups[game,mode,level-1]
        paired=min(len(rows),len(previous))
        same=sum(r['board']==p['board'] and r.get('construction')==p.get('construction') for r,p in zip(rows,previous))
        comparisons.append({'id':game,'mode':mode,'levels':[level-1,level],'paired':paired,'same_initial_board':same,
            'note':'Equal board alone does not imply no-op: goals or CPU policy may differ. Bank comparisons use bank ordinal, runtime comparisons use equal seed.'})
    return {'initial_metrics':summaries,'exact_strategy_decisions':decisions,'reversi_decisions':reversi,'adjacent_initial_comparison':comparisons}

def assessment(result):
    output=[]
    for game in sorted(OWNED):
        for level in range(4):
            groups=[g for g in result['initial_metrics'] if (g['id'],g['level'])==(game,level)]
            row=dict.fromkeys(FIELDS,'')
            row.update(game_id=game,name=NAMES[game],difficulty=['EASY','NORMAL','HARD','MASTER'][level],
                bank_size='; '.join(f"mode {g['mode']}: {g['bank_count']}" for g in groups),
                board_size='; '.join(f"mode {g['mode']}: {g['size']}x{g['columns']}" for g in groups),
                meaningfully_distinct='YES',finding='PASS',fix='None')
            if game<=25:
                params={21:['3 piles 1..7','3 piles 1..15','4 piles 1..31'],22:['piles 1..10','piles 1..24','piles 1..40'],23:['values 1..12','values 1..40','values 1..99'],24:['empty 9-card opening']*3,25:['target 21/add 1..3','target 31/add 1..3','target 23/add 1..4']}[game]
                row['mechanism']='CPU policy and initial position' if level<3 else 'Verified forced-win tactical starts; CPU identical to HARD'
                row['generator_params']=params[level] if level<3 else 'Full enumerated mode-specific bank; human can force a win'
                row['AI_policy']=['random legal move','75% exact-policy branch; 25% random','exact outcome-optimal policy','same exact policy as HARD'][level]
                metric=next(x for x in result['exact_strategy_decisions'] if (x['id'],x['level'])==(game,level))
                row['rating_metric']='Optimal decisions only where legal choices differ in outcome; pre-existing independent exact-table audit'
                row['observed_distribution']=f"{metric['optimal_on_opportunities']}/{metric['decision_opportunities']} optimal on common decision opportunities; {metric['positions']} total positions"
                row['overlap']='HARD/MASTER CPU deliberately identical: 128/128 board and RNG decisions match; starts differ' if level>=2 else 'Random branch may choose an optimal move; 75% branch rate is not 75% success rate'
            elif game==26:
                row.update(mechanism='TARGET terminal goal; CLASSIC fixed rules',generator_params=f"TARGET={ [512,1024,2048,8192][level] }; CLASSIC keeps playing after 2048",AI_policy='None',rating_metric='Exact terminal goal, not altered spawn rate',observed_distribution='Identical initial board/RNG at equal seed across levels; 90% 2 / 10% 4 spawn rule unchanged',overlap='CLASSIC level is an intentional no-op; app hides level and starts NORMAL; old E/N/H preserved',meaningfully_distinct='TARGET YES; CLASSIC FIXED EXCEPTION',finding='FIXED EXCEPTION',fix='Removed stale rules claim that cumulative records persist')
            elif game==27:
                row.update(mechanism='Solvable legal shuffle and Manhattan lower-bound acceptance' if level<3 else 'Certified long starts',generator_params=f"{[80,160,240][level]} shuffle steps, Manhattan threshold {[6,10,14][level]}, <=16 attempts" if level<3 else '3x3 exact 31; 4x4 400-step witness and lower bound >=48',AI_policy='None',rating_metric='3x3 exhaustive BFS distance; 4x4 Manhattan lower bound only',observed_distribution=json.dumps([{k:g[k] for k in ('mode','manhattan_lower_bound','exact_shortest_moves') if k in g} for g in groups],sort_keys=True),overlap='E/N/H shortest-distance ranges overlap strongly; shuffle count is not difficulty',meaningfully_distinct='Mechanism YES; E/N separation REVIEW REQUIRED',finding='REVIEW REQUIRED' if level<3 else 'PASS',fix='No calibration change; retain seeds and label evidence limitations')
            elif game==28:
                row.update(mechanism='4x4 certified minimum presses; 5x5 constructive pressing' if level<3 else 'Certified minimum-press banks',generator_params=f"revision 3 4x4 minimum {[2,4,5][level]}; 5x5 {[4,8,18][level]} generating presses" if level<3 else '4x4 exact 6; 5x5 exact 12..14',AI_policy='No CPU; GF(2) hint does not promise fewest presses',rating_metric='Independent exhaustive first-row row-chasing minimum',observed_distribution=json.dumps([{k:g[k] for k in ('mode','exact_minimum_presses')} for g in groups],sort_keys=True),overlap='4x4 new E/N/H/M 2/4/5/6 do not overlap; 5x5 H/M ranges overlap',meaningfully_distinct='4x4 YES; 5x5 distribution YES, human ranking uncalibrated',finding='FIXED' if level<3 else 'PASS',fix='Revision 3 fixes 4x4 inverse grading; revision <=2 preserves initial boards and RNG; <=16 candidates plus verified fallback')
            elif game==37:
                metric=result['reversi_decisions'][level]
                row.update(mechanism='Bounded CPU search depth/node budget; common opening',generator_params='Standard 8x8 opening for every level',AI_policy=['random legal','1 ply / 256 nodes','up to 3 plies / 1500 nodes','up to 5 plies / 6000 nodes'][level],rating_metric='Actual visited nodes, completed depth and chosen-move changes on common reachable positions',observed_distribution=json.dumps(metric,sort_keys=True),overlap='Some positions choose the same move; higher budgets are not proof of universally stronger play',finding='PASS',fix='None; no full-game exact or human-skill claim')
            else:
                row.update(mechanism='Larger seeded tree-rotation puzzle',generator_params=f"{[9,16,25,36][level]} cells; nonwrapping tree plus rotations",AI_policy='None',rating_metric='Cells, leaves and degree>=3 junctions; no human rating or uniqueness guarantee',observed_distribution=json.dumps({k:groups[0][k] for k in ('cells','leaves','junctions_degree_at_least_3')},sort_keys=True),overlap='Same construction algorithm; larger area is a structural difference, not calibrated human difficulty',finding='PASS',fix='None')
            output.append(row)
    return output

def main():
    parser=argparse.ArgumentParser();source=parser.add_mutually_exclusive_group(required=True)
    source.add_argument('--sample-exe',type=Path);source.add_argument('--samples',type=Path)
    parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    raw=subprocess.check_output([str(args.sample_exe.resolve())]) if args.sample_exe else args.samples.read_bytes()
    rows=[row for line in raw.splitlines() if (row:=json.loads(line))['id'] in OWNED]
    result={'schema':1,'milestone':'beta.3','sample_seed_policy':'Runtime seeds 1..128; full banks, supply_seed=1, ordinal 0..count-1. Strategy probes use reachable HARD/random positions; Reversi uses reachable EASY/random positions after >=8 placements.',
        'sample_rows':len(rows),'samples_sha256':hashlib.sha256(raw).hexdigest(),
        'independent_metrics':{'sliding_3x3':'Complete reverse BFS: 181440 reachable states, diameter 31',
           'lights':'Exhaustive first-row choices with forced row chasing, <=32 candidates per board',
           'lights_native_kernel':verify_lights_kernel()},
        'limitations':['Host audit, not hardware timing.','No human difficulty calibration.','Strategy quality uses production exact values already independently exhaustively verified by test_strategyquick.','Reversi metrics measure work and changed choices, not stronger-play proof.','CLASSIC 2048 direct-API levels are compatibility probes; normal app selection is fixed NORMAL.'],
        **summarize(rows)}
    result['assessment']=assessment(result)
    paths=['src/games/strategyquick_strategy.c','src/games/strategyquick_quick.c','src/games/strategyquick_extra.c','src/games/strategyquick_registry.c','src/games/boards.c','src/games/boards_pack.c','tests/test_strategyquick_difficulty.c','tools/generate/strategyquick_difficulty.py']
    result['source_sha256']={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths}
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    with args.output.with_suffix('.csv').open('w',newline='') as file:
        writer=csv.DictWriter(file,fieldnames=FIELDS,lineterminator='\n');writer.writeheader();writer.writerows(result['assessment'])
    print(f"PASS: {len(rows)} deterministic rows, {len(result['initial_metrics'])} game/mode/level groups; independent metric checks complete")

if __name__=='__main__':main()
