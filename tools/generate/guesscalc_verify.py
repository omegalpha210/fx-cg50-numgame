#!/usr/bin/env python3
"""Independent pack validators: no generator imports or witness equality checks."""
import ast, collections, itertools, json, math, time
from fractions import Fraction
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
P=json.loads((ROOT/'assets/guesscalc_packs.json').read_text())
START=time.monotonic()
checks=0

def require(value):
    global checks
    checks+=1
    if not value:raise AssertionError(f'check {checks}')

def expression(text,positive=False):
    literals=[]
    def walk(t):
        if isinstance(t,ast.Constant) and type(t.value) is int:
            literals.append(t.value);v=Fraction(t.value)
        elif isinstance(t,ast.UnaryOp) and isinstance(t.op,ast.USub):v=-walk(t.operand)
        elif isinstance(t,ast.BinOp):
            a,b=walk(t.left),walk(t.right)
            if isinstance(t.op,ast.Add):v=a+b
            elif isinstance(t.op,ast.Sub):v=a-b
            elif isinstance(t.op,ast.Mult):v=a*b
            elif isinstance(t.op,ast.Div):v=a/b
            else:raise ValueError('operator')
        else:raise ValueError('syntax')
        require(abs(v.numerator)<=10**9 and v.denominator<=10**9)
        if positive:require(v>0 and v.denominator==1)
        return v
    value=walk(ast.parse(text,mode='eval').body)
    return value,collections.Counter(literals)

def verify_mind():
    for p in P['number_mind']:
        n=p['n'];solutions=[]
        for x in itertools.product(range(p['alphabet']),repeat=n):
            valid=True
            for clue,count in p['clues']:
                if sum(x[i]==clue[i] for i in range(n))!=count:valid=False;break
            if valid:solutions.append(x)
            if len(solutions)==2:break
        require(len(solutions)==1)
        require(list(solutions[0])==p['solution'])
        require(len(p['clues'])<=15 and n*len(p['clues'])<=81)
        require(any(c==0 for _,c in p['clues']))
    print('NUMBER MIND: 90 independently enumerated unique domains',flush=True)

def verify_lock():
    for p in P['clue_lock']:
        lo,hi,parity,ds,m1,r1,m2,r2,digit=p['params'];solutions=[]
        require(m1!=m2)
        for x in range(p['limit']+1):
            text=str(x)
            if not lo<=x<=hi:continue
            if parity!=-1 and x%2!=parity:continue
            if ds!=-1 and sum(int(c) for c in text)!=ds:continue
            if digit!=-1 and str(digit) not in text:continue
            if divmod(x,m1)[1]!=r1 or divmod(x,m2)[1]!=r2:continue
            solutions.append(x)
        require(solutions==[p['solution']])
    print('CLUE LOCK: 90 full finite-domain unique solutions',flush=True)

