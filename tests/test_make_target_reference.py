#!/usr/bin/env python3
"""Independent, bounded Make Target expression-tree reference (2..4 cards).

No subset/value DP, production solver or generator is imported. Enumerates
ordered leaves, every binary split/operator and optional unary minus at each
node. Actual parser reduced-value bounds apply; there is no heuristic cutoff.

Canonical equivalence: +/* child exchange, equal-valued occurrence renaming,
and removal of consecutive double negation only. Association is retained;
negated zero is a separate canonical tree. Leaves have depth 0. Unary minus
counts as an operation and contributes fractional/negative result steps; it
is not a binary subtraction. Magnitude includes leaves and operation results.

brute_force(cards) returns Fraction -> canonical_count, minimal_tuple,
easiest_expression and raw features. Compare values/counts/tuples to a solver;
the chosen expression is only a deterministic tie-break representative.
"""
from __future__ import annotations

import argparse
import ast
import collections
import csv
from dataclasses import dataclass
from fractions import Fraction
from functools import lru_cache
import itertools
import hashlib
import json
from pathlib import Path
import random
import time

PARSER_BOUND = 10**9
CORE_FEATURES = ('fractional_steps', 'division_steps', 'noncommutative_steps',
                 'tree_depth', 'max_denominator')


@dataclass(frozen=True)
class Tree:
    value: Fraction
    ast: tuple
    fractional: int = 0
    division: int = 0
    subtraction: int = 0
    depth: int = 0
    denominator: int = 1
    negative: int = 0
    magnitude: Fraction = Fraction(0)
    unary: int = 0

    @property
    def core(self):
        return (self.fractional, self.division, self.division+self.subtraction,
                self.depth, self.denominator)

    @property
    def grade(self):
        return (self.fractional, self.division,
                self.division+self.subtraction+self.unary,
                self.negative, self.depth, self.denominator)


def render(ast):
    op = ast[0]
    if op == 'leaf':
        return str(ast[1])
    if op == 'neg':
        return '-'+render(ast[1])
    return '('+render(ast[1])+op+render(ast[2])+')'


def leaves(ast):
    if ast[0] == 'leaf':
        return [ast[1]]
    if ast[0] == 'neg':
        return leaves(ast[1])
    return leaves(ast[1])+leaves(ast[2])


def negate(tree):
    assert tree.ast[0] != 'neg'  # Double negation is the chosen quotient.
    value = -tree.value
    return Tree(value, ('neg', tree.ast), tree.fractional+(value.denominator > 1),
                tree.division, tree.subtraction, tree.depth+1, tree.denominator,
                tree.negative+(value < 0), tree.magnitude, tree.unary+1)


def combine(left, right, op, bound):
    a, b = left.value, right.value
    if op == '+':
        value = a+b
    elif op == '-':
        value = a-b
    elif op == '*':
        value = a*b
    else:
        if not b:
            return None
        value = a/b
    # Fraction has already reduced the numerator/denominator, like normalize().
    if abs(value.numerator) > bound or value.denominator > bound:
        return None
    la, ra = left.ast, right.ast
    if op in '+*' and ra < la:
        la, ra = ra, la
    return Tree(value, (op, la, ra),
                left.fractional+right.fractional+(value.denominator > 1),
                left.division+right.division+(op == '/'),
                left.subtraction+right.subtraction+(op == '-'),
                1+max(left.depth, right.depth),
                max(left.denominator, right.denominator, value.denominator),
                left.negative+right.negative+(value < 0),
                max(left.magnitude, right.magnitude, abs(value)),
                left.unary+right.unary)


