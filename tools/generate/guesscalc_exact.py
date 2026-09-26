#!/usr/bin/env python3
"""Host exact-rational Make Target DP interface (C++17 backend, no SDK needed).

All card occurrences are used; equal-value occurrence permutations and +/*
reversals are canonicalized, association retained. Unary minus is legal; only
consecutive double minus is removed. -0 remains distinct. Bounds are the actual
native parser's reduced numerator/denominator <=1e9, never a search heuristic.
At most six literals and this normal form fit the native 31-op/12-depth/96-char
limits. Raw lex minimum and a separate unary-aware grade are both returned.
"""
import argparse
from fractions import Fraction
import hashlib
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT/'tools/generate/guesscalc_exact.cpp'


def executable():
    digest = hashlib.sha256(SOURCE.read_bytes()).hexdigest()[:16]
    path = ROOT/'build-host/guesscalc-beta4'/('exact-'+digest)
    if not path.exists():
        path.parent.mkdir(parents=True, exist_ok=True)
        temporary = path.with_name(path.name+f'-{os.getpid()}.tmp')
        subprocess.run([os.environ.get('CXX', 'clang++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
                        '-O2', str(SOURCE), '-o', str(temporary)], check=True)
        temporary.replace(path)
    return path


class Solver:
    def __init__(self, *, unary=True, all_values=False, integer=False):
        args = [str(executable())]
        if not unary:
            args.append('--no-unary')
        if all_values:
            args.append('--all')
        if integer:
            args.append('--integer')
        self.process = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True)

    def solve(self, cards, target=0):
        assert 1 <= len(cards) <= 6 and all(isinstance(x, int) and 1 <= x <= 1000000 for x in cards)
        self.process.stdin.write(f'{len(cards)} {target} '+ ' '.join(map(str, cards))+'\n')
        self.process.stdin.flush()
        line = self.process.stdout.readline()
        if not line:
            raise RuntimeError('exact solver exited without a record')
        return {Fraction(row['num'], row['den']): row for row in json.loads(line)}

    def close(self):
        self.process.stdin.close()
        self.process.stdout.close()
        if self.process.wait() != 0:
            raise RuntimeError('exact solver failed')

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.close()


def solve(cards, target=None, *, unary=True):
    with Solver(unary=unary, all_values=target is None) as solver:
        return solver.solve(cards, 0 if target is None else target)


def target_deck(seed, difficulty, target):
    """Host candidate construction, exactly matching rev3 native target_deck.

    The candidate is not graded by this witness. Only exact solve() can accept
    it into revision4. Native/host parity is separately checked over the bank.
    """
    state = seed or 1

    def rand(limit):
        nonlocal state
        state ^= state << 13 & 0xffffffff
        state ^= state >> 17
        state ^= state << 5 & 0xffffffff
        return state % limit

    if difficulty == 0:
        b = 2+rand(11);a = target//b+1;c = 2+rand(9);d = c+a*b-target
        cards = [a, b, c, d];witness = f'{a}*{b}+{c}-{d}'
    elif difficulty == 1:
        d = 2+rand(8);a = d+2+rand(18);b = target*d//a+1;c = a*b-target*d
        cards = [a, b, c, d];witness = f'({a}*{b}-{c})/{d}'
    elif difficulty == 2:
        d = 2+rand(8);e = 2+rand(8);a = d+e+2+rand(18);b = target*(d+e)//a+1;c = a*b-target*(d+e)
        cards = [a, b, c, d, e];witness = f'({a}*{b}-{c})/({d}+{e})'
    else:
        f = 2+rand(8);k = 2+rand(8);d = f*k;e = 1+rand(f-1);a = d+2+rand(18)
        numerator = target*d-e*k;b = numerator//a+1;c = a*b-numerator
        cards = [a, b, c, d, e, f];witness = f'({a}*{b}-{c})/{d}+{e}/{f}'
    for i in range(len(cards)-1, 0, -1):
        j = rand(i+1);cards[i], cards[j] = cards[j], cards[i]
    return cards, witness, state


