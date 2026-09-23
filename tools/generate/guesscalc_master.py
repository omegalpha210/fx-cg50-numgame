#!/usr/bin/env python3
"""Append-only original MASTER packs. All search is bounded and runs on the host.

The legacy generator and its 900 emitted records are deliberately unchanged.
No external puzzle source, network access or runtime puzzle solving is involved.
"""
import ast
import functools
import itertools
import json
import random
from fractions import Fraction
from pathlib import Path
import guesscalc_generate as legacy

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'assets'
R = random.Random(0x4D41535445524743)
OLD = json.loads((OUT / 'guesscalc_packs.json').read_text())


def first_step(text):
    def visit(node):
        if isinstance(node, ast.BinOp):
            for child in (node.left, node.right):
                found = visit(child)
                if found:
                    return found
            op = {ast.Add: '+', ast.Sub: '-', ast.Mult: '*', ast.Div: '/'}[type(node.op)]
            return f'Combine {ast.unparse(node.left)} {op} {ast.unparse(node.right)} first.'
        return None
    return visit(ast.parse(text, mode='eval').body)


def mind():
    domain = list(itertools.product(range(8), repeat=6))
    made = []
    seen = set()
    for attempt in range(2000):
        secret = R.choice(domain)
        candidates = domain[:]
        clues = []
        for _ in range(13):
            guess = R.choice(domain)
            count = sum(a == b for a, b in zip(secret, guess))
            if count == 6 or any(g == guess for g, _ in clues):
                continue
            reduced = [x for x in candidates if sum(a == b for a, b in zip(x, guess)) == count]
            if len(reduced) == len(candidates):
                continue
            clues.append((guess, count))
            candidates = reduced
            if len(candidates) == 1:
                break
        signature = tuple(sorted(clues))
        if len(candidates) != 1 or not any(c == 0 for _, c in clues) or signature in seen:
            continue
        seen.add(signature)
        made.append(dict(n=6, alphabet=8, clues=clues, solution=secret,
                         solution_count=1, domain_size=8**6))
        if len(made) == 30:
            return made
    raise RuntimeError('MASTER Mind bounded search exhausted')


def lock():
    made = []
    seen = set()
    for _ in range(10000):
        secret = R.randrange(1000, 10000)
        # Three remainders and one digit sum: every clue must contribute.
        mods = R.sample((7, 11, 13, 17, 19), 3)
        ds = sum(map(int, str(secret)))
        checks = [lambda x, m=m: x % m == secret % m for m in mods]
        checks.append(lambda x: sum(map(int, str(x))) == ds)
        solutions = [x for x in range(10000) if all(check(x) for check in checks)]
        counts = [sum(all(check(x) for j, check in enumerate(checks) if j != omit)
                      for x in range(10000)) for omit in range(4)]
        if solutions != [secret] or min(counts) < 2:
            continue
        p = [0, 9999, -1, ds, mods[0], secret % mods[0], mods[1], secret % mods[1], -1,
             mods[2], secret % mods[2]]
        signature = (ds, tuple(sorted(zip(mods, (secret % m for m in mods)))))
        if signature in seen:
            continue
        seen.add(signature)
        made.append(dict(limit=9999, params=p, solution=secret, solution_count=1,
                         omit_one_clue_candidates=counts))
        if len(made) == 30:
            return made
    raise RuntimeError('MASTER Lock bounded search exhausted')