def brute_force(cards, *, unary=True, bound=PARSER_BOUND, operators='+-*/'):
    """Enumerate complete trees; never prune a tree for a better equal value.

    Duplicate card values are legal separate leaves. Permutations use exactly
    the supplied multiset; AST canonicalization discards occurrence names only
    after legality has been established. The optional operators argument is
    for narrow sanity tests, not the default Make Target rules.
    """
    cards = tuple(cards)
    if not 2 <= len(cards) <= 4 or any(type(v) is not int or not 1 <= v <= 10**6 for v in cards):
        raise ValueError('Reference is bounded to 2..4 positive integer cards <= 1000000')
    if not operators or any(op not in '+-*/' for op in operators) or len(set(operators)) != len(operators):
        raise ValueError('Distinct legal binary operators required')
    if bound != PARSER_BOUND:
        raise ValueError('Only the actual parser bound is supported')

    @lru_cache(None)
    def trees(sequence):
        if len(sequence) == 1:
            leaf = Tree(Fraction(sequence[0]), ('leaf', sequence[0]), magnitude=Fraction(sequence[0]))
            return (leaf, negate(leaf)) if unary else (leaf,)
        result = []
        for cut in range(1, len(sequence)):
            for left in trees(sequence[:cut]):
                for right in trees(sequence[cut:]):
                    for op in operators:
                        tree = combine(left, right, op, bound)
                        if tree is not None:
                            result.append(tree)
                            if unary:
                                result.append(negate(tree))
        return tuple(result)

    # Cache smaller ordered expression lists, but stream each four-leaf list
    # through canonical aggregation and release it after that permutation.
    seen = set()
    result = {}
    occurrence_multiset = collections.Counter(cards)
    for permutation in sorted(set(itertools.permutations(cards))):
        for tree in trees(permutation):
            if tree.ast in seen:
                continue
            seen.add(tree.ast)
            assert collections.Counter(leaves(tree.ast)) == occurrence_multiset
            expression = render(tree.ast)
            # All normalized <=4-card trees fit the native text/operation/depth
            # limits. Parentheses generated here mirror the native grammar.
            assert len(expression) <= 96 and len(cards)-1+tree.unary <= 31
            record = result.setdefault(tree.value, {'canonical_count': 0})
            record['canonical_count'] += 1
            if 'minimal_tuple' not in record or (tree.core, expression) < (record['minimal_tuple'], record['easiest_expression']):
                record.update(minimal_tuple=tree.core, easiest_expression=expression,
                              card_count=len(cards), fractional_steps=tree.fractional,
                              division_steps=tree.division, subtraction_steps=tree.subtraction,
                              noncommutative_steps=tree.division+tree.subtraction,
                              tree_depth=tree.depth, max_denominator=tree.denominator,
                              negative_intermediate_steps=tree.negative,
                              unary_negation_steps=tree.unary,
                              max_intermediate_magnitude=str(tree.magnitude))
            graded = record.get('graded')
            if graded is None or (tree.grade, expression) < (graded['minimal_tuple'], graded['expression']):
                # A separate complete minimization, not the score of the raw
                # tuple winner. Unary negation must not become a free '-' hack.
                record['graded'] = dict(minimal_tuple=tree.grade,
                    score=65536*tree.fractional+8192*tree.division+
                          256*(tree.division+tree.subtraction+tree.unary)+16*tree.negative+tree.depth,
                    fractional_steps=tree.fractional, division_steps=tree.division,
                    subtraction_steps=tree.subtraction,
                    noncommutative_steps=tree.division+tree.subtraction,
                    unary_steps=tree.unary, negative_intermediate_steps=tree.negative,
                    tree_depth=tree.depth, max_denominator=tree.denominator,
                    max_intermediate_magnitude=str(tree.magnitude), expression=expression)
        trees.cache_clear()
    return result


def fixed_cases():
    """Stable four-card cross-check domain; callers may sample or run all."""
    rng = random.Random(0xBE7A4)
    return [(3, 3, 8, 8), (1, 1, 1, 1), (1, 2, 3, 4), (1, 1, 2, 2)]+[
        tuple(sorted(rng.randint(1, 9) for _ in range(4))) for _ in range(4)]


def check_candidate(candidate):
    cards = candidate['cards']
    values = brute_force(cards)
    target = Fraction(str(candidate.get('target', 24)))
    answer = values.get(target)
    expected = candidate.get('expected')
    if expected is not None:
        assert answer is not None, 'Expected target is unreachable'
        for key in ('canonical_count', 'minimal_tuple'):
            if key in expected:
                actual = list(answer[key]) if key == 'minimal_tuple' else answer[key]
                assert actual == expected[key], f'{key}: independent reference mismatch'
    return dict(cards=cards, target=str(target), answer=answer,
                reachable_targets=[v.numerator for v in sorted(values) if v.denominator == 1 and 1 <= v <= 1000],
                reachable_rational_values=len(values))


def self_test():
    plus = brute_force([1, 1, 1, 1], unary=False, operators='+')
    assert plus[Fraction(4)]['canonical_count'] == 2  # Association retained.
    answer = brute_force([3, 3, 8, 8])[Fraction(24)]
    assert answer['minimal_tuple'] == (2, 2, 2, 3, 3)
    assert answer['canonical_count'] == 96
    assert answer['graded']['minimal_tuple'] == (2, 2, 3, 0, 3, 3)
    assert answer['fractional_steps'] > 0
    two = brute_force([3, 3])
    assert Fraction(0) in two and Fraction(-6) in two and Fraction(1) in two
    assert two[Fraction(0)]['canonical_count'] > 1  # -0 is retained.
    a = Tree(Fraction(10**6), ('leaf', 10**6), magnitude=Fraction(10**6))
    assert combine(a, a, '*', PARSER_BOUND) is None
    assert combine(a, Tree(Fraction(0), ('leaf', 0)), '/', PARSER_BOUND) is None
    print(json.dumps(dict(status='PASS', known_3388_24=answer, fixed_cases=fixed_cases()), indent=2))


