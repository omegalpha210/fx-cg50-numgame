#!/usr/bin/env python3
"""Convert the licensed gint single-stroke font and draw original native icons."""
from pathlib import Path
from PIL import Image, ImageDraw
ROOT = Path(__file__).resolve().parents[1]
atlas = Image.open(ROOT/'assets/font/font5x7.png').convert('RGB')
widths, rows = [], []
for code in range(32,127):
    # fxconv padding surrounds each 5x7 glyph: 7x9 atlas cells, origin +1.
    x,y=(code%(atlas.width//7))*7+1,(code//(atlas.width//7))*9+1
    bits=[[atlas.getpixel((x+c,y+r))==(0,0,0) for c in range(5)] for r in range(7)]
    used=[c for c in range(5) if any(bits[r][c] for r in range(7))]
    lo,hi=(min(used),max(used)+1) if used else (0,3)
    widths.append(hi-lo)
    rows.append([sum(int(bits[r][c])<<(c-lo) for c in range(lo,hi)) for r in range(7)])
normal = Image.open(ROOT/'assets/font/font8x9.png').convert('RGB')
normal_widths, normal_rows = [], []
for code in range(95):
    # Same proportional 8x11 atlas cells and trimming as DIFF EQ/gint.
    x,y=(code%(normal.width//10))*10+1,(code//(normal.width//10))*13+1
    left,right=0,8
    blank=lambda col: all(normal.getpixel((x+col,y+r))==(255,255,255) for r in range(11))
    while left+1<right and blank(left):left+=1
    while right-1>left and blank(right-1):right-=1
    normal_widths.append(right-left)
    normal_rows.append([sum((normal.getpixel((x+c,y+r))==(0,0,0))<<(c-left)
                           for c in range(left,right)) for r in range(11)])
text='/* Licensed gint fonts; normal 8x9 matches DIFF EQ at native size. */\n'
for name,ww,rr,height in [('normal',normal_widths,normal_rows,11),('small',widths,rows,7)]:
    text+=f'static const unsigned char {name}_width[95]={{'+','.join(map(str,ww))+'};\n'
    text+=f'static const unsigned char {name}_rows[95][{height}]={{\n'+',\n'.join('{'+','.join(map(str,r))+'}' for r in rr)+'\n};\n'
(ROOT/'src/ui/font_data.h').write_text(text)
for selected in (False,True):
    im=Image.new('RGB',(92,64),'#162638' if selected else '#ffffff')
    d=ImageDraw.Draw(im)
    for k,(label,color) in enumerate(zip(('2','4','+','8'),('#d1eced','#f8e8b0','#e7daf3','#d8e8cd'))):
        x=9+(k%2)*38;y=2+(k//2)*20
        d.rectangle((x,y,x+34,y+17),fill=color,outline='#1d3549',width=1)
        code=ord(label)-32
        w=widths[code];xx=x+(35-w*2)//2
        for row,bits in enumerate(rows[code]):
            for col in range(w):
                if bits&(1<<col):d.rectangle((xx+col*2,y+2+row*2,xx+col*2+1,y+3+row*2),fill='#14283b')
    # Rows 42..63 deliberately remain clear for the OS label NUM GAME.
    im.save(ROOT/'assets'/('icon-sel.png' if selected else 'icon-uns.png'))
