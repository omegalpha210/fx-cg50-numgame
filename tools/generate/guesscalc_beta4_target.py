#!/usr/bin/env python3
"""Revision4 target-indexed bank: 1000 targets × four levels × two records.

Generation is deterministic and bounded to 512 candidate indices per cell.
Every accepted deck is exhaustively graded under the real rational/unary rules.
Only uint16 construction indices enter the native header (16,000 bytes).
"""
import argparse
import ast
import collections
import concurrent.futures
import csv
from fractions import Fraction
import hashlib
import io
import json
from pathlib import Path
import time
from guesscalc_exact import Solver, SOURCE, executable, target_deck, target_deck_v4

ROOT = Path(__file__).resolve().parents[2]
JSON = ROOT/'assets/guesscalc_target_beta4.json'
HEADER = ROOT/'assets/guesscalc_target_beta4.h'
CSV = ROOT/'assets/guesscalc_make_target_complexity.csv'
LEVELS = ('EASY', 'NORMAL', 'HARD', 'MASTER')
_solver = None


def initializer():
    global _solver
    _solver = Solver()


def magnitude(text):
    maximum = Fraction(0)
    def visit(node):
        nonlocal maximum
        if isinstance(node, ast.Constant):
            value = Fraction(node.value)
        elif isinstance(node, ast.UnaryOp):
            value = -visit(node.operand)
        else:
            a, b = visit(node.left), visit(node.right)
            value = a+b if isinstance(node.op, ast.Add) else a-b if isinstance(node.op, ast.Sub) else a*b if isinstance(node.op, ast.Mult) else a/b
        maximum = max(maximum, abs(value))
        return value
    value = visit(ast.parse(text, mode='eval').body)
    return str(maximum), value


def annotate(result):
    result['max_intermediate_magnitude'] = magnitude(result['easiest_expression'])[0]
    result['graded']['max_intermediate_magnitude'] = magnitude(result['graded']['expression'])[0]
    # A small inverse-frequency modifier follows (never overrides) exact cost.
    rarity = max(0, 16-(result['canonical_count'].bit_length()-1))
    result['rarity_bonus'] = rarity
    result['minimal_complexity_score'] = 32*result['graded']['score']+rarity
    return result


def accepted(d, result):
    g = result['graded']
    if d == 0:
        return g['fractional_steps'] == g['division_steps'] == 0 and g['score'] <= 259
    if d == 1:
        return g['score'] == 8450
    if d == 2:
        return g['fractional_steps'] == 0 and g['division_steps'] == 1 and 8451 <= g['score'] <= 8715
    return g['fractional_steps'] >= 1


def make_target(target):
    output, attempts, rejected, duplicate = [], [], [], []
    for difficulty in range(4):
        seen = set();made = [];fail = 0;dups = 0
        for index in range(1, 513):
            cards, witness, _ = target_deck_v4(index, difficulty, target)
            assert len(witness) < 40 and all(1 <= v <= 32767 for v in cards)
            assert magnitude(witness)[1] == target
            signature = tuple(sorted(cards))
            if signature in seen:
                dups += 1
                continue
            seen.add(signature)
            result = _solver.solve(cards, target).get(Fraction(target))
            assert result is not None, (difficulty, target, index, cards, witness)
            if not accepted(difficulty, result):
                fail += 1
                continue
            made.append(dict(puzzle_id=240+(difficulty*1000+target-1)*2+len(made), difficulty=difficulty,
                             target=target, candidate_index=index, cards=cards, generator_witness=witness,
                             exact=annotate(result), old_difficulty=None, new_difficulty=LEVELS[difficulty],
                             decision='ACCEPTED / EXHAUSTIVE COMPLEXITY GRADING'))
            if len(made) == 2:
                break
        assert len(made) == 2, ('bounded candidate exhaustion', difficulty, target)
        output.extend(made);attempts.append(index);rejected.append(fail);duplicate.append(dups)
    return dict(target=target, records=output, attempts=attempts, rejects=rejected, duplicate_candidates=duplicate)


def verify_target(records):
    for record in records:
        d, target, index = record['difficulty'], record['target'], record['candidate_index']
        cards, witness, _ = target_deck_v4(index, d, target)
        assert record['cards'] == cards and record['generator_witness'] == witness
        result = annotate(_solver.solve(cards, target)[Fraction(target)])
        assert result == record['exact'] and accepted(d, result), (d, target, index)
    return len(records)


def header(data):
    indices = [r['candidate_index'] for r in data['records']]
    assert len(indices) == 8000 and max(indices) <= 65535
    rows = [','.join(map(str, indices[i:i+20]))+',' for i in range(0, len(indices), 20)]
    return '\n'.join(['/* Original revision4 exact-graded target seeds. Host metadata is not linked. */',
                      '#ifndef GUESSCALC_TARGET_BETA4_H', '#define GUESSCALC_TARGET_BETA4_H',
                      'static const unsigned short gc_target_beta4_index[8000]={', *rows, '};', '#endif', ''])


