#!/usr/bin/env python3
"""Copy selected actual host-renderer frames into stable README image names."""
from pathlib import Path
from PIL import Image

root = Path(__file__).resolve().parents[1]
sources = {
    'main': '00-main',
    'baseball': '01-play',
    'make-target': '06-play',
    'prime-editor': '73-prime-editor-middle',
    'prime-result': '74-prime-result',
    'prime-view-result': '75-prime-view-result',
    'cryptarithm': '34-play',
    'sudoku': '11-play',
    'kakuro': '13-play',
    'nonogram': '36-play',
    'magic-square': '19-play',
    'reversi': '37-play',
    '2048': '26-play',
    'shikaku': '31-play',
    'slitherlink': '32-play',
}
output = root / 'docs/images'
output.mkdir(exist_ok=True)
icon = Image.open(root / 'assets/icon-sel.png').convert('RGBA')
assert icon.size == (92, 64)
icon.resize((184, 128), Image.Resampling.NEAREST).save(output / 'icon-2x.png')
for name, frame in sources.items():
    image = Image.open(root / 'build-host/captures' / f'{frame}.ppm')
    assert image.size == (396, 224) and image.mode == 'RGB'
    image.save(output / f'{name}.png')
print(f'{len(sources)} current 396x224 renderer frames and original 2x icon in {output}')
