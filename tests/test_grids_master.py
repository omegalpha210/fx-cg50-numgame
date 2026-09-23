#!/usr/bin/env python3
"""Small exhaustive cross-checks of MASTER inference soundness, public only."""
import itertools,json,pathlib,random,sys,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/generate'))
from grids_master_logic import Logic,analyze,visible
from grids_master_reference import sky_count
from grids_solver import sudoku,latin
from grids_master_generate import CANDIDATES

class MasterLogicTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        rows=list(itertools.permutations(range(1,5)));cls.latin=[]
        for board in itertools.product(rows,repeat=4):
            if all(len(set(board[r][c] for r in range(4)))==4 for c in range(4)):cls.latin.append(tuple(v for row in board for v in row))
        assert len(cls.latin)==576

    def compatible(self,p,s):
        if any(v and s[i]!=v for i,v in enumerate(p['cells'])):return False
        if p['id']==14:
            return all(not sign or (s[i]-s[i+step])*sign<0 for values,step in ((p['a'],1),(p['b'],4)) for i,sign in enumerate(values))
        a=p['a'];cols=[s[c::4] for c in range(4)];rows=[s[r*4:r*4+4] for r in range(4)]
        measured=[visible(v) for v in cols]+[visible(v[::-1]) for v in cols]+[visible(v) for v in rows]+[visible(v[::-1]) for v in rows]
        return all(not v or measured[i]==v for i,v in enumerate(a))

    def test_small_latin_exhaustive(self):
        rng=random.Random(6401)
        for gid in (14,15):
            for _ in range(24):
                solution=rng.choice(self.latin);p=dict(id=gid,n=4,cells=[v if rng.random()<.2 else 0 for v in solution],solution=[255]*16)
                if gid==14:
                    a=[0]*16;b=a[:]
                    for i in range(16):
                        if i%4<3 and rng.random()<.4:a[i]=1 if solution[i]<solution[i+1] else -1
                        if i<12 and rng.random()<.4:b[i]=1 if solution[i]<solution[i+4] else -1
                    p.update(a=a,b=b)
                else:
                    rows=[solution[r*4:r*4+4] for r in range(4)];cols=[solution[c::4] for c in range(4)]
                    p['a']=[visible(v) if rng.random()<.7 else 0 for lines in (cols,[x[::-1] for x in cols],rows,[x[::-1] for x in rows]) for v in lines]
                actual=[s for s in self.latin if self.compatible(p,s)];logic=Logic(p);metrics,found=logic.search()
                self.assertEqual(metrics['solution_count'],min(2,len(actual)))
                self.assertTrue(all(tuple(s) in actual for s in found))
                domains=logic.propagate(logic.initial(),True)
                self.assertTrue(all(all(v in domains[i] for i,v in enumerate(s)) for s in actual))
                if gid==15:self.assertEqual(sky_count(p)[0],min(2,len(actual)))

    def test_calcudoku_exhaustive(self):
        # Independent 576-board oracle checks every operator and propagation.
        rng=random.Random(6402)
        for _ in range(30):
            solution=rng.choice(self.latin);order=list(range(16));rng.shuffle(order)
            cages=[order[i:i+2] for i in range(0,16,2)];a=[0]*16;b=[]
            for cid,cc in enumerate(cages):
                x,y=(solution[i] for i in cc);ops=[1,2,3]
                if max(x,y)%min(x,y)==0:ops.append(4)
                op=rng.choice(ops);target={1:x+y,2:x*y,3:abs(x-y),4:max(x,y)//min(x,y)}[op]
                b.extend((op,target))
                for i in cc:a[i]=cid
            p=dict(id=12,n=4,cells=[0]*16,a=a,b=b,solution=[255]*16)
            def compatible(s):
                for cid,cc in enumerate(cages):
                    x,y=(s[i] for i in cc);op,target=b[cid*2:cid*2+2]
                    if op==1 and x+y!=target:return False
                    if op==2 and x*y!=target:return False
                    if op==3 and abs(x-y)!=target:return False
                    if op==4 and max(x,y)!=min(x,y)*target:return False
                return True
            actual=[s for s in self.latin if compatible(s)];logic=Logic(p);metrics,found=logic.search()
            self.assertEqual(metrics['solution_count'],min(2,len(actual)))
            self.assertEqual(latin(p)[0],min(2,len(actual)))
            self.assertTrue(all(tuple(s) in actual for s in found))
            domains=logic.propagate(logic.initial(),True)
            self.assertTrue(all(all(v in domains[i] for i,v in enumerate(s)) for s in actual))

    def test_generation_reproduces_accepted_seeds(self):
        for gid in (11,12,14,15):
            for record in json.loads((ROOT/f'assets/grids/master/{gid}.json').read_text()):
                candidate=CANDIDATES[gid](random.Random(record['seed']))
                self.assertIsNotNone(candidate)
                for key in ('id','n','cells','solution','a','b'):
                    self.assertEqual(candidate.get(key),record.get(key))

    def test_sudoku_rejects_invalid_givens(self):
        board=[1,2,3,4,3,4,1,2,2,1,4,3,4,3,2,1]
        self.assertEqual(sudoku(dict(n=4,cells=board))[0],1)
        board[0]=2;self.assertEqual(sudoku(dict(n=4,cells=board)),(0,0))
        self.assertEqual(sudoku(dict(n=5,cells=[0]*25)),(0,0))

    def test_master_proof_without_witness(self):
        for gid in (11,12,14,15):
            records=json.loads((ROOT/f'assets/grids/master/{gid}.json').read_text())
            for p in records:
                public={k:v for k,v in p.items() if k!='solution'};rating,solutions=analyze(public)
                self.assertEqual(rating,p['logical_rating']);self.assertEqual(solutions,[p['solution']])
                self.assertEqual(rating['after_chains_unfilled'],0)
                self.assertTrue(rating['forcing_trace'])
                self.assertTrue(all(p['solution'][step['cell']]!=step['excluded'] for step in rating['forcing_trace']))

if __name__=='__main__':unittest.main()