def fixed_report():
    """Reproducible complete-domain digest for external DP cross-checks."""
    report = []
    for cards in fixed_cases():
        values = brute_force(cards)
        rows = [(str(v), p['canonical_count'], p['minimal_tuple'], p['graded']['minimal_tuple'])
                for v, p in sorted(values.items())]
        report.append(dict(cards=cards, rational_values=len(values),
                           canonical_trees=sum(p['canonical_count'] for p in values.values()),
                           count_and_minima_sha256=hashlib.sha256(json.dumps(rows, separators=(',', ':')).encode()).hexdigest()))
    return report


def expression_metrics(text):
    """Evaluate a supplied witness independently of the production parser.

    Only the four binary operators, unary minus and positive integer card
    literals are accepted. This evaluator does not call the tree enumerator's
    combine/negate helpers, so reported witness metrics are checked separately.
    """
    assert len(text) <= 96, 'Expression exceeds native text limit'
    parentheses = maximum_parentheses = 0
    for char in text:
        if char == '(':
            parentheses += 1
            maximum_parentheses = max(maximum_parentheses, parentheses)
        elif char == ')':
            parentheses -= 1
            assert parentheses >= 0, 'Unbalanced expression'
    assert parentheses == 0 and maximum_parentheses <= 12

    def visit(node):
        if isinstance(node, ast.Constant):
            assert type(node.value) is int and 1 <= node.value <= 10**6
            return dict(value=Fraction(node.value), cards=[node.value],
                        fractional_steps=0, division_steps=0,
                        subtraction_steps=0, unary_steps=0,
                        negative_intermediate_steps=0, tree_depth=0,
                        max_denominator=1,
                        max_intermediate_magnitude=Fraction(node.value))
        if isinstance(node, ast.UnaryOp):
            assert isinstance(node.op, ast.USub), 'Unsupported unary operator'
            result = visit(node.operand)
            result['value'] = -result['value']
            result['fractional_steps'] += result['value'].denominator > 1
            result['negative_intermediate_steps'] += result['value'] < 0
            result['unary_steps'] += 1
            result['tree_depth'] += 1
            return result
        assert isinstance(node, ast.BinOp), 'Unsupported expression node'
        left, right = visit(node.left), visit(node.right)
        a, b = left['value'], right['value']
        if isinstance(node.op, ast.Add):
            value = a+b
        elif isinstance(node.op, ast.Sub):
            value = a-b
        elif isinstance(node.op, ast.Mult):
            value = a*b
        else:
            assert isinstance(node.op, ast.Div) and b, 'Invalid division'
            value = a/b
        assert abs(value.numerator) <= PARSER_BOUND and value.denominator <= PARSER_BOUND
        return dict(value=value, cards=left['cards']+right['cards'],
                    fractional_steps=left['fractional_steps']+right['fractional_steps']+(value.denominator > 1),
                    division_steps=left['division_steps']+right['division_steps']+isinstance(node.op, ast.Div),
                    subtraction_steps=left['subtraction_steps']+right['subtraction_steps']+isinstance(node.op, ast.Sub),
                    unary_steps=left['unary_steps']+right['unary_steps'],
                    negative_intermediate_steps=left['negative_intermediate_steps']+right['negative_intermediate_steps']+(value < 0),
                    tree_depth=1+max(left['tree_depth'], right['tree_depth']),
                    max_denominator=max(left['max_denominator'], right['max_denominator'], value.denominator),
                    max_intermediate_magnitude=max(left['max_intermediate_magnitude'], right['max_intermediate_magnitude'], abs(value)))

    result = visit(ast.parse(text, mode='eval').body)
    assert len(result['cards'])-1+result['unary_steps'] <= 31
    result['noncommutative_steps'] = result['division_steps']+result['subtraction_steps']
    result['max_intermediate_magnitude'] = str(result['max_intermediate_magnitude'])
    return result