def new_sequence_rules():
    for x, y, a, b, c in itertools.product(range(-3, 7), range(-3, 7),
                                          (-2, -1, 1, 2), (-2, -1, 1, 2), (-2, -1, 1, 2)):
        seq = [x, y]
        for _ in range(5):
            seq.append(a * seq[-1] + b * seq[-2] + c)
        if max(map(abs, seq)) < 10000:
            yield 6, [a, b, c], tuple(seq)
    for x, y, a, b in itertools.product(range(-6, 10), range(-6, 10),
                                       (-5, -4, -3, -2, -1, 1, 2, 3, 4, 5),
                                       (-5, -4, -3, -2, -1, 1, 2, 3, 4, 5)):
        if a != b:
            yield 7, [a, b, 0], tuple((x if i % 2 == 0 else y) + (i // 2) * (a if i % 2 == 0 else b) for i in range(7))


def sequence():
    old_rules = legacy.sequence_rules()
    rules = list(new_sequence_rules())
    answers = {}
    for _, _, seq in old_rules + rules:
        answers.setdefault(seq[:6], set()).add(seq[6])
    old_prefixes = {seq[:6] for _, _, seq in old_rules}
    R.shuffle(rules)
    made = []
    seen = set()
    family_counts = {6: 0, 7: 0}
    for family, params, seq in rules:
        if seq[:6] in seen or seq[:6] in old_prefixes or len(answers[seq[:6]]) != 1 or family_counts[family] >= 15:
            continue
        family_counts[family] += 1
        seen.add(seq[:6])
        made.append(dict(family=family, params=params, sequence=seq, distinct_next_answers=1))
        if len(made) == 30:
            return made
    raise RuntimeError('MASTER Sequence not enough distinct unambiguous prefixes')


@functools.lru_cache(maxsize=12000)
def integer_values(cards):
    """All values of signed-integer-intermediate trees, including negative/zero."""
    if len(cards) == 1:
        return frozenset(cards)
    values = set()
    for mask in range(1, 1 << (len(cards) - 1)):
        left = tuple(x for i, x in enumerate(cards) if mask >> i & 1)
        right = tuple(x for i, x in enumerate(cards) if not mask >> i & 1)
        for a in integer_values(left):
            for b in integer_values(right):
                values.update((a + b, a - b, b - a, a * b))
                if b and a % b == 0:
                    values.add(a // b)
                if a and b % a == 0:
                    values.add(b // a)
    return frozenset(v for v in values if abs(v) <= 10**9)


def targets():
    old = {(p['target'], tuple(sorted(p['cards']))) for p in OLD['target']}
    candidates = list(itertools.combinations_with_replacement(range(1, 25), 4))
    R.shuffle(candidates)
    made = []
    for target in (24, 10):
        count = 0
        for cards in candidates:
            if (target, cards) in old or target in integer_values(cards):
                continue
            answer = legacy.solve_cards(cards, target)
            if not answer:
                continue
            # Four leaves have only one depth-two shape: two pairs combined.
            # Check all three pair partitions, both operand directions and signs.
            def pair_values(a, b):
                out = {a+b, a-b, b-a, a*b}
                if b:
                    out.add(a/b)
                if a:
                    out.add(b/a)
                return out | {-v for v in out}
            balanced = False
            for second in range(1, 4):
                rest = [i for i in range(1, 4) if i != second]
                left = pair_values(Fraction(cards[0]), Fraction(cards[second]))
                right = pair_values(Fraction(cards[rest[0]]), Fraction(cards[rest[1]]))
                if any(target in pair_values(a, b) for a in left for b in right):
                    balanced = True
                    break
            if balanced:
                continue
            expr, _ = answer
            made.append(dict(target=target, cards=cards, witness=expr, hint=first_step(expr),
                             requires_fraction=True, min_tree_depth=3))
            count += 1
            if count == 30:
                break
        if count != 30:
            raise RuntimeError(f'MASTER Target {target}: found only {count} fraction-required sets')
    integer_values.cache_clear()
    return made


def positive_tables(cards, divide=True):
    table = [{} for _ in range(1 << len(cards))]
    for i, n in enumerate(cards):
        table[1 << i][n] = str(n)
    for mask in range(1, 1 << len(cards)):
        if mask & (mask - 1) == 0:
            continue
        part = (mask - 1) & mask
        while part:
            rest = mask ^ part
            if part < rest:
                for a, ae in table[part].items():
                    for b, be in table[rest].items():
                        opts = [(a + b, '+', ae, be), (a * b, '*', ae, be)]
                        if a > b:
                            opts.append((a - b, '-', ae, be))
                        if b > a:
                            opts.append((b - a, '-', be, ae))
                        if divide and a % b == 0:
                            opts.append((a // b, '/', ae, be))
                        if divide and b % a == 0:
                            opts.append((b // a, '/', be, ae))
                        for value, op, x, y in opts:
                            if 0 < value <= 10**9 and value not in table[mask]:
                                table[mask][value] = f'({x}{op}{y})'
            part = (part - 1) & mask
    return table


def countdown():
    made = []
    seen = {(tuple(sorted(x['cards'])), x['target']) for x in OLD['countdown']}
    for attempt in range(120):
        deck = list(range(1, 11)) * 2
        R.shuffle(deck)
        cards = deck[:3] + R.sample((25, 50, 75, 100), 3)
        R.shuffle(cards)
        tables = positive_tables(cards)
        shorter = set().union(*(v.keys() for v in tables[1:63]))
        no_div = positive_tables(cards, False)
        without_division = set().union(*(v.keys() for v in no_div[1:]))
        targets = [n for n in tables[63] if 100 <= n <= 999 and n not in shorter and n not in without_division]
        R.shuffle(targets)
        for target in targets[:2]:
            signature = (tuple(sorted(cards)), target)
            if signature in seen:
                continue
            seen.add(signature)
            expr = tables[63][target]
            made.append(dict(cards=cards[:], target=target, witness=expr, hint=first_step(expr),
                             min_cards=6, division_required=True, large_cards=3))
            if len(made) == 30:
                return made
    raise RuntimeError('MASTER Countdown bounded search exhausted')


def operators():
    made = []
    seen = set()
    all_ops = tuple(itertools.product(range(4), repeat=5))
    for _ in range(1000):
        numbers = tuple(R.randrange(2, 10) for _ in range(6))
        by_value = {}
        for ops in all_ops:
            value = legacy.arith(numbers, ops)
            if value is not None and value.denominator == 1 and -999 <= value <= 999:
                by_value.setdefault(int(value), []).append(ops)
        choices = [(target, solutions) for target, solutions in by_value.items()
                   if 1 <= len(solutions) <= 3 and all(any(op < 2 for op in ops) and any(op >= 2 for op in ops)
                                                      and len(set(ops)) >= 3 for ops in solutions)]
        R.shuffle(choices)
        for target, solutions in choices[:1]:
            if (numbers, target) in seen:
                continue
            seen.add((numbers, target))
            made.append(dict(numbers=numbers, target=target, operators=solutions[0],
                             solution_count=len(solutions), combinations=1024))
        if len(made) == 30:
            return made
    raise RuntimeError('MASTER Operators bounded search exhausted')


def cross_candidates(ops, targets):
    rows = [[v for v in itertools.permutations(range(1, 10), 3)
             if legacy.arith(v, ops[2 * r:2 * r + 2]) == targets[r]] for r in range(3)]
    count = 0
    for a in rows[0]:
        aset = set(a)
        for b in rows[1]:
            if aset.intersection(b):
                continue
            used = aset | set(b)
            count += sum(not used.intersection(c) for c in rows[2])
    return count


def crossmath():
    # Both orientations must leave more whole-board candidates than every old Hard puzzle.
    baseline = [min(cross_candidates(p['ops'][:6], p['targets'][:3]),
                    cross_candidates(p['ops'][6:], p['targets'][3:]))
                for p in OLD['crossmath'] if p['difficulty'] == 2]
    threshold = max(baseline)
    made = []
    seen = {(tuple(p['ops']), tuple(p['targets'])) for p in OLD['crossmath']}
    for _ in range(30000):
        solution = R.sample(range(1, 10), 9)
        ops = [R.randrange(3) for _ in range(12)]
        if len(set(ops)) != 3:
            continue
        targets = [int(legacy.arith(solution[3*r:3*r+3], ops[2*r:2*r+2])) for r in range(3)]
        targets += [int(legacy.arith(solution[c::3], ops[6+2*c:8+2*c])) for c in range(3)]
        row_count = cross_candidates(ops[:6], targets[:3])
        if row_count <= threshold:
            continue
        col_count = cross_candidates(ops[6:], targets[3:])
        if col_count <= threshold or legacy.cross_count(ops, targets, [0]*9)[0] != 1:
            continue
        signature = (tuple(ops), tuple(targets))
        if signature in seen:
            continue
        seen.add(signature)
        made.append(dict(ops=ops, targets=targets, givens=[0]*9, solution=solution,
                         solution_count=1, row_only_boards=row_count, column_only_boards=col_count,
                         old_hard_max_min_candidates=threshold))
        if len(made) == 30:
            return made
    raise RuntimeError(f'MASTER Cross bounded search exhausted; threshold={threshold}, count={len(made)}')


def equations():
    made = []
    old = {p['equation'] for p in OLD['equation']}
    for mode, (length, count) in enumerate(((9, 3), (8, 2), (10, 3))):
        seen = set()
        for _ in range(100000):
            nums = [R.randrange(2, 20 if length == 10 else 10) for _ in range(count + 1)]
            ops = [R.randrange(4) for _ in range(count)]
            if not any(op < 2 for op in ops) or not any(op >= 2 for op in ops):
                continue
            value = legacy.arith(nums, ops)
            if value is None or value.denominator != 1 or value < 0:
                continue
            text = ''.join(str(n) + ('+-*/'[ops[i]] if i < count else '') for i, n in enumerate(nums)) + '=' + str(value)
            if len(text) != length or text in seen or text in old:
                continue
            # Standard precedence must matter, so a left-to-right mistake differs.
            left = Fraction(nums[0])
            for op, n in zip(ops, nums[1:]):
                left = (left + n, left - n, left * n, left / n)[op]
            if left == value:
                continue
            seen.add(text)
            made.append(dict(mode=mode, length=length, equation=text, operator_count=count,
                             mixed_precedence=True))
            if len(seen) == 30:
                break
        if len(seen) != 30:
            raise RuntimeError(f'MASTER Equation mode {mode} bounded search exhausted')
    return made


def write_header(p):
    a = legacy.arr
    lines = ['/* Original verified MASTER additions. Legacy IDs/arrays remain unchanged. */',
             '#ifndef GUESSCALC_MASTER_H', '#define GUESSCALC_MASTER_H',
             'typedef struct { unsigned char count,clues[13][6],matches[13],solution[6]; } GcMasterMindPack;',
             'static const GcMasterMindPack gc_master_mind[30]={']
    for x in p['number_mind']:
        lines.append('{%d,{%s},%s,%s},' % (len(x['clues']), ','.join(a(g) for g, _ in x['clues']),
                                          a([v for _, v in x['clues']]), a(x['solution'])))
    lines += ['};', 'typedef struct { short p[11],solution; } GcMasterLockPack;',
              'static const GcMasterLockPack gc_master_lock[30]={']
    lines += ['{%s,%d},' % (a(x['params']), x['solution']) for x in p['clue_lock']]
    lines += ['};', 'static const GcSequencePack gc_master_sequence[30]={']
    lines += ['{%s,%s,%d},' % (a(x['sequence']), a(x['params']), x['family']) for x in p['sequence']]
    lines += ['};']
    for name in ('target', 'countdown'):
        lines += [f'static const GcCardPack gc_master_{name}[{len(p[name])}]={{']
        lines += ['{%s,%d,%s,%s},' % (a(x['cards']), x['target'], json.dumps(x['witness']), json.dumps(x['hint'])) for x in p[name]]
        lines += ['};']
    lines += ['typedef struct { unsigned char numbers[6],ops[5]; short target; } GcMasterOperatorsPack;',
              'static const GcMasterOperatorsPack gc_master_operators[30]={']
    lines += ['{%s,%s,%d},' % (a(x['numbers']), a(x['operators']), x['target']) for x in p['operators']]
    lines += ['};', 'static const GcCrossPack gc_master_cross[30]={']
    lines += ['{%s,%s,%s,%s},' % (a(x['ops']), a(x['givens']), a(x['solution']), a(x['targets'])) for x in p['crossmath']]
    lines += ['};', 'static const char gc_master_equation[90][11]={']
    lines += [json.dumps(x['equation']) + ',' for x in p['equation']]
    lines += ['};', '#endif']
    (OUT / 'guesscalc_master.h').write_text('\n'.join(lines) + '\n')


def main():
    p = dict(rules_version=2, seed='0x4D41535445524743', original_records=900, external_records=0)
    specs = [('number_mind', mind, 3, 90), ('clue_lock', lock, 4, 90), ('sequence', sequence, 5, 90),
             ('target', targets, 6, 180), ('countdown', countdown, 7, 90), ('operators', operators, 8, 0),
             ('crossmath', crossmath, 9, 90), ('equation', equations, 2, 270)]
    for name, generate, game, start in specs:
        p[name] = generate()
        for i, record in enumerate(p[name]):
            record.update(difficulty=3, game_id=game, puzzle_id=start+i, rules_version=2)
        print(name, len(p[name]), flush=True)
    (OUT / 'guesscalc_master.json').write_text(json.dumps(p, indent=2) + '\n')
    write_header(p)


if __name__ == '__main__':
    main()
