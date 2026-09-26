#!/usr/bin/env python3
"""Exact DP interface regressions; independent full-tree oracle is separate."""
from fractions import Fraction
from pathlib import Path
import json
import sys
import unittest
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools/generate'))
from guesscalc_exact import Solver, solve, target_deck_v4, target_deck_v5
from guesscalc_beta4_target import accepted, magnitude


class ExactTests(unittest.TestCase):
    def test_fraction_regression_and_unary_cost(self):
        result = solve([3,3,8,8],24)[Fraction(24)]
        self.assertEqual(result['canonical_count'],96)
        self.assertEqual(result['minimal_tuple'],[2,2,2,3,3])
        self.assertEqual(result['graded']['score'],148227)
        self.assertEqual(result['graded']['subtraction_steps'],1)
        self.assertEqual(result['graded']['unary_steps'],0)
        no_unary = solve([3,3,8,8],24,unary=False)[Fraction(24)]
        self.assertEqual(no_unary['canonical_count'],1)
        self.assertEqual(no_unary['minimal_tuple'],[2,2,3,3,3])

    def test_target_filter_preserves_counts_and_both_minima(self):
        for cards in ([1,1,2,3],[2,3,5,7],[3,3,8,8]):
            full = solve(cards)
            with Solver() as solver:
                for value in [v for v in full if v.denominator==1 and -5<=v<=15]:
                    self.assertEqual(solver.solve(cards,int(value))[value],full[value])

    def test_duplicate_occurrence_order(self):
        expected = solve([3,3,8,8],24)
        self.assertEqual(solve([8,3,8,3],24),expected)
        self.assertEqual(solve([8,8,3,3],24),expected)
        self.assertNotEqual(solve([3,8,8],24),expected)

    def test_native_bounds_and_zero_divisors(self):
        values = solve([1000000,1000000,1000000])
        self.assertTrue(all(abs(v.numerator)<=10**9 and v.denominator<=10**9 for v in values))
        self.assertIn(Fraction(1000000),values)
        self.assertNotIn(Fraction(10**18),values)
        self.assertIn(Fraction(0),solve([1,1]))

    def test_countdown_integer_mode(self):
        with Solver(integer=True,all_values=True) as solver:
            values = solver.solve([1,1])
            self.assertEqual(set(values),{Fraction(1),Fraction(2)})
            self.assertEqual(values[Fraction(1)]['canonical_count'],2) # 1*1,1/1
            values = solver.solve([2,3])
            self.assertEqual(set(values),{Fraction(1),Fraction(5),Fraction(6)})

    def test_all_bank_records_keep_their_acceptance(self):
        for revision, constructor in ((4,target_deck_v4),(5,target_deck_v5)):
            records = json.loads((ROOT/f'assets/guesscalc_target_beta{revision}.json').read_text())['records']
            self.assertEqual(len(records),8000)
            seen = set()
            for r in records:
                self.assertTrue(accepted(r['difficulty'],r['exact']))
                cards,witness,_ = constructor(r['candidate_index'],r['difficulty'],r['target'])
                self.assertEqual((cards,witness),(r['cards'],r['generator_witness']))
                self.assertEqual(magnitude(witness)[1],r['target'])
                if revision == 5:self.assertTrue(all(1<=card<=999 for card in cards))
                key = (r['target'],tuple(sorted(cards)));self.assertNotIn(key,seen);seen.add(key)


if __name__=='__main__':unittest.main()
