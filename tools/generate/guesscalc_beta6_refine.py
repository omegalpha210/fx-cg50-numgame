#!/usr/bin/env python3
"""Second deterministic selection pass for Make Target MASTER template variety.

Run after guesscalc_beta6_target.py --generate. This does not lower any
minimum-solution threshold: each replacement is re-solved and must satisfy
the same exact beta.5 acceptance predicate. The base generated bank remains
well-defined, and --check validates that each saved alternate came from the
specified construction index before the full --verify re-solve.
"""
import argparse
import collections
import concurrent.futures
from fractions import Fraction
import json
from pathlib import Path
import random
import time

from guesscalc_exact import Solver
from guesscalc_beta5_target import accepted, annotate
from guesscalc_beta6_target import (HEADER, JSON, TARGETS, c_header,
                                   expression_result, first_step, signature,
                                   validate)

REPLACE_PER_TARGET = 40
MAX_INDEX = 12000
SEED = 0xB6A17E29


def alternative(index, target):
    """Six leaves in a different rational topology: a-b/(c-d/(e/f))."""
    rng = random.Random(SEED ^ target*0x9E3779B9 ^ index*0x85EBCA6B)
    e = rng.randint(7,100)
    f = rng.randint(2,35)
    d = rng.randint(2,80)
    c = rng.randint(2,45)
    if e % f == 0 or (d*f) % e == 0:
        return [], ''
    denominator = Fraction(c) - Fraction(d*f,e)
    if denominator <= 0:
        return [], ''
    q = denominator.denominator * rng.randint(1,8)
    b = q * denominator
    a = target+q
    if b.denominator != 1 or not (1 <= b <= 999 and 1 <= a <= 999):
        return [], ''
    b = int(b)
    cards = [a,b,c,d,e,f]
    rng.shuffle(cards)
    witness = f'{a}-{b}/({c}-{d}/({e}/{f}))'
    assert expression_result(witness) == (Fraction(target), sorted(cards))
    return cards, witness


def replace_target(args):
    target, rows, existing = args
    started = time.monotonic()
    original_top, original_count = collections.Counter(r['easiest_template'] for r in rows).most_common(1)[0]
    slots = [i for i,r in enumerate(rows) if r['easiest_template']==original_top][-REPLACE_PER_TARGET:]
    assert len(slots)==REPLACE_PER_TARGET
    seen = set(existing)
    replacements=[]
    rejected = invalid = duplicate = same_template = 0
    with Solver() as solver:
        for index in range(1,MAX_INDEX+1):
            cards,witness=alternative(index,target)
            if not cards:
                invalid+=1
                continue
            key=(target,tuple(sorted(cards)))
            if key in seen:
                duplicate+=1
                continue
            seen.add(key)
            exact0=solver.solve(cards,target).get(Fraction(target))
            if exact0 is None or not accepted(3,exact0):
                rejected+=1
                continue
            exact=annotate(exact0)
            answer=exact['graded']['expression']
            template=signature(answer)
            if template==original_top:
                same_template+=1
                continue
            assert len(answer)<40 and expression_result(answer)==(Fraction(target),sorted(cards))
            hint=list(first_step(cards,witness))
            old=rows[slots[len(replacements)]]
            replacements.append(dict(puzzle_id=old['puzzle_id'],difficulty=3,target=target,
                                     candidate_index=index,candidate_family='bilateral-fractions-v1',
                                     cards=cards,construction_witness=witness,
                                     answer=answer,hint=hint,easiest_template=template,exact=exact))
            if len(replacements)==REPLACE_PER_TARGET:
                break
            if len(replacements)%10==0:
                print('MASTER alternative',target,len(replacements),'index',index,
                      'elapsed',round(time.monotonic()-started,1),flush=True)
    if len(replacements)!=REPLACE_PER_TARGET:
        raise RuntimeError(('MASTER alternative shortage',target,len(replacements),index))
    updated=rows[:]
    for slot,record in zip(slots,replacements):
        updated[slot]=record
    new_top,new_count=collections.Counter(r['easiest_template'] for r in updated).most_common(1)[0]
    print('MASTER refinement',target,'before',original_count,'after',new_count,
          'index',index,'elapsed',round(time.monotonic()-started,1),flush=True)
    return target,updated,dict(target=target,replaced=REPLACE_PER_TARGET,
                               before_top_template=original_top,before_top_count=original_count,
                               after_top_template=new_top,after_top_count=new_count,
                               last_index=index,invalid=invalid,duplicate=duplicate,
                               rejected_by_exact_grade=rejected,same_original_template=same_template,
                               seconds=round(time.monotonic()-started,3))


def check_refined(data):
    stats=data.get('master_refinement')
    assert stats and stats['construction']=='bilateral-fractions-v1'
    assert stats['replacements_per_target']==REPLACE_PER_TARGET
    for r in data['records']:
        if r.get('candidate_family')=='bilateral-fractions-v1':
            cards,witness=alternative(r['candidate_index'],r['target'])
            assert cards==r['cards'] and witness==r['construction_witness']
    assert sum(r.get('candidate_family')=='bilateral-fractions-v1' for r in data['records'])==len(TARGETS)*REPLACE_PER_TARGET
    for s in stats['cells']:
        rows=[r for r in data['records'] if r['difficulty']==3 and r['target']==s['target']]
        assert len(rows)==200
        top=collections.Counter(r['easiest_template'] for r in rows).most_common(1)[0][1]
        assert top==s['after_top_count'] and top<s['before_top_count']


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    mode=parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--write',action='store_true')
    mode.add_argument('--check',action='store_true')
    parser.add_argument('--jobs',type=int,default=5)
    args=parser.parse_args()
    data=json.loads(JSON.read_text())
    validate(data)
    if args.write:
        assert 'master_refinement' not in data, 'Refine only the pristine base bank'
        existing={(r['target'],tuple(sorted(r['cards']))) for r in data['records']}
        tasks=[(target,[r for r in data['records'] if r['difficulty']==3 and r['target']==target],existing)
               for target in TARGETS]
        with concurrent.futures.ProcessPoolExecutor(max_workers=args.jobs) as pool:
            results=list(pool.map(replace_target,tasks))
        by_id={r['puzzle_id']:r for _,rows,_ in results for r in rows}
        data['records']=[by_id.get(r['puzzle_id'],r) for r in data['records']]
        data['master_refinement']=dict(construction='bilateral-fractions-v1',
                                       replacements_per_target=REPLACE_PER_TARGET,
                                       cells=[s for _,_,s in results])
        validate(data)
        check_refined(data)
        JSON.write_text(json.dumps(data,indent=2)+'\n')
        HEADER.write_text(c_header(data))
    else:
        check_refined(data)
        assert HEADER.read_text()==c_header(data)
    print('PASS beta.6 MASTER refinement',len(TARGETS)*REPLACE_PER_TARGET,'records',flush=True)


if __name__=='__main__':
    main()
