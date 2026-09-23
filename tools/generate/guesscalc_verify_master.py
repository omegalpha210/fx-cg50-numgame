#!/usr/bin/env python3
"""Independent MASTER acceptance/rating checks; never imports either generator.

Checks every finite code domain and every Cross Math board, verifies all card
trees by independent subset recursion, and exhausts all 1024 operator strings.
Host search costs are not presented as device timings or human difficulty.
"""
import ast
import collections
import functools
import hashlib
import itertools
import json
import time
from fractions import Fraction
from pathlib import Path
import guesscalc_verify as reference

ROOT = Path(__file__).resolve().parents[2]
OLD = json.loads((ROOT / 'assets/guesscalc_packs.json').read_text())
P = json.loads((ROOT / 'assets/guesscalc_master.json').read_text())
checks = 0


def require(condition, message=''):
    global checks
    checks += 1
    if not condition:
        raise AssertionError(f'check {checks}: {message}')


def splits(cards):
    # Include the first card in the left part: exactly one of each partition.
    for mask in range(1, (1 << len(cards)) - 1, 2):
        yield tuple(x for i, x in enumerate(cards) if mask >> i & 1), tuple(x for i, x in enumerate(cards) if not mask >> i & 1)


@functools.lru_cache(maxsize=None)
def integer_reachable(cards, positive, divide):
    if len(cards) == 1:
        return frozenset(cards if positive else (cards[0], -cards[0]))
    results = set()
    for left, right in splits(cards):
        for a in integer_reachable(left, positive, divide):
            for b in integer_reachable(right, positive, divide):
                values = [a + b, a - b, b - a, a * b]
                if divide and b and not a % b:
                    values.append(a // b)
                if divide and a and not b % a:
                    values.append(b // a)
                for value in values:
                    if abs(value) > 10**9 or (positive and value <= 0):
                        continue
                    results.add(value)
                    if not positive:
                        results.add(-value)  # Arbitrary legal unary minus is included.
    return frozenset(results)


def rational_options(a, b):
    values = {a+b, a-b, b-a, a*b}
    if b:
        values.add(a/b)
    if a:
        values.add(b/a)
    return values | {-x for x in values}


def balanced_target(cards, target):
    for order in set(itertools.permutations(cards)):
        left = rational_options(Fraction(order[0]), Fraction(order[1]))
        right = rational_options(Fraction(order[2]), Fraction(order[3]))
        if any(Fraction(target) in rational_options(a, b) for a in left for b in right):
            return True
    return False


def metadata():
    sizes = {'number_mind': (3, 90, 30), 'clue_lock': (4, 90, 30), 'sequence': (5, 90, 30),
             'target': (6, 180, 60), 'countdown': (7, 90, 30), 'operators': (8, 0, 30),
             'crossmath': (9, 90, 30), 'equation': (2, 270, 90)}
    require(P['rules_version'] == 2 and P['external_records'] == 0)
    for name, (game, start, count) in sizes.items():
        require(len(P[name]) == count, name)
        for i, record in enumerate(P[name]):
            require((record['game_id'], record['difficulty'], record['rules_version'], record['puzzle_id']) == (game, 3, 2, start+i), name)
        def signature(p):
            if name in ('target', 'countdown'):
                return tuple(sorted(p['cards'])), p['target']
            if name == 'number_mind':
                return p['n'], p['alphabet'], tuple(sorted((tuple(g), n) for g, n in p['clues']))
            if name == 'equation':
                return p['equation']
            if name == 'sequence':
                return tuple(p['sequence'][:6])
            if name == 'crossmath':
                return tuple(p['ops']), tuple(p['targets']), tuple(p['givens'])
            if name == 'operators':
                return tuple(p['numbers']), p['target']
            return p['limit'], tuple(p['params'])
        old = {signature(p) for p in OLD.get(name, [])}
        new = {signature(p) for p in P[name]}
        require(len(new) == count and not old.intersection(new), name + ' duplicate')
    for file, expected in [('guesscalc_packs.json', 'e11fa9f8720b813570f686558837e24acab1afe73d4f700610c4651423bd7ef2'),
                           ('guesscalc_packs.h', '430df5b94b2ffa37871443d84319aaa449576a55ad8944875317322d6521259a')]:
        require(hashlib.sha256((ROOT/'assets'/file).read_bytes()).hexdigest() == expected, 'legacy bytes changed')
    print('METADATA: 330 new canonical-distinct records; legacy 900 bytes unchanged', flush=True)


def mind():
    for p in P['number_mind']:
        require(p['n'] == 6 and p['alphabet'] == 8 and p['domain_size'] == 262144)
        require(1 <= len(p['clues']) <= 13 and any(n == 0 for _, n in p['clues']))
        solutions = []
        for candidate in itertools.product(range(8), repeat=6):
            if all(sum(a == b for a, b in zip(candidate, clue)) == n for clue, n in p['clues']):
                solutions.append(candidate)
                if len(solutions) > 1:
                    break
        require(solutions == [tuple(p['solution'])], 'Mind uniqueness')
    print('MIND: 30 exhaustive 8^6 domains, unique solution each', flush=True)


def lock():
    for p in P['clue_lock']:
        a = p['params']
        require(p['limit'] == 9999 and a[:3] == [0, 9999, -1] and a[8] == -1)
        mods = [a[4], a[6], a[9]]
        remainders = [a[5], a[7], a[10]]
        require(len(set(mods)) == 3)
        solutions = []
        omitted = [0]*4
        for value in range(10000):
            matches = [divmod(value, m)[1] == r for m, r in zip(mods, remainders)]
            matches.append(sum(int(digit) for digit in str(value)) == a[3])
            if all(matches):
                solutions.append(value)
            for skip in range(4):
                omitted[skip] += all(v for i, v in enumerate(matches) if i != skip)
        require(solutions == [p['solution']] and omitted == p['omit_one_clue_candidates'])
        require(min(omitted) >= 2, 'Each clue must contribute')
    print('LOCK: 30 unique domains; removal of any one constraint makes each ambiguous', flush=True)


def sequence():
    families = collections.Counter()
    for p in P['sequence']:
        values = p['sequence']
        prefix = values[:6]
        predictions = reference.grammar_predictions(prefix)
        require(not predictions, 'MASTER prefix must not belong to an old grammar')
        if all(-3 <= x <= 6 for x in prefix[:2]):
            for a, b, c in itertools.product((-2, -1, 1, 2), repeat=3):
                if all(prefix[i] == a*prefix[i-1]+b*prefix[i-2]+c for i in range(2, 6)):
                    predictions.add(a*prefix[5]+b*prefix[4]+c)
        if all(-6 <= x <= 9 for x in prefix[:2]):
            for a, b in itertools.product(range(-5, 6), repeat=2):
                if not a or not b or a == b:
                    continue
                if all(prefix[i] == prefix[i % 2] + i//2*(b if i % 2 else a) for i in range(6)):
                    predictions.add(prefix[4]+a)
        require(predictions == {values[6]}, 'Expanded grammar ambiguous')
        a, b, c = p['params']
        expected = a*prefix[5]+b*prefix[4]+c if p['family'] == 6 else prefix[4]+a
        require(expected == values[6] and p['family'] in (6, 7))
        families[p['family']] += 1
    require(families == {6: 15, 7: 15})
    print('SEQUENCE: 30 prefixes unambiguous across all eight bounded grammars', flush=True)


def cards():
    for name in ('target', 'countdown'):
        for p in P[name]:
            numbers = tuple(sorted(p['cards']))
            value, literals = reference.expression(p['witness'], name == 'countdown')
            require(value == p['target'] and literals == collections.Counter(numbers))
            require(len(p['witness']) < 40 and len(p['hint']) < 40)
            if name == 'target':
                require(p['target'] in (10, 24) and len(numbers) == 4 and min(numbers) >= 1 and max(numbers) <= 24)
                require(p['target'] not in integer_reachable(numbers, False, True), 'Integer-only solution exists')
                require(not balanced_target(numbers, p['target']), 'Depth-two solution exists')
                require(p['requires_fraction'] and p['min_tree_depth'] == 3)
            else:
                large = [v for v in numbers if v > 10]
                small = [v for v in numbers if v <= 10]
                require(100 <= p['target'] <= 999 and len(large) == 3 and len(set(large)) == 3 and set(large) <= {25, 50, 75, 100})
                require(len(small) == 3 and min(small) >= 1 and max(collections.Counter(small).values()) <= 2)
                # Different from generation's bitmask/witness table: recurse on
                # canonical tuples and independently inspect every legal subset.
                for used in range(1, 7):
                    for subset in set(itertools.combinations(numbers, used)):
                        require(p['target'] not in integer_reachable(subset, True, False), 'Division-free solution')
                        if used < 6:
                            require(p['target'] not in integer_reachable(subset, True, True), 'Fewer than six cards')
                require(p['min_cards'] == 6 and p['division_required'])
            integer_reachable.cache_clear()
        print(f'{name.upper()}: {len(P[name])} exact witnesses and exhaustive structural ratings', flush=True)


def operators():
    for p in P['operators']:
        numbers = p['numbers']
        solutions = []
        for ops in itertools.product('+-*/', repeat=5):
            expr = ''.join(str(n)+ (ops[i] if i < 5 else '') for i, n in enumerate(numbers))
            value, _ = reference.expression(expr)
            if value == p['target']:
                solutions.append(ops)
        require(len(solutions) == p['solution_count'] and 1 <= len(solutions) <= 3)
        require(all(len(set(ops)) >= 3 and set(ops).intersection('+-') and set(ops).intersection('*/') for ops in solutions))
        require(tuple('+-*/'[i] for i in p['operators']) in solutions)
    print('OPERATORS: 30 x 1024 assignments; 1..3 mixed-precedence solutions each', flush=True)


def cross():
    # Independently calibrate the reference threshold; do not trust the
    # generator's metadata or a hard-coded rating label.
    hard_counts = []
    for p in OLD['crossmath']:
        if p['difficulty'] != 2:
            continue
        rows = cols = 0
        for board in itertools.permutations(range(1, 10)):
            rows += all(reference.evaluate3(board[3*r:3*r+3], p['ops'][2*r:2*r+2]) == p['targets'][r] for r in range(3))
            cols += all(reference.evaluate3(board[c::3], p['ops'][6+2*c:8+2*c]) == p['targets'][3+c] for c in range(3))
        hard_counts.append(min(rows, cols))
    threshold = max(hard_counts)
    require(len(hard_counts) == 30 and threshold == 168)
    for p in P['crossmath']:
        rows = cols = count = 0
        answer = None
        require(p['givens'] == [0]*9)
        for board in itertools.permutations(range(1, 10)):
            row_ok = all(reference.evaluate3(board[3*r:3*r+3], p['ops'][2*r:2*r+2]) == p['targets'][r] for r in range(3))
            col_ok = all(reference.evaluate3(board[c::3], p['ops'][6+2*c:8+2*c]) == p['targets'][3+c] for c in range(3))
            rows += row_ok
            cols += col_ok
            if row_ok and col_ok:
                count += 1
                answer = board
        require(count == 1 and list(answer) == p['solution'])
        require(rows == p['row_only_boards'] and cols == p['column_only_boards'])
        require(min(rows, cols) > p['old_hard_max_min_candidates'] == threshold)
    print('CROSS: 30 x 9! boards; unique solutions, both one-orientation counts exceed old Hard 168', flush=True)


def equations():
    counts = collections.Counter()
    for p in P['equation']:
        lhs, rhs = p['equation'].split('=')
        value, _ = reference.expression(lhs)
        require(value == int(rhs) and len(p['equation']) == (9, 8, 10)[p['mode']])
        ops = [c for c in lhs if c in '+-*/']
        require(len(ops) == (3, 2, 3)[p['mode']] and set(ops).intersection('+-') and set(ops).intersection('*/'))
        import re
        nums = list(map(int, re.split(r'[+*/-]', lhs)))
        left = Fraction(nums[0])
        for op, n in zip(ops, nums[1:]):
            left = {'+': lambda: left+n, '-': lambda: left-n, '*': lambda: left*n, '/': lambda: left/n}[op]()
        require(left != value)
        counts[p['mode']] += 1
    require(counts == {0: 30, 1: 30, 2: 30})
    print('EQUATION: 90 true 9/8/10-character equations; precedence changes every answer', flush=True)


if __name__ == '__main__':
    started = time.monotonic()
    metadata(); mind(); lock(); sequence(); cards(); operators(); cross(); equations()
    print(f'PASS: {checks + reference.checks} independent checks in {time.monotonic()-started:.3f}s')
