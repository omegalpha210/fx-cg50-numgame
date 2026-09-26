#!/usr/bin/env python3
"""Independent factor and native-table checks for Prime Factor revision 5."""

from __future__ import annotations

import collections
import json
import math
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
DOC = json.loads((ROOT / "assets/guesscalc_prime_beta6.json").read_text())
GROUPS = ("E3", "N3", "N4", "H4", "H5", "M5", "M6")
LEVEL = {"E3": 0, "N3": 1, "N4": 1, "H4": 2, "H5": 2, "M5": 3, "M6": 3}


def independent_factors(value: int) -> tuple[int, ...]:
    # Deliberately does not import the generator's prime list or factor code.
    assert value >= 2
    rest, divisor, found = value, 2, []
    while divisor * divisor <= rest:
        if rest % divisor == 0:
            found.append(divisor)
            rest //= divisor
        else:
            divisor += 1
    if rest != 1:
        found.append(rest)
    return tuple(found)


def prime(n: int) -> bool:
    return n > 1 and all(n % d for d in range(2, math.isqrt(n) + 1))


def qualifies(group: str, target: int, f: tuple[int, ...]) -> bool:
    digits, largest, omega = len(str(target)), max(f), len(f)
    large_distinct = len({p for p in f if p >= 11})
    return {
        "E3": digits == 3 and largest <= 11 and omega >= 3,
        "N3": digits == 3 and 17 <= largest <= 31 and omega >= 3,
        "N4": digits == 4 and 11 <= largest <= 19 and omega >= 4,
        "H4": digits == 4 and 23 <= largest <= 97 and omega >= 4,
        "H5": digits == 5 and 11 <= largest <= 31 and omega >= 5,
        "M5": digits == 5 and 37 <= largest <= 97 and omega >= 4 and large_distinct >= 2,
        "M6": digits == 6 and 37 <= largest <= 97 and 5 <= omega <= 12 and large_distinct >= 2,
    }[group]


def native_table() -> list[int]:
    source = r'''
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "prime_beta6.h"
#include "guesscalc_math.h"
int main(void) {
 assert(ng_prime_beta6_count(0)==128u);
 for(unsigned d=1; d<4; d++) assert(ng_prime_beta6_count(d)==256u);
 assert(ng_prime_beta6_count(4)==0u);
 assert(ng_prime_beta6_target(4,0)==0u);
 assert(ng_prime_beta6_target(0,128)==0u);
 assert(ng_prime_beta6_target(1,256)==0u);
 assert(!ng_prime_beta6_valid(4,360));
 assert(!ng_prime_beta6_valid(0,1));
 assert(!ng_prime_beta6_valid(3,1000000));
 for(unsigned d=0; d<4; d++)
   for(unsigned i=0; i<ng_prime_beta6_count(d); i++) {
     uint32_t n=ng_prime_beta6_target(d,i);
     assert(n>=100u && n<=999999u && ng_prime_beta6_valid(d,n));
     for(unsigned other=0; other<4; other++)
       if(other!=d) assert(!ng_prime_beta6_valid(other,n));
     unsigned rest=n;
     char answer[97]={0};
     unsigned len=0;
     for(unsigned p=2; p<=rest/p; p++)
       while(rest%p==0) {
         int written=snprintf(answer+len,sizeof(answer)-len,"%s%u",len?"*":"",p);
         assert(written>0 && (unsigned)written<sizeof(answer)-len);
         len+=(unsigned)written;
         rest/=p;
       }
     if(rest>1) {
       int written=snprintf(answer+len,sizeof(answer)-len,"%s%u",len?"*":"",rest);
       assert(written>0 && (unsigned)written<sizeof(answer)-len);
     }
     assert(gc_factorization(answer,(int)n));
     printf("%u %u %u\n",d,i,n);
   }
 return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix="numgame-prime-beta6-") as tmp:
        src, binary = Path(tmp) / "test.c", Path(tmp) / "test"
        src.write_text(source)
        subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-Wall", "-Wextra", "-Werror",
                        "-I", str(ROOT / "src/games"), str(src),
                        str(ROOT / "src/games/prime_beta6.c"),
                        str(ROOT / "src/games/guesscalc_math.c"), "-o", str(binary)], check=True)
        output = subprocess.check_output([str(binary)], text=True)
    lines = [tuple(int(x) for x in line.split()) for line in output.splitlines()]
    assert len(lines) == 896
    for expected, (d, index, value) in enumerate(lines):
        assert d == (0 if expected < 128 else 1 + (expected - 128)//256)
        assert index == (expected if d == 0 else (expected - 128) % 256)
        assert value > 0
    return [line[2] for line in lines]


def main() -> None:
    assert DOC["schema"] == "prime-factor-revision-5"
    assert tuple(DOC["group_order"]) == GROUPS
    records, native = DOC["records"], native_table()
    assert len(records) == len(native) == 896
    assert len(set(native)) == 896
    from_data = [r["target"] for r in records]
    assert native == from_data
    digit_hist = collections.defaultdict(collections.Counter)
    max_hist = collections.defaultdict(collections.Counter)
    smooth_easy = 0
    for group_index, group in enumerate(GROUPS):
        block = records[group_index * 128:(group_index + 1) * 128]
        assert len(block) == 128
        assert [r["target"] for r in block] == sorted(r["target"] for r in block)
        for i, r in enumerate(block):
            n = r["target"]
            f = independent_factors(n)
            assert r["group"] == group and r["level"] == LEVEL[group] and r["index"] == i
            assert r["level_index"] == i + (128 if group in ("N4", "H5", "M6") else 0)
            assert 100 <= n <= 999999 and len(f) >= 2 and math.prod(f) == n
            assert all(prime(p) and p <= 97 for p in f)
            assert qualifies(group, n, f)
            assert r["factors"] == list(f) and r["omega"] == len(f)
            assert r["max_prime"] == max(f)
            assert r["distinct"] == len(set(f))
            assert r["repeated_prime_count"] == sum(c > 1 for c in collections.Counter(f).values())
            rest = n
            for p in (2, 3, 5, 7):
                while rest % p == 0:
                    rest //= p
            assert r["residual_after_2_3_5_7"] == rest
            assert r["residual_factors"] == [p for p in f if p > 7]
            assert r["removed_small_factor_count"] == sum(p <= 7 for p in f)
            if group == "E3" and max(f) <= 7:
                smooth_easy += 1
            digit_hist[LEVEL[group]][len(str(n))] += 1
            max_hist[LEVEL[group]][max(f)] += 1
    assert smooth_easy == 95
    assert [dict(digit_hist[d]) for d in range(4)] == [{3: 128}, {3: 128, 4: 128},
                                                       {4: 128, 5: 128}, {5: 128, 6: 128}]
    print("Prime beta6 independent/native PASS: 896 distinct composites; E/N/H/M digit groups 128 / 128+128 / 128+128 / 128+128")
    print("Maximum-prime histograms:", {d: dict(sorted(max_hist[d].items())) for d in range(4)})


if __name__ == "__main__":
    main()
