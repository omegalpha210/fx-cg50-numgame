#!/usr/bin/env python3
"""Original beta.4 Sliding banks. Exact work is host-only.

3x3: select across complete BFS distance layers. 4x4: generate legal paths,
then certify the shortest path with IDA*; reject over-budget candidates rather
than declaring their witness optimal. Production stores only tiles+distance.
"""
from __future__ import annotations
import argparse
from collections import Counter, deque
import json
from pathlib import Path
import random

ROOT=Path(__file__).resolve().parents[2]
SEED=2026092827
COUNT=128
BANDS=((8,14),(15,20),(21,26))
DIRECTIONS='URDL'

def neighbours(n):
    return [[(q,DIRECTIONS[k]) for k,q in enumerate((p-n,p+1,p+n,p-1))
             if (k!=0 or p>=n) and (k!=1 or p%n<n-1) and
                (k!=2 or p<n*(n-1)) and (k!=3 or p%n>0)] for p in range(n*n)]

def bfs3():
    goal=bytes([1,2,3,4,5,6,7,8,0]);dist={goal:0};queue=deque([goal]);adj=neighbours(3)
    while queue:
        board=queue.popleft();blank=board.index(0)
        for q,_ in adj[blank]:
            child=bytearray(board);child[blank],child[q]=child[q],child[blank];child=bytes(child)
            if child not in dist:dist[child]=dist[board]+1;queue.append(child)
    assert len(dist)==181440 and max(dist.values())==31
    return dist

def raw_d4(board,n):
    variants=[];a=list(board)
    for _ in range(4):
        variants.extend((tuple(a),tuple(a[r*n+n-1-c] for r in range(n) for c in range(n))))
        a=[a[(n-1-c)*n+r] for r in range(n) for c in range(n)]
    return min(variants)

