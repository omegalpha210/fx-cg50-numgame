#!/usr/bin/env python3
"""Reproducible, bounded GUESS/CALC level audit; no human difficulty claims.

Compile the actual C modules with strict warnings and UBSan, inspect every
current bank ordinal, and sample deterministic runtime generation. --write
updates only assets/guesscalc_difficulty_audit.json. JSON also goes to stdout.
This is a structural audit, not a replacement for the independent pack solvers.
"""
import argparse
import ast
import collections
import functools
import hashlib
import itertools
import json
import math
import os
from pathlib import Path
import statistics
import subprocess
import tempfile
from fractions import Fraction

ROOT = Path(__file__).resolve().parents[2]
NAMES = {1: 'NUMBER BASEBALL', 2: 'EQUATION GUESS', 3: 'NUMBER MIND',
         4: 'CLUE LOCK', 5: 'SEQUENCE DETECTIVE', 6: 'MAKE TARGET',
         7: 'COUNTDOWN', 8: 'MISSING OPERATORS', 9: 'CROSS MATH',
         10: 'PRIME FACTOR', 33: 'BLACK BOX', 34: 'CRYPTARITHM'}
LEVELS = ('EASY', 'NORMAL', 'HARD', 'MASTER')
FAMILIES = {2: 'equation', 3: 'number_mind', 4: 'clue_lock', 5: 'sequence',
            7: 'countdown', 8: 'operators', 9: 'crossmath'}


def factor_shape_domains():
    # These are mathematical shapes admitted by the construction, not proof
    # that every shape is reachable by some 32-bit PRNG state.
    domains = []
    for primes, lengths in [((2, 3, 5, 7), range(2, 5)),
                            ((2, 3, 5, 7, 11, 13), range(3, 6)),
                            ((2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31), range(4, 7))]:
        domains.append({math.prod(p) for count in lengths for p in itertools.combinations_with_replacement(primes, count)
                        if math.prod(p) <= 1000000})
    domains.append({math.prod(primes)*a*b for primes in itertools.combinations((2, 3, 5, 7, 11, 13), 4)
                    for a, b in itertools.combinations(primes, 2)})
    return domains


def finding(id_, d):
    notes = ['Human difficulty uncalibrated; measured structural parameters only.']
    if id_ in (1, 2) and d == 3:
        notes.append('REVIEW REQUIRED: longer secret but more permitted guesses than HARD.')
    if id_ == 5 and d < 2:
        notes.append('REVIEW REQUIRED: E and N each have 25/30 shared-step alternating puzzles; median largest term both11. E bank contains no GP puzzle despite allowed family.')
    if id_ == 6:
        notes.append('REVIEW REQUIRED: constructive witness complexity is not a lower bound on all valid solutions; E/N both four cards.')
    if id_ == 7 and d < 3:
        notes.append('REVIEW REQUIRED: large-card composition differs; minimum-card distributions overlap and do not increase monotonically.')
        if d == 2:
            notes.append('HARD puzzle60 and71 are solved by one card100; saved-bank compatibility prevents in-place replacement.')
    if id_ == 10:
        notes.append('REVIEW REQUIRED: numeric target domains overlap; MASTER shape domain90 is a subset of HARD shape domain and uses smaller primes.')
    if id_ == 34:
        notes.append('REVIEW REQUIRED: letter-count and column-count increase; carry/node distributions overlap. Solver nodes are not human ratings.')
    return ' '.join(notes)


def frozen(value):
    return json.dumps(value, separators=(',', ':'), sort_keys=True)


def summary(values):
    values = list(values)
    assert values
    return {'min': min(values), 'median': statistics.median(values), 'max': max(values)}


def histogram(values):
    return dict(sorted(collections.Counter(str(v) for v in values).items()))


