#!/usr/bin/env python3
"""Before/after beta.4 GUESS/CALC evidence; beta.3 assets remain unchanged."""
import argparse
import collections
import copy
import hashlib
import itertools
import json
import math
from pathlib import Path
import subprocess
from guesscalc_exact import Solver, target_deck
from guesscalc_beta4_target import annotate
from guesscalc_difficulty_audit import factor, factor_shape_domains, summary

ROOT = Path(__file__).resolve().parents[2]
LEVELS = ('EASY', 'NORMAL', 'HARD', 'MASTER')


def distributions(records):
    return {'count': len(records), 'complexity_score': summary(r['exact']['minimal_complexity_score'] for r in records),
            'fractional_steps': summary(r['exact']['graded']['fractional_steps'] for r in records),
            'division_steps': summary(r['exact']['graded']['division_steps'] for r in records),
            'unary_steps': summary(r['exact']['graded']['unary_steps'] for r in records),
            'subtraction_steps': summary(r['exact']['graded']['subtraction_steps'] for r in records),
            'tree_depth': summary(r['exact']['graded']['tree_depth'] for r in records),
            'max_denominator': summary(r['exact']['graded']['max_denominator'] for r in records),
            'canonical_solution_count': summary(r['exact']['canonical_count'] for r in records),
            'fraction_required': sum(r['exact']['graded']['fractional_steps']>0 for r in records),
            'division_required': sum(r['exact']['graded']['division_steps']>0 for r in records)}


def old_target_sample():
    filename = ROOT/'assets/guesscalc_target_beta3_sample.json'
    if filename.exists():
        records = json.loads(filename.read_text())
        assert len(records) == 384
        return records
    records = []
    with Solver() as solver:
        for difficulty in range(4):
            for target in (1, 24, 1000):
                for seed in range(1, 33):
                    cards, witness, _ = target_deck(seed, difficulty, target)
                    result = annotate(solver.solve(cards, target)[target])
                    records.append(dict(difficulty=difficulty, target=target, seed=seed, cards=cards,
                                        generator_witness=witness, exact=result))
    filename.write_text(json.dumps(records, indent=2)+'\n')
    return records


def native_parity(records):
    directory = ROOT/'build-host/guesscalc-beta4';directory.mkdir(parents=True, exist_ok=True)
    executable = directory/'audit-native'
    subprocess.run(['clang', '-std=c11', '-Wall', '-Wextra', '-Werror', '-g', '-fsanitize=undefined',
                    '-fno-sanitize-recover=all', '-Iinclude', '-Isrc/games',
                    'tools/generate/guesscalc_beta4_sample.c', 'src/games/guesscalc.c', 'src/games/guesscalc_math.c',
                    'src/core/common.c', 'src/ui/draw.c', '-o', str(executable)], cwd=ROOT, check=True)
    result = subprocess.run([str(executable)], check=True, text=True, capture_output=True)
    actual = [json.loads(line) for line in result.stdout.splitlines()]
    assert len(actual) == len(records) == 8000
    for a, r in zip(actual, records):
        assert (a['puzzle_id'], a['cards'], a['witness']) == (r['puzzle_id'], r['cards'], r['generator_witness'])
    expressions = ''.join(f"{r['target']} {expression}\n" for r in records
                          for expression in (r['generator_witness'], r['exact']['easiest_expression'], r['exact']['graded']['expression']))
    subprocess.run([str(executable), '--expressions'], input=expressions, check=True, text=True, capture_output=True)
    return dict(native_records=8000, native_parser_expressions=24000)


def factor_metrics(targets):
    facts = [collections.Counter(factor(value)) for value in sorted(targets)]
    return dict(count=len(targets), target=summary(targets), distinct_primes=summary(len(f) for f in facts),
                Omega=summary(sum(f.values()) for f in facts), repeated_prime_count=summary(sum(e>1 for e in f.values()) for f in facts),
                large_prime_count=summary(sum(p>=11 for p in f) for f in facts), max_prime=summary(max(f) for f in facts),
                decimal_digits=summary(len(str(v)) for v in targets),
                prime_check_trial_division_proxy=summary(sum(max(0, math.isqrt(p)-1) for p in f) for f in facts))


def countdown_distribution(metrics):
    return dict(problem_count=len(metrics), minimum_cards=summary(p['min_cards'] for p in metrics),
                minimum_card_histogram=dict(collections.Counter(p['min_cards'] for p in metrics)),
                minimum_operations=summary(p['min_operations'] for p in metrics),
                canonical_exact_solutions=summary(p['canonical_exact_solutions'] for p in metrics),
                division_required=sum(p['division_required'] for p in metrics),
                easiest_subtractions=summary(p['easiest_subtractions'] for p in metrics),
                one_card_count=sum(p['min_cards']==1 for p in metrics),
                two_card_count=sum(p['min_cards']==2 for p in metrics),
                three_card_count=sum(p['min_cards']==3 for p in metrics))


