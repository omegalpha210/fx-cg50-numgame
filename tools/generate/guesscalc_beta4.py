#!/usr/bin/env python3
"""Original revision4 Countdown HARD and Sequence E/N additions.
Old arrays and puzzle IDs remain byte-for-byte untouched.
"""
import argparse
import collections
import itertools
import json
from pathlib import Path
import random
from guesscalc_exact import Solver
import guesscalc_generate as legacy

ROOT = Path(__file__).resolve().parents[2]
PATH = ROOT/'assets/guesscalc_beta4.json'
OLD = json.loads((ROOT/'assets/guesscalc_packs.json').read_text())
MASTER = json.loads((ROOT/'assets/guesscalc_master.json').read_text())
SEED = 0xb4c05e71


def cd_metrics(p, solver):
    found = []
    cards = sorted(p['cards'])
    for count in range(1, 7):
        for subset in sorted(set(itertools.combinations(cards, count))):
            result = solver.solve(subset, p['target']).get(p['target'])
            if result:
                found.append((count, result))
    assert found, p
    best = min(found, key=lambda item: (item[0], item[1]['graded']['score'], item[1]['graded']['expression']))
    return dict(min_cards=best[0], min_operations=best[0]-1,
                canonical_exact_solutions=sum(r['canonical_count'] for _, r in found),
                division_required=min(r['division_steps'] for _, r in found)>0,
                easiest_expression=best[1]['graded']['expression'],
                easiest_divisions=best[1]['graded']['division_steps'],
                easiest_subtractions=best[1]['graded']['subtraction_steps'])