def grammar_predictions(prefix):
    results=set()
    # Independently enumerate every documented grammar coefficient tuple.
    for a,b in itertools.product(range(-9,10),range(-5,6)):
        if b and all(v==a+i*b for i,v in enumerate(prefix)):results.add(a+6*b)
    for a,b in itertools.product(range(1,6),(-3,-2,2,3)):
        if all(v==a*pow(b,i) for i,v in enumerate(prefix)):results.add(a*pow(b,6))
    for a,b,c in itertools.product(range(-5,6),range(-4,5),range(1,4)):
        if all(v==a+i*(b+i*c) for i,v in enumerate(prefix)):results.add(a+6*(b+6*c))
    if 1<=prefix[0]<=9 and 1<=prefix[1]<=9 and all(prefix[i]==prefix[i-1]+prefix[i-2] for i in range(2,6)):results.add(prefix[4]+prefix[5])
    if all(-6<=x<=9 for x in prefix[:2]):
        for step in range(1,6):
            if all(prefix[i]==prefix[i%2]+i//2*step for i in range(6)):results.add(prefix[4]+step)
    if -3<=prefix[0]<=6:
        for b,c in itertools.product((-2,-1,2,3),range(-3,4)):
            if c and all(prefix[i]==b*prefix[i-1]+c for i in range(1,6)):results.add(b*prefix[5]+c)
    return results

def verify_sequence():
    for p in P['sequence']:require(grammar_predictions(p['sequence'][:6])=={p['sequence'][6]})
    print('SEQUENCE: 90 prefixes have one next value within all six bounded grammars',flush=True)

def verify_cards():
    for name in ('target','countdown'):
        for p in P[name]:
            value,literals=expression(p['witness'],name=='countdown');cards=collections.Counter(p['cards'])
            require(value==p['target'])
            require(literals==cards if name=='target' else not (literals-cards))
            require(len(p['witness'])<40 and len(p['hint'])<40)
            if name=='countdown':
                require(100<=p['target']<=999)
                large=[x for x in p['cards'] if x>10];small=[x for x in p['cards'] if x<=10]
                require(len(large)==p['difficulty']+1 and len(set(large))==len(large) and set(large)<={25,50,75,100})
                require(all(1<=x<=10 for x in small) and max(collections.Counter(small).values())<=2)
        print(f'{name.upper()}: {len(P[name])} exact witnesses independently evaluated',flush=True)

def evaluate3(v,op):
    # Distinct direct precedence table, no generator arithmetic routine.
    a,b,c=v;x,y=op
    table={(0,0):a+b+c,(0,1):a+b-c,(0,2):a+b*c,
           (1,0):a-b+c,(1,1):a-b-c,(1,2):a-b*c,
           (2,0):a*b+c,(2,1):a*b-c,(2,2):a*b*c}
    return table[x,y]

def verify_cross():
    # Exhaustive full-board permutations, unlike generation's row-combination solver.
    for pi,p in enumerate(P['crossmath']):
        count=0;found=None;fixed=[(i,v) for i,v in enumerate(p['givens']) if v]
        for board in itertools.permutations(range(1,10)):
            if any(board[i]!=v for i,v in fixed):continue
            if any(evaluate3(board[r*3:r*3+3],p['ops'][r*2:r*2+2])!=p['targets'][r] for r in range(3)):continue
            if any(evaluate3(board[c::3],p['ops'][6+c*2:8+c*2])!=p['targets'][3+c] for c in range(3)):continue
            count+=1;found=board
            if count==2:break
        require(count==1 and list(found)==p['solution'])
        require(sum(bool(x) for x in p['givens'])==(4,2,0)[p['difficulty']])
    print('CROSS MATH: 90 unique boards; exhaustive 9! permutation reference',flush=True)

def verify_equations():
    for p in P['equation']:
        lhs,rhs=p['equation'].split('=');value,_=expression(lhs)
        require(value==int(rhs) and len(p['equation'])==p['length'])
    print('EQUATION: 270 independently checked true equations',flush=True)

def metadata():
    for name in ('number_mind','clue_lock','sequence','target','countdown','crossmath','equation'):
        for d in range(3):
            items=[p for p in P[name] if p['difficulty']==d]
            expected=60 if name=='target' else 90 if name=='equation' else 30
            require(len(items)==expected)
            require(len({json.dumps({k:v for k,v in x.items() if k not in ('puzzle_id','difficulty','rules_version','game_id')},sort_keys=True) for x in items})==expected)
        fields={'number_mind':['n','alphabet','clues'],'clue_lock':['limit','params'],
                'sequence':['sequence'],'target':['cards','target'],'countdown':['cards','target'],
                'crossmath':['ops','targets','givens'],'equation':['equation']}[name]
        signatures={json.dumps({k:x[k] for k in fields},sort_keys=True) for x in P[name]}
        require(len(signatures)==len(P[name]))
        for p in P[name]:
            require(p['rules_version']==1)
            require(p['game_id']=={'number_mind':3,'clue_lock':4,'sequence':5,'target':6,'countdown':7,'crossmath':9,'equation':2}[name])
    print('METADATA: exact per-difficulty counts and distinct records verified',flush=True)

if __name__=='__main__':
    metadata();verify_mind();verify_lock();verify_sequence();verify_cards();verify_cross();verify_equations()
    print(f'PASS: {checks} independent checks in {time.monotonic()-START:.3f}s')
