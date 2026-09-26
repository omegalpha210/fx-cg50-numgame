#!/usr/bin/env python3
"""Measure every supplied LOGIC/PUZZLE bank using public clues, without edits.

Use --output PATH to write a deterministic structured report. These are
instrument/structure bands, not calibrated human difficulty measurements.
"""
import argparse
import collections
import hashlib
import importlib.util
import json
import pathlib
import statistics
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/generate'))
from grids_compact import records
from grids_master_logic import Logic
from grids_puzzle_master import PuzzleLogic
from grids_verify import base_key

spec = importlib.util.spec_from_file_location('grids_extra_generate', ROOT / 'assets/grids/extra/generate.py')
extra = importlib.util.module_from_spec(spec)
spec.loader.exec_module(extra)


def structure(p):
    gid, n = p['id'], p['n']
    result = dict(order=n, cells=n*n)
    if gid in (11, 12, 14, 15, 18, 19):
        result['givens'] = sum(bool(v) for v in p['cells'])
    if gid == 12:
        result['cages'] = max(p['a'])+1
        result['fixed_cages'] = sum(p['b'][2*i] == 0 for i in range(result['cages']))
    if gid == 13:
        result['white_cells'] = sum(v == 255 for v in p['cells'])
        lengths = []
        for clues, step in ((p['a'], 1), (p['b'], n)):
            for i, clue in enumerate(clues):
                if not clue:
                    continue
                j, length = i+step, 0
                while j < n*n and p['cells'][j] == 255 and (step != 1 or j//n == i//n):
                    length += 1
                    j += step
                lengths.append(length)
        result.update(runs=len(lengths), longest_run=max(lengths))
    if gid == 14:
        result['inequalities'] = sum(bool(v) for v in p['a']+p['b'])
    if gid == 15:
        result['outer_clues'] = sum(bool(v) for v in p['a'])
    if gid == 16:
        result['duplicate_pairs'] = sum(p['cells'][i] == p['cells'][j] and (i//n == j//n or i % n == j % n)
                                        for i in range(n*n) for j in range(i+1, n*n))
    if gid == 17:
        result['givens'] = sum(v != 255 for v in p['cells'])
    if gid == 20:
        result['maximum_value'] = max(p['cells'])
        result['maximum_target'] = max(p['a'])
    if gid == 35:
        result.update(islands=len(p['pos']), candidate_edges=len(extra.geometry(n, p['pos'])[0]), largest_degree=max(p['clues']))
    if gid == 36:
        result.update(runs=sum(map(len, p['clues'])), zero_lines=sum(not c for c in p['clues']), longest_clue_list=max(map(len, p['clues'])))
    return result


def measure(p):
    # The witness is deliberately absent from every inference/search input.
    public = {k: v for k, v in p.items() if k not in ('solution', 'logical_rating', 'rating')}
    if p['id'] == 19:
        return structure(p)
    if p['id'] in (35, 36):
        metrics, found = (extra.hashi_solve if p['id'] == 35 else extra.nono_solve)(public)
        assert len(found) == 1
        return dict(structure(p), **metrics)
    solver = (Logic if p['id'] <= 15 else PuzzleLogic)(public)
    rating = solver.rate(512)
    metrics, found = solver.search(30000)
    assert len(found) == 1 and metrics['solution_count'] == 1
    result = dict(structure(p), **metrics)
    for key in ('basic_unfilled', 'advanced_unfilled', 'after_chains_unfilled', 'forcing_eliminations', 'forcing_probes'):
        result[key] = rating[key]
    return result


def bounds(values):
    return dict(min=min(values), median=statistics.median(values), max=max(values))


def all_records():
    puzzles = records(ROOT)
    for gid in (35, 36):
        for d in range(5 if gid == 35 else 4):
            puzzles += json.loads((ROOT / f'assets/grids/extra/{gid}-{d}.json').read_text())
    assert len(puzzles) == 2020
    return puzzles


def enrich(report):
    # Allow a completed measurement run to be formatted again only against
    # precisely the same puzzle inputs. No result can silently follow new data.
    for name, digest in report['source_sha256'].items():
        assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest() == digest
    grouped = collections.defaultdict(list)
    for p in all_records():
        grouped[p['id'], p['difficulty']].append(p)
    exact, canonical = {}, {}
    for key, bank in grouped.items():
        fields = ('id', 'n', 'cells', 'a', 'b', 'pos', 'clues', 'variant')
        exact[key] = {json.dumps({f: p[f] for f in fields if f in p}, sort_keys=True) for p in bank}
        canonical[key] = {(p.get('variant', 'classic'), extra.canonical(p) if p['id'] >= 35 else base_key(p)) for p in bank}
    names = {11:'SUDOKU',12:'CALCUDOKU',13:'KAKURO',14:'FUTOSHIKI',15:'SKYSCRAPERS',
             16:'HITORI',17:'BINARY PUZZLE',18:'NUMBRIX',19:'MAGIC SQUARE',20:'SUM GRID',35:'HASHI',36:'NONOGRAM'}
    mechanisms = {
        11:'9x9 box/row/column constraints; clue density and inference/search bands',
        12:'Latin order, connected arithmetic cages and inference/search bands',
        13:'Crossing distinct-digit sum runs, white-cell geometry and inference/search bands',
        14:'Latin order, givens/inequalities and inference/search bands',
        15:'Latin order, directional visibility clues and inference/search bands',
        16:'Duplicate conflicts, nonadjacent shading and white connectivity',
        17:'Balanced unique binary lines, no triples; order/givens/inference',
        18:'Orthogonal consecutive-number path; order/givens/path inference',
        19:'PARTIAL: order/givens, MASTER PAN wrapped diagonals; FREE: normal orders 3/4/5/6',
        20:'Weighted row/column subset sums; order/weights and crossing inference',
        35:'Nearest-island degree graph, no crossings, connectivity; geometry/inference bands',
        36:'Ordered row/column runs; order and line-pattern interaction/search bands',
    }
    level_names = ['EASY','NORMAL','HARD','MASTER','HELL']
    csv_rows = []
    for group in report['groups']:
        if group['mode'] == 'FREE':
            continue
        gid, d = group['game_id'], group['difficulty']
        key = gid, d
        overlaps = [{'difficulty': level_names[other[1]],
                     'exact': len(exact[key] & exact[other]),
                     'canonical': len(canonical[key] & canonical[other])}
                    for other in sorted(grouped) if other[0] == gid and other != key]
        group.update(unique_solution_records=0 if gid == 19 else group['records'],
                     exact_distinct=len(exact[key]), canonical_distinct=len(canonical[key]),
                     cross_level_overlap=overlaps)
        m = group['metrics']
        node_metric = m.get('search_nodes')
        adjacent_overlap = None
        if d and node_metric:
            prev = next(g for g in report['groups'] if g['game_id'] == gid and g['difficulty'] == d-1 and g['mode'] != 'FREE')['metrics']['search_nodes']
            adjacent_overlap = max(prev['min'], node_metric['min']) <= min(prev['max'], node_metric['max'])
        finding = 'Structural/inference parameters differ; human difficulty remains provisional.'
        if adjacent_overlap:
            finding += ' Search-node ranges overlap the previous level; no per-puzzle strict ranking claimed.'
        if any(o['exact'] or o['canonical'] for o in overlaps):
            finding += ' Public-clue overlap exists across levels and is explicitly counted.'
        if m.get('basic_unfilled', {}).get('max') == 0:
            finding += ' Every sample finishes by basic propagation; its distinction is structural or arithmetic, not a need for advanced inference.'
        if gid == 19:
            finding = 'PARTIAL allows multiple solutions. Legacy FREE E/N were both 3x3; new FREE fixes that no-op with 3/4/5/6 orders.'
        params = {k: v for k, v in m.items() if k in ('givens','cages','fixed_cages','white_cells','runs','longest_run','inequalities','outer_clues','duplicate_pairs','maximum_value','maximum_target','islands','candidate_edges','largest_degree','zero_lines','longest_clue_list')}
        board = m['order']
        if gid == 19:
            board = dict(PARTIAL=board, FREE=d+3)
            params['FREE'] = dict(givens=0, range=[1,(d+3)**2], magic_sum=(d+3)*((d+3)**2+1)//2, bank_size=0, wrapped_diagonals=False)
        metric = 'public normal/PAN rules and board order; no solver uniqueness rating' if gid == 19 else 'public-clue propagation and bounded search; algorithm-specific counts'
        fix = 'FREE rev3 normal orders 3/4/5/6; old FREE orders/rules preserved' if gid == 19 else 'Optional X/blank completion; inference/packs unchanged' if gid == 36 else 'None; evidence/caveats documented'
        csv_rows.append(dict(game_id=gid, name=names[gid], difficulty=level_names[d], mechanism=mechanisms[gid],
                             bank_size=group['records'], board_size=board, generator_params=params, AI_policy='N/A',
                             rating_metric=metric, observed_distribution=m,
                             overlap=dict(with_other_levels=overlaps, search_range_overlaps_previous=adjacent_overlap),
                             meaningfully_distinct='YES: structural/inference band; provisional human ranking', finding=finding, fix=fix))
    report['csv_rows'] = csv_rows
    report['canonical_scope'] = 'D4; Sudoku/Hitori relabeling, Binary complement, Numbrix reversal, Futoshiki order reversal. HASHI D4 clue geometry; NONOGRAM D4/complement of its unique bitmap. Magic partial D4 with rule variant retained; no abstract-graph or all symmetry claim.'
    report['instrument_sha256'] = {name: hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in
        ('tests/test_grids_difficulty.py','tools/generate/grids_master_logic.py','tools/generate/grids_puzzle_master.py','tools/generate/grids_verify.py','assets/grids/extra/generate.py')}
    return report


def audit():
    puzzles = all_records()
    grouped = collections.defaultdict(list)
    for p in puzzles:
        grouped[p['id'], p['difficulty']].append(p)
    result = []
    for (gid, d), bank in sorted(grouped.items()):
        measurements = [measure(p) for p in bank]
        metrics = {key: bounds([row[key] for row in measurements]) for key in measurements[0]}
        result.append(dict(game_id=gid, difficulty=d, mode='PARTIAL' if gid == 19 else 'CLASSIC',
                           records=len(bank), metrics=metrics,
                           interpretation='public-rule construction, multiple answers allowed' if gid == 19 else 'unique public clues; provisional structure/inference band'))
        print(f'{gid}/{d}: {len(bank)} records measured', file=sys.stderr, flush=True)
    for d in range(4):
        n = d+3
        result.append(dict(game_id=19, difficulty=d, mode='FREE', bank_records=0, rule_challenges=1,
                           order=n, value_range=[1, n*n], magic_sum=n*(n*n+1)//2,
                           givens=0, wrapped_diagonals=False,
                           interpretation='normal magic construction by order; no human solving-time ranking claimed'))
    paths = sorted(set(list((ROOT/'assets/grids').glob('[0-9][0-9].json'))+
                       list((ROOT/'assets/grids/master').glob('[0-9][0-9].json'))+
                       list((ROOT/'assets/grids/expanded').glob('[0-9][0-9]-[0-4].json'))+
                       list((ROOT/'assets/grids/extra').glob('[0-9][0-9]-[0-4].json'))))
    return dict(records=2020, unique_records=1900, magic_partial_layouts=120,
                free_rule_challenges=4, human_rating='PROVISIONAL; overlapping per-puzzle bands are reported, not hidden',
                search_limit=30000, forcing_probe_limit=512,
                metric_note='Fresh public-clue replay on every supplied record. Nodes and deductions are algorithm-specific, including search branches. Magic has no uniqueness claim.',
                source_sha256={str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},
                groups=result)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=pathlib.Path)
    parser.add_argument('--metrics-input', type=pathlib.Path, help='Reformat a completed report only when all source hashes match')
    args = parser.parse_args()
    measured = json.loads(args.metrics_input.read_text()) if args.metrics_input else audit()
    text = json.dumps(enrich(measured), indent=2)+'\n'
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(text)
    else:
        print(text, end='')
