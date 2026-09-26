#!/usr/bin/env python3
"""Build and verify 800 revision-5 Countdown base puzzles.

The generated cards preserve the existing 1..10 twice / 25,50,75,100 deck
domain and 100..999 target range. Reachability and minimum card use are
computed over every card subset with positive integer intermediates and exact
division. A separate unchanged C++ solver rechecks every emitted record.
"""
import argparse
import collections
import hashlib
import json
from pathlib import Path
import random
import time

from guesscalc_exact import Solver, SOURCE, executable
from guesscalc_master import positive_tables
from guesscalc_beta6_target import expression_result, first_step

ROOT = Path(__file__).resolve().parents[2]
JSON = ROOT / 'assets/guesscalc_countdown_beta6.json'
HEADER = ROOT / 'assets/guesscalc_countdown_beta6.h'
OLD = json.loads((ROOT / 'assets/guesscalc_packs.json').read_text())
MASTER = json.loads((ROOT / 'assets/guesscalc_master.json').read_text())
BETA4 = json.loads((ROOT / 'assets/guesscalc_beta4.json').read_text())
SEED = 0xB6C0D021
PER_LEVEL = 200
START_ID = 200
QUOTAS = ({2: 13, 3: 80, 4: 87, 5: 20},
          {2: 7, 3: 67, 4: 120, 5: 6},
          {4: 127, 5: 73},
          {6: 200})
MAX_DECK_ATTEMPTS = 20000


def domain(cards, difficulty):
    if len(cards) != 6:
        return False
    small = [c for c in cards if 1 <= c <= 10]
    large = [c for c in cards if c > 10]
    return (len(large) == (difficulty+1 if difficulty < 3 else 3)
            and len(set(large)) == len(large)
            and set(large) <= {25, 50, 75, 100}
            and len(small) + len(large) == 6
            and max(collections.Counter(small).values(), default=0) <= 2)


def positive_eval(expression):
    """Check all tree intermediates, not just the final result."""
    import ast
    seen = []

    def go(node):
        if isinstance(node, ast.Constant) and type(node.value) is int:
            seen.append(node.value)
            return node.value
        if not isinstance(node, ast.BinOp):
            raise ValueError('illegal Countdown expression')
        a, b = go(node.left), go(node.right)
        if isinstance(node.op, ast.Add):
            v = a+b
        elif isinstance(node.op, ast.Sub):
            v = a-b
        elif isinstance(node.op, ast.Mult):
            v = a*b
        elif isinstance(node.op, ast.Div) and b and a % b == 0:
            v = a//b
        else:
            raise ValueError('non-integer division / illegal op')
        if not 1 <= v <= 1_000_000_000:
            raise ValueError('nonpositive or oversized intermediate')
        return v

    return go(ast.parse(expression, mode='eval').body), sorted(seen)


def exact_profile(cards, target, table=None, no_division=None):
    table = positive_tables(cards) if table is None else table
    found = [(mask.bit_count(), mask, table[mask][target])
             for mask in range(1, 64) if target in table[mask]]
    if not found:
        return None
    minimum = min(n for n, _, _ in found)
    chosen = min((item for item in found if item[0] == minimum),
                 key=lambda x: (len(x[2]), x[2]))
    if no_division is None:
        no_division = positive_tables(cards, False)
    division_required = not any(target in no_division[mask] for mask in range(1, 64))
    return dict(min_cards=minimum, division_required=division_required,
                expression=chosen[2], subset_mask=chosen[1])


def inherited(difficulty):
    if difficulty < 2:
        return [p for p in OLD['countdown'] if p['difficulty'] == difficulty]
    if difficulty == 2:
        return BETA4['countdown']
    return MASTER['countdown']


def make_record(cards, target, difficulty, profile, source=None):
    witness = profile['expression']
    assert len(witness) < 40, witness
    assert positive_eval(witness)[0] == target
    hint = first_step(cards, witness)
    return dict(difficulty=difficulty, target=target, cards=list(cards),
                answer=witness, hint=list(hint), min_cards=profile['min_cards'],
                division_required=profile['division_required'],
                source_puzzle_id=source)


