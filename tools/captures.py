#!/usr/bin/env python3
"""Convert actual common-C-renderer PPM captures; no mockups or redraws."""
from pathlib import Path
from PIL import Image, ImageDraw
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'docs/captures'
OUT.mkdir(parents=True,exist_ok=True)
capture_root=ROOT/'build-host/captures'
paths=[capture_root/name for name in (capture_root/'manifest.txt').read_text().splitlines()]
assert all(p.parent==capture_root and p.suffix=='.ppm' for p in paths)
assert len(set(paths))==len(paths)
assert paths,'Run test_capture from build-host first'
for p in paths:
    im=Image.open(p)
    assert im.size==(396,224)
    im.save(OUT/(p.stem+'.png'))
diagnostic_root=ROOT/'build-host-diagnostic/captures'
diagnostic_names=('54-diagnostics-memory','54-diagnostics-runtime')
for name in diagnostic_names:
    diagnostic_file=diagnostic_root/(name+'.ppm')
    if diagnostic_file.exists():
        diagnostic_image=Image.open(diagnostic_file)
        assert diagnostic_image.size==(396,224)
        diagnostic_image.save(OUT/(name+'.png'))
visible=list(range(1,29))+[31,32]
games=[capture_root/f'{id:02d}-play.ppm' for id in visible]
assert all(p in paths for p in games)
def sheet(items,name,scale,columns):
    width,height=396*scale,224*scale
    caption=18
    im=Image.new('RGB',(columns*(width+12)+12,((len(items)+columns-1)//columns)*(height+caption+12)+12),'#e7ecee')
    d=ImageDraw.Draw(im)
    for i,p in enumerate(items):
        x=12+(i%columns)*(width+12);y=12+(i//columns)*(height+caption+12)
        d.text((x,y),p.stem,fill='#172a3a')
        capture=Image.open(p).resize((width,height),Image.Resampling.NEAREST)
        im.paste(capture,(x,y+caption))
    im.save(OUT/name)
sheet(games,'contact-native.png',1,3)
menus=[ROOT/'build-host/captures/00-main.ppm']+[ROOT/'build-host/captures'/f'00-category-{i}.ppm' for i in range(1,7)]
sheet(menus,'menus-native.png',1,2)
sheet(menus,'menus-2x.png',2,2)
for group in range(6):
    category=ROOT/'build-host/captures'/f'00-category-{group+1}.ppm'
    items=[category]+games[group*5:group*5+5]
    sheet(items,f'category-{group+1}-2x.png',2,2)
sheet([p for p in paths if p.stem.endswith('-hard')],'hard-grids-native.png',1,2)
sheet([p for p in paths if p not in games and p.stem[:2].isdigit() and int(p.stem[:2])>=31],'states-native.png',1,2)
entry_frames=[p for p in paths if p.stem[:2].isdigit() and 42<=int(p.stem[:2])<=47]
overflow_frames=[p for p in paths if p.stem[:2].isdigit() and 48<=int(p.stem[:2])<=53]
master_frames=[p for p in paths if '-master' in p.stem]
if master_frames:sheet(master_frames,'master-native.png',1,2)
board_frames=[p for p in paths if p.stem.startswith(('31-level','32-level')) and not p.stem.endswith('-complete')]
if board_frames:sheet(board_frames,'boards-native.png',1,2)
board_complete=[p for p in paths if p.stem.startswith(('31-level','32-level')) and p.stem.endswith('-complete')]
if board_complete:sheet(board_complete,'boards-complete-native.png',1,2)
if entry_frames:sheet(entry_frames,'entry-layout-native.png',1,2)
if overflow_frames:sheet(overflow_frames,'overflow-native.png',1,2)
(OUT/'README.md').write_text('# Actual renderer captures\n\n396×224 pixels from `src/ui/render.c` and the same game render adapters as the SH build.\nHost RGB565 backend, not a desktop mockup or a hardware LCD photograph.\n\n[All menus / game icons, native](menus-native.png) · [Integer 2×](menus-2x.png).\n[Entry settings](entry-layout-native.png) · [Overflow/large values](overflow-native.png).\nThe normal font matches DIFF EQ/gint 8×9; compact clues retain 5×7.\n\n'+''.join(f'- [{p.stem}]({p.stem}.png)\n' for p in paths)+''.join(f'- [NUM DIAG {name}]({name}.png)\n' for name in diagnostic_names if (OUT/(name+'.png')).exists())+'\n`contact-native.png`: 1×. `category-*-2x.png`: nearest-neighbor integer 2×.\n39-2048-large, 48–53 and 57–58 are explicit renderer stress fixtures (52 selects an actual verified pack entry); maximum scores are not played results.\n')
for obsolete in ('29-play','30-play','37-memory-pause','38-memory-loss','44-entry-long-mode'):
    (OUT/(obsolete+'.png')).unlink(missing_ok=True)
print(f'{len(paths)} native captures; 1x and 2x sheets in {OUT}')
