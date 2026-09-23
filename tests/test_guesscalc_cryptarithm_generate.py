#!/usr/bin/env python3
"""Original alphametic addition pack, deterministic public-column construction.
Run --generate to replace owned assets. No network/corpus/dictionary inputs.
Column-wise carry search is independently checked by the C weighted-sum model.
"""
import argparse,hashlib,itertools,json,pathlib,random,time
ROOT=pathlib.Path(__file__).resolve().parents[1]
SEED=202609233334

def solutions(words,limit=2,node_limit=200000):
    letters=sorted(set(''.join(words)));lead={word[0] for word in words if len(word)>1};values={};used=set();found=[];nodes=0
    maxlen=max(map(len,words));addends=words[:-1];result=words[-1]
    def column(pos,carry):
        nonlocal nodes
        nodes+=1
        if nodes>node_limit:raise TimeoutError
        if pos==maxlen:
            if not carry:found.append(dict(values))
            return
        pending=[word[-1-pos] for word in addends if pos<len(word)]
        out=result[-1-pos] if pos<len(result) else None
        def add(k,total):
            if len(found)>=limit:return
            if k<len(pending):
                letter=pending[k]
                if letter in values:add(k+1,total+values[letter]);return
                for digit in range(10):
                    if digit not in used and (digit or letter not in lead):
                        values[letter]=digit;used.add(digit);add(k+1,total+digit);used.remove(digit);del values[letter]
                return
            digit=total%10
            if out is None:
                if digit==0:column(pos+1,total//10)
            elif out in values:
                if values[out]==digit:column(pos+1,total//10)
            elif digit not in used and (digit or out not in lead):
                values[out]=digit;used.add(digit);column(pos+1,total//10);used.remove(digit);del values[out]
        add(0,carry)
    column(0,0)
    return found,nodes

def canonical(numbers):
    # Normalize letter renaming AND commutative operand order; numeric ordering
    # alone is not invariant under replacing digits with other digits.
    candidates=[]
    for operands in itertools.permutations(numbers[:-1]):
        mapping={};words=[]
        for number in (*operands,numbers[-1]):
            words.append(''.join(mapping.setdefault(c,chr(65+len(mapping))) for c in str(number)))
        candidates.append((words,[int(c) for c in mapping]))
    return min(candidates)

def generate():
    rng=random.Random(SEED);records=[];seen=set();started=time.monotonic()
    for difficulty in range(4):
        length=(2,3,4,5)[difficulty];minimum=(3,5,7,8)[difficulty];maximum=(5,7,9,10)[difficulty];addends=2;made=0
        for attempt in range(1,40001):
            nums=sorted(rng.randrange(10**(length-1),10**length) for _ in range(addends));total=sum(nums)
            words,witness=canonical(nums+[total])
            if not minimum<=len(witness)<=maximum or tuple(words) in seen:continue
            try:found,nodes=solutions(words)
            except TimeoutError:continue
            if len(found)!=1:continue
            assert [found[0][chr(65+k)] for k in range(len(witness))]==witness
            seen.add(tuple(words));records.append(dict(game_id=34,difficulty=difficulty,puzzle_id=difficulty*30+made,words=words,letters=len(witness),solution=witness,solution_count=1,column_nodes=nodes));made+=1
            if made==30:break
        if made!=30:raise RuntimeError(f'bounded generator exhausted: level{difficulty} made{made}')
        print('level',difficulty,'records',made,'attempts',attempt,'seconds',round(time.monotonic()-started,2),flush=True)
    return dict(seed=SEED,rules_version=1,external_files=0,external_records=0,records=records)

def header(data):
    lines=['/* Original deterministic alphametics; host witness is not embedded. */','#ifndef GUESSCALC_CRYPT_PACK_H','#define GUESSCALC_CRYPT_PACK_H','typedef struct {unsigned char count,letters;char words[3][7];} GcCryptPack;','static const GcCryptPack gc_crypt_pack[120]={']
    for p in data['records']:
        words=p['words'];lines.append('{%d,%d,{%s}},'%(len(p['words'])-1,p['letters'],','.join(json.dumps(word) for word in words)))
    return '\n'.join(lines+['};','#endif',''])

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--generate',action='store_true');args=parser.parse_args()
    jp=ROOT/'assets/guesscalc_cryptarithm.json';hp=ROOT/'assets/guesscalc_cryptarithm.h'
    if args.generate:
        data=generate();jp.write_text(json.dumps(data,indent=2)+'\n');hp.write_text(header(data))
    else:
        data=json.loads(jp.read_text());assert hp.read_text()==header(data)
        seen=set();counts=[0]*4
        assert data['external_files']==data['external_records']==0
        assert len(data['records'])==120 and data['seed']==SEED
        for index,p in enumerate(data['records']):
            assert p['puzzle_id']==index and p['difficulty']==index//30 and p['game_id']==34
            counts[p['difficulty']]+=1
            assert len(p['words'])==3 and len(p['words'][0])==len(p['words'][1])==2+p['difficulty']
            values=[int(''.join(str(p['solution'][ord(c)-65]) for c in word)) for word in p['words']]
            assert values[0]+values[1]==values[2]
            assert canonical(values)==(p['words'],p['solution'])
            assert tuple(p['words']) not in seen;seen.add(tuple(p['words']))
            found,nodes=solutions(p['words']);assert len(found)==1 and nodes==p['column_nodes']
            assert [found[0][chr(65+k)] for k in range(p['letters'])]==p['solution']
        assert counts==[30]*4
        print('PASS',len(seen),'original unique alphametics',hashlib.sha256(jp.read_bytes()).hexdigest())
if __name__=='__main__':main()
