#!/usr/bin/env python3
"""Independent four-card expression-tree samples from the beta.6 bank."""
import json
from pathlib import Path
import sys
from fractions import Fraction

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tests'))
from test_make_target_reference import brute_force  # noqa: E402


def main():
    bank=json.loads((ROOT/'assets/guesscalc_target_beta6.json').read_text())
    assert len(bank['records'])==4000
    sample=[]
    for difficulty in (0,1):
        for target in (10,24,50,100,200):
            rows=[r for r in bank['records'] if r['difficulty']==difficulty and r['target']==target]
            assert len(rows)==200
            sample.append(rows[73])
    canonical=0
    rational_values=0
    for r in sample:
        values=brute_force(r['cards'])
        rational_values+=len(values)
        actual=values[Fraction(r['target'])]
        expected=r['exact']
        assert actual['canonical_count']==expected['canonical_count'],r['puzzle_id']
        assert list(actual['minimal_tuple'])==expected['minimal_tuple'],r['puzzle_id']
        g=expected['graded']
        graded=(g['fractional_steps'],g['division_steps'],
                g['noncommutative_steps']+g['unary_steps'],
                g['negative_intermediate_steps'],g['tree_depth'],g['max_denominator'])
        assert actual['graded']['minimal_tuple']==graded,r['puzzle_id']
        canonical+=sum(row['canonical_count'] for row in values.values())
    print('PASS beta.6 independent four-card tree samples:',len(sample),
          'rational values',rational_values,'canonical trees',canonical)


if __name__=='__main__':
    main()