def goal_reflection(board,n):
    # Main-diagonal transpose fixes the blank's bottom-right goal. Relabel
    # each numbered tile's goal position with the same transpose.
    transformed=[]
    for r in range(n):
        for c in range(n):
            value=board[c*n+r]
            transformed.append(0 if not value else ((value-1)%n)*n+(value-1)//n+1)
    return min(tuple(board),tuple(transformed))

def path3(board,dist):
    path=[];adj=neighbours(3)
    while dist[board]:
        blank=board.index(0)
        for q,move in adj[blank]:
            child=bytearray(board);child[blank],child[q]=child[q],child[blank];child=bytes(child)
            if dist[child]==dist[board]-1:board=child;path.append(move);break
        else:raise AssertionError('No BFS predecessor')
    return ''.join(path)

class Budget(Exception):pass

def exact4(start,node_limit=2000000):
    """IDA*, admissible Manhattan bound, no unsafe state/magnitude pruning."""
    board=list(start);adj=neighbours(4);nodes=0;path=[]
    costs=[[abs(p//4-(v-1)//4)+abs(p%4-(v-1)%4) if v else 0 for p in range(16)] for v in range(16)]
    initial=sum(costs[v][p] for p,v in enumerate(board));blank=board.index(0)
    def search(p,previous,depth,h,bound):
        nonlocal nodes
        nodes+=1
        if nodes>node_limit:raise Budget
        if depth+h>bound:return False
        if h==0:return True
        # Sorting changes search order only; all admissible children remain.
        choices=sorted((h-costs[board[q]][q]+costs[board[q]][p],q,key) for q,key in adj[p] if q!=previous)
        for child_h,q,key in choices:
            board[p],board[q]=board[q],board[p];path.append(key)
            if search(q,p,depth+1,child_h,bound):return True
            path.pop();board[p],board[q]=board[q],board[p]
        return False
    for bound in range(initial,27,2):
        if search(blank,-1,0,initial,bound):return len(path),''.join(path),nodes
    return None,None,nodes

def legal_path(rng,length):
    board=list(range(1,16))+[0];blank=15;previous=-1;adj=neighbours(4)
    for _ in range(length):
        choices=[q for q,_ in adj[blank] if q!=previous];q=rng.choice(choices)
        board[blank],board[q]=board[q],board[blank];previous,blank=blank,q
    return bytes(board)

def build():
    rng=random.Random(SEED);dist=bfs3();records=[];raw_seen=set();goal_seen=set();attempts=Counter();nodes=Counter();rejects=Counter(dict(manhattan_filter=0,duplicate=0,node_budget=0,shorter_solution=0))
    for level,(low,high) in enumerate(BANDS):
        buckets={d:sorted(b for b,value in dist.items() if value==d) for d in range(low,high+1)}
        for bucket in buckets.values():rng.shuffle(bucket)
        for index in range(COUNT):
            wanted=low+index%(high-low+1)
            while True:
                board=buckets[wanted].pop();raw=raw_d4(board,3);goal=goal_reflection(board,3)
                if raw not in raw_seen and goal not in goal_seen:break
            raw_seen.add(raw);goal_seen.add(goal)
            records.append(dict(mode=0,difficulty=level,index=index,board=list(board),distance=wanted,witness=path3(board,dist)))
    raw_seen=set();goal_seen=set()
    for level,(low,high) in enumerate(BANDS):
        for index in range(COUNT):
            wanted=low+index%(high-low+1)
            for attempt in range(20000):
                attempts[level]+=1;board=legal_path(rng,wanted)
                lower=sum(abs(p//4-(v-1)//4)+abs(p%4-(v-1)%4) for p,v in enumerate(board) if v)
                if lower<wanted-4:rejects['manhattan_filter']+=1;continue
                raw=raw_d4(board,4);goal=goal_reflection(board,4)
                if raw in raw_seen or goal in goal_seen:rejects['duplicate']+=1;continue
                try:distance,path,work=exact4(board)
                except Budget:rejects['node_budget']+=1;continue
                nodes[level]+=work
                if distance!=wanted:rejects['shorter_solution']+=1;continue
                break
            else:raise AssertionError('Insufficient certified candidates')
            raw_seen.add(raw);goal_seen.add(goal)
            records.append(dict(mode=1,difficulty=level,index=index,board=list(board),distance=distance,witness=path))
    result={'format':1,'revision':4,'seed':SEED,'records_per_new_bucket':COUNT,
            'bands':[list(x) for x in BANDS],'bfs3_reachable':len(dist),'bfs3_histogram':dict(sorted(Counter(dist.values()).items())),
            'available_3x3_states_by_band':[sum(lo<=d<=hi for d in dist.values()) for lo,hi in BANDS],
            'four_by_four_candidate_attempts':dict(attempts),'four_by_four_generator_ida_nodes':dict(nodes),
            'four_by_four_rejections':dict(rejects),'records':records}
    header=['/* Original exact-distance Sliding starts; generated by strategyquick_sliding.py. */',
            '#ifndef SQ_SLIDING_BANDS_H','#define SQ_SLIDING_BANDS_H']
    for mode,width in ((0,6),(1,9)):
        header.append(f'static const uint8_t sq_sliding_banded{mode+3}[3][128][{width}]={{')
        for level in range(3):
            header.append(' {')
            for r in records:
                if (r['mode'],r['difficulty'])!=(mode,level):continue
                board=r['board']+[0]*(len(r['board'])%2)
                packed=[board[i]|board[i+1]<<4 for i in range(0,len(board),2)]+[r['distance']]
                header.append('  {'+','.join(str(v) for v in packed)+'},')
            header.append(' },')
        header.append('};')
    header.extend(['#endif',''])
    return json.dumps(result,indent=2)+'\n','\n'.join(header)

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    payload,header=build();paths=[ROOT/'assets/strategyquick/sliding_bands.json',ROOT/'assets/strategyquick/sliding_bands.h']
    for path,content in zip(paths,(payload,header)):
        if args.verify:assert path.read_text()==content,path.name
        else:path.write_text(content)
    result=json.loads(payload)
    print(json.dumps({k:v for k,v in result.items() if k!='records'},sort_keys=True))
    print('PASS: 768 certified original E/N/H boards; 5760 native packed bytes; no device solver')

if __name__=='__main__':main()