def generate():
    rng = random.Random(SEED)
    output = []
    statistics = []
    for d in range(4):
        quota = collections.Counter(QUOTAS[d])
        seen = set()
        deck_use = collections.Counter()
        records = []
        for p in inherited(d):
            cards = p['cards']
            assert domain(cards, d) and 100 <= p['target'] <= 999
            profile = exact_profile(cards, p['target'])
            assert profile is not None
            assert profile['min_cards'] in quota and quota[profile['min_cards']] > 0
            if d == 2:
                assert profile['min_cards'] >= 4
            if d == 3:
                assert profile['min_cards'] == 6 and profile['division_required']
            key = (p['target'], tuple(sorted(cards)))
            assert key not in seen
            seen.add(key)
            deck_use[key[1]] += 1
            quota[profile['min_cards']] -= 1
            records.append(make_record(cards, p['target'], d, profile, p['puzzle_id']))
        attempts = 0
        candidates = 0
        while len(records) < PER_LEVEL and attempts < MAX_DECK_ATTEMPTS:
            attempts += 1
            small_pool = list(range(1, 11))*2
            rng.shuffle(small_pool)
            count_large = d+1 if d < 3 else 3
            cards = small_pool[:6-count_large] + rng.sample((25,50,75,100), count_large)
            rng.shuffle(cards)
            deck_key = tuple(sorted(cards))
            if deck_use[deck_key] >= 2:
                continue
            table = positive_tables(cards)
            min_by_target = {}
            answer_by_target = {}
            for mask in range(1, 64):
                card_count = mask.bit_count()
                if card_count not in quota or quota[card_count] <= 0:
                    continue
                for target, expression in table[mask].items():
                    if 100 <= target <= 999 and (target, deck_key) not in seen:
                        old_count = min_by_target.get(target)
                        if old_count is None or card_count < old_count:
                            min_by_target[target] = card_count
                            answer_by_target[target] = (mask, expression)
            # The shortest count must consider *every* subset. The loop above
            # only visits currently desired counts, so recalculate it below.
            possible = []
            no_division = positive_tables(cards, False) if d == 3 else None
            for target in min_by_target:
                shortest = min(mask.bit_count() for mask in range(1,64) if target in table[mask])
                if shortest not in quota or quota[shortest] <= 0:
                    continue
                if d == 3 and any(target in no_division[mask] for mask in range(1,64)):
                    continue
                possible.append((target, shortest))
            rng.shuffle(possible)
            for target, shortest in possible:
                profile = exact_profile(cards, target, table, no_division)
                assert profile and profile['min_cards'] == shortest
                if d == 3 and not profile['division_required']:
                    continue
                if d == 2 and profile['min_cards'] < 4:
                    continue
                if len(profile['expression']) >= 40:
                    continue
                key = (target, deck_key)
                assert key not in seen
                seen.add(key)
                deck_use[deck_key] += 1
                quota[shortest] -= 1
                records.append(make_record(cards, target, d, profile))
                candidates += 1
                break
            if attempts % 100 == 0:
                print('Countdown level', d, 'records', len(records), 'deck attempts', attempts, flush=True)
        statistics.append(dict(difficulty=d, accepted=len(records), deck_attempts=attempts,
                               new_candidates=candidates, remaining_quota=dict(quota),
                               unique_decks=len(deck_use)))
        print('Countdown level', d, 'accepted', len(records), 'attempts', attempts,
              'remaining', dict(quota), flush=True)
        if len(records) != PER_LEVEL:
            raise RuntimeError(('Countdown shortage', d, len(records), dict(quota)))
        for r in records:
            r['puzzle_id'] = START_ID+len(output)
            output.append(r)
    return dict(revision=5, per_difficulty=PER_LEVEL, external_records=0,
                deck_seed=SEED, solver_sha256=hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
                records=output, generation_stats=statistics)


