#!/usr/bin/env python3
"""Update only Make Target's four integrated difficulty rows for beta.5."""
import argparse
import copy
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'assets/guesscalc_difficulty_rows_beta5.json'


def create():
    rows=copy.deepcopy(json.loads((ROOT/'assets/guesscalc_difficulty_rows_beta4.json').read_text()))
    audit=json.loads((ROOT/'assets/guesscalc_target_beta5_audit.json').read_text())
    for row in rows:
        if row['game_id']!=6:continue
        level=('EASY','NORMAL','HARD','MASTER').index(row['difficulty'])
        old,new=audit['before'][level],audit['after'][level]
        beta4=json.loads(row['observed_distribution'])['after']
        before=copy.deepcopy(beta4)
        before.update(card_value=old['card_value'],decimal_digit_distribution=old['decimal_digit_distribution'])
        after=copy.deepcopy(beta4)
        for key in ('complexity_score','canonical_solution_count'):
            after[key]=new[key]
        after.update(card_value=new['card_value'],decimal_digit_distribution=new['decimal_digit_distribution'],
                     fraction_required=new['fractional_required'],division_required=new['division_required'],
                     count=new['problem_count'],max_card=999,revision=5)
        row['observed_distribution']=json.dumps(dict(before=before,after=after),sort_keys=True,separators=(',',':'))
        row['generator_params']=json.dumps(after,sort_keys=True,separators=(',',':'))
        row['fix']='Revision5 1..999-card decks, two per target/level; beta.4 saves retained unchanged'
        row['finding']='GC-D05 RESOLVED / EXHAUSTIVE GRADING; beta.5 CARD BOUND'
        row['rating_metric']='Exact rational minimum tuple and card magnitude; human rating NOT MEASURED'
    assert len(rows)==48 and sum(row['game_id']==6 for row in rows)==4
    return json.dumps(rows,indent=2)+'\n'


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    group=parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--write',action='store_true');group.add_argument('--check',action='store_true')
    args=parser.parse_args();payload=create()
    if args.check:
        assert OUT.read_text()==payload
        print('PASS: 48 GUESS/CALC rows, only Make Target updated')
    else:
        OUT.write_text(payload)
        print('wrote',OUT.relative_to(ROOT))


if __name__=='__main__':main()