def expression(text):
    """Independent exact AST evaluator plus fractional-intermediate evidence."""
    leaves = []
    fractions = 0

    def walk(node):
        nonlocal fractions
        if isinstance(node, ast.Constant):
            assert isinstance(node.value, int)
            leaves.append(node.value)
            return Fraction(node.value)
        if isinstance(node, ast.UnaryOp):
            assert isinstance(node.op, ast.USub)
            return -walk(node.operand)
        assert isinstance(node, ast.BinOp)
        a, b = walk(node.left), walk(node.right)
        if isinstance(node.op, ast.Add):
            value = a+b
        elif isinstance(node.op, ast.Sub):
            value = a-b
        elif isinstance(node.op, ast.Mult):
            value = a*b
        else:
            assert isinstance(node.op, ast.Div) and b
            value = a/b
        fractions += value.denominator != 1
        return value

    return walk(ast.parse(text, mode='eval').body), leaves, fractions


def factor(value):
    answer = []
    p = 2
    while p*p <= value:
        while not value % p:
            answer.append(p)
            value //= p
        p += 1
    if value > 1:
        answer.append(value)
    return answer


def eval_ops(numbers, ops):
    # Direct rational term accumulation, independent of the C recursive parser.
    total, term, sign = Fraction(0), Fraction(numbers[0]), 1
    for op, n in zip(ops, numbers[1:]):
        if op == '*':
            term *= n
        elif op == '/':
            term /= n
        else:
            total += sign*term
            term, sign = Fraction(n), 1 if op == '+' else -1
    return total+sign*term


def cross_candidates(p, transpose=False):
    ops = p['ops'][6:] if transpose else p['ops'][:6]
    targets = p['targets'][3:] if transpose else p['targets'][:3]
    givens = [p['givens'][r+3*c] for r in range(3) for c in range(3)] if transpose else p['givens']
    rows = []
    for r in range(3):
        options = []
        for values in itertools.permutations(range(1, 10), 3):
            if any(givens[3*r+c] and givens[3*r+c] != values[c] for c in range(3)):
                continue
            if eval_ops(values, ['+-*'[x] for x in ops[2*r:2*r+2]]) == targets[r]:
                options.append(sum(1 << v for v in values))
        rows.append(options)
    return sum(not (a & b or a & c or b & c) for a in rows[0] for b in rows[1] for c in rows[2])


