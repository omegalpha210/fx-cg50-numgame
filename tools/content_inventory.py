#!/usr/bin/env python3
"""Inventory existing source/native puzzle banks; never generate a puzzle.

Baseline facts are retained below, so a clean source export needs no old Git
objects. Counts come from JSON and native arrays, not anticipated pack targets.
An optional staged grid tree is reported separately from integrated content.
"""
from __future__ import annotations
import argparse
import csv
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import re
import sys

BASELINE = '4e7da73bef82e7e20dce2a8d80ff85d9b8f66639'
IDS = list(range(1, 29)) + [31, 32]
LEVELS = ['EASY', 'NORMAL', 'HARD', 'MASTER', 'HELL']
NAMES = ['NUMBER BASEBALL', 'EQUATION GUESS', 'NUMBER MIND', 'CLUE LOCK',
         'SEQUENCE DETECTIVE', 'MAKE TARGET', 'COUNTDOWN', 'MISSING OPERATORS',
         'CROSS MATH', 'PRIME FACTOR', 'SUDOKU', 'CALCUDOKU', 'KAKURO',
         'FUTOSHIKI', 'SKYSCRAPERS', 'HITORI', 'BINARY PUZZLE', 'NUMBRIX',
         'MAGIC SQUARE', 'SUM GRID', 'NIM', 'WYTHOFF', 'EUCLID', 'MAKE FIFTEEN',
         'RACE', '2048', 'SLIDING', 'LIGHTS OUT', 'SHIKAKU', 'SLITHERLINK']
CATEGORIES = ['GUESS', 'CALC', 'LOGIC', 'PUZZLE', 'STRATEGY', 'BOARD']
GC_KEYS = {2: 'equation', 3: 'number_mind', 4: 'clue_lock', 5: 'sequence',
           6: 'target', 7: 'countdown', 8: 'operators', 9: 'crossmath'}
GC_SYMBOLS = {2: 'equation', 3: 'mind', 4: 'lock', 5: 'sequence', 6: 'target',
              7: 'countdown', 8: 'operators', 9: 'cross'}
GC_BYTES_OLD = [0, 9, 98, 22, 22, 94, 94, 0, 42, 0]
GC_BYTES_MASTER = [0, 11, 98, 24, 22, 94, 94, 14, 42, 0]
GC_FRAMES = [28, 16, 52, 8, 8, 8, 8, 128, 8, 60]
GC_INIT = ['baseball', 'equation', 'mind', 'lock', 'sequence', 'cards',
           'cards', 'operators', 'cross', 'factor']
BASELINE_HASHES = {
    'assets/guesscalc_packs.json': 'e11fa9f8720b813570f686558837e24acab1afe73d4f700610c4651423bd7ef2',
    'assets/guesscalc_packs.h': '430df5b94b2ffa37871443d84319aaa449576a55ad8944875317322d6521259a',
    'assets/boards/puzzles.json': '2f426769c6a3252ba1f64bb7d80fd925502b897f6d24483c46ad5d82d531bb8c',
    'src/games/boards_pack.c': 'bae258a1653fe5f6f8592e5be5972d85e13c35db705a9fd849e1c981c2128f5a',
    'assets/strategyquick/tables.h': '21520069458881ee29026926ca68e3e561f4fcd19af3229c40ead788d75b747c',
}
MODES = {1: ['MID UNIQUE', 'SHORT UNIQUE', 'LONG UNIQUE', 'MID REPEAT', 'SHORT REPEAT', 'LONG REPEAT'],
         2: ['STANDARD', 'SHORT', 'LONG'], 6: ['TARGET 24', 'TARGET 10'],
         19: ['PARTIAL', 'FREE'], 26: ['CLASSIC', 'TARGET'],
         27: ['3x3', '4x4'], 28: ['4x4', '5x5']}
for _id in range(21, 26):
    MODES[_id] = ['CPU / YOU FIRST', 'CPU / CPU FIRST', 'LOCAL 2P']

# Audited source/input history, not an inference from an empty directory.
RUNTIME_BOUNDS = {
    1: 'At most 7 digits; finite 10-symbol scan for each UNIQUE selection; fixed attempt budget',
    8: 'E/N/H at most 128 generation attempts then fixed rule-valid fallback; MASTER direct bank load',
    10: 'E/N/H bounded prime product; MASTER five shuffle steps plus two choices, four distinct primes/two squared',
    21: 'Bounded random pile initialization; MASTER direct bank load; CPU finite exact policy',
    22: 'Bounded random pair initialization; MASTER direct bank load; CPU bounded DP lookup',
    23: 'Bounded random positive pair; MASTER direct bank load; CPU bounded DP lookup',
    24: 'Empty E/N/H start; MASTER direct bank load; CPU finite ownership policy',
    25: 'Fixed E/N/H target/add rules; MASTER direct bank load; CPU exact residue policy',
    26: 'Finite 16-cell spawn/merge scans; fixed RNG; no puzzle generator or puzzle bank',
    27: 'E/N/H 80/160/240 legal shuffle steps; MASTER direct bank load (3x3 exact31 / 4x4 lower bound)',
    28: 'E/N/H finite legal press construction; MASTER direct bank load; bounded GF(2) hint',
}

