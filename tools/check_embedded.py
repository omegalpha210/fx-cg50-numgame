#!/usr/bin/env python3
"""Ensure verified JSON/AI sources match embedded C without changing repo files."""
import importlib.util,json,pathlib,runpy,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/generate'))
def module(name):
    spec=importlib.util.spec_from_file_location(name,ROOT/'tools/generate'/f'{name}.py')
    obj=importlib.util.module_from_spec(spec);spec.loader.exec_module(obj);return obj
(ROOT/'build-host').mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix='embedded-',dir=ROOT/'build-host') as directory:
    tmp=pathlib.Path(directory)
    gc=module('guesscalc_generate');gc.OUT=tmp
    gc.write_header(json.loads((ROOT/'assets/guesscalc_packs.json').read_text()))
    assert (tmp/'guesscalc_packs.h').read_bytes()==(ROOT/'assets/guesscalc_packs.h').read_bytes()
    gc_master=module('guesscalc_master');gc_master.OUT=tmp
    gc_master.write_header(json.loads((ROOT/'assets/guesscalc_master.json').read_text()))
    assert (tmp/'guesscalc_master.h').read_bytes()==(ROOT/'assets/guesscalc_master.h').read_bytes()
    grids=module('grids_generate');grids.ROOT=tmp;grids.OUT=tmp/'assets/grids'
    (tmp/'src/games').mkdir(parents=True)
    (tmp/'assets/grids/master').mkdir(parents=True)
    (tmp/'assets/grids/expanded').mkdir(parents=True)
    for id in range(11,21):
        (tmp/f'assets/grids/{id}.json').write_bytes((ROOT/f'assets/grids/{id}.json').read_bytes())
    for id in (11,12,14,15):
        (tmp/f'assets/grids/master/{id}.json').write_bytes((ROOT/f'assets/grids/master/{id}.json').read_bytes())
    expanded=list((ROOT/'assets/grids/expanded').glob('[0-9][0-9]-[0-9].json'))
    assert len(expanded)==30, f'Expected 30 expanded grade groups, found {len(expanded)}'
    for source in expanded:
        (tmp/'assets/grids/expanded'/source.name).write_bytes(source.read_bytes())
    grids.emit()
    assert (tmp/'src/games/grids_pack.c').read_bytes()==(ROOT/'src/games/grids_pack.c').read_bytes()
    assert (tmp/'src/games/grids_pack_data.h').read_bytes()==(ROOT/'src/games/grids_pack_data.h').read_bytes()
    # Run the deterministic table generator with its file location relocated to
    # the isolated temporary root. No imported native table is used as input.
    (tmp/'tools/generate').mkdir(parents=True);(tmp/'assets/strategyquick').mkdir(parents=True)
    script=tmp/'tools/generate/strategyquick_tables.py'
    script.write_bytes((ROOT/'tools/generate/strategyquick_tables.py').read_bytes())
    runpy.run_path(str(script),run_name='__main__')
    for name in ('tables.h','metadata.json'):
        assert (tmp/'assets/strategyquick'/name).read_bytes()==(ROOT/'assets/strategyquick'/name).read_bytes()
boards=module('boards_verify');boards.embedded_records(json.loads((ROOT/'assets/boards/puzzles.json').read_text())['puzzles'])
sq_master=module('strategyquick_master_verify');sq_master.embedded(json.loads((ROOT/'assets/strategyquick/master.json').read_text()))
print('Verified JSON -> native C and independently regenerated AI bytes MATCH')