def target_deck_v4(index, difficulty, target):
    seed = (0xb4a61e37 ^ target*0x9e3779b9 ^ difficulty*0x85ebca6b ^ index*0xc2b2ae35) & 0xffffffff
    state = seed or 1

    def rand(limit):
        nonlocal state
        state ^= state << 13 & 0xffffffff
        state ^= state >> 17
        state ^= state << 5 & 0xffffffff
        return state % limit

    if difficulty == 0:
        return target_deck(state, difficulty, target)
    if difficulty == 1:
        total = 4+rand(min(200, 32767//target)-3)
        c = 2+rand(total-3);d = total-c
        a = 2+rand(target*total-3);b = target*total-a
        cards = [a, b, c, d];witness = f'({a}+{b})/({c}+{d})'
    elif difficulty == 2:
        denominator = 2+rand(min(128, 32767//target)-1)
        c = 12+rand(52);d = 12+rand(52);e = c*d-denominator
        a = 1+rand(target*denominator-1);b = target*denominator-a
        cards = [a, b, c, d, e];witness = f'({a}+{b})/({c}*{d}-{e})'
    else:
        a = 2+rand(30);b = 67+rand(133);d = 29+rand(99);k = 2+rand(2)
        low = max(1, d*b-32767//k);c = low+rand(d*b-low)
        if c % b == 0:
            c += 1
        e = (d*b-c)*k;f = b*k
        cards = [target*a, a+b, c, d, e, f]
        witness = f'{target*a}/({a+b}-{c}/({d}-{e}/{f}))'
    for i in range(len(cards)-1, 0, -1):
        j = rand(i+1);cards[i], cards[j] = cards[j], cards[i]
    return cards, witness, state


def target_deck_v5(index, difficulty, target):
    """Bounded-card candidate construction mirrored in native guesscalc.c.

    A rejected construction returns an empty deck; accepted candidate indices
    are selected only after the unchanged exact solver grades their solutions.
    """
    state = (0xb5a61e37 ^ target*0x9e3779b9 ^ difficulty*0x85ebca6b ^ index*0xc2b2ae35) & 0xffffffff
    state = state or 1

    def rand(limit):
        nonlocal state
        state ^= state << 13 & 0xffffffff
        state ^= state >> 17
        state ^= state << 5 & 0xffffffff
        return state % limit if limit else 0

    cards = []
    witness = ''
    if difficulty == 0:
        return target_deck(state, 0, target)
    if difficulty == 1:
        variant = index % 3
        if variant == 0 and target <= 999:
            maximum = min(48, 1998 // target)
            if maximum >= 2:
                total = 2 + rand(maximum - 1)
                c = 1 + rand(total - 1);d = total - c
                lower = max(1, target*total - 999)
                upper = min(999, target*total - 1)
                a = lower + rand(upper - lower + 1);b = target*total - a
                cards = [a,b,c,d];witness = f'({a}+{b})/({c}+{d})'
        if not cards:
            if variant == 2:
                c = 1 + rand(12);d = 1 + rand(12);total = c*d
                denominator = f'{c}*{d}'
            else:
                total = 2 + rand(31);c = 1 + rand(total-1);d = total-c
                denominator = f'{c}+{d}'
            product = target*total
            factors = [a for a in range(2, min(999, product)+1)
                       if product%a == 0 and product//a <= 999]
            if factors:
                a = factors[rand(len(factors))];b = product//a
                cards = [a,b,c,d];witness = f'({a}*{b})/({denominator})'
    elif difficulty == 2:
        if index % 2:
            cards,witness,state = target_deck(state, 2, target)
            return (cards,witness,state) if max(cards) <= 999 else ([], '', state)
        denominator = 2 + rand(min(48, max(2, 1998//target))-1)
        c = 12 + rand(52);d = 12 + rand(52);e = c*d-denominator
        total = target*denominator
        lower = max(1, total-999);upper = min(999,total-1)
        if lower <= upper and 1 <= e <= 999:
            a = lower+rand(upper-lower+1);b = total-a
            cards=[a,b,c,d,e];witness=f'({a}+{b})/({c}*{d}-{e})'
    else:
        half = target == 1000
        gap = 7 + rand(30);d = 7 + rand(40);k = 2 + rand(5)
        if half:
            b = 1 + gap;factor = 2*b-1
            low = max(1, (d*factor-999//k+1)//2)
            high = min(999, (d*factor-1)//2)
            if low <= high:
                c = low + rand(high-low+1);e = k*(d*factor-2*c);f = k*factor
                cards=[500,b,c,d,e,f]
        else:
            low = max(1,d*gap-999//k);high = min(999,d*gap-1)
            if low <= high:
                c = low + rand(high-low+1);e = k*(d*gap-c);f = k*gap
                cards=[target,1+gap,c,d,e,f]
        if cards:
            a,b,c,d,e,f = cards
            witness=f'{a}/({b}-{c}/({d}-{e}/{f}))'
    if cards and not all(1 <= card <= 999 for card in cards):
        cards=[];witness=''
    if cards:
        for i in range(len(cards)-1, 0, -1):
            j=rand(i+1);cards[i],cards[j]=cards[j],cards[i]
    return cards,witness,state


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cards', type=int, nargs='+')
    parser.add_argument('--target', type=int)
    parser.add_argument('--no-unary', action='store_true')
    args = parser.parse_args()
    print(json.dumps(list(solve(args.cards, args.target, unary=not args.no_unary).values()), indent=2))


if __name__ == '__main__':
    main()