EVIDENCE = {
    'GC': 'docs/GUESS_CALC_AUDIT.md; generator input trace/seed; original introduction 577606f, retained verified corpus d267e6b; legacy and MASTER byte-identical deterministic regeneration; independent rule validators',
    'GRID': 'docs/GRID_AUDIT.md + docs/GRID_MASTER_AUDIT.md; original local constructors/seeded clue removal; introductions d267e6b (legacy) and 693b44a (first MASTER); retained legacy SHA manifest; independent counters; no downloaded corpus/parser input',
    'SQ': 'docs/STRATEGY_QUICK_AUDIT.md; policy generator introduction 577606f and baseline byte-identical regeneration; MASTER deterministic seed 202609230128; independent public-state verifier',
    'BOARD': 'docs/BOARD_AUDIT.md; generator introduction 591018a; original 180 records regenerated from seed 202609223132; MASTER separate seed 202610223132; independent counters',
}
NORMALIZATION = {
    1: 'No finite shipped bank; generated secret, UNIQUE versus REPEAT is a rule mode',
    2: 'Exact equation text; arithmetic-equivalent texts remain different guessing targets',
    3: 'Digit count/alphabet plus sorted (clue,match) pairs; no arbitrary alphabet permutation quotient',
    4: 'Finite domain plus complete ordered clue parameters; no arithmetic-isomorphism quotient',
    5: 'Six displayed terms; finite documented grammar defines next-answer uniqueness',
    6: 'Sorted card multiset plus target; card ordering and witness expression do not create bases',
    7: 'Sorted card multiset plus target; witness expression does not create a base',
    8: 'Ordered numbers plus target; alternative valid operators do not create a base',
    9: 'Operators, six targets and givens; no rotation quotient for directional arithmetic',
    10: 'No finite shipped bank; bounded prime-product construction',
    11: 'D4 plus first-occurrence digit renaming; not the full Sudoku equivalence group',
    12: 'D4; cage membership canonicalized with operation/target, ignoring cage labels',
    13: 'D4 of white-cell/run-sum constraints',
    14: 'D4 plus order reversal and reversed inequalities',
    15: 'D4 with directed visibility clues',
    16: 'D4 plus first-occurrence public-number renaming',
    17: 'D4 plus global 0/1 complement',
    18: 'D4 plus path reversal v -> n*n+1-v',
    19: 'Partial clues: D4 only; witness D4/complement/torus classes reported separately',
    20: 'D4 of weights and line sums',
    21: 'Sorted piles, separately by side-to-move group; LOCAL shares YOU FIRST',
    22: 'Sorted pile pair, separately by side-to-move group; LOCAL shares YOU FIRST',
    23: 'Sorted pair divided by gcd, separately by side-to-move group',
    24: 'Lo Shu D4 ownership pattern, separately by side-to-move group',
    25: '(maximum add, target-current total), separately by side-to-move group',
    26: 'No puzzle bank; CLASSIC and goal-based TARGET are rule modes',
    27: 'Exact labeled board plus size; rotation changes the fixed goal, so is not factored out',
    28: 'D4 of starting light pattern, separately by size',
    31: 'Counted here: D4 public clues; independent verifier also deduplicates rectangle structures',
    32: 'Counted here: D4 public clues; independent verifier also deduplicates loop structures',
}
VALIDATION = {
    1: 'Duplicate-aware strikes/balls, bounded attempts, exact secret-text completion; deterministic old-state fingerprints',
    2: 'Independent exact arithmetic, true equation grammar, duplicate feedback; MASTER mixed-precedence and operator counts',
    3: 'Full alphabet^length public-clue enumeration with solution cap two; MASTER 8^6 domains',
    4: 'Full integer domain; distinct MOD divisors; MASTER deleting each clue leaves multiple candidates',
    5: 'Independent enumeration of six/eight finite rule families and distinct next answers; not universal sequence uniqueness',
    6: 'Exact-rational card multiset evaluation; MASTER impossible without a fraction or with a balanced depth-two tree',
    7: 'Independent positive-integer subset reachability; MASTER all six cards and division necessary for exact target',
    8: 'All 4^5 MASTER operator assignments; 1..3 solutions, each >=3 operator kinds; accept all legal answers',
    9: 'All 1..9 permutations satisfying six expressions; MASTER row/column coupling exceeds retained HARD threshold',
    10: 'Prime test and overflow-bounded factor product; all factor orderings/repeated factors accepted',
    11: 'Independent bitset Sudoku counter, public-clue singles/Hall/locked-candidate and contradiction traces',
    12: 'Independent Latin/cage counter, cage supports and failed-literal public-clue proofs',
    13: 'Independent run permutations/crossings, distinct-digit sum support and contradiction proofs',
    14: 'Independent Latin/inequality counter; public inequality propagation and contradiction proofs',
    15: 'Independent row/column visibility counter; public permutation supports and contradiction proofs',
    16: 'Independent mask/row counter; white connectivity/articulation checked against complete small spaces',
    17: 'Independent balanced-row counter; uniqueness/no-three and small full board enumeration',
    18: 'Independent Hamiltonian-path counter; value-to-position propagation; duplicate/range/parity negative cases',
    19: 'All visible Magic/PAN rules checked; multiple answers permitted; construction match count is not uniqueness',
    20: 'Independent subset-sum counter; full small bit-board spaces',
    21: 'Independent exhaustive move DAG; MASTER winning/losing public starts and exact CPU',
    22: 'Independent bounded pair DP versus generation order',
    23: 'Independent increasing-sum successor DAG; coprime MASTER starts',
    24: 'Arithmetic triples versus Lo Shu minimax; legal reachable ownership states',
    25: 'Independent remaining-distance DP versus exact residue policy',
    26: 'Merge-once, spawn/no-op RNG, replay, score bounds, separate CLASSIC/TARGET terminal goals',
    27: 'Independent parity and complete 181440-state 3x3 BFS; 4x4 legal path and admissible Manhattan lower bound',
    28: 'GF(2) RREF and full affine nullspace versus row-chasing generator; exact minimum press counts',
    31: 'Public-clue cell-first exact cover versus clue-first generator; complete small rectangle space',
    32: 'Public-clue face-colour counter versus edge generator; independent single-loop graph and complete 2x2 edges',
}


