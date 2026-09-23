#!/usr/bin/env python3
"""Original NUM GAME arithmetic/code packs; deterministic seed, no external corpus."""
import itertools, json, random
from fractions import Fraction
from pathlib import Path
R=random.Random(0x4e554d47414d45)
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'assets'

def arith(nums,ops):
    terms=[Fraction(nums[0])]; signs=[1]
    for op,n in zip(ops,nums[1:]):
        if op==2:terms[-1]*=n
        elif op==3:
            if not n:return None
            terms[-1]/=n
        else:terms.append(Fraction(n));signs.append(1 if op==0 else -1)
    return sum(t*s for t,s in zip(terms,signs))

def solve_cards(cards,target,integer=False):
    def go(items):
        if len(items)==1:return items[0][1:] if items[0][0]==target else None
        seen=set()
        for i in range(len(items)):
            for j in range(i):
                a,ae,ah=items[i];b,be,bh=items[j]
                rest=[v for k,v in enumerate(items) if k not in (i,j)]
                options=[(a+b,'+',ae,be,a,b),(a*b,'*',ae,be,a,b),(a-b,'-',ae,be,a,b),(b-a,'-',be,ae,b,a)]
                if b:options.append((a/b,'/',ae,be,a,b))
                if a:options.append((b/a,'/',be,ae,b,a))
                for v,op,x,y,vx,vy in options:
                    if integer and (v<=0 or v.denominator!=1):continue
                    if abs(v.numerator)>10**9 or v.denominator>10**9:continue
                    key=(v,tuple(sorted(t[0] for t in rest)))
                    if key in seen:continue
                    seen.add(key)
                    hint=ah or bh or f'Combine {vx} {op} {vy} first.'
                    res=go(rest+[(v,f'({x}{op}{y})',hint)])
                    if res:return res
        return None
    return go([(Fraction(v),str(v),'') for v in cards])

def number_mind():
    packs=[]
    for d,(n,alphabet) in enumerate(((4,6),(4,8),(5,8))):
        domain=list(itertools.product(range(alphabet),repeat=n));made=set()
        while len(made)<30:
            secret=R.choice(domain);candidates=domain[:];clues=[]
            for _ in range(15):
                guess=R.choice(domain)
                count=sum(x==y for x,y in zip(guess,secret))
                if count==n or any(g==guess for g,c in clues):continue
                reduced=[v for v in candidates if sum(x==y for x,y in zip(guess,v))==count]
                if len(reduced)==len(candidates):continue
                clues.append((guess,count));candidates=reduced
                if len(candidates)==1:break
            if len(candidates)!=1 or not any(c==0 for _,c in clues):continue
            signature=tuple(clues)
            if signature in made:continue
            made.add(signature);packs.append(dict(difficulty=d,n=n,alphabet=alphabet,clues=clues,solution=secret,solution_count=1,domain_size=len(domain)))
    return packs

