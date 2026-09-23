#!/usr/bin/env python3
"""Small complete spaces independently check new public-clue deductions."""
import collections,itertools,pathlib,random,sys,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/generate'))
from grids_master_logic import Logic,Contradiction
from grids_puzzle_master import PuzzleLogic,panmagic_squares
from grids_solver import count
from grids_verify import base_key

class ExpandedLogicTests(unittest.TestCase):
    def check(self,p,actual):
        # Deliberately poison the witness: both engines must use public clues.
        p=dict(p,solution=[255]*(p['n']**2))
        logic=Logic(p) if p['id']==13 else PuzzleLogic(p)
        metrics,found=logic.search(30000)
        converted=[logic.board_solution(s) if isinstance(logic,PuzzleLogic) else s for s in found]
        self.assertEqual(metrics['solution_count'],min(2,len(actual)))
        self.assertTrue(all(tuple(s) in actual for s in converted))
        self.assertEqual(count(p,200000)[0],min(2,len(actual)))
        try:domains=logic.propagate(logic.initial(),True)
        except Contradiction:self.assertFalse(actual);return
        for solution in actual:
            values=solution
            if p['id']==18:values=[solution.index(v) for v in range(1,len(solution)+1)]
            self.assertTrue(all(v in domains[i] for i,v in enumerate(values)))
        if actual:
            rating=logic.rate(512)
            for step in rating['forcing_trace']:
                for solution in actual:
                    value=solution.index(step['cell']+1) if p['id']==18 else solution[step['cell']]
                    self.assertNotEqual(value,step['excluded'])

    def test_kakuro_complete_four_cell_space(self):
        cases=collections.defaultdict(list)
        for a,b,c,d in itertools.product(range(1,10),repeat=4):
            if a==b or a==c or b==d or c==d:continue
            s=(0,0,0,0,a,b,0,c,d);cases[a+b,c+d,a+c,b+d].append(s)
        for sums in random.Random(7001).sample(list(cases),80):
            a=[0]*9;b=[0]*9;a[3],a[6],b[1],b[2]=sums
            self.check(dict(id=13,n=3,cells=[0,0,0,0,255,255,0,255,255],a=a,b=b),cases[sums])

    def test_hitori_complete_nine_cell_space(self):
        rng=random.Random(7002)
        for _ in range(40):
            cells=[rng.randint(1,3) for _ in range(9)];actual=[]
            for bits in itertools.product((0,1),repeat=9):
                black={i for i,v in enumerate(bits) if v};white=set(range(9))-black
                if not white:continue
                if any(abs(i//3-j//3)+abs(i%3-j%3)==1 for i in black for j in black):continue
                if any(cells[i]==cells[j] and (i//3==j//3 or i%3==j%3) for i in white for j in white if i<j):continue
                seen={next(iter(white))}
                for _ in range(9):seen|={j for j in white for i in tuple(seen) if abs(i//3-j//3)+abs(i%3-j%3)==1}
                if seen==white:actual.append(bits)
            self.check(dict(id=16,n=3,cells=cells),actual)

    def test_binary_complete_four_by_four_space(self):
        rows=[v for v in itertools.product((0,1),repeat=4) if sum(v)==2 and all(len(set(v[k:k+3]))>1 for k in range(2))];solutions=[]
        for board in itertools.permutations(rows,4):
            columns=list(zip(*board))
            if len(set(columns))==4 and all(v in rows for v in columns):solutions.append(tuple(itertools.chain.from_iterable(board)))
        rng=random.Random(7003)
        for _ in range(40):
            s=rng.choice(solutions);cells=[v if rng.random()<.3 else 255 for v in s]
            actual=[v for v in solutions if all(g==255 or g==x for g,x in zip(cells,v))]
            self.check(dict(id=17,n=4,cells=cells),actual)

    def test_numbrix_complete_three_by_three_space(self):
        solutions=[]
        def walk(path):
            if len(path)==9:
                s=[0]*9
                for v,i in enumerate(path,1):s[i]=v
                solutions.append(tuple(s));return
            for i in range(9):
                if i not in path and (not path or abs(i//3-path[-1]//3)+abs(i%3-path[-1]%3)==1):walk(path+[i])
        walk([]);rng=random.Random(7004)
        for _ in range(40):
            s=rng.choice(solutions);cells=[v if rng.random()<.3 else 0 for v in s]
            actual=[v for v in solutions if all(not g or g==x for g,x in zip(cells,v))]
            self.check(dict(id=18,n=3,cells=cells),actual)

    def test_sum_complete_nine_cell_space(self):
        rng=random.Random(7005)
        for _ in range(40):
            cells=[rng.randint(1,5) for _ in range(9)];bits=[rng.randrange(2) for _ in cells]
            def sums(v):return [sum(cells[r*3+c]*v[r*3+c] for c in range(3)) for r in range(3)]+[sum(cells[r*3+c]*v[r*3+c] for r in range(3)) for c in range(3)]
            targets=sums(bits);actual=[v for v in itertools.product((0,1),repeat=9) if sums(v)==targets]
            self.check(dict(id=20,n=3,cells=cells,a=targets),actual)

    def test_numbrix_contradictory_public_givens(self):
        for cells in ([1,1,0,0],[0,5,0,0],[-1,0,0,0],[1,3,0,0]):
            p=dict(id=18,n=2,cells=cells,solution=[255]*4)
            self.assertEqual(PuzzleLogic(p).search()[0]['solution_count'],0)
            self.assertEqual(count(p)[0],0)

    def test_panmagic_construction_and_distinct_bases(self):
        squares=panmagic_squares();self.assertEqual(len(set(squares)),384)
        canonical=set()
        for square in squares:
            self.assertEqual(sorted(square),list(range(1,17)))
            lines=[square[r*4:r*4+4] for r in range(4)]+[square[c::4] for c in range(4)]
            lines += [tuple(square[r*4+(start+step*r)%4] for r in range(4)) for start in range(4) for step in (-1,1)]
            self.assertTrue(all(sum(v)==34 for v in lines))
            canonical.add(base_key(dict(id=19,n=4,cells=list(square))))
        self.assertEqual(len(canonical),48)

if __name__=='__main__':unittest.main()
