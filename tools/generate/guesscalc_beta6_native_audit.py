#!/usr/bin/env python3
"""All-record C header/JSON parity through the production C expression parser."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'assets/guesscalc_beta6_native_audit.json'


def run():
    folder=ROOT/'build-host/guesscalc-beta6'
    folder.mkdir(parents=True,exist_ok=True)
    executable=folder/'native-audit'
    subprocess.run(['clang','-std=c11','-Wall','-Wextra','-Werror','-O1',
                    '-fsanitize=undefined','-fno-sanitize-recover=all',
                    '-Isrc/games','tools/generate/guesscalc_beta6_native.c',
                    'src/games/guesscalc_math.c','-o',str(executable)],
                   cwd=ROOT,check=True)
    result=subprocess.run([str(executable)],cwd=ROOT,capture_output=True,text=True,check=True)
    actual=[json.loads(line) for line in result.stdout.splitlines()]
    targets=json.loads((ROOT/'assets/guesscalc_target_beta6.json').read_text())['records']
    countdown=json.loads((ROOT/'assets/guesscalc_countdown_beta6.json').read_text())['records']
    assert len(actual)==len(targets)+len(countdown)==4800
    for got,want in zip(actual[:4000],targets):
        assert ((got['game_id'],got['puzzle_id'],got['difficulty'],got['target'],
                 got['cards'],got['answer'],got['hint']) ==
                (6,want['puzzle_id'],want['difficulty'],want['target'],
                 want['cards'],want['answer'],want['hint']))
    for got,want in zip(actual[4000:],countdown):
        assert ((got['game_id'],got['puzzle_id'],got['difficulty'],got['target'],
                 got['cards'],got['answer'],got['hint']) ==
                (7,want['puzzle_id'],want['difficulty'],want['target'],
                 want['cards'],want['answer'],want['hint']))
    return dict(target_native_records=4000,countdown_native_records=800,
                native_expression_parser_accepts=4800,
                native_card_occurrence_validator_accepts=4800,
                c_header_size_bytes={name:(ROOT/'assets'/name).stat().st_size for name in
                                     ('guesscalc_target_beta6.h','guesscalc_countdown_beta6.h')},
                source_sha256={name:hashlib.sha256((ROOT/'assets'/name).read_bytes()).hexdigest() for name in
                               ('guesscalc_target_beta6.json','guesscalc_countdown_beta6.json')})


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    mode=parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--write',action='store_true')
    mode.add_argument('--check',action='store_true')
    args=parser.parse_args()
    audit=run()
    payload=json.dumps(audit,indent=2)+'\n'
    if args.write:
        OUT.write_text(payload)
    else:
        assert OUT.read_text()==payload
    print('PASS native beta.6 card/answer/header parity 4000 + 800',flush=True)


if __name__=='__main__':
    main()
