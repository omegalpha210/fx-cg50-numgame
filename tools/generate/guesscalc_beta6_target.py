#!/usr/bin/env python3
"""Build the fixed-target revision-6 Make Target bank.

The five target values are the complete new-play domain. The C header stores
only cards, target, offsets into per-level answer strings and a first-step
hint. The old revision-3/4/5 constructors and IDs are never changed.
"""
import argparse
import ast
import collections
import concurrent.futures
from fractions import Fraction
import hashlib
import json
from pathlib import Path
import random
import time

from guesscalc_exact import Solver, SOURCE, executable, target_deck_v5
from guesscalc_beta5_target import accepted, annotate

ROOT = Path(__file__).resolve().parents[2]
JSON = ROOT / 'assets/guesscalc_target_beta6.json'
HEADER = ROOT / 'assets/guesscalc_target_beta6.h'
TARGETS = (10, 24, 50, 100, 200)
LEVELS = ('EASY', 'NORMAL', 'HARD', 'MASTER')
PER_CELL = 200
START_ID = 16240
MAX_INDEX = 12000


def easy_candidate(index, target):
    """Several elementary four-leaf constructions, before exact grading."""
    rng = random.Random(0xB6E15A11 ^ target * 0x9E3779B9 ^ index * 0x85EBCA6B)
    variant = index % 5
    if variant == 0:
        a, b, c = rng.randint(2, 40), rng.randint(2, 40), rng.randint(1, 50)
        d = a * b + c - target
        witness = f'({a}*{b})+{c}-{d}'
    elif variant == 1:
        a, b, c = (rng.randint(1, 120) for _ in range(3))
        d = a + b + c - target
        witness = f'{a}+{b}+{c}-{d}'
    elif variant == 2:
        divisors = [k for k in range(2, target + 1) if target % k == 0 and target // k >= 2]
        k = rng.choice(divisors)
        total = target // k
        a = rng.randint(1, total - 1)
        b = total - a
        d = rng.randint(1, 80)
        c = d + k
        witness = f'({a}+{b})*({c}-{d})'
    elif variant == 3:
        q = rng.randint(2, 30)
        b = rng.randint(1, 60)
        a = b + q
        c = rng.randint(2, 30)
        d = target - q * c
        witness = f'({a}-{b})*{c}+{d}'
    else:
        a, b, c = rng.randint(2, 45), rng.randint(2, 45), rng.randint(2, 30)
        numerator = a * b - target
        d = numerator // c if numerator % c == 0 else 0
        witness = f'{a}*{b}-{c}*{d}'
    cards = [a, b, c, d]
    if not all(1 <= x <= 999 for x in cards):
        return [], ''
    rng.shuffle(cards)
    return cards, witness


def expression_result(expression):
    """Independent rational AST evaluation and occurrence extraction."""
    leaves = []

    def visit(node):
        if isinstance(node, ast.Constant) and type(node.value) is int:
            leaves.append(node.value)
            return Fraction(node.value)
        if isinstance(node, ast.UnaryOp) and isinstance(node.op, ast.USub):
            return -visit(node.operand)
        if isinstance(node, ast.BinOp):
            a, b = visit(node.left), visit(node.right)
            if isinstance(node.op, ast.Add):
                return a + b
            if isinstance(node.op, ast.Sub):
                return a - b
            if isinstance(node.op, ast.Mult):
                return a * b
            if isinstance(node.op, ast.Div) and b:
                return a / b
        raise ValueError('illegal expression: ' + expression)

    return visit(ast.parse(expression, mode='eval').body), sorted(leaves)


def first_step(cards, witness):
    """Find a real two-card step and address displayed card occurrences."""
    ops = {ast.Add: '+', ast.Sub: '-', ast.Mult: '*', ast.Div: '/'}
    root = ast.parse(witness, mode='eval').body

    def walk(node):
        if isinstance(node, ast.BinOp):
            if isinstance(node.left, ast.Constant) and isinstance(node.right, ast.Constant):
                a, b = node.left.value, node.right.value
                if type(a) is int and type(b) is int and type(node.op) in ops:
                    ia = cards.index(a)
                    ib = next(i for i, x in enumerate(cards) if x == b and i != ia)
                    return ia, ib, ops[type(node.op)]
            return walk(node.left) or walk(node.right)
        if isinstance(node, ast.UnaryOp):
            return walk(node.operand)
        return None

    pair = walk(root)
    if pair is None:
        raise ValueError('construction lacks a two-card hint: ' + witness)
    return pair


def signature(expression):
    def walk(node):
        if isinstance(node, ast.Constant):
            return 'n'
        if isinstance(node, ast.UnaryOp):
            return '-' + walk(node.operand)
        return '(' + walk(node.left) + {ast.Add: '+', ast.Sub: '-', ast.Mult: '*', ast.Div: '/'}[type(node.op)] + walk(node.right) + ')'
    return walk(ast.parse(expression, mode='eval').body)


def generate_target(target):
    started = time.monotonic()
    rows = []
    seen = set()
    stats = []
    with Solver() as solver:
        for difficulty in range(4):
            accepted_count = duplicate_count = graded_rejects = invalid = 0
            for index in range(1, MAX_INDEX + 1):
                if difficulty == 0:
                    cards, witness = easy_candidate(index, target)
                else:
                    cards, witness, _ = target_deck_v5(index, difficulty, target)
                if not cards:
                    invalid += 1
                    continue
                assert len(cards) == (4 if difficulty < 2 else difficulty + 3)
                assert all(1 <= x <= 999 for x in cards)
                assert expression_result(witness) == (Fraction(target), sorted(cards)), (difficulty, target, index)
                key = (target, tuple(sorted(cards)))
                if key in seen:
                    duplicate_count += 1
                    continue
                # Count rejected signatures too; a later construction of the
                # same deck cannot change the exact minimum.
                seen.add(key)
                result = solver.solve(cards, target).get(Fraction(target))
                if result is None or not accepted(difficulty, result):
                    graded_rejects += 1
                    continue
                exact = annotate(result)
                answer = exact['graded']['expression']
                if len(answer) >= 40 or expression_result(answer) != (Fraction(target), sorted(cards)):
                    raise AssertionError(('native answer width / occurrence', difficulty, target, index, answer))
                hint = first_step(cards, witness)
                rows.append(dict(difficulty=difficulty, target=target,
                                 candidate_index=index, cards=cards,
                                 construction_witness=witness,
                                 answer=answer, hint=list(hint),
                                 easiest_template=signature(answer), exact=exact))
                accepted_count += 1
                if accepted_count == PER_CELL:
                    break
            stats.append(dict(difficulty=difficulty, target=target,
                              accepted=accepted_count, last_index=index,
                              invalid_constructions=invalid,
                              duplicate_candidates=duplicate_count,
                              rejected_by_exact_grade=graded_rejects,
                              seconds=round(time.monotonic()-started, 3)))
            print('Make Target', target, LEVELS[difficulty], accepted_count,
                  'index', index, 'elapsed', round(time.monotonic()-started, 1), flush=True)
            if accepted_count != PER_CELL:
                break
    return target, rows, stats


def validate(data, re_solve=False):
    assert data['revision'] == 6 and data['targets'] == list(TARGETS)
    assert data['per_target_difficulty'] == PER_CELL and len(data['records']) == 4000
    assert data['solver_sha256'] == hashlib.sha256(SOURCE.read_bytes()).hexdigest()
    seen = set()
    solver = Solver() if re_solve else None
    try:
        for ordinal, record in enumerate(data['records']):
            difficulty, rest = divmod(ordinal, 5 * PER_CELL)
            ti, slot = divmod(rest, PER_CELL)
            target = TARGETS[ti]
            assert (record['puzzle_id'], record['difficulty'], record['target']) == (START_ID+ordinal, difficulty, target)
            cards = record['cards']
            assert len(cards) == (4 if difficulty < 2 else difficulty + 3)
            assert all(1 <= c <= 999 for c in cards)
            key = (target, tuple(sorted(cards)))
            assert key not in seen, ('duplicate base puzzle', ordinal, key)
            seen.add(key)
            assert expression_result(record['construction_witness']) == (Fraction(target), sorted(cards))
            assert expression_result(record['answer']) == (Fraction(target), sorted(cards))
            assert len(record['answer']) < 40
            a, b, op = record['hint']
            assert 0 <= a < len(cards) and 0 <= b < len(cards) and a != b and op in '+-*/'
            assert record['hint'] == list(first_step(cards, record['construction_witness']))
            exact = record['exact']
            assert accepted(difficulty, exact)
            assert exact['graded']['expression'] == record['answer']
            assert record['easiest_template'] == signature(record['answer'])
            if re_solve:
                actual = annotate(solver.solve(cards, target)[Fraction(target)])
                assert actual == exact, ('solver mismatch', ordinal)
                if (ordinal+1) % 200 == 0:
                    print('Make Target re-solved', ordinal+1, flush=True)
    finally:
        if solver is not None:
            solver.close()


def verify_records(records):
    with Solver() as solver:
        for r in records:
            actual = annotate(solver.solve(r['cards'], r['target'])[Fraction(r['target'])])
            assert actual == r['exact'], ('solver mismatch', r['puzzle_id'])
    print('Make Target re-solved target', records[0]['target'],len(records),flush=True)
    return len(records)


def c_header(data):
    rows = data['records']
    blobs = []
    offset = {}
    for d in range(4):
        pieces = []
        position = 0
        for r in rows[d*1000:(d+1)*1000]:
            offset[r['puzzle_id']] = position
            pieces.append(r['answer'])
            position += len(r['answer']) + 1
        assert position <= 65535, ('answer offset overflow', d, position)
        blobs.append(pieces)
    out = ['/* Generated revision-6 fixed-target bank: 4,000 distinct base puzzles. */',
           '#ifndef GUESSCALC_TARGET_BETA6_H', '#define GUESSCALC_TARGET_BETA6_H',
           '#ifndef GC_BETA6_CARD_RECORD_DEFINED', '#define GC_BETA6_CARD_RECORD_DEFINED',
           'typedef struct { int16_t cards[6]; uint16_t target, answer_offset; uint8_t hint_a,hint_b,hint_op; } GcBeta6CardRecord;',
           '#endif',
           'static const GcBeta6CardRecord gc_target_beta6[4000]={']
    for r in rows:
        cards = r['cards'] + [0] * (6-len(r['cards']))
        a,b,op = r['hint']
        out.append('{{%s},%d,%d,%d,%d,\'%s\'},' % (','.join(map(str,cards)), r['target'], offset[r['puzzle_id']], a,b,op))
    out.append('};')
    for d, pieces in enumerate(blobs):
        out.append(f'static const char gc_target_beta6_answer_{d}[]=')
        # A C octal terminator must be separated from the next literal;
        # JSON's ``\u0000`` spelling is not a portable C string escape.
        out += [json.dumps(p)[:-1] + r'\0"' for p in pieces]
        out[-1] += ';'
    out += ['static const char *const gc_target_beta6_answer[4]={gc_target_beta6_answer_0,gc_target_beta6_answer_1,gc_target_beta6_answer_2,gc_target_beta6_answer_3};',
            '#endif', '']
    return '\n'.join(out)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--generate', action='store_true')
    mode.add_argument('--check', action='store_true')
    mode.add_argument('--verify', action='store_true')
    parser.add_argument('--jobs', type=int, default=5)
    args = parser.parse_args()
    start = time.monotonic()
    executable()
    if args.generate:
        with concurrent.futures.ProcessPoolExecutor(max_workers=args.jobs) as pool:
            batches = list(pool.map(generate_target, TARGETS))
        shortage = [(t, s) for t, _, stats in batches for s in stats if s['accepted'] != PER_CELL]
        if shortage:
            print('SHORTAGE', json.dumps(shortage, indent=2), flush=True)
            raise SystemExit(1)
        lookup = {(r['difficulty'], r['target']): [] for _, records, _ in batches for r in records}
        for _, records, _ in batches:
            for r in records:
                lookup[r['difficulty'], r['target']].append(r)
        records = []
        for d in range(4):
            for target in TARGETS:
                part = lookup[d, target]
                assert len(part) == PER_CELL
                for r in part:
                    r['puzzle_id'] = START_ID+len(records)
                    records.append(r)
        data = dict(revision=6, targets=list(TARGETS), per_target_difficulty=PER_CELL,
                    external_records=0, construction='beta6-five-fixed-targets-v1',
                    solver_sha256=hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
                    records=records, generation_stats=[s for _,_,stats in batches for s in stats])
        validate(data)
        JSON.write_text(json.dumps(data, indent=2)+'\n')
        HEADER.write_text(c_header(data))
    else:
        data = json.loads(JSON.read_text())
        validate(data)
        assert HEADER.read_text() == c_header(data)
        if args.verify:
            batches=[[r for r in data['records'] if r['target']==target] for target in TARGETS]
            with concurrent.futures.ProcessPoolExecutor(max_workers=args.jobs) as pool:
                assert sum(pool.map(verify_records,batches))==4000
    print('PASS Make Target beta.6', len(data['records']), 'records, seconds', round(time.monotonic()-start, 2), flush=True)


if __name__ == '__main__':
    main()