def countdown(rng, solver):
    before = []
    after = []
    retained = []
    for p in OLD['countdown']+MASTER['countdown']:
        m = cd_metrics(p, solver);before.append(dict(puzzle_id=p['puzzle_id'], difficulty=p['difficulty'], **m))
        if p['difficulty'] == 3:
            assert m['min_cards'] == 6 and m['division_required']
        if p['difficulty'] == 2 and m['min_cards'] >= 4:
            after.append(dict(p, source_puzzle_id=p['puzzle_id'], exact=m));retained.append(p['puzzle_id'])
    seen = {(tuple(sorted(p['cards'])), p['target']) for p in OLD['countdown']+MASTER['countdown']}
    attempts = 0;rejected = 0
    while len(after) < 30 and attempts < 20000:
        attempts += 1
        deck = list(range(1, 11))*2;rng.shuffle(deck)
        cards = deck[:3]+rng.sample([25, 50, 75, 100], 3);rng.shuffle(cards)
        items = [(v, str(v)) for v in cards]
        while len(items)>1:
            a, ae = items.pop(rng.randrange(len(items)));b, be = items.pop(rng.randrange(len(items)))
            choices = [(a+b, f'({ae}+{be})'), (a*b, f'({ae}*{be})')]
            if a>b:choices.append((a-b, f'({ae}-{be})'))
            if b>a:choices.append((b-a, f'({be}-{ae})'))
            if a%b == 0:choices.append((a//b, f'({ae}/{be})'))
            if b%a == 0:choices.append((b//a, f'({be}/{ae})'))
            items.append(rng.choice(choices))
        target, witness = items[0];key = (tuple(sorted(cards)), target)
        if not 100 <= target <= 999 or key in seen:
            continue
        p = dict(cards=cards, target=target, witness=witness, hint='Use at least four cards for an exact hit.')
        exact = cd_metrics(p, solver)
        if exact['min_cards'] < 4:
            rejected += 1
            continue
        seen.add(key);after.append(dict(p, difficulty=2, source_puzzle_id=None, exact=exact))
    assert len(after) == 30
    for ordinal, p in enumerate(after):
        p.update(puzzle_id=120+ordinal, rules_version=4, game_id=7)
        p['witness'] = p['exact']['easiest_expression']
        p['hint'] = 'Exact answer needs at least 4 cards.'
        assert len(p['witness']) < 40 and len(p['hint']) < 40
    return after, dict(before=before, retained_hard_content_ids=retained, generation_attempts=attempts, rejected_shortcuts=rejected)


def sequence_grammar(difficulty):
    rules = legacy.sequence_rules()
    if difficulty == 0:
        return [(f, p, s) for f, p, s in rules if f in (0, 1, 4)]
    out = [(f, p, s) for f, p, s in rules if f in (2, 3, 5)]
    for x, y, a, b in itertools.product(range(1, 10), range(1, 10), range(1, 4), range(1, 4)):
        if a != b:
            out.append((7, [a, b, 0], tuple((x if i%2 == 0 else y)+(i//2)*(a if i%2 == 0 else b) for i in range(7))))
    return out


def affine_shape(sequence):
    # Normalize translation and nonzero rational scaling, including sign.
    from fractions import Fraction
    values = [x-sequence[0] for x in sequence]
    pivot = next((x for x in values if x), 1)
    return tuple(str(Fraction(v, pivot)) for v in values)


def sequences(rng):
    output = [];used = {tuple(p['sequence'][:6]) for p in OLD['sequence']+MASTER['sequence']}
    rejects = [0, 0]
    for d, quota in enumerate(({0:12, 1:9, 4:9}, {2:9, 3:9, 7:8, 5:4})):
        grammar = sequence_grammar(d);answers = collections.defaultdict(set)
        for _, _, s in grammar:answers[s[:6]].add(s[6])
        for family, count in quota.items():
            choices = [(f,p,s) for f,p,s in grammar if f == family]
            if d == 0:
                choices = [(f,p,s) for f,p,s in choices if s[0]>0 and (f != 1 or p[1]>0) and (f != 4 or p[2]<=2)]
            rng.shuffle(choices);made = 0
            for f, params, s in choices:
                if s[:6] in used:continue
                if len(answers[s[:6]]) != 1:rejects[d] += 1;continue
                if max(map(abs, s)) >= 10000:continue
                used.add(s[:6]);output.append(dict(game_id=5, difficulty=d, puzzle_id=120+len(output), rules_version=4,
                                                 family=f, params=params, sequence=s, distinct_next_answers=1))
                made += 1
                if made == count:break
            assert made == count, (d, family, made)
    shapes = [{affine_shape(p['sequence'][:6]) for p in output if p['difficulty'] == d} for d in range(2)]
    return output, dict(ambiguity_rejects=rejects, affine_unique=[len(s) for s in shapes], affine_cross_level_overlap=len(shapes[0]&shapes[1]))


def header(data):
    arr = legacy.arr
    lines = ['/* Original revision4 append-only content. Legacy IDs remain intact. */',
             '#ifndef GUESSCALC_BETA4_H', '#define GUESSCALC_BETA4_H',
             'static const GcSequencePack gc_sequence_beta4[60]={']
    lines += ['{%s,%s,%d},' % (arr(p['sequence']), arr(p['params']), p['family']) for p in data['sequence']]
    lines += ['};', 'static const GcCardPack gc_countdown_beta4[30]={']
    lines += ['{%s,%d,%s,%s},' % (arr(p['cards']), p['target'], json.dumps(p['witness']), json.dumps(p['hint'])) for p in data['countdown']]
    return '\n'.join(lines+['};', '#endif', ''])


def verify(data, solver):
    assert data['revision'] == 4 and len(data['sequence']) == 60 and len(data['countdown']) == 30
    for d, expected in enumerate(({0:12, 1:9, 4:9}, {2:9, 3:9, 7:8, 5:4})):
        records = [p for p in data['sequence'] if p['difficulty'] == d]
        assert dict(collections.Counter(p['family'] for p in records)) == expected
        grammar = sequence_grammar(d)
        for p in records:
            predictions = {s[6] for _, _, s in grammar if s[:6] == tuple(p['sequence'][:6])}
            assert predictions == {p['sequence'][6]}
    assert len({tuple(p['sequence'][:6]) for p in data['sequence']}) == 60
    for p in data['countdown']:
        actual = cd_metrics(p, solver);assert actual == p['exact'] and actual['min_cards'] >= 4 and p['target'] not in p['cards']
    for p in MASTER['countdown']:
        actual = cd_metrics(p, solver);assert actual['min_cards'] == 6 and actual['division_required']


def main():
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--generate', action='store_true');args = parser.parse_args()
    with Solver(integer=True) as solver:
        if args.generate:
            rng = random.Random(SEED);seq, seq_audit = sequences(rng);print('Sequence generated60', flush=True)
            cards, card_audit = countdown(rng, solver);print('Countdown generated30', flush=True)
            data = dict(revision=4, seed=SEED, external_records=0, sequence=seq, sequence_audit=seq_audit,
                        countdown=cards, countdown_audit=card_audit)
            verify(data, solver);PATH.write_text(json.dumps(data, indent=2)+'\n')
            (ROOT/'assets/guesscalc_beta4.h').write_text(header(data))
        else:
            data = json.loads(PATH.read_text());verify(data, solver)
            assert (ROOT/'assets/guesscalc_beta4.h').read_text() == header(data)
    print('PASS Sequence60/Countdown30 rev4; unchanged MASTER30 reverified', flush=True)


if __name__ == '__main__':main()
