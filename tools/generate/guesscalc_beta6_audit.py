#!/usr/bin/env python3
"""Reproducible distribution audit for the beta.6 arithmetic banks."""
import argparse
import ast
import collections
import hashlib
import json
from pathlib import Path
import statistics

ROOT = Path(__file__).resolve().parents[2]
TARGET = ROOT / 'assets/guesscalc_target_beta6.json'
COUNTDOWN = ROOT / 'assets/guesscalc_countdown_beta6.json'
OUT = ROOT / 'assets/guesscalc_beta6_content_audit.json'
DOC = ROOT / 'docs/MAKE_TARGET_COUNTDOWN_BETA6_AUDIT.md'
TARGETS = (10,24,50,100,200)
LEVELS = ('EASY','NORMAL','HARD','MASTER')


def counts(values):
    return dict(sorted(collections.Counter(values).items(), key=lambda x: str(x[0])))


def quantiles(values):
    values = sorted(values)
    return dict(min=values[0], median=statistics.median(values),
                p90=values[(9*len(values)+9)//10-1],
                p95=values[(19*len(values)+19)//20-1], max=values[-1])


def operations(text):
    kinds = {ast.Add:'+',ast.Sub:'-',ast.Mult:'*',ast.Div:'/'}
    return ''.join(sorted(kinds[type(node.op)] for node in ast.walk(ast.parse(text,mode='eval'))
                          if isinstance(node,ast.BinOp)))


def simple_pair(cards,target):
    for i,a in enumerate(cards):
        for b in cards[i+1:]:
            if a*b == target or (b and a%b == 0 and a//b == target) or (a and b%a == 0 and b//a == target):
                return True
    return False


def analyze():
    mt = json.loads(TARGET.read_text())
    cd = json.loads(COUNTDOWN.read_text())
    assert len(mt['records']) == 4000 and len(cd['records']) == 800
    target_cells = []
    for d in range(4):
        for target in TARGETS:
            rows = [r for r in mt['records'] if r['difficulty']==d and r['target']==target]
            assert len(rows)==200
            unique = {(r['target'],tuple(sorted(r['cards']))) for r in rows}
            assert len(unique)==200
            templates = collections.Counter(r['easiest_template'] for r in rows)
            target_cells.append(dict(difficulty=LEVELS[d],target=target,records=len(rows),
                                     unique_base_puzzles=len(unique),distinct_easiest_templates=len(templates),
                                     top_template=templates.most_common(1)[0][0],
                                     top_template_count=templates.most_common(1)[0][1],
                                     operator_multisets=counts(operations(r['answer']) for r in rows),
                                     simple_pair_product_or_quotient=sum(simple_pair(r['cards'],target) for r in rows),
                                     card_digits=counts(len(str(c)) for r in rows for c in r['cards']),
                                     card_values=quantiles([c for r in rows for c in r['cards']]),
                                     score=quantiles([r['exact']['minimal_complexity_score'] for r in rows]),
                                     canonical_solution_count=quantiles([r['exact']['canonical_count'] for r in rows]),
                                     answer_length=quantiles([len(r['answer']) for r in rows])))
    target_levels = []
    for d in range(4):
        rows = [r for r in mt['records'] if r['difficulty']==d]
        templates = collections.Counter(r['easiest_template'] for r in rows)
        target_levels.append(dict(difficulty=LEVELS[d],records=len(rows),
                                  random_pool_records=len(rows),
                                  distinct_easiest_templates=len(templates),
                                  top_template=templates.most_common(1)[0][0],
                                  top_template_count=templates.most_common(1)[0][1],
                                  card_digits=counts(len(str(c)) for r in rows for c in r['cards']),
                                  card_values=quantiles([c for r in rows for c in r['cards']]),
                                  score=quantiles([r['exact']['minimal_complexity_score'] for r in rows]),
                                  canonical_solution_count=quantiles([r['exact']['canonical_count'] for r in rows]),
                                  division_required=sum(r['exact']['graded']['division_steps']>=1 for r in rows),
                                  fraction_required=sum(r['exact']['graded']['fractional_steps']>=1 for r in rows),
                                  simple_pair_product_or_quotient=sum(simple_pair(r['cards'],r['target']) for r in rows)))
    countdown_levels = []
    for d in range(4):
        rows=cd['records'][d*200:(d+1)*200]
        assert len(rows)==200 and all(r['difficulty']==d for r in rows)
        countdown_levels.append(dict(difficulty=LEVELS[d],records=len(rows),
                                     unique_base_puzzles=len({(r['target'],tuple(sorted(r['cards']))) for r in rows}),
                                     unique_card_decks=len({tuple(sorted(r['cards'])) for r in rows}),
                                     minimum_card_distribution=counts(r['min_cards'] for r in rows),
                                     division_required=sum(r['division_required'] for r in rows),
                                     inherited_in_new_bank=sum(r['source_puzzle_id'] is not None for r in rows),
                                     target_values=quantiles([r['target'] for r in rows]),
                                     answer_length=quantiles([len(r['answer']) for r in rows])))
    return dict(make_target=dict(revision=6,fixed_targets=list(TARGETS),records=4000,
                                 random_copy_records=0,cells=target_cells,levels=target_levels,
                                 master_refinement=mt.get('master_refinement'),
                                 source_sha256=hashlib.sha256(TARGET.read_bytes()).hexdigest()),
                countdown=dict(revision=5,records=800,levels=countdown_levels,
                               source_sha256=hashlib.sha256(COUNTDOWN.read_bytes()).hexdigest()))


def document(audit):
    mt = audit['make_target'];cd=audit['countdown']
    lines = [
        '# Make Target and Countdown content — beta.6', '',
        'New Make Target games select 10, 24, 50, 100, 200, or RANDOM. Each fixed',
        'target × difficulty has 200 different **target + card-multiset** base puzzles.',
        'RANDOM selects from the same five fixed pools within the chosen difficulty:',
        '1,000 reachable puzzles, with no extra copied RANDOM records. The total new',
        'Make Target bank is 4,000. Earlier 1–1000 arbitrary-target revisions remain',
        'for unfinished saved games; their records are excluded from the new count.', '',
        'Countdown has 200 base puzzles at each difficulty, 800 in all. Thirty old',
        'problems per difficulty were included once within the new 200 and 170 new',
        'ones were added. Each new Countdown problem is distinct by target plus six',
        'card values with multiplicity, regardless of display order.', '',
        '## Make Target measured cells', '',
        '| Level | Target | Base puzzles | Easiest-expression templates | Largest template | Pair product/quotient proxy | Score min–median–max |',
        '|---|---:|---:|---:|---:|---:|---|',
    ]
    for cell in mt['cells']:
        score=cell['score']
        lines.append(f"| {cell['difficulty']} | {cell['target']} | {cell['records']} | {cell['distinct_easiest_templates']} | {cell['top_template_count']}/200 | {cell['simple_pair_product_or_quotient']}/200 | {score['min']:,}–{score['median']:,.0f}–{score['max']:,} |")
    if mt['master_refinement']:
        lines += ['', 'The MASTER second pass replaced 40 of the dominant-template',
                  'records per target with a different rational construction. Every',
                  'replacement passed the same full exact minimum grade. The largest',
                  'template share changed as follows:', '',
                  '| MASTER target | Before | After |', '|---:|---:|---:|']
        for cell in sorted(mt['master_refinement']['cells'],key=lambda x:x['target']):
            lines.append(f"| {cell['target']} | {cell['before_top_count']}/200 | {cell['after_top_count']}/200 |")
    lines += ['', 'The pair proxy counts decks containing two cards whose product or',
              'exact quotient alone equals the target; it is **not** a shortcut under',
              'Make Target rules, which require all cards exactly once. Template counts',
              'replace leaf numbers with `n` while keeping operators and tree shape.',
              'Different cards can therefore have the same easiest-solution pattern.',
              'The full JSON report also includes operator multiset, card digit',
              'distribution, value quantiles, answer length, and normalized exact',
              'solution-count quantiles for every cell.', '',
              'To regenerate the selected bank, run',
              '`python3 tools/generate/guesscalc_beta6_target.py --generate` followed by',
              '`python3 tools/generate/guesscalc_beta6_refine.py --write`. The second',
              'pass replaces only MASTER records after exact grading and is itself',
              'deterministic. Its `--check` mode verifies each alternate construction',
              'index in the published bank.', '',
              '## Make Target exact difficulty and readability', '',
              '| Level | Records / RANDOM pool | Card digits 1/2/3 | Card min / median / p90 / max | Exact score min / median / max | Division / fraction required |',
              '|---|---:|---|---|---|---|']
    for level in mt['levels']:
        digits=level['card_digits'];v=level['card_values'];s=level['score']
        lines.append(f"| {level['difficulty']} | {level['records']:,} | {digits.get(1,0):,}/{digits.get(2,0):,}/{digits.get(3,0):,} | {v['min']} / {v['median']:,.0f} / {v['p90']:,} / {v['max']:,} | {s['min']:,} / {s['median']:,.0f} / {s['max']:,} | {level['division_required']:,} / {level['fraction_required']:,} |")
    lines += ['', 'All Make Target cards are 1–999. The unchanged exhaustive rational',
              'solver evaluates every legal full-card expression and assigns the',
              'minimum complexity across *all* solutions, rather than accepting a',
              'hard-looking construction witness. EASY has four cards, no required',
              'division/fractions and graded score ≤259; NORMAL has four cards and',
              'graded score 8450; HARD has five, one required division, no fractional',
              'intermediate and graded score 8451–8715. MASTER has six cards, every',
              'legal exact solution needs a fractional intermediate, and the published',
              'structural-plus-rarity score remains ≥4,472,963. These are exact solver',
              'metrics, not measured human solving times.', '',
              'The solver retains all rational values for every card multiset. For a',
              'given subset and rational value, it keeps Pareto features on tree depth',
              'and maximum denominator after first minimizing the additive structural',
              'cost tuple. Every parent operation adds the same value-dependent cost to',
              'either child candidate; parent depth and denominator are monotone maxima.',
              'A discarded dominated child therefore cannot become the easiest parent.',
              'Canonical solution counts are aggregated separately from witness choice.',
              'The bound is the native parser’s reduced numerator/denominator ≤10^9;',
              'there is no content-search timeout or arbitrary rational cutoff.', '',
              '## Countdown exact minimum cards', '',
              '| Level | Base puzzles | Min-card distribution | Division required | Distinct six-card decks |',
              '|---|---:|---|---:|---:|']
    for level in cd['levels']:
        distribution=', '.join(f'{n} cards: {v}' for n,v in level['minimum_card_distribution'].items())
        lines.append(f"| {level['difficulty']} | {level['records']} | {distribution} | {level['division_required']} | {level['unique_card_decks']} |")
    lines += ['', 'Countdown uses six cards drawn from two copies each of 1–10 and',
              'distinct 25/50/75/100 large cards: 1/2/3/3 large cards for',
              'EASY/NORMAL/HARD/MASTER. The target remains 100–999. The exact subset',
              'dynamic program checks every card occurrence, positive integer',
              'intermediates and only exact division. HARD has no 1–3-card exact',
              'shortcut; MASTER needs six cards and has no legal exact expression',
              'without division. The old EASY/NORMAL minimum-card mix is preserved',
              'proportionally, without changing the game rule.', '',
              '## Verification scope and limits', '',
              'The reproducible [machine audit](../assets/guesscalc_beta6_content_audit.json)',
              'summarizes [4,000 Make Target records](../assets/guesscalc_target_beta6.json)',
              'and [800 Countdown records](../assets/guesscalc_countdown_beta6.json).',
              'The target generator verifies construction witnesses and displayed',
              'answers against card occurrences, 1–999 bounds, duplicate signatures,',
              'all acceptance thresholds and answer width. Its `--verify` mode re-solves',
              'all 4,000 with the C++ exact rational DP. The Countdown generator checks',
              'all 800 with an independent Python positive-integer subset DP, then its',
              '`--verify` mode rechecks shortest-card reachability with the unchanged',
              'C++ integer solver. The [native payload audit](../assets/guesscalc_beta6_native_audit.json)',
              'compares all 4,800 C-header records to JSON and passes every embedded',
              'answer through the production `gc_expression` and `gc_cards` functions.',
              'Ten EASY/NORMAL records (one per fixed target and level) received a',
              'separate expression-tree cross-check: 13,221 exact rational-value groups',
              'and 3,944,640 canonical trees agreed on reachability, normalized count,',
              'raw minimum and graded minimum. The independent tree check is a sample,',
              'not an exhaustive 4,000-record verification.',
              'Native/device timing and perceived variety require hardware/player testing.',
              'The previous MENU flicker report remains HARDWARE TEST REQUIRED.', '']
    return '\n'.join(lines)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--write',action='store_true')
    args=p.parse_args()
    audit=analyze()
    text=json.dumps(audit,indent=2)+'\n'
    md=document(audit)
    if args.write:
        OUT.write_text(text)
        DOC.write_text(md)
    else:
        assert OUT.read_text()==text and DOC.read_text()==md
    print('PASS Make Target/Countdown beta.6 audit: 4000 + 800 records')


if __name__=='__main__':
    main()