def clue_lock():
    packs=[]
    for d,limit in enumerate((99,499,999)):
        seen=set()
        while len(seen)<30:
            secret=R.randrange(limit+1)
            lo=R.randrange(max(1,secret//2+1));hi=R.randrange(max(secret+1,(limit*3)//4),limit+2)-1
            if hi<secret:continue
            if d==2:m1,m2=R.sample((17,19,23,29,31,37),2);ds=-1;parity=-1;digit=-1
            else:m1=R.choice((3,4,5,7,9));m2=R.choice([v for v in (7,11,13,17) if v!=m1]);ds=sum(map(int,str(secret)));parity=secret%2;digit=R.choice(list(map(int,str(secret)))) if d==0 else -1
            p=[lo,hi,parity,ds,m1,secret%m1,m2,secret%m2,digit]
            matches=[x for x in range(limit+1) if lock_match(x,p)]
            if len(matches)!=1 or tuple(p) in seen:continue
            seen.add(tuple(p));packs.append(dict(difficulty=d,limit=limit,params=p,solution=secret,solution_count=1))
    return packs

def lock_match(x,p):
    lo,hi,parity,ds,m1,r1,m2,r2,digit=p
    return lo<=x<=hi and (parity<0 or x%2==parity) and (ds<0 or sum(map(int,str(x)))==ds) and x%m1==r1 and x%m2==r2 and (digit<0 or str(digit) in str(x))

def sequence_rules():
    out=[]
    for a in range(-9,10):
        for b in range(-5,6):
            if b:out.append((0,[a,b,0],tuple(a+b*i for i in range(7))))
    for a in range(1,6):
        for b in (-3,-2,2,3):out.append((1,[a,b,0],tuple(a*b**i for i in range(7))))
    for a in range(-5,6):
        for b in range(-4,5):
            for c in (1,2,3):out.append((2,[a,b,c],tuple(a+b*i+c*i*i for i in range(7))))
    for a in range(1,10):
        for b in range(1,10):
            seq=[a,b]
            for _ in range(5):seq.append(seq[-1]+seq[-2])
            out.append((3,[a,b,0],tuple(seq)))
    for a in range(-6,10):
        for b in range(-6,10):
            for c in range(1,6):out.append((4,[a,b,c],tuple((a if i%2==0 else b)+(i//2)*c for i in range(7))))
    for a in range(-3,7):
        for b in (-2,-1,2,3):
            for c in range(-3,4):
                if c==0:continue
                seq=[a]
                for _ in range(6):seq.append(b*seq[-1]+c)
                out.append((5,[a,b,c],tuple(seq)))
    return out

def sequences():
    rules=sequence_rules();answers={};all_seen=set()
    for f,p,s in rules:answers.setdefault(s[:6],set()).add(s[6])
    packs=[]
    for d,families in enumerate(((0,1,4),(2,3,4),(2,3,5))):
        choices=[(f,p,s) for f,p,s in rules if f in families and len(answers[s[:6]])==1 and max(map(abs,s))<10000]
        R.shuffle(choices);seen=set()
        for f,p,s in choices:
            if s[:6] in all_seen:continue
            seen.add(s[:6]);all_seen.add(s[:6]);packs.append(dict(difficulty=d,family=f,params=p,sequence=s,distinct_next_answers=1))
            if len(seen)==30:break
    return packs

def targets():
    packs=[];all_seen=set()
    for target in (24,10):
        for d in range(3):
            low=1 if d<2 else 3;high=6 if d==0 else 9
            choices=list(itertools.combinations_with_replacement(range(low,high+1),4));R.shuffle(choices)
            if d==2 and target==24:choices.remove((3,3,8,8));choices.insert(0,(3,3,8,8))
            made=0
            for cards in choices:
                if (target,cards) in all_seen:continue
                answer=solve_cards(cards,target)
                if not answer:continue
                expr,hint=answer
                all_seen.add((target,cards))
                packs.append(dict(difficulty=d,target=target,cards=cards,witness=expr,hint=hint));made+=1
                if made==30:break
            assert made==30
    return packs

def countdown():
    packs=[]
    for d in range(3):
        seen=set()
        if d==2:
            cards=[1,2,3,25,75,100];target=100
            seen.add((tuple(cards),target))
            packs.append(dict(difficulty=d,cards=cards,target=target,witness='(75+25)',hint='Combine 75 + 25 first.'))
        while len(seen)<30:
            deck=list(range(1,11))*2;R.shuffle(deck);large=R.sample((25,50,75,100),d+1);cards=deck[:5-d]+large
            R.shuffle(cards)
            # Build an exact positive-integer expression from a random subset.
            chosen=R.sample(cards,R.randrange(3,7));items=[(x,str(x),'') for x in chosen]
            while len(items)>1:
                a,ae,ah=items.pop(R.randrange(len(items)));b,be,bh=items.pop(R.randrange(len(items)))
                opts=[(a+b,'+',ae,be),(a*b,'*',ae,be)]
                if a>b:opts.append((a-b,'-',ae,be))
                if b>a:opts.append((b-a,'-',be,ae))
                if b and a%b==0:opts.append((a//b,'/',ae,be))
                if a and b%a==0:opts.append((b//a,'/',be,ae))
                v,op,x,y=R.choice(opts);hint=ah or bh or f'Combine {x} {op} {y} first.'
                items.append((v,f'({x}{op}{y})',hint))
            target,expr,hint=items[0]
            if not 100<=target<=999 or (tuple(cards),target) in seen:continue
            seen.add((tuple(cards),target));packs.append(dict(difficulty=d,cards=cards,target=target,witness=expr,hint=hint))
    return packs

def cross_count(ops,targets,givens,stop=2):
    rows=[]
    for r in range(3):
        options=[]
        for v in itertools.permutations(range(1,10),3):
            if any(givens[3*r+c] and givens[3*r+c]!=v[c] for c in range(3)):continue
            if arith(v,ops[2*r:2*r+2])==targets[r]:options.append(v)
        rows.append(options)
    count=0;solution=None
    for a in rows[0]:
        aset=set(a)
        for b in rows[1]:
            if aset.intersection(b):continue
            used=aset|set(b)
            for c in rows[2]:
                if used.intersection(c):continue
                if all(arith((a[j],b[j],c[j]),ops[6+2*j:8+2*j])==targets[3+j] for j in range(3)):
                    count+=1;solution=a+b+c
                    if count>=stop:return count,solution
    return count,solution

def crossmath():
    packs=[]
    for d,num in enumerate((4,2,0)):
        made=set()
        while len(made)<30:
            solution=R.sample(range(1,10),9);ops=[R.randrange(3) for _ in range(12)]
            if len(set(ops))<3:continue
            targets=[int(arith(solution[3*r:3*r+3],ops[2*r:2*r+2])) for r in range(3)]+[int(arith(solution[c::3],ops[6+2*c:8+2*c])) for c in range(3)]
            givens=[0]*9
            for i in R.sample(range(9),num):givens[i]=solution[i]
            if cross_count(ops,targets,givens)[0]!=1:continue
            sig=tuple(ops+targets+givens)
            if sig in made:continue
            made.add(sig);packs.append(dict(difficulty=d,ops=ops,targets=targets,givens=givens,solution=solution,solution_count=1))
    return packs

def equations():
    packs=[];all_seen=set()
    for length in (7,6,8):
        for d,allowed in enumerate(('+ -'.split(),'+ - *'.split(),'+ - * /'.split())):
            choices=[]
            for a in range(1,100):
                for b in range(1,100):
                    for op in allowed:
                        value=arith((a,b),['+-*/'.index(op)])
                        if value is not None and value.denominator==1 and value>=0:
                            s=f'{a}{op}{b}={value}'
                            if len(s)==length:choices.append(s)
            if d==2 and length>=7:
                complex_choices=[]
                for a,b,c in itertools.product(range(1,10),repeat=3):
                    for op1,op2 in itertools.product(range(4),repeat=2):
                        value=arith((a,b,c),(op1,op2))
                        if value is not None and value.denominator==1 and value>=0:
                            text=f'{a}{"+-*/"[op1]}{b}{"+-*/"[op2]}{c}={value}'
                            if len(text)==length:complex_choices.append(text)
                choices=complex_choices
            choices=[s for s in choices if s not in all_seen]
            assert len(choices)>=30
            for s in R.sample(choices,30):
                all_seen.add(s);packs.append(dict(difficulty=d,length=length,equation=s))
    return packs

def cs(s):return json.dumps(s)
def arr(v):return '{'+','.join(str(x) for x in v)+'}'
def write_header(p):
    lines=['/* Generated original content; see guesscalc_packs.json and independent audit. */','#ifndef GUESSCALC_PACKS_H','#define GUESSCALC_PACKS_H']
    lines+=['typedef struct { unsigned char n,alphabet,count,clues[15][5],matches[15],solution[5]; } GcMindPack;','static const GcMindPack gc_mind_pack[90]={']
    for x in p['number_mind']:
        clues=['{'+','.join(map(str,list(g)+[0]*(5-len(g))))+'}' for g,c in x['clues']]
        lines.append('{%d,%d,%d,{%s},%s,%s},'%(x['n'],x['alphabet'],len(clues),','.join(clues),arr([c for g,c in x['clues']]),arr(x['solution'])))
    lines+=['};','typedef struct { short limit,p[9],solution; } GcLockPack;','static const GcLockPack gc_lock_pack[90]={']
    for x in p['clue_lock']:lines.append('{%d,%s,%d},'%(x['limit'],arr(x['params']),x['solution']))
    lines+=['};','typedef struct { short seq[7],p[3]; unsigned char family; } GcSequencePack;','static const GcSequencePack gc_sequence_pack[90]={']
    for x in p['sequence']:lines.append('{%s,%s,%d},'%(arr(x['sequence']),arr(x['params']),x['family']))
    lines+=['};','typedef struct { short cards[6],target; char answer[40],hint[40]; } GcCardPack;','static const GcCardPack gc_target_pack[180]={']
    for x in p['target']:lines.append('{%s,%d,%s,%s},'%(arr(x['cards']),x['target'],cs(x['witness']),cs(x['hint'])))
    lines+=['};','static const GcCardPack gc_countdown_pack[90]={']
    for x in p['countdown']:lines.append('{%s,%d,%s,%s},'%(arr(x['cards']),x['target'],cs(x['witness']),cs(x['hint'])))
    lines+=['};','typedef struct { unsigned char ops[12],givens[9],solution[9]; short targets[6]; } GcCrossPack;','static const GcCrossPack gc_cross_pack[90]={']
    for x in p['crossmath']:lines.append('{%s,%s,%s,%s},'%(arr(x['ops']),arr(x['givens']),arr(x['solution']),arr(x['targets'])))
    lines+=['};','static const char gc_equation_pack[270][9]={']
    for x in p['equation']:lines.append(cs(x['equation'])+',')
    lines+=['};','#endif']
    (OUT/'guesscalc_packs.h').write_text('\n'.join(lines)+'\n')

if __name__=='__main__':
    p=dict(rules_version=1,seed='0x4e554d47414d45')
    for name,fn in [('number_mind',number_mind),('clue_lock',clue_lock),('sequence',sequences),('target',targets),('countdown',countdown),('crossmath',crossmath),('equation',equations)]:
        p[name]=fn();print(name,len(p[name]),flush=True)
        for i,x in enumerate(p[name]):
            x['puzzle_id']=i;x['rules_version']=1
            x['game_id']={'number_mind':3,'clue_lock':4,'sequence':5,'target':6,'countdown':7,'crossmath':9,'equation':2}[name]
    (OUT/'guesscalc_packs.json').write_text(json.dumps(p,indent=2)+'\n');write_header(p)