def jdump(value):
    return json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(',', ':'))


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load(path):
    return json.loads(path.read_text())


def counts(records):
    return [sum(p.get('difficulty', 3) == d for p in records) for d in range(5)]


def baseline_counts(gid):
    if gid == 2: return [90, 90, 90, 0, 0]
    if gid == 6: return [60, 60, 60, 0, 0]
    if gid in (3, 4, 5, 7, 9, 31, 32): return [30, 30, 30, 0, 0]
    if 11 <= gid <= 20: return [30, 30, 30, 20 if gid in (11, 12, 14, 15) else 0, 0]
    return [0] * 5


def array_text(text, name):
    match = re.search(r'\b' + re.escape(name) + r'\s*(?:\[[^\]]*\])+\s*=\s*\{', text)
    if not match: raise ValueError(f'Native array missing: {name}')
    # Generated native arrays contain no braces inside string literals except
    # arithmetic expressions; skip all quoted characters for brace accounting.
    start = match.end(); depth = 1; quoted = False; escaped = False
    for end in range(start, len(text)):
        ch = text[end]
        if quoted:
            if escaped: escaped = False
            elif ch == '\\': escaped = True
            elif ch == '"': quoted = False
        elif ch == '"': quoted = True
        elif ch == '{': depth += 1
        elif ch == '}':
            depth -= 1
            if not depth: return text[start:end]
    raise ValueError(f'Unclosed native array: {name}')


def c_numbers(text, name):
    return [int(x, 0) for x in re.findall(r'0x[\da-fA-F]+|\b\d+\b', array_text(text, name))]


def declared_count(text, name):
    match = re.search(r'\b' + re.escape(name) + r'\[(\d+)\]', text)
    return int(match[1]) if match else 0


def d4(values, n):
    variants = []
    for flip in (False, True):
        for turns in range(4):
            out = [0] * (n * n)
            for i, value in enumerate(values):
                y, x = divmod(i, n)
                if flip: x = n - 1 - x
                for _ in range(turns): y, x = x, n - 1 - y
                out[y*n+x] = value
            variants.append(tuple(out))
    return variants


def gc_key(gid, p, canonical=True):
    if gid == 2: return p['equation']
    if gid == 3:
        clues = [(tuple(a), n) for a, n in p['clues']]
        return p['n'], p['alphabet'], tuple(sorted(clues) if canonical else clues)
    if gid == 4: return p['limit'], tuple(p['params'])
    if gid == 5: return tuple(p['sequence'][:6])
    if gid in (6, 7): return tuple(sorted(p['cards']) if canonical else p['cards']), p['target']
    if gid == 8: return tuple(p['numbers']), p['target']
    return tuple(p['ops']), tuple(p['targets']), tuple(p['givens'])