def csv_text(data):
    fields = ['puzzle_id', 'difficulty', 'cards', 'target', 'minimal_complexity_score', 'fractional_steps',
              'division_steps', 'subtraction_steps', 'tree_depth', 'max_denominator', 'solution_count',
              'easiest_expression', 'generator_witness', 'old_difficulty', 'new_difficulty', 'decision',
              'unary_steps', 'negative_intermediate_steps', 'max_intermediate_magnitude', 'raw_minimal_tuple']
    out = io.StringIO();writer = csv.DictWriter(out, fieldnames=fields, lineterminator='\n');writer.writeheader()
    for r in data['records']:
        g = r['exact']['graded']
        row = dict(puzzle_id=r['puzzle_id'], difficulty=LEVELS[r['difficulty']], cards=' '.join(map(str, r['cards'])),
                   target=r['target'], minimal_complexity_score=r['exact']['minimal_complexity_score'],
                   solution_count=r['exact']['canonical_count'], easiest_expression=g['expression'],
                   generator_witness=r['generator_witness'], old_difficulty='NEW RECORD', new_difficulty=r['new_difficulty'],
                   decision=r['decision'], raw_minimal_tuple=json.dumps(r['exact']['minimal_tuple']))
        row.update({key: g[key] for key in ['fractional_steps', 'division_steps', 'subtraction_steps', 'tree_depth',
                                          'max_denominator', 'unary_steps', 'negative_intermediate_steps', 'max_intermediate_magnitude']})
        writer.writerow(row)
    return out.getvalue()


def validate(data):
    assert data['revision'] == 4 and data['targets'] == 1000 and data['per_target_level'] == 2
    assert len(data['records']) == 8000
    keys = set()
    for ordinal, r in enumerate(data['records']):
        d, rem = divmod(ordinal, 2000);target, slot = divmod(rem, 2);target += 1
        assert (r['puzzle_id'], r['difficulty'], r['target']) == (240+ordinal, d, target)
        cards, witness, _ = target_deck_v4(r['candidate_index'], d, target)
        assert (cards, witness) == (r['cards'], r['generator_witness']) and accepted(d, r['exact'])
        key = (target, tuple(sorted(cards)));assert key not in keys;keys.add(key)
        assert magnitude(witness)[1] == target and magnitude(r['exact']['graded']['expression'])[1] == target
    assert data['solver_sha256'] == hashlib.sha256(SOURCE.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--generate', action='store_true');mode.add_argument('--check', action='store_true');mode.add_argument('--verify', action='store_true')
    parser.add_argument('--jobs', type=int, default=4)
    args = parser.parse_args();start = time.monotonic();executable()
    if args.generate:
        batches = []
        with concurrent.futures.ProcessPoolExecutor(max_workers=args.jobs, initializer=initializer) as pool:
            for batch in pool.map(make_target, range(1, 1001), chunksize=4):
                batches.append(batch)
                if len(batches) % 50 == 0:
                    print('targets', len(batches), 'accepted records', 8*len(batches), 'elapsed', round(time.monotonic()-start, 1), flush=True)
        records = sorted((r for batch in batches for r in batch['records']), key=lambda r: r['puzzle_id'])
        data = dict(revision=4, targets=1000, per_target_level=2, external_records=0, construction_version='beta4-xorshift-v1',
                    solver_sha256=hashlib.sha256(SOURCE.read_bytes()).hexdigest(), records=records,
                    attempts_by_level=[sum(b['attempts'][d] for b in batches) for d in range(4)],
                    rejects_by_level=[sum(b['rejects'][d] for b in batches) for d in range(4)],
                    duplicate_candidates_by_level=[sum(b['duplicate_candidates'][d] for b in batches) for d in range(4)])
        validate(data);JSON.write_text(json.dumps(data, indent=2)+'\n');HEADER.write_text(header(data));CSV.write_text(csv_text(data))
    else:
        data = json.loads(JSON.read_text());validate(data)
        assert HEADER.read_text() == header(data) and CSV.read_text() == csv_text(data)
        if args.verify:
            by_target = [[r for r in data['records'] if r['target'] == target] for target in range(1, 1001)]
            done = 0
            with concurrent.futures.ProcessPoolExecutor(max_workers=args.jobs, initializer=initializer) as pool:
                for count in pool.map(verify_target, by_target, chunksize=4):
                    done += count
                    if done % 800 == 0:
                        print('verified', done, 'elapsed', round(time.monotonic()-start, 1), flush=True)
            assert done == 8000
    print('PASS', 8000, 'records', 'seconds', round(time.monotonic()-start, 2), flush=True)


if __name__ == '__main__':
    main()