def minimum_countdown_cards(p):
    """All positive-integer binary trees for each proper card submultiset.

    A checked six-card witness bounds the search; no difficulty field is used.
    Memoization lives for one six-card puzzle only, not an unbounded corpus.
    """
    @functools.lru_cache(maxsize=64)
    def values(cards):
        if len(cards) == 1:
            return frozenset(cards)
        out = set()
        for mask in range(1, (1 << len(cards))-1, 2):
            left = tuple(v for i, v in enumerate(cards) if mask >> i & 1)
            right = tuple(v for i, v in enumerate(cards) if not mask >> i & 1)
            for a in values(left):
                for b in values(right):
                    out.add(a+b);out.add(a*b)
                    if a != b:
                        out.add(abs(a-b))
                    if a % b == 0:
                        out.add(a//b)
                    if b % a == 0:
                        out.add(b//a)
        return frozenset(x for x in out if 0 < x <= 10**9)

    result, used, _ = expression(p['witness'])
    assert result == p['target'] and not (collections.Counter(used)-collections.Counter(p['cards']))
    for count in range(1, len(used)):
        if any(p['target'] in values(subset) for subset in set(itertools.combinations(sorted(p['cards']), count))):
            return count
    return len(used)


def canonical(id_, p):
    if id_ == 2:
        return p['equation']
    if id_ == 3:
        return frozen([p['n'], p['alphabet'], sorted(p['clues'])])
    if id_ == 4:
        a = p['params']
        mods = [(a[4], a[5]), (a[6], a[7])]
        if len(a) == 11:
            mods.append((a[9], a[10]))
        return frozen([p['limit'], a[:4], a[8], sorted(mods)])
    if id_ == 5:
        return frozen(p['sequence'][:6])
    if id_ == 7:
        return frozen([sorted(p['cards']), p['target']])
    if id_ == 8:
        return frozen([p['numbers'], p['target']])
    if id_ == 9:
        normal = [p['ops'], p['targets'], p['givens']]
        transposed = [p['ops'][6:]+p['ops'][:6], p['targets'][3:]+p['targets'][:3],
                      [p['givens'][r+3*c] for r in range(3) for c in range(3)]]
        return min(frozen(normal), frozen(transposed))
    assert id_ == 34
    candidates = []
    for operands in itertools.permutations(p['words'][:2]):
        mapping = {}
        candidates.append([''.join(mapping.setdefault(c, chr(65+len(mapping))) for c in word)
                           for word in (*operands, p['words'][2])])
    return frozen(min(candidates))


def sample_key(p):
    id_, a, b = p['id'], p['data'], p['board']
    if id_ == 1:
        return frozen(b[:a[0]])
    if id_ == 6:
        return frozen([a[0], sorted(b[:a[1]])])
    if id_ == 8:
        return frozen([b[:a[0]], a[1]])
    if id_ == 10:
        return str(a[0])
    assert id_ == 33
    n = p['rows']
    return frozen([n, p['fixed'][:n*n]])


def d4_atoms(p):
    n, variants = p['rows'], []
    grid = p['fixed'][:n*n]
    for _ in range(4):
        variants.append(grid)
        variants.append([grid[r*n+n-1-c] for r in range(n) for c in range(n)])
        grid = [grid[(n-1-c)*n+r] for r in range(n) for c in range(n)]
    return frozen(min(variants))


def check_payload(sample, p):
    id_, a, b = sample['id'], sample['data'], sample['board']
    if id_ == 2:
        assert ''.join(chr(v) for v in b[:a[0]]) == p['equation']
    elif id_ == 3:
        assert a[:3] == [p['n'], p['alphabet'], len(p['clues'])]
        assert b[:p['n']*len(p['clues'])] == [v for clue, _ in p['clues'] for v in clue]
        assert a[16:16+len(p['clues'])] == [match for _, match in p['clues']]
    elif id_ == 4:
        assert a[:9] == p['params'][:9] and a[9] == p['limit']
        if sample['level'] == 3:
            assert a[10:12] == p['params'][9:]
    elif id_ == 5:
        assert b[:6] == p['sequence'][:6] and a[:4] == [p['family'], *p['params']]
    elif id_ == 7:
        assert b[:6] == p['cards'] and a[0] == p['target']
    elif id_ == 8:
        assert b[:6] == p['numbers'] and a[1] == p['target']
    elif id_ == 9:
        assert b[:9] == p['givens'] and a[:12] == p['ops'] and a[12:18] == p['targets']
    else:
        assert id_ == 34 and sample['words'] == p['words'] and a[1] == p['letters']


MECHANISMS = {
 1: ['4 digits; repeats and leading zero allowed; 16 guesses', '5 digits; 12 guesses', '6 digits; 10 guesses', '7 digits; 16 guesses'],
 2: ['one + or -; mode lengths 7/6/8; 12 guesses', 'one +, -, or *; mode lengths 7/6/8; 10 guesses', 'STANDARD/LONG two operators, SHORT one; + - * /; 8 guesses', '3/2/3 operators; mode lengths 9/8/10; mixed precedence must affect value; 12 guesses'],
 3: ['4 positions × 6 symbols', '4 positions × 8 symbols', '5 positions × 8 symbols', '6 positions × 8 symbols; up to 13 clues'],
 4: ['domain 0..99; range/parity/digit sum/two remainders/contains digit', 'domain 0..499; contains-digit clue removed', 'domain 0..999; range and two distinct large moduli, no parity or digit sum', 'domain 0..9999; three remainders plus digit sum; every clue essential'],
 5: ['AP/GP/shared-step alternating grammar', 'quadratic/Fibonacci/shared-step alternating grammar', 'quadratic/Fibonacci/affine recurrence grammar', 'second-order affine recurrence or unequal-step alternating grammar; 15 each'],
 6: ['4 cards; constructive a*b+c-d', '4 cards; constructive (a*b-c)/d', '5 cards; constructive (a*b-c)/(d+e)', '6 cards; constructive (a*b-c)/d+e/f; witness has fractional intermediates'],
 7: ['6 cards; 1 large + 5 small', '6 cards; 2 large + 4 small', '6 cards; 3 large + 3 small', '6 cards; 3 large + 3 small; every exact solution uses all six and division'],
 8: ['3 numbers; witness + - *; all four operators accepted', '4 numbers; witness + - * /', '5 numbers; witness + - * /', '6 numbers; 1..3 solutions; every solution uses at least 3 operator kinds'],
 9: ['3×3; four fixed numbers', '3×3; two fixed numbers', '3×3; no fixed numbers', '3×3; no fixed numbers; row-only and column-only candidates both exceed old HARD min-axis maximum'],
 10: ['2..4 prime draws from 2,3,5,7', '3..5 prime draws from primes through 13', '4..6 prime draws through 31; multiplication over 1e6 skipped', 'four distinct primes through 13; exactly two squared; six factors counting multiplicity'],
 33: ['5×5; 3 atoms; 20 ports', '6×6; 4 atoms; 24 ports', '7×7; 5 atoms; 28 ports', '8×8; 6 atoms; 32 ports'],
 34: ['two 2-digit addends; 3..5 letters', 'two 3-digit addends; 5..7 letters', 'two 4-digit addends; 7..9 letters', 'two 5-digit addends; 8..10 letters'],
}


def metrics(id_, d, records, samples):
    if id_ == 1:
        assert all(x['data'][:2] == [4+d, (16, 12, 10, 16)[d]] for x in samples)
        return {'code_domain': 10**(4+d), 'digits': 4+d, 'guess_limit': (16, 12, 10, 16)[d],
                'samples_with_repeated_digit': sum(len(set(x['board'][:4+d])) < 4+d for x in samples)}
    if id_ == 2:
        by_mode = {}
        for mode in range(3):
            chosen = [p for p in records if (p.get('mode', p['puzzle_id']//90) if d < 3 else p['mode']) == mode]
            by_mode[str(mode)] = {'count': len(chosen), 'length': summary(p['length'] for p in chosen),
                                 'operator_count': summary(sum(c in '+-*/' for c in p['equation']) for p in chosen),
                                 'division_puzzles': sum('/' in p['equation'] for p in chosen),
                                 'multiplication_puzzles': sum('*' in p['equation'] for p in chosen)}
        return {'by_mode': by_mode, 'guess_limit': (12, 10, 8, 12)[d]}
    if id_ == 3:
        return {'domain': records[0]['alphabet']**records[0]['n'], 'clue_count': summary(len(p['clues']) for p in records),
                'zero_match_clues': summary(sum(n == 0 for _, n in p['clues']) for p in records)}
    if id_ == 4:
        result = {'domain': records[0]['limit']+1, 'public_range_width': summary(p['params'][1]-p['params'][0]+1 for p in records),
                  'modulus_values': sorted({p['params'][i] for p in records for i in ([4, 6, 9] if d == 3 else [4, 6])})}
        if d == 3:
            result['omit_one_clue_candidates'] = summary(v for p in records for v in p['omit_one_clue_candidates'])
        return result
    if id_ == 5:
        return {'families': histogram(p['family'] for p in records),
                'max_absolute_term': summary(max(map(abs, p['sequence'])) for p in records)}
    if id_ == 6:
        fractions = []
        for p in samples:
            value, leaves, count = expression(p['witness'])
            assert value == p['data'][0] and sorted(leaves) == sorted(p['board'][:p['data'][1]])
            assert p['data'][1] == (4, 4, 5, 6)[d] and all(1 <= x <= 1000 for x in leaves)
            fractions.append(count)
        return {'cards': (4, 4, 5, 6)[d], 'target_domain_tested': [1, 1000], 'all_targets_at_fixed_seed': 1000,
                'seed_samples_per_target': 256, 'seed_sample_targets': [1, 24, 1000],
                'card_value': summary(v for p in samples for v in p['board'][:p['data'][1]]),
                'witness_fractional_intermediates': summary(fractions),
                'fraction_required_for_every_solution': 'NOT PROVEN',
                'arithmetic_rules': 'all cards exactly once; arbitrary valid exact rational steps'}
    if id_ == 7:
        leaf_counts = [len(expression(p['witness'])[1]) for p in records]
        minima = [minimum_countdown_cards(p) for p in records]
        if d == 3:
            assert minima == [6]*30
        return {'large_cards': summary(sum(v > 10 for v in p['cards']) for p in records),
                'witness_card_count': summary(leaf_counts), 'witness_division_count': summary(p['witness'].count('/') for p in records),
                'target': summary(p['target'] for p in records),
                'minimum_cards_exhaustive': summary(minima), 'minimum_cards_histogram': histogram(minima),
                'one_card_puzzle_ids': [p['puzzle_id'] for p, count in zip(records, minima) if count == 1],
                'division_required': True if d == 3 else 'NOT CONSTRAINED'}
    if id_ == 8:
        counts, kinds = [], []
        for p in samples:
            n = p['data'][0]
            solutions = [ops for ops in itertools.product('+-*/', repeat=n-1)
                         if eval_ops(p['board'][:n], ops) == p['data'][1]]
            assert solutions
            if d == 3:
                assert 1 <= len(solutions) <= 3 and all(len(set(s)) >= 3 for s in solutions)
            counts.append(len(solutions));kinds.extend(len(set(s)) for s in solutions)
        return {'numbers': 3+d, 'operator_assignments_per_puzzle': 4**(2+d),
                'solution_count_exhaustive': summary(counts), 'operator_kinds_per_solution': summary(kinds),
                'fallback': '128 attempts then fixed plus-only puzzle' if d < 3 else 'none; bank-backed',
                'fallback_observed': 'not applicable' if d == 3 else 'not instrumented; not inferred from absence'}
    if id_ == 9:
        axis = [(cross_candidates(p), cross_candidates(p, True)) for p in records]
        return {'givens': summary(sum(bool(v) for v in p['givens']) for p in records),
                'row_only_all_different_boards': summary(a for a, _ in axis),
                'column_only_all_different_boards': summary(b for _, b in axis),
                'minimum_axis_candidates': summary(min(a, b) for a, b in axis)}
    if id_ == 10:
        facts = [factor(p['data'][0]) for p in samples]
        domains = factor_shape_domains()
        for p, f in zip(samples, facts):
            assert math.prod(f) == p['data'][0]
            if d == 3:
                assert sorted(collections.Counter(f).values()) == [1, 1, 2, 2] and max(f) <= 13
        return {'target': summary(p['data'][0] for p in samples), 'multiplicity': summary(map(len, facts)),
                'distinct_primes': summary(len(set(f)) for f in facts), 'largest_prime': summary(max(f) for f in facts),
                'mathematical_shape_domain': len(domains[d]),
                'shape_domain_overlap': {LEVELS[other]: len(domains[d] & domains[other]) for other in range(4) if other != d},
                'shape_vs_rng': 'admissible mathematical shapes; no exhaustive PRNG reachability proof'}
    if id_ == 33:
        n, atoms = 5+d, 3+d
        assert all(x['rows'] == n and sum(x['fixed']) == atoms for x in samples)
        return {'board_side': n, 'atoms': atoms, 'ports': 4*n, 'atom_layout_domain': math.comb(n*n, atoms),
                'unique_sample_D4_layouts': len({d4_atoms(p) for p in samples}),
                'unique_sample_full_ray_vectors': len({frozen(p['rays']) for p in samples}),
                'ray_checks': len(samples)*4*n, 'unique_hidden_layout_guarantee': False}
    assert id_ == 34
    carries = []
    for p in records:
        carry, count = 0, 0
        for col in range(max(map(len, p['words']))):
            carry = (carry+sum(p['solution'][ord(word[-1-col])-65] for word in p['words'][:2] if len(word) > col))//10
            count += bool(carry)
        carries.append(count)
    return {'addend_length': 2+d, 'letters': summary(p['letters'] for p in records),
            'column_solver_nodes': summary(p['column_nodes'] for p in records),
            'carry_columns': summary(carries), 'solution_count_declared_and_independently_verified': 1}


def run_samples():
    sources = ['tools/generate/guesscalc_difficulty_sample.c', 'src/games/guesscalc.c',
               'src/games/guesscalc_extra.c', 'src/games/guesscalc_math.c', 'src/core/common.c', 'src/ui/draw.c']
    build = ROOT/'build-host/guesscalc-difficulty'
    build.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='audit-', dir=build) as folder:
        executable = str(Path(folder)/'sample')
        subprocess.run([os.environ.get('CC', 'clang'), '-std=c11', '-Wall', '-Wextra', '-Werror',
                        '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all', '-Iinclude', '-Isrc/games',
                        *sources, '-o', executable], cwd=ROOT, check=True)
        result = subprocess.run([executable], cwd=ROOT, check=True, capture_output=True, text=True)
    return [json.loads(line) for line in result.stdout.splitlines()]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group()
    action.add_argument('--write', action='store_true')
    action.add_argument('--check', action='store_true', help='recompute and assert both committed JSON reports agree exactly')
    args = parser.parse_args()
    old = json.loads((ROOT/'assets/guesscalc_packs.json').read_text())
    master = json.loads((ROOT/'assets/guesscalc_master.json').read_text())
    crypt = json.loads((ROOT/'assets/guesscalc_cryptarithm.json').read_text())
    records = {id_: old.get(name, [])+master.get(name, []) for id_, name in FAMILIES.items()}
    records[34] = crypt['records']
    samples = run_samples()
    rows, keys = [], {}
    for id_, name in NAMES.items():
        for d, level in enumerate(LEVELS):
            chosen = [p for p in records.get(id_, []) if p['difficulty'] == d]
            actual = [p for p in samples if p['id'] == id_ and p['level'] == d]
            if chosen:
                by_id = {p['puzzle_id']: p for p in chosen}
                assert len(actual) == len(chosen)
                for sample in actual:
                    check_payload(sample, by_id[sample['puzzle_id']])
                unique = {canonical(id_, p) for p in chosen}
                assert len(unique) == len(chosen)
            else:
                unique = {sample_key(p) for p in actual}
            keys[id_, d] = unique
            rows.append(dict(id=id_, name=name, level=level, level_index=d, hell_supported=False,
                             mechanism=MECHANISMS[id_][d], supply='BANK' if chosen else 'RUNTIME',
                             modes=3 if id_ == 2 else 1, bank_count=len(chosen), bank_count_per_mode=30 if chosen else 0,
                             unique_bank_count=len(unique) if chosen else None,
                             sampled_initializations=len(actual), unique_sample_count=len(unique),
                             metric=metrics(id_, d, chosen, actual),
                             deterministic_replays=len(actual), initial_state_validations=2*len(actual),
                             classification='STRUCTURAL DIFFERENCE VERIFIED; HUMAN RATING REVIEW REQUIRED',
                             finding=finding(id_, d),
                             level_no_op=False))
    for row in rows:
        id_, d = row['id'], row['level_index']
        row['overlap_scope'] = 'complete current bank' if row['bank_count'] else 'bounded runtime sample only, not full generator domain'
        row['overlap_with_other_levels'] = {LEVELS[other]: len(keys[id_, d] & keys[id_, other])
                                            for other in range(4) if other != d}
    cross = [row['metric'] for row in rows if row['id'] == 9]
    assert cross[3]['minimum_axis_candidates']['min'] > cross[2]['minimum_axis_candidates']['max']
    files = ['src/games/guesscalc.c', 'src/games/guesscalc_extra.c', 'src/games/guesscalc_math.c',
             'src/core/common.c', 'assets/guesscalc_packs.json', 'assets/guesscalc_packs.h',
             'assets/guesscalc_master.json', 'assets/guesscalc_master.h',
             'assets/guesscalc_cryptarithm.json', 'assets/guesscalc_cryptarithm.h']
    report = dict(schema=1, audit='beta.3 GUESS/CALC structural difficulty',
                  external_puzzle_records=0, current_bank_records=sum(row['bank_count'] for row in rows),
                  legacy_only_target_records=len(old['target'])+len(master['target']),
                  sampled_initializations=len(samples), rows=rows,
                  method={'runtime_seeds': '1..256; Target additionally all 1..1000 targets at seed 0x6e554d47',
                          'sample_coverage': 'bounded consecutive-seed sample, not unbiased sampling or exhaustion of the 32-bit seed domain',
                          'bank_selection': 'actual C init, 30-ordinal complete cycle for each mode and level',
                          'canonicalization': {'2': 'exact equation string; arithmetic-equivalent strings intentionally distinct',
                                               '3': 'positions/alphabet plus unordered exact-match clues; no symbol/position relabel normalization',
                                               '4': 'numeric domain/range/nonmodular clues plus unordered modulus/remainder pairs',
                                               '5': 'six visible terms; no affine scaling/translation normalization',
                                               '6': 'target and sorted card multiset', '7': 'target and sorted card multiset',
                                               '8': 'ordered numbers and target', '9': 'public constraints normalized under transpose only',
                                               '10': 'numeric target', '33': 'exact atom grid; separate D4 and ray-vector metrics',
                                               '34': 'letter relabeling and addend commutation'},
                          'unique_meaning': 'distinct public problem, NOT solution count; bank solution uniqueness is checked by separate verifiers',
                          'human_rating': 'NOT MEASURED; solver nodes and structural parameters are not human ratings',
                          'runtime': 'host C11 strict warnings + UBSan; no device timing or ASan claim'},
                  findings=[
                      'No level selector is a no-op. Structural difference does not establish monotonic human difficulty.',
                      'Baseball MASTER increases length but relaxes guesses 10->16; Equation MASTER 8->12. Intentional playability tradeoff, review required.',
                      'Countdown E/N/H changes large-card composition, not minimum solution length; HARD puzzle60 target100 is immediately solved by its single 100 card.',
                      'Make Target E/N both use four cards. Templates differ, but no level guarantees every solution needs the template operators or fractions.',
                      'Prime Factor targets overlap between levels and MASTER has at most 90 possible distinct targets, all in the HARD mathematical shape domain; no no-repeat promise exists for runtime supply.',
                      'Cryptarithm length increases; carry counts, letter counts and solver nodes overlap. No human-hardness calibration.',
                      'Sequence E/N/H family sets overlap; visible six-term problem keys do not.',
                      'Black Box accepts ray-equivalent atom layouts; distinct generated layouts are not a unique-hidden-solution claim.'
                  ],
                  source_sha256={file: hashlib.sha256((ROOT/file).read_bytes()).hexdigest() for file in files})
    assert len(rows) == 48 and report['current_bank_records'] == 1110 and report['legacy_only_target_records'] == 240
    text = json.dumps(report, indent=2, sort_keys=True)+'\n'
    if args.write or args.check:
        audit_file = ROOT/'assets/guesscalc_difficulty_audit.json'
        if args.write:
            audit_file.write_text(text)
        else:
            assert audit_file.read_text() == text, 'detailed report is stale; regenerate with --write'
        integration = []
        for row in rows:
            id_, d = row['id'], row['level_index']
            if id_ == 33:
                size = f'{5+d}x{5+d}'
            elif id_ == 9:
                size = '3x3'
            else:
                size = 'N/A; see generator_params'
            integration.append(dict(game_id=id_, name=row['name'], difficulty=row['level'],
                                    mechanism=row['mechanism'], bank_size=row['bank_count'], board_size=size,
                                    generator_params=frozen(row['metric']), AI_policy='N/A; no AI opponent',
                                    rating_metric='Structural metric; human rating NOT MEASURED',
                                    observed_distribution=frozen({'metrics': row['metric'], 'validated_initializations': row['sampled_initializations'],
                                                                  'unique': row['unique_sample_count'], 'unique_bank': row['unique_bank_count'],
                                                                  'modes': row['modes'], 'bank_per_mode': row['bank_count_per_mode']}),
                                    overlap=frozen({'scope': row['overlap_scope'], 'counts': row['overlap_with_other_levels']}),
                                    meaningfully_distinct='STRUCTURAL YES; human ordering REVIEW REQUIRED' if id_ in (5, 6, 7, 10) else 'STRUCTURAL YES; human rating uncalibrated',
                                    finding=row['finding'],
                                    fix='No demonstrated selector no-op; generation and legacy banks preserved'))
        row_file = ROOT/'assets/guesscalc_difficulty_rows.json'
        row_text = json.dumps(integration, indent=2)+'\n'
        if args.write:
            row_file.write_text(row_text)
        else:
            assert row_file.read_text() == row_text, 'integration rows are stale; regenerate with --write'
    print(text, end='')


if __name__ == '__main__':
    main()