def bank_sample20():
    """Replay the beta.4 independent EASY/NORMAL bank audit without writes.

    At each level choose targets 1,2,10,24,99,250,500,777,999,1000. Sort the
    two records for each target by puzzle_id, then select zero-based ordinal
    (target-list index + difficulty) % 2. Every tree is enumerated under the
    actual unary/rational rules, including repeated-card occurrences. This is
    a 20-record sample, not a full independent audit of the 8,000-record bank.
    """
    started = time.monotonic()
    root = Path(__file__).resolve().parents[1]
    paths = (root/'assets/guesscalc_target_beta4.json',
             root/'assets/guesscalc_make_target_complexity.csv')
    contents = tuple(path.read_bytes() for path in paths)
    bank = json.loads(contents[0])['records']
    rows = list(csv.DictReader(contents[1].decode().splitlines()))
    assert len(bank) == len(rows) == 8000, 'Expected revision-4 bank size'
    index = {int(row['puzzle_id']): row for row in rows}
    assert len(index) == len(rows) and len({row['puzzle_id'] for row in bank}) == len(bank)
    targets = (1, 2, 10, 24, 99, 250, 500, 777, 999, 1000)
    feature_keys = ('fractional_steps', 'division_steps', 'subtraction_steps',
                    'unary_steps', 'negative_intermediate_steps', 'tree_depth',
                    'max_denominator')
    total_trees = total_values = samples = 0

    def check_expression(text, record, expected=None):
        result = expression_metrics(text)
        assert result['value'] == record['target'], 'Witness target mismatch'
        assert collections.Counter(result['cards']) == collections.Counter(record['cards']), 'Witness card multiset mismatch'
        if expected is not None:
            for key in feature_keys+('max_intermediate_magnitude',):
                assert result[key] == expected[key], f'Witness {key} mismatch'
        return result

    for difficulty in range(2):
        for target_index, target in enumerate(targets):
            options = sorted((row for row in bank if row['difficulty'] == difficulty and row['target'] == target),
                             key=lambda row: row['puzzle_id'])
            assert len(options) == 2, f'Expected two records for level {difficulty}, target {target}'
            record = options[(target_index+difficulty) % 2]
            puzzle_id = record['puzzle_id']
            try:
                exact, row = record['exact'], index[puzzle_id]
                graded = exact['graded']
                assert len(record['cards']) == 4
                assert [int(value) for value in row['cards'].split()] == record['cards']
                assert int(row['target']) == target and row['difficulty'] == ('EASY', 'NORMAL')[difficulty]
                assert exact['num'] == target and exact['den'] == 1
                values = brute_force(record['cards'])
                reference = values[Fraction(target)]
                assert reference['canonical_count'] == exact['canonical_count'] == int(row['solution_count']), 'Canonical count mismatch'
                assert list(reference['minimal_tuple']) == exact['minimal_tuple'] == json.loads(row['raw_minimal_tuple']), 'Raw minimum mismatch'
                grade = (graded['fractional_steps'], graded['division_steps'],
                         graded['noncommutative_steps']+graded['unary_steps'],
                         graded['negative_intermediate_steps'], graded['tree_depth'],
                         graded['max_denominator'])
                assert grade == reference['graded']['minimal_tuple'], 'Graded minimum mismatch'
                assert graded['noncommutative_steps'] == graded['division_steps']+graded['subtraction_steps']
                assert graded['score'] == reference['graded']['score'], 'Graded score mismatch'
                for key in feature_keys:
                    assert int(row[key]) == graded[key], f'CSV {key} mismatch'
                assert row['max_intermediate_magnitude'] == graded['max_intermediate_magnitude']
                assert row['easiest_expression'] == graded['expression']
                assert row['generator_witness'] == record['generator_witness']
                rarity = max(0, 16-(reference['canonical_count'].bit_length()-1))
                score = 32*reference['graded']['score']+rarity
                assert exact['rarity_bonus'] == rarity
                assert exact['minimal_complexity_score'] == score == int(row['minimal_complexity_score']), 'Combined score mismatch'
                check_expression(record['generator_witness'], record)
                check_expression(graded['expression'], record, graded)
                raw = check_expression(exact['easiest_expression'], record, exact)
                assert tuple(raw[key] for key in CORE_FEATURES) == reference['minimal_tuple']
                total_values += len(values)
                total_trees += sum(value['canonical_count'] for value in values.values())
                samples += 1
                del values
            except (AssertionError, KeyError, TypeError, ValueError) as error:
                raise AssertionError(f'Puzzle {puzzle_id}, level {difficulty}, target {target}: {error}') from error
    assert contents == tuple(path.read_bytes() for path in paths), 'Bank changed during audit'
    return dict(status='PASS', samples=samples, canonical_trees=total_trees,
                rational_value_groups=total_values,
                seconds=round(time.monotonic()-started, 3))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--candidate-json', type=Path)
    parser.add_argument('--all-fixed', action='store_true')
    parser.add_argument('--bank-sample20', action='store_true',
                        help='independently check 20 fixed beta.4 JSON/CSV bank records')
    args = parser.parse_args()
    if args.candidate_json:
        candidates = json.loads(args.candidate_json.read_text())
        if isinstance(candidates, dict):
            candidates = [candidates]
        print(json.dumps([check_candidate(p) for p in candidates], indent=2))
    elif args.all_fixed:
        print(json.dumps(fixed_report(), indent=2))
    elif args.bank_sample20:
        print(json.dumps(bank_sample20()))
    else:
        self_test()
