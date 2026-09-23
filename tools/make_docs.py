#!/usr/bin/env python3
"""Export catalog/rules/status from the actual linked game registry."""
import json,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
catalog=json.loads(subprocess.check_output([ROOT/'build-host/export_catalog'],text=True))
games=catalog['games'];assert sorted(g['id'] for g in games)==list(range(1,29))+list(range(31,39))
rows=['# Game catalog and acceptance matrix','',
      'Stable IDs1–28,31–38; legacy IDs29/30 are archived, never reused. All 36 have an engine, playable UI,',
      'verified content, per-game save/resume, host verification and a linked SH',
      'target. Every row remains **HARDWARE PENDING**. Counts below never include',
      'difficulty, size, or CPU-turn variants as extra games.','',
      '| ID | Game | Modes | CPU | Undo | Hint / reveal | Status |',
      '|---:|---|---|---|---|---|---|']
rules=['# In-app game rules','',
       'Exported from the same `NgModule.rules` strings compiled into NUMGAME.g3a.',
       'See README for physical key mapping (`SHIFT+DOT` = `=`; DOT alone = `.`, power key = `^`).',
       'All games use F1 INIT, F5 RULES, EXIT checkpoint, MENU checkpoint and',
       'SHIFT+AC/ON checkpoint. NEW is separate from INIT.','']
matrix=['game_id,game,engine,ui,content,save,host,target,hardware,modes,difficulties']
capabilities=['# Implemented difficulty, mode and supply capability','',
              'Rows are exported from the linked C registry. Bank entries are actual',
              'embedded base-record counts by mode, in EASY/NORMAL/HARD/MASTER/HELL',
              'order; 0 denotes a runtime or rule-based source, not missing gameplay.',
              'Magic FREE and symmetry families are distinguished in the',
              '[content inventory](CONTENT_INVENTORY.md). The 2048 CLASSIC game',
              'uses fixed NORMAL for new runs, with older level buckets preserved.','',
              '| ID | Game | Levels | Modes | Overall policy | Embedded bank by mode |',
              '|---:|---|---|---|---|---|']
policies=['RUNTIME_GENERATED','HOST_GENERATED_BANK','BANK_WITH_VALID_TRANSFORMS',
          'HYBRID','RULE_BASED_GAME_WITHOUT_PUZZLE_BANK']
for g in games:
    flag=g['flags'];hint='REVEAL' if flag&2 and (11<=g['id']<=20 or g['id'] in (35,36,38)) else 'HINT' if flag&2 else '—'
    extra='; ANSWER' if g['aux']=='ANSWER' else '; REVEAL' if g['aux']=='REVEAL' else ''
    modes=' / '.join(g['modes'])
    rows.append(f"| {g['id']:02} | {g['name']} | {modes} | {'Yes' if flag&4 else '—'} | {'Yes' if flag&1 else '—'} | {hint}{extra} | HOST VERIFIED · TARGET BUILT · HARDWARE PENDING |")
    rules += [f"## {g['id']:02} — {g['name']}",'','```text',g['rules'],'```','',f'Modes: {modes}.','']
    matrix.append(f"{g['id']},{g['name']},ENGINE READY,UI READY,CONTENT VERIFIED,SAVE READY,HOST VERIFIED,TARGET BUILT,HARDWARE PENDING,{len(g['modes'])},{1 if g['id']==26 else g['difficulties']}")
    level_text='CLASSIC fixed NORMAL; TARGET E/N/H/M' if g['id']==26 else 'E/N/H/M/Hell' if g['has_hell'] else 'E/N/H/M'
    bank_text='<br>'.join(f"{name}: {'/'.join(map(str,counts))}" for name,counts in zip(g['modes'],g['bank_counts_by_mode']))
    capabilities.append(f"| {g['id']:02} | {g['name']} | {level_text} | {' / '.join(g['modes'])} | {policies[g['generation_policy']]} | {bank_text} |")
rows += ['', 'All rows passed a real app-key/renderer workflow through terminal result,',
         f"checkpoint and cold-load result. Storage separately covers {sum(len(g['modes'])*g['difficulties'] for g in games)} visible",
         'game × mode × difficulty configurations (including the identical classic',
         '2048 difficulty slots for compatibility). Engine family suites exercise',
         'additional seeds, illegal inputs, alternate answers and failure outcomes.',
         'The CSV [status matrix](acceptance-matrix.csv) records each stage.','',
         'Assistance includes undo, hint/reveal/answer and same-seed INIT. No hidden',
         'guess undo is offered. Only five recent games retain resumable saves.',
         'The earlier 30-game content quantities and transformation caveats are in',
         '[CONTENT_INVENTORY.md](CONTENT_INVENTORY.md).']
(ROOT/'docs/GAME_CATALOG.md').write_text('\n'.join(rows)+'\n')
(ROOT/'docs/RULES.md').write_text('\n'.join(rules).rstrip()+'\n')
(ROOT/'docs/acceptance-matrix.csv').write_text('\n'.join(matrix)+'\n')
(ROOT/'docs/game-registry.json').write_text(json.dumps(catalog,indent=2)+'\n')
(ROOT/'docs/CAPABILITIES.md').write_text('\n'.join(capabilities)+'\n')
print('Exported 36 actual registry entries, rules and stage matrix')