def validate(data, re_solve=False):
    assert data['revision'] == 5 and data['per_difficulty'] == 200
    assert len(data['records']) == 800
    assert data['solver_sha256'] == hashlib.sha256(SOURCE.read_bytes()).hexdigest()
    seen = set()
    solver = Solver(integer=True) if re_solve else None
    try:
        for index, r in enumerate(data['records']):
            d, _ = divmod(index, PER_LEVEL)
            assert (r['puzzle_id'], r['difficulty']) == (START_ID+index, d)
            cards, target = r['cards'], r['target']
            assert domain(cards,d) and 100 <= target <= 999
            key = (target, tuple(sorted(cards)))
            assert key not in seen, ('duplicate Countdown base', key)
            seen.add(key)
            profile = exact_profile(cards, target)
            assert profile is not None
            assert profile['min_cards'] == r['min_cards']
            assert profile['division_required'] == r['division_required']
            if d == 2:
                assert profile['min_cards'] >= 4
            elif d == 3:
                assert profile['min_cards'] == 6 and profile['division_required']
            value, literals = positive_eval(r['answer'])
            assert value == target and not (collections.Counter(literals)-collections.Counter(cards))
            assert len(r['answer']) < 40
            a,b,op = r['hint']
            assert 0 <= a < 6 and 0 <= b < 6 and a != b and op in '+-*/'
            assert r['hint'] == list(first_step(cards, r['answer']))
            if re_solve:
                # Reuse the original C++ exact integer subset solver as a
                # genuinely separate implementation of the native rules.
                shortest_found = False
                for count in range(1, profile['min_cards']+1):
                    for indices in __import__('itertools').combinations(range(6), count):
                        subset = [cards[i] for i in indices]
                        result = solver.solve(subset, target).get(target)
                        if count < profile['min_cards']:
                            assert result is None, (index, count, subset)
                        else:
                            shortest_found |= result is not None
                assert shortest_found, ('C++ solver found no shortest solution', index)
                if (index+1) % 100 == 0:
                    print('Countdown C++ subset rechecked', index+1, flush=True)
    finally:
        if solver is not None:
            solver.close()


def c_header(data):
    rows = data['records']
    position = 0
    offsets = []
    pieces = []
    for r in rows:
        offsets.append(position)
        pieces.append(r['answer'])
        position += len(r['answer'])+1
    assert position <= 65535, ('Countdown answer offset overflow', position)
    out = ['/* Generated revision-5 Countdown bank: 800 distinct base puzzles. */',
           '#ifndef GUESSCALC_COUNTDOWN_BETA6_H', '#define GUESSCALC_COUNTDOWN_BETA6_H',
           '#ifndef GC_BETA6_CARD_RECORD_DEFINED', '#define GC_BETA6_CARD_RECORD_DEFINED',
           'typedef struct { int16_t cards[6]; uint16_t target, answer_offset; uint8_t hint_a,hint_b,hint_op; } GcBeta6CardRecord;',
           '#endif',
           'static const GcBeta6CardRecord gc_countdown_beta6[800]={']
    for r, offset in zip(rows, offsets):
        a,b,op = r['hint']
        out.append('{{%s},%d,%d,%d,%d,\'%s\'},' % (','.join(map(str,r['cards'])),r['target'],offset,a,b,op))
    out += ['};', 'static const char gc_countdown_beta6_answer[]=']
    out += [json.dumps(p)[:-1]+r'\0"' for p in pieces]
    out[-1] += ';'
    out += ['#endif', '']
    return '\n'.join(out)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--generate', action='store_true')
    mode.add_argument('--check', action='store_true')
    mode.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    start = time.monotonic()
    executable()
    if args.generate:
        data = generate()
        validate(data)
        JSON.write_text(json.dumps(data, indent=2)+'\n')
        HEADER.write_text(c_header(data))
    else:
        data = json.loads(JSON.read_text())
        validate(data, re_solve=args.verify)
        assert HEADER.read_text() == c_header(data)
    print('PASS Countdown beta.6', len(data['records']), 'records, seconds', round(time.monotonic()-start,2), flush=True)


if __name__ == '__main__':
    main()
