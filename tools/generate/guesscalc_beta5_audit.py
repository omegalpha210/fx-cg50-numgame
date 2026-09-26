#!/usr/bin/env python3
"""Card-readability and exact-grade comparison for retained beta.4/new beta.5."""
import argparse
import collections
import hashlib
import json
import math
from pathlib import Path
import statistics
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT/'assets/guesscalc_target_beta5_audit.json'
LEVELS = ('EASY', 'NORMAL', 'HARD', 'MASTER')


def measure(values):
    ordered = sorted(values)
    assert ordered
    return dict(min=ordered[0], median=statistics.median(ordered),
                p90=ordered[math.ceil(.90*len(ordered))-1],
                p95=ordered[math.ceil(.95*len(ordered))-1], max=ordered[-1])


def bank_metrics(records):
    levels = []
    for difficulty, name in enumerate(LEVELS):
        chosen = [r for r in records if r['difficulty'] == difficulty]
        cards = [value for record in chosen for value in record['cards']]
        digit_counts = collections.Counter(min(4,len(str(value))) for value in cards)
        levels.append(dict(difficulty=name, problem_count=len(chosen), card_count=len(cards),
                           card_value=measure(cards),
                           decimal_digit_distribution={str(d):digit_counts[d] for d in range(1,5)},
                           complexity_score=measure(r['exact']['minimal_complexity_score'] for r in chosen),
                           canonical_solution_count=measure(r['exact']['canonical_count'] for r in chosen),
                           fractional_required=sum(r['exact']['graded']['fractional_steps']>0 for r in chosen),
                           division_required=sum(r['exact']['graded']['division_steps']>0 for r in chosen)))
    return levels


def native_parity(records):
    folder = ROOT/'build-host/guesscalc-beta5';folder.mkdir(parents=True,exist_ok=True)
    executable = folder/'audit-native'
    subprocess.run(['clang','-std=c11','-Wall','-Wextra','-Werror','-g','-fsanitize=undefined',
                    '-fno-sanitize-recover=all','-Iinclude','-Isrc/games',
                    'tools/generate/guesscalc_beta5_sample.c','src/games/guesscalc.c','src/games/prime_beta6.c',
                    'src/games/guesscalc_math.c','src/core/common.c','src/ui/draw.c',
                    '-o',str(executable)],cwd=ROOT,check=True)
    result = subprocess.run([str(executable)],capture_output=True,text=True,check=True)
    actual = [json.loads(line) for line in result.stdout.splitlines()]
    assert len(actual) == len(records) == 8000
    for row, expected in zip(actual,records):
        assert (row['puzzle_id'],row['cards'],row['witness']) == (expected['puzzle_id'],expected['cards'],expected['generator_witness'])
    expressions = ''.join(f"{r['target']} {expression}\n" for r in records
                          for expression in (r['generator_witness'],r['exact']['easiest_expression'],r['exact']['graded']['expression']))
    parsed = subprocess.run([str(executable),'--expressions'],input=expressions,
                            capture_output=True,text=True,check=True)
    assert 'PASS 24000 exact expressions' in parsed.stdout
    return dict(native_records=8000,native_parser_expressions=24000)


def run():
    old = json.loads((ROOT/'assets/guesscalc_target_beta4.json').read_text())['records']
    new = json.loads((ROOT/'assets/guesscalc_target_beta5.json').read_text())['records']
    assert len(old) == len(new) == 8000
    assert max(v for row in new for v in row['cards']) <= 999
    assert min(v for row in new for v in row['cards']) >= 1
    parity = native_parity(new)
    return dict(revision=5,baseline='assets/guesscalc_target_beta4.json',
                source='assets/guesscalc_target_beta5.json',
                source_sha256=hashlib.sha256((ROOT/'assets/guesscalc_target_beta5.json').read_bytes()).hexdigest(),
                before=bank_metrics(old),after=bank_metrics(new),native=parity)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    group=parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--write',action='store_true');group.add_argument('--check',action='store_true')
    args=parser.parse_args()
    result=run();payload=json.dumps(result,indent=2)+'\n'
    if args.check:
        assert OUT.read_text()==payload
        print('PASS: beta.5 bounded-card audit and 8000 native payloads / 24000 expressions')
    else:
        OUT.write_text(payload)
        print('wrote',OUT.relative_to(ROOT))


if __name__=='__main__':main()
