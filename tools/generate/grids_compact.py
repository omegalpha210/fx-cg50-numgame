"""Per-record compact grid codec. No runtime whole-bank decompression.

Values are bit-packed least significant bit first. Public metadata/witnesses
remain in original JSON; only bounded clue/answer arrays enter native flash.
"""
import json,pathlib,re
ROOT=pathlib.Path(__file__).resolve().parents[2]

def records(root=None):
    root=ROOT if root is None else pathlib.Path(root)
    result=[]
    for gid in range(11,21):result+=json.loads((root/f'assets/grids/{gid}.json').read_text())
    for gid in (11,12,14,15):result+=json.loads((root/f'assets/grids/master/{gid}.json').read_text())
    directory=root/'assets/grids/expanded'
    if directory.exists():
        for path in sorted(directory.glob('[0-9][0-9]-[0-4].json')):result+=json.loads(path.read_text())
    result.sort(key=lambda p:p['puzzle_id'])
    assert [p['puzzle_id'] for p in result]==list(range(len(result))), 'Stable IDs must be contiguous; incomplete generation is not a pack'
    return result

def width(maximum):return max(1,maximum.bit_length())

class Bits:
    def __init__(self,source=None):self.data=bytearray(source or b'');self.pos=0
    def put(self,value,bits):
        assert 0<=value<1<<bits
        for k in range(bits):
            if self.pos//8==len(self.data):self.data.append(0)
            self.data[self.pos//8]|=((value>>k)&1)<<(self.pos%8);self.pos+=1
    def get(self,bits):
        assert self.pos+bits<=len(self.data)*8
        value=0
        for k in range(bits):value|=((self.data[self.pos//8]>>(self.pos%8))&1)<<k;self.pos+=1
        return value

def encode(p):
    w=Bits();gid,n=p['id'],p['n'];nn=n*n
    for value in (gid,p['difficulty'],n):w.put(value,8)
    cv=2 if gid==17 else 1 if gid==13 else nn if gid in (18,19) else 12 if gid==20 else n
    sv=1 if gid in (16,17,20) else 9 if gid==13 else nn if gid in (18,19) else n
    cells=[2 if v==255 else v for v in p['cells']] if gid==17 else [int(v==255) for v in p['cells']] if gid==13 else p['cells']
    for v in cells:w.put(v,width(cv))
    for v in p['solution']:w.put(v,width(sv))
    if gid==12:
        cages=max(p['a'])+1;assert cages<=40;w.put(cages,6)
        for v in p['a']:w.put(v,width(cages-1))
        for i in range(cages):w.put(p['b'][2*i],3);w.put(p['b'][2*i+1],15)
    elif gid==13:
        for i in range(nn):
            a,b=p['a'][i],p['b'][i];w.put(bool(a),1);w.put(bool(b),1)
            if a:w.put(a,6)
            if b:w.put(b,6)
    elif gid==14:
        for values in (p['a'],p['b']):
            for v in values:w.put({0:0,1:1,-1:2}[v],2)
    elif gid==15:
        for v in p['a']:w.put(v,width(n))
    elif gid==20:
        for v in p['a']:w.put(v,7)
    return bytes(w.data)

def decode(data):
    r=Bits(data);gid,d,n=(r.get(8) for _ in range(3));nn=n*n
    cv=2 if gid==17 else 1 if gid==13 else nn if gid in (18,19) else 12 if gid==20 else n
    sv=1 if gid in (16,17,20) else 9 if gid==13 else nn if gid in (18,19) else n
    cells=[r.get(width(cv)) for _ in range(nn)];solution=[r.get(width(sv)) for _ in range(nn)]
    if gid==17:cells=[255 if v==2 else v for v in cells]
    if gid==13:cells=[255 if v else 0 for v in cells]
    a=[0]*81;b=[0]*81
    if gid==12:
        cages=r.get(6);a[:nn]=[r.get(width(cages-1)) for _ in range(nn)]
        for i in range(cages):b[2*i]=r.get(3);b[2*i+1]=r.get(15)
    elif gid==13:
        for i in range(nn):
            aa,bb=r.get(1),r.get(1)
            if aa:a[i]=r.get(6)
            if bb:b[i]=r.get(6)
    elif gid==14:
        a[:nn]=[{0:0,1:1,2:-1}[r.get(2)] for _ in range(nn)];b[:nn]=[{0:0,1:1,2:-1}[r.get(2)] for _ in range(nn)]
    elif gid==15:a[:4*n]=[r.get(width(n)) for _ in range(4*n)]
    elif gid==20:a[:2*n]=[r.get(7) for _ in range(2*n)]
    assert (r.pos+7)//8==len(data)
    return dict(id=gid,difficulty=d,n=n,cells=cells+[0]*(81-nn),solution=solution+[0]*(81-nn),a=a,b=b)

def padded(p):return {**{k:p[k] for k in ('id','difficulty','n')},**{k:p.get(k,[])+[0]*(81-len(p.get(k,[]))) for k in ('cells','solution','a','b')}}

def emit(destination=None,input_root=None):
    destination=ROOT if destination is None else pathlib.Path(destination)
    pp=records(input_root);data=bytearray();offsets=[0]
    for p in pp:
        encoded=encode(p);assert decode(encoded)==padded(p);data.extend(encoded);offsets.append(len(data))
    ids=[];groups=[]
    for gid in range(11,21):
        for d in range(5):
            group=[p['puzzle_id'] for p in pp if p['id']==gid and p['difficulty']==d];groups.append((len(ids),len(group)));ids+=group
    assert sorted(ids)==list(range(len(pp)))
    header=f'''/* Generated compact pack dimensions. */
#ifndef NUMGAME_GRIDS_PACK_DATA_H
#define NUMGAME_GRIDS_PACK_DATA_H
#define GRIDS_PACK_COUNT {len(pp)}u
#define GRIDS_PACK_BYTES {len(data)}u
extern const uint8_t grids_pack_bytes[GRIDS_PACK_BYTES];
extern const uint32_t grids_pack_offsets[GRIDS_PACK_COUNT+1];
extern const uint16_t grids_bank_ids[GRIDS_PACK_COUNT];
extern const uint16_t grids_bank_groups[50][2];
#endif
'''
    (destination/'src/games/grids_pack_data.h').write_text(header)
    out=['/* Original bit-packed grid content; regenerate with grids_generate.py --emit. */','#include "grids_internal.h"']
    def array(kind,name,values,columns=24):
        out.append(f'const {kind} {name}[{len(values)}] = {{')
        for i in range(0,len(values),columns):out.append(','.join(str(v) for v in values[i:i+columns])+',')
        out.append('};')
    array('uint8_t','grids_pack_bytes',data);array('uint32_t','grids_pack_offsets',offsets,16);array('uint16_t','grids_bank_ids',ids,20)
    out.append('const uint16_t grids_bank_groups[50][2] = {');out.extend('{%d,%d},'%v for v in groups);out.append('};')
    (destination/'src/games/grids_pack.c').write_text('\n'.join(out)+'\n')
    print(f'Compact grids: {len(pp)} records; {len(data)} encoded bytes + {(len(pp)+1)*4+len(pp)*2+50*4} index bytes')
def verify_native():
    pp=records();source=(ROOT/'src/games/grids_pack.c').read_text()
    def values(name):
        match=re.search(r'const uint(?:8|16|32)_t '+name+r'\[[^;=]+?= \{(.*?)\};',source,re.S)
        assert match,name
        return [int(v) for v in re.findall(r'\d+',match[1])]
    data=bytes(values('grids_pack_bytes'));offsets=values('grids_pack_offsets');ids=values('grids_bank_ids');groups=values('grids_bank_groups')
    assert len(offsets)==len(pp)+1 and offsets[0]==0 and offsets[-1]==len(data)
    assert sorted(ids)==list(range(len(pp))) and len(groups)==100
    for p,begin,end in zip(pp,offsets,offsets[1:]):
        assert begin<end and decode(data[begin:end])==padded(p)
        assert data[begin:end]==encode(p)
    for gid in range(11,21):
        for d in range(5):
            index=((gid-11)*5+d)*2;first,count=groups[index:index+2]
            assert ids[first:first+count]==[p['puzzle_id'] for p in pp if p['id']==gid and p['difficulty']==d]
    return dict(records=len(pp),encoded_bytes=len(data),index_bytes=len(offsets)*4+len(ids)*2+200)

if __name__=='__main__':emit()