def sq_key(gid, p, canonical=True):
    mode = p.get('cpu_first', p.get('mode', 0))
    b = p.get('board', [])
    if gid in (21, 22): return mode, tuple(sorted(b) if canonical else b)
    if gid == 23:
        divisor = math.gcd(*b) if canonical else 1
        return mode, tuple(x // divisor for x in sorted(b))
    if gid == 24:
        grid = [b[k-1] for k in (8, 1, 6, 3, 5, 7, 4, 9, 2)]
        return mode, min(d4(grid, 3)) if canonical else tuple(b)
    if gid == 25:
        return (mode, p['max_add'], p['target']-b[0]) if canonical else (mode, p['max_add'], p['target'], b[0])
    if gid == 27: return mode, tuple(b)
    n = p['size']; bits = [p['board_bits'] >> i & 1 for i in range(n*n)]
    return mode, min(d4(bits, n)) if canonical else tuple(bits)


def grid_records(root):
    result = []
    paths = [root / f'assets/grids/{g}.json' for g in range(11, 21)]
    paths += sorted((root / 'assets/grids/master').glob('[0-9][0-9].json'))
    paths += sorted((root / 'assets/grids/expanded').glob('[0-9][0-9]-[0-4].json'))
    for path in paths:
        for record in load(path): result.append((record, path))
    ids = [p['puzzle_id'] for p, _ in result]
    if len(ids) != len(set(ids)): raise ValueError('Duplicate GRID stable IDs in published source')
    return result


def import_grid_key(root):
    # Only definitions are imported; no generation, search or file output runs.
    path = root / 'tools/generate/grids_verify.py'
    sys.path.insert(0, str(path.parent))
    try:
        spec = importlib.util.spec_from_file_location('_inventory_grid_verify', path)
        module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
        return module.base_key
    finally: sys.path.pop(0)


def magic_stats(records):
    result = {}
    for d in sorted({p['difficulty'] for p in records}):
        group = [p for p in records if p['difficulty'] == d]
        result[LEVELS[d]] = dict(layouts=len(group), oriented_witnesses=len({tuple(p['solution']) for p in group}),
            d4_witnesses=len({(p['n'], min(d4(p['solution'], p['n']))) for p in group}))
        if d == 3:
            def canonical(p, torus=False):
                n=p['n']; keys=[]
                for values in d4(p['solution'], n):
                    for dy in range(n if torus else 1):
                        for dx in range(n if torus else 1):
                            shifted=tuple(values[((r+dy)%n)*n+(c+dx)%n] for r in range(n) for c in range(n))
                            keys += [shifted, tuple(n*n+1-v for v in shifted)]
                return min(keys)
            result[LEVELS[d]].update(d4_complement_witnesses=len({canonical(p) for p in group}),
                torus_complement_witnesses=len({canonical(p, True) for p in group}))
    return result


def build_inventory(root, staged=None, catalog=None):
    root = root.resolve(); manifest = {}; warnings = []; rows = []
    def track(path, prefix=''):
        label = prefix + path.relative_to(staged if prefix else root).as_posix()
        manifest[label] = {'bytes': path.stat().st_size, 'sha256': digest(path)}
        return label
    def paths_exist(names):
        for name in names:
            path = root / name
            if not path.exists(): raise ValueError(f'Required inventory evidence missing: {name}')
            track(path)
        return names
    def read(name):
        track(root / name); return load(root / name)
    gc_old = read('assets/guesscalc_packs.json'); gc_master = read('assets/guesscalc_master.json')
    sq = read('assets/strategyquick/master.json'); boards = read('assets/boards/puzzles.json')['puzzles']
    all_grid = grid_records(root); grid_by_id = {p['puzzle_id']: p for p, _ in all_grid}
    for _, path in all_grid: track(path)
    grid_native = (root / 'src/games/grids_pack.c').read_text()
    offsets = c_numbers(grid_native, 'grids_pack_offsets')
    bank_ids = c_numbers(grid_native, 'grids_bank_ids')
    group_flat = c_numbers(grid_native, 'grids_bank_groups')
    if len(group_flat) != 100 or len(offsets) != len(bank_ids)+1: raise ValueError('GRID index dimensions mismatch')
    payload = c_numbers(grid_native, 'grids_pack_bytes')
    if offsets[0] != 0 or offsets[-1] != len(payload) or any(a >= b for a,b in zip(offsets, offsets[1:])):
        raise ValueError('GRID payload offsets/lengths mismatch')
    if sum(group_flat[1::2]) != len(bank_ids): raise ValueError('GRID group counts do not cover bank IDs')
    groups = [group_flat[i:i+2] for i in range(0, 100, 2)]
    memory_path=root/'assets/grids/expanded/memory.json'
    grid_memory=read('assets/grids/expanded/memory.json') if memory_path.exists() else {}
    if grid_memory and (grid_memory['pack_payload_bytes']!=len(payload) or grid_memory['pack_rodata_bytes']!=len(payload)+6*len(bank_ids)+204):
        raise ValueError('GRID memory evidence is stale')
    if sorted(bank_ids) != list(range(len(bank_ids))): raise ValueError('GRID native IDs are not a permutation')
    if any(i not in grid_by_id for i in bank_ids): raise ValueError('GRID native record has no published source JSON')
    staged_grid = []
    if staged:
        for p, path in grid_records(staged):
            old = grid_by_id.get(p['puzzle_id'])
            if old is not None:
                if old != p: raise ValueError(f'Staged GRID identity conflict: {p["puzzle_id"]}')
            else:
                staged_grid.append(p); track(path, 'staged-grids/')
        warnings.append('Staged GRID JSON is a separate worktree snapshot; it is not counted as integrated/native content. Pending JSON is excluded.')
    grid_key = import_grid_key(root)
    native_gc_old = (root / 'assets/guesscalc_packs.h').read_text()
    native_gc_master = (root / 'assets/guesscalc_master.h').read_text()
    sq_native = (root / 'assets/strategyquick/master_strategy.h').read_text()
    strategy_counts = c_numbers(sq_native, 'sq_master_start_count')
    if len(strategy_counts) != 10: raise ValueError('Strategy count dimensions mismatch')
    catalog_games = {g['id']: g for g in catalog['games']} if catalog else {}
    if catalog and set(catalog_games) != set(IDS): raise ValueError('Actual registry visible IDs differ')
    for position, gid in enumerate(IDS):
        before = baseline_counts(gid); mode_names = MODES.get(gid, ['CLASSIC' if 11 <= gid <= 20 else 'STANDARD'])
        generators = []; native_assets = []; source_assets = []; record_bytes = ''; frame = ''; flash = 0
        policy_bytes = 0; scratch = '0 heap / 0 decode scratch'; staged_records = []; native_records = []; records = []
        native_counts = [0]*5; bank_modes = [[0]*5 for _ in mode_names]; symbols = []
        before_supply = 'HOST_GENERATED_BANK'; current_supply = before_supply
        if gid <= 10:
            family='GC'; entry='src/games/guesscalc.c:init_'+GC_INIT[gid-1]+'()'
            source_assets=['assets/guesscalc_packs.json', 'assets/guesscalc_master.json']
            generators=['tools/generate/guesscalc_generate.py', 'tools/generate/guesscalc_master.py']
            native_assets=['assets/guesscalc_packs.h', 'assets/guesscalc_master.h']
            if gid in GC_KEYS:
                key=GC_KEYS[gid]; records=gc_old.get(key, [])+gc_master.get(key, [])
                symbol=GC_SYMBOLS[gid]; old_name=f'gc_{symbol}_pack'; new_name=f'gc_master_{symbol}'
                for text, name, actual in ((native_gc_old, old_name, len(gc_old.get(key, []))), (native_gc_master, new_name, len(gc_master.get(key, [])))):
                    if declared_count(text, name) != actual: raise ValueError(f'{name}: source/native count mismatch')
                    if actual: symbols.append(name)
                native_records=records; native_counts=counts(records)
                for p in records:
                    mode=p.get('mode', p['puzzle_id']//90 if gid==2 and p['difficulty']<3 else int(p['target']==10) if gid==6 else 0)
                    bank_modes[mode][p['difficulty']]+=1
                exact=len({gc_key(gid,p,False) for p in records}); canonical=len({gc_key(gid,p) for p in records})
            else:
                exact=canonical=0; before_supply=current_supply='RUNTIME_GENERATED'
                source_assets=[]; native_assets=[]; generators=[]
            if gid==8: before_supply='RUNTIME_GENERATED'; current_supply='HYBRID'
            flash=sum(GC_BYTES_MASTER[gid-1] if p['difficulty']==3 else GC_BYTES_OLD[gid-1] for p in records)
            record_bytes=f'E/N/H {GC_BYTES_OLD[gid-1]}; MASTER {GC_BYTES_MASTER[gid-1]}'
            frame=str(GC_FRAMES[gid-1])
        elif gid<=20:
            family='GRID'; entry='src/games/grids.c:init() -> ng_bank_pick -> grids_bank_id -> grids_record -> grids_decode'
            generators=['tools/generate/grids_generate.py', 'tools/generate/grids_master_generate.py']
            if (root/'tools/generate/grids_expand.py').exists(): generators+=['tools/generate/grids_expand.py']
            native_assets=['src/games/grids_pack.c', 'src/games/grids_pack_data.h']
            symbols=['grids_pack_bytes','grids_pack_offsets','grids_bank_ids','grids_bank_groups']
            source_assets=sorted({path.relative_to(root).as_posix() for p,path in all_grid if p['id']==gid})
            records=[p for p,_ in all_grid if p['id']==gid]
            staged_records=[p for p in staged_grid if p['id']==gid]
            for d in range(5):
                start,n=groups[(gid-11)*5+d]; native_counts[d]=n
                ids=bank_ids[start:start+n]
                if len(ids)!=n or any(grid_by_id[i]['id']!=gid or grid_by_id[i]['difficulty']!=d for i in ids): raise ValueError('GRID native group semantics differ')
                native_records.extend(grid_by_id[i] for i in ids)
            bank_modes=[native_counts[:] for _ in mode_names]
            exact=len({jdump({k:p.get(k) for k in ('n','cells','a','b')}) for p in records})
            canonical=len({(p['n'],grid_key(p)) for p in records})
            lengths=[offsets[p['puzzle_id']+1]-offsets[p['puzzle_id']] for p in native_records]
            flash=sum(lengths)+len(native_records)*6+20
            record_bytes=f'packed {min(lengths)}..{max(lengths)}; decoded 490'
            scratch='shared decoded cache 490 B + cache ID 4 B, ONCE for family; decoder local Reader; heap 0'
            frame=str(grid_memory['frames_bytes']['init']) if grid_memory else 'legacy load 60; compact/expanded frame: see final SH report'
            if grid_memory:
                measured=next(p for p in grid_memory['groups'] if p['game_id']==gid)
                if (measured['records'],measured['encoded_payload_bytes'],measured['record_bytes_min'],measured['record_bytes_max'])!=(len(lengths),sum(lengths),min(lengths),max(lengths)):
                    raise ValueError('GRID per-game memory evidence is stale')
            if gid==19: before_supply=current_supply='BANK_WITH_VALID_TRANSFORMS'
        elif gid<=28:
            family='SQ'; entry='src/games/strategyquick_'+('strategy.c:sq_strategy_init()' if gid<=25 else 'quick.c:sq_quick_init()')
            generators=['tools/generate/strategyquick_tables.py','tools/generate/strategyquick_master.py']
            source_assets=['assets/strategyquick/master.json']; before_supply='RULE_BASED_GAME_WITHOUT_PUZZLE_BANK' if gid<=26 else 'RUNTIME_GENERATED'
            current_supply='RULE_BASED_GAME_WITHOUT_PUZZLE_BANK' if gid==26 else 'HYBRID'
            if gid<=25:
                records=[p for p in sq['strategy'] if p['game_id']==gid]
                bank_modes[0][3]=strategy_counts[(gid-21)*2];bank_modes[1][3]=strategy_counts[(gid-21)*2+1];bank_modes[2]=bank_modes[0][:]
                if [sum(p['cpu_first']==v for p in records) for v in range(2)]!=[bank_modes[0][3],bank_modes[1][3]]:raise ValueError('Strategy native counts differ')
                native_assets=['assets/strategyquick/master_strategy.h']; symbols=['sq_master_starts','sq_master_start_count'];flash=722;record_bytes='12; arrays reserve 60 slots/game including unused padding';frame='28'
                policy_bytes={22:211,23:1250,24:4921}.get(gid,0)
                if policy_bytes:native_assets+=['assets/strategyquick/tables.h']
            elif gid!=26:
                key='sliding' if gid==27 else 'lights'; records=sq[key]
                for p in records:bank_modes[p['mode']][3]+=1
                native_assets=['assets/strategyquick/master_quick.h'];symbols=['sq_master_sliding3','sq_master_sliding4'] if gid==27 else ['sq_master_lights','sq_master_lights_min']
                flash=498 if gid==27 else 300;record_bytes='9 / 16 by size' if gid==27 else '4 board bits + 1 minimum';frame='148 (shared quick init)'
            else:
                source_assets=[]; generators=[]; frame='148 (shared quick init)';record_bytes='0'
            native_records=records;native_counts=counts(records)
            exact=len({sq_key(gid,p,False) for p in records});canonical=len({sq_key(gid,p) for p in records})
        else:
            family='BOARD';entry='src/games/boards.c:boards_init() -> ng_bank_pick'
            generators=['tools/generate/boards_generate.py'];source_assets=['assets/boards/puzzles.json'];native_assets=['src/games/boards_pack.c']
            symbols=['nb_shikaku_pack' if gid==31 else 'nb_slitherlink_pack'];records=[p for p in boards if p['game_id']==gid]
            body=array_text((root/'src/games/boards_pack.c').read_text(),symbols[0])
            if len(re.findall(r'\{\d+,\{',body))!=len(records):raise ValueError('BOARD initializer count mismatch')
            native_records=records;native_counts=counts(records);bank_modes=[native_counts[:]]
            exact=len({(p['size'],tuple(p['clues'])) for p in records});canonical=len({(p['size'],min(d4(p['clues'],p['size']))) for p in records})
            flash=65*len(records);record_bytes='65';frame='12'
        paths_exist(generators+native_assets+source_assets)
        paths_exist([entry.split(':')[0]])
        base_flash=sum(before)*(GC_BYTES_OLD[gid-1] if gid<=10 else 490 if gid<=20 else 65 if gid>=31 else 0)
        row=dict(game_id=gid,name=NAMES[position],category=CATEGORIES[position//5],supply_before=before_supply,supply_current=current_supply,
            runtime_entry=entry,runtime_generation_bound=RUNTIME_BOUNDS.get(gid,'Direct selection and one-record load; no native search-based construction'),host_generators=generators,native_assets=native_assets,native_arrays=symbols,source_assets=source_assets,
            baseline_levels=before,source_levels=counts(records),native_levels=native_counts,staged_additional_levels=counts(staged_records),
            baseline_records=sum(before),baseline_exact_distinct=sum(before),baseline_canonical_distinct=85 if gid==19 else sum(before),
            source_records=len(records),native_records=len(native_records),staged_additional_records=len(staged_records),
            exact_distinct_source=exact,canonical_distinct_source=canonical,normalization=NORMALIZATION[gid],modes=mode_names,bank_counts_by_mode=bank_modes,
            baseline_difficulty_count=4 if gid in (11,12,14,15) else 3,current_difficulty_count=5 if 11<=gid<=15 else 4,regular_difficulty_count=4,has_hell=11<=gid<=15,
            runtime_variants_per_selected_record=1 if records else 0,external_files=0,external_parsed_records=0,external_bundled_puzzles=0,
            source_url='N/A: original local generation; rules references are separate',baseline_revision=BASELINE,current_revision='Source file SHA256 manifest (works without Git history)',source_rules_versions=sorted({p.get('rules_version',1) for p in records}),license='MIT (original code/problem data); external puzzle-data license N/A',
            external_evidence=EVIDENCE[family],validation=VALIDATION[gid],baseline_bank_flash_bytes=base_flash,current_bank_flash_bytes=flash,
            policy_flash_bytes=policy_bytes,record_bytes=record_bytes,active_game_bytes=1848,heap_decode_scratch=scratch,individual_init_frame_bytes=frame,
            device_stack_heap_peak='NOT MEASURED; HARDWARE TEST REQUIRED')
        if catalog:
            actual=catalog_games[gid]; actual_counts=[v+[0]*(5-len(v)) for v in actual['bank_counts_by_mode']]
            if bank_modes!=actual_counts:raise ValueError(f'ID {gid}: actual registry bank counts differ: {bank_modes} vs {actual_counts}')
            if len(actual['modes'])!=len(mode_names):raise ValueError(f'ID {gid}: actual mode count differs')
            if (actual['difficulties'],actual['regular_difficulties'],actual['has_hell']) != (row['current_difficulty_count'],4,row['has_hell']):
                raise ValueError(f'ID {gid}: actual difficulty support differs')
            policies={'RUNTIME_GENERATED':0,'HOST_GENERATED_BANK':1,'BANK_WITH_VALID_TRANSFORMS':2,'HYBRID':3,'RULE_BASED_GAME_WITHOUT_PUZZLE_BANK':4}
            if actual['generation_policy']!=policies[current_supply]:raise ValueError(f'ID {gid}: actual supply classification differs')
            if catalog['game_size_host']!=1848:raise ValueError('NgGame ABI changed: update measured memory evidence')
            row['name']=actual['name'];row['modes']=actual['modes']
        if row['native_levels']!=row['source_levels']:warnings.append(f'ID {gid}: published JSON levels {row["source_levels"]} differ from native levels {row["native_levels"]}.')
        rows.append(row)
    paths_exist(['LICENSE','docs/ASSET_PROVENANCE.md','THIRD_PARTY_NOTICES.md','docs/GUESS_CALC_AUDIT.md','docs/GRID_AUDIT.md','docs/GRID_MASTER_AUDIT.md','docs/STRATEGY_QUICK_AUDIT.md','docs/BOARD_AUDIT.md','tools/generate/grids_verify.py','assets/grids/master/legacy-sha256.json'])
    for row in rows:
        row['asset_sha256']={name:manifest[name]['sha256'] for name in row['source_assets']+row['native_assets']}
    published_grid = [p for p,_ in all_grid]
    if len(bank_ids)!=1750:warnings.append(f'GRID native currently contains {len(bank_ids)} records. 1750 is a target, not the measured integrated total.')
    audit_path=root/'assets/grids/expanded/audit.json'
    final_grid_audit=load(audit_path) if audit_path.exists() else {}
    if not final_grid_audit.get('complete'):
        warnings.append('No complete:true integrated expanded GRID audit is present; expanded completion is not certified here.')
    else:
        track(audit_path)
        if final_grid_audit.get('total_records') != len(bank_ids):raise ValueError('Complete GRID audit/native count mismatch')
        audited=set()
        for group in final_grid_audit.get('groups',[]):
            path=root/f"assets/grids/expanded/{group['game_id']}-{group['difficulty']}.json"
            if not path.exists() or digest(path)!=group['sha256']:raise ValueError('Complete GRID audit hash is stale')
            audited.add(path)
        published=set((root/'assets/grids/expanded').glob('[0-9][0-9]-[0-4].json'))
        if audited!=published:raise ValueError('Complete GRID audit does not cover published expanded files')
    magic={'integrated_source':magic_stats([p for p in published_grid if p['id']==19]),'staged_additions':magic_stats([p for p in staged_grid if p['id']==19])}
    return rows,manifest,warnings,magic


def md_table(headers, rows):
    def esc(value):return str(value).replace('|','\\|').replace('\n',' ')
    return '\n'.join(['| '+' | '.join(headers)+' |','| '+' | '.join(['---']*len(headers))+' |']+['| '+' | '.join(esc(v) for v in row)+' |' for row in rows])


def render_md(rows,manifest,warnings,magic,catalog_used):
    levels=lambda values:'/'.join(map(str,values))
    total=lambda field:sum(r[field] for r in rows)
    out=['# Content inventory',
         'Generated by `tools/content_inventory.py` (options and regeneration commands below). This inventories the files that exist in the selected source tree. It does not generate content or rerun expensive solution counters. Re-run family verifiers for content correctness. Stable visible IDs are 1–28, 31, 32; legacy-only 29/30 are excluded.',
         f'Baseline: `{BASELINE}`. Verified baseline counts/hashes are embedded in the script so the inventory also works in a clean public source snapshot without historical Git objects. Current assets are identified by SHA256 below. Actual registry cross-check: **{"USED" if catalog_used else "NOT RUN (pass --catalog from tools/export_catalog.c)"}**.',
         f'Baseline bank records: **{total("baseline_records")}**. Current source records: **{total("source_records")}**. Current native bank records: **{total("native_records")}**. Additional published staged GRID records: **{total("staged_additional_records")}**. These totals mix unique-solution puzzles, expression challenges, tactical starts and Magic layouts; they are not a count of unique-solution puzzles. Rule-based/runtime games are not assigned fictitious bank records.',
         '## Snapshot boundaries',
         '\n'.join('- '+x for x in warnings) if warnings else 'Published source/native counts agree. Final independent verification remains separate.',
         'A staged count is development evidence only. `.pending.json`, generation logs, witnesses, alternate solutions, AI policy states, shared LOCAL modes and unbuilt target counts are never added as new native puzzle bases. A runtime variant count of 1 means no additional runtime transform is applied to the selected record; stored mathematical orientations are already included in the record count.',
         '## Before and current counts',
         'Level vectors are EASY / NORMAL / HARD / MASTER / HELL. Zero denotes no bank records, not an unsupported rule-based game. General games have four selectable levels; LOGIC 11–15 supports HELL. CLASSIC 2048 has a fixed rule bucket and its level row is hidden. Missing native GRID groups during generation are explicitly visible as zeros.',
         md_table(['ID / category / game','Baseline levels','Current source levels','Native levels','Staged additions','Baseline exact / canonical','Current exact / canonical'],[(f'{r["game_id"]:02} {r["category"]} {r["name"]}',levels(r['baseline_levels']),levels(r['source_levels']),levels(r['native_levels']),levels(r['staged_additional_levels']),f'{r["baseline_exact_distinct"]} / {r["baseline_canonical_distinct"]}',f'{r["exact_distinct_source"]} / {r["canonical_distinct_source"]}') for r in rows]),
         'Canonical counts use the stated equivalence relation, not an unspecified “different puzzle” claim. Exact keys contain public content and exclude identity/seed/difficulty metadata. Sequence uniqueness is only within its finite published grammar. Equation target text is the puzzle, so arithmetic-equivalent equations remain distinct.',
         '## Supply, modes and normalization',
         md_table(['ID','Before → current supply','Modes / native bank vectors','Normalization'],[(r['game_id'],r['supply_before']+' → '+r['supply_current'],'; '.join(m+': '+levels(c) for m,c in zip(r['modes'],r['bank_counts_by_mode'])),r['normalization']) for r in rows]),
         'Strategy LOCAL 2P selects the YOU FIRST bank; it adds no new originals. Nim/Wythoff/Euclid/Race E/N/H begin from bounded randomized rule states; Make Fifteen starts empty. MASTER tactical banks change starts, while HARD and MASTER share the same exact CPU. Sliding retains only two maximum-distance 3×3 starts and 30 4×4 starts; it is not padded to 30 by duplicates.',
         '### Magic layouts and open rules',
         'Magic FREE still selects a bank ID to obtain dimensions/rule metadata and a reveal witness. **A bank count of 30 does not mean 30 different FREE challenges.** Clearing all givens leaves one open challenge per size/rule set: ordinary 3×3 or 4×4 Magic, and MASTER 4×4 PAN. PARTIAL supplies clue layouts and permits every rule-valid answer; no unique-solution claim is made. Baseline 90 layouts contain two underlying construction squares, 16 oriented witnesses and 85 D4-distinct public layouts.',
         '```json\n'+json.dumps(magic,indent=2,sort_keys=True)+'\n```',
         'MASTER PAN witness counts, when present, explicitly distinguish D4, D4 plus complement, and cyclic row/column translations plus complement. These are mathematical square classes, not additional downloaded or generated puzzle records.',
         '## Runtime path, host construction and verification',
         md_table(['ID','Runtime entry','Host generators','Native arrays','Generation bound / validation'],[(r['game_id'],r['runtime_entry'],'; '.join(r['host_generators']) or 'None: runtime/rules','; '.join(r['native_arrays']) or 'None',r['runtime_generation_bound']+'; '+r['validation']) for r in rows]),
         'Human difficulty is not certified by these instruments. GRID search nodes and failed-literal traces are instrument-specific; BOARD structural ranges overlap HARD; Sliding Manhattan is a lower bound; Lights minimum counts are exact for the selected rules. See family audits for measured independent tests and limits.',
         '## Per-puzzle memory',
         'These are source/SH-ABI payload and compiler-frame facts, not a final G3A/ELF size or observed RAM peak. Each game occupies the already allocated common NgGame (1848 B current, 1832 B baseline). Do not multiply that by the number of bank records or sum repeated cache/frame values. Shared session, undo, storage, renderer and SDK arenas belong to the common memory audit.',
         md_table(['ID','Bank flash before → current B','Separate AI policy B','Native bytes/record','Init frame B (individual)','Heap / decoding'],[(r['game_id'],f'{r["baseline_bank_flash_bytes"]} → {r["current_bank_flash_bytes"]}',r['policy_flash_bytes'],r['record_bytes'],r['individual_init_frame_bytes'],r['heap_decode_scratch']) for r in rows]),
         'GRID per-game flash includes its packed record bytes, 4-byte offsets and 2-byte bank IDs per record, plus its five 4-byte group entries. Add **one shared final offset of 4 B** to the family total. The decoded 490-byte cache and 4-byte cache ID occur once for the whole GRID family; no whole bank is inflated into RAM. Strategy arrays include unused fixed padding, so physical bytes can exceed the number of reachable starts times record size. AI policy bytes (6382 B total) are never counted as puzzles.',
         'GC expression parser project-only conservative frame sum is 3044 B including the rejecting depth frame; it excludes caller/library frames and is not an observed peak. The old strategy whole-NgGame temporaries were removed: pick/valid individual frames are now 56/56 B versus 1880/1872 B baseline. Quick init is shared; Lights solver frame is 236 B, board completion maximum individual frame 316 B. GRID individual init frames, when available, are read from assets/grids/expanded/memory.json and its payload figures are cross-checked against the native offsets. The measured compact decoder/cache-accessor frames are 76/12 B and rules/render/valid/action frames are 508/224/60/28 B; compiler settings and the exact source snapshot accompany that report. These are not call-chain peaks. **Device peaks remain NOT MEASURED / HARDWARE TEST REQUIRED.**',
         '## External data and licenses',
         '**External puzzle files: 0. External parsed puzzle records: 0. External puzzle records bundled: 0, for all 30 rows.** These are audited input-provenance findings: deterministic constructors read seeds/local authored structures, the runtime call paths above read embedded arrays or bounded rule state, and introduction/regeneration/independent verification evidence is preserved below. They are not inferred solely from a README claim or the lack of a downloads folder. No external corpus URL or external puzzle-data license applies.',
         md_table(['Family','Evidence'],EVIDENCE.items()),
         'Original code and generated problem data are covered by the repository MIT license. Rules-only references (Nikoli, Project Euler, Simon Tatham, James Harvey, Gabriele Cirulli) are recorded in `docs/ASSET_PROVENANCE.md`; referencing rules is not importing their puzzle examples. Imported gint font atlases/runtime, fxSDK, FxLibc, OpenLibm, GCC runtime and adapted DIFF EQ/SOKOBAN infrastructure have separate notices in `THIRD_PARTY_NOTICES.md`. “External puzzle files 0” does not mean “no third-party software/font assets”.',
         '### Frozen baseline hashes',
         md_table(['Baseline path','SHA256'],BASELINE_HASHES.items()),
         'Additional retained grid baseline hashes are in `assets/grids/master/legacy-sha256.json`; the four original MASTER JSON hashes are checked by the GRID expanded verifier. Existing record identities are preserved rather than renumbered by inventory generation.',
         '### Current source manifest',
         'Paths prefixed `staged-grids/` are relative to the optional staged worktree and are not integrated assets. The CSV includes complete per-game asset hashes. Native data files and JSON have different representations; family embedded validators, not equal file hashes, establish their equivalence.',
         md_table(['Path','Bytes','SHA256'],[(name,v['bytes'],v['sha256']) for name,v in sorted(manifest.items())]),
         '## Regeneration',
         '```sh\npython3 tools/content_inventory.py\n# Optional actual compiled registry cross-check:\n<export_catalog executable> > <catalog.json>\npython3 tools/content_inventory.py --catalog <catalog.json>\n# Development-only separate staged counts:\npython3 tools/content_inventory.py --staged-grid-root .worktrees/grids\n```',
         'Use `--source-root PATH --output-dir PATH` to inspect a root integration from an isolated worktree without writing root files. `--check` compares the generated MD/CSV with existing files; it performs no writes. Re-run without a staged tree for a clean final source export. All paths and baseline facts required by the default command are contained in the source snapshot.']
    return '\n\n'.join(out)+'\n'


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root',type=Path,default=Path(__file__).resolve().parents[1])
    parser.add_argument('--output-dir',type=Path)
    parser.add_argument('--staged-grid-root',type=Path)
    parser.add_argument('--catalog',type=Path)
    parser.add_argument('--check',action='store_true')
    args=parser.parse_args(); output=args.output_dir or args.source_root/'docs'
    staged=args.staged_grid_root.resolve() if args.staged_grid_root else None
    catalog=load(args.catalog) if args.catalog else None
    rows,manifest,warnings,magic=build_inventory(args.source_root,staged,catalog)
    import io
    buffer=io.StringIO(newline=''); writer=csv.DictWriter(buffer,fieldnames=list(rows[0]),lineterminator='\n');writer.writeheader()
    for row in rows:writer.writerow({key:jdump(value) if isinstance(value,(dict,list)) else value for key,value in row.items()})
    documents={'CONTENT_INVENTORY.csv':buffer.getvalue(),'CONTENT_INVENTORY.md':render_md(rows,manifest,warnings,magic,bool(catalog))}
    for name,text in documents.items():
        path=output/name
        if args.check:
            if not path.exists() or path.read_text()!=text:raise SystemExit(f'Inventory differs: {path}')
        else:
            output.mkdir(parents=True,exist_ok=True);path.write_text(text)
    print(jdump(dict(visible_games=len(rows),baseline_records=sum(r['baseline_records'] for r in rows),source_records=sum(r['source_records'] for r in rows),native_records=sum(r['native_records'] for r in rows),staged_additional_records=sum(r['staged_additional_records'] for r in rows),warnings=warnings)))

if __name__=='__main__':main()