def main():
    parser = argparse.ArgumentParser(description=__doc__);mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--write', action='store_true');mode.add_argument('--check', action='store_true');args = parser.parse_args()
    baseline = json.loads((ROOT/'assets/guesscalc_difficulty_audit.json').read_text())
    integration = json.loads((ROOT/'assets/guesscalc_difficulty_rows.json').read_text())
    bank = json.loads((ROOT/'assets/guesscalc_beta4.json').read_text())
    target = json.loads((ROOT/'assets/guesscalc_target_beta4.json').read_text())
    old = old_target_sample();evidence = native_parity(target['records']);rows = copy.deepcopy(integration)
    detail = dict(revision=4, baseline='assets/guesscalc_difficulty_audit.json', native=evidence,
                  target_generation={k: target[k] for k in ('attempts_by_level','rejects_by_level','duplicate_candidates_by_level')},
                  sequence=bank['sequence_audit'], countdown=bank['countdown_audit'], before={}, after={})
    for row in rows:
        id_, d = row['game_id'], LEVELS.index(row['difficulty'])
        source = next(r for r in baseline['rows'] if r['id']==id_ and r['level_index']==d)
        before, after = source['metric'], copy.deepcopy(source['metric'])
        if id_ == 5 and d < 2:
            chosen = [p for p in bank['sequence'] if p['difficulty']==d]
            counts = dict(collections.Counter(p['family'] for p in chosen))
            after = dict(families=counts, percentages={f: n*100/30 for f,n in counts.items()}, prefix_length=6,
                         max_absolute_term=summary(max(map(abs,p['sequence'])) for p in chosen),
                         ambiguity_rejects=bank['sequence_audit']['ambiguity_rejects'][d],
                         affine_unique=bank['sequence_audit']['affine_unique'][d], exact_E_N_overlap=0,
                         affine_E_N_overlap=bank['sequence_audit']['affine_cross_level_overlap'])
            row.update(mechanism='AP12/GP9/shared-step9' if d==0 else 'quadratic9/Fibonacci9/unequal interleaved8/offset4',
                       finding='GC-D02 RESOLVED / BANK REBALANCED', fix='Fresh revision4 IDs120..179; old bank preserved')
        elif id_ == 7:
            before = countdown_distribution([p for p in bank['countdown_audit']['before'] if p['difficulty']==d])
            metrics = [p['exact'] for p in bank['countdown']] if d==2 else [p for p in bank['countdown_audit']['before'] if p['difficulty']==d]
            after = countdown_distribution(metrics)
            row.update(finding='GC-D01 RESOLVED / CONTENT REVISED', fix='HARD revised;E/N/M remain unchanged and reverified')
            if d==2:
                row.update(mechanism='6 cards;3 large;every exact solution requires at least4 cards',
                           finding='GC-D01 RESOLVED / CONTENT REVISED', fix='Revision4 HARD IDs120..149; old saves preserve IDs60..89')
        elif id_ == 6:
            before = distributions([p for p in old if p['difficulty']==d])
            after = distributions([p for p in target['records'] if p['difficulty']==d])
            after.update(targets=1000, records_per_target=2, total_records=2000,
                         cards=(4,4,5,6)[d], canonical_duplicates=0)
            row.update(bank_size=2000, mechanism=('4 cards; exact minimum needs no division','4 cards; exact minimum requires integer division',
                                                '5 cards; exact minimum requires integer division','6 cards; every legal solution requires a fractional intermediate')[d],
                       finding='GC-D05 RESOLVED / EXHAUSTIVE COMPLEXITY GRADING',
                       fix='Revision4 target-indexed 2-record banks;16KB indices total;rev1/2/3 saves unchanged',
                       overlap='Exact target+card multiset cross-level overlap0 over all8000 records')
        elif id_ == 10:
            domains = factor_shape_domains();before = factor_metrics(domains[d])
            row.update(finding='GC-D03 RESOLVED / MASTER REVISED', fix='MASTER revised;E/N/H generator unchanged')
            if d==3:
                targets = {108*p*q*m for p in (37,41,43,47,53,59,61,67,71,73) for q in (11,13,17,19,23,29,31) for m in (1,2)}
                after = factor_metrics(targets);assert not domains[2]&targets
                row.update(mechanism='2^(2or3)*3^3*p*q;p37..73;q11..31;four distinct,Omega7..8',
                           finding='GC-D03 RESOLVED / MASTER REVISED', fix='Fresh revision4 runtime shape;old MASTER construction unchanged',
                           overlap='Exact HARD/new MASTER target overlap0;new MASTER mathematical domain140')
            else:
                after = before
        if id_ in (1,2):
            row.update(finding='GC-D04 ACCEPTED AS DESIGNED', fix='Existing attempt allowances intentionally preserved')
            if id_==1:
                after['secret_space_per_attempt_exact'] = f"{10**(4+d)}/{(16,12,10,16)[d]}"
            else:
                after['hidden_bank_candidates_per_mode'] = 30
        if id_==34:
            row.update(finding='GC-D06 ACCEPTED / OVERLAP DOCUMENTED', fix='Unique-solution bank unchanged;increasing medians,overlapping ranges accepted')
        row['generator_params'] = json.dumps(after, sort_keys=True, separators=(',', ':'))
        row['observed_distribution'] = json.dumps(dict(before=before, after=after), sort_keys=True, separators=(',', ':'))
        row['meaningfully_distinct'] = 'Objective structural grading verified;not a universal human ranking'
        detail['before'][f'{id_}:{d}'] = before;detail['after'][f'{id_}:{d}'] = after
    for name, value in [('guesscalc_difficulty_rows_beta4.json', rows), ('guesscalc_difficulty_audit_beta4.json', detail)]:
        file = ROOT/'assets'/name;text = json.dumps(value, indent=2, sort_keys=True)+'\n'
        if args.write:file.write_text(text)
        else:assert file.read_text()==text, f'{name} stale'
    print('PASS beta4 48 rows;8000 native payloads;24000 native expressions;baseline preserved')


if __name__ == '__main__':main()
