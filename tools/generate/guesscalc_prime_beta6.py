#!/usr/bin/env python3
"""Generate the bounded, factor-verified Prime Factor revision-5 target table.

Each target is selected once from a finite mathematical candidate domain. The
published sample is balanced across overlapping digit groups and largest-prime
classes. The order in each group is numeric so the native table is auditable.
"""

from __future__ import annotations

import argparse
import collections
import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
JSON = ROOT / "assets/guesscalc_prime_beta6.json"
HEADER = ROOT / "assets/guesscalc_prime_beta6.h"
PRIMES = tuple(p for p in range(2, 98) if all(p % q for q in range(2, math.isqrt(p) + 1)))
GROUPS = ("E3", "N3", "N4", "H4", "H5", "M5", "M6")
GROUP_LEVEL = {"E3": 0, "N3": 1, "N4": 1, "H4": 2, "H5": 2, "M5": 3, "M6": 3}
PER_GROUP = 128
HASH_SEED = 0x5046494D45423600


def factors(n: int) -> tuple[int, ...]:
    rest = n
    result = []
    for p in PRIMES:
        while rest % p == 0:
            result.append(p)
            rest //= p
    if rest > 1:
        result.append(rest)
    assert math.prod(result) == n
    return tuple(result)


def trial_checks(n: int) -> int:
    """Modulus checks by a simple ascending divisor search, a CPU proxy only."""
    rest, checks, p = n, 0, 2
    while p <= rest // p:
        checks += 1
        if rest % p == 0:
            rest //= p
        else:
            p += 1
    return checks


def group_of(n: int, f: tuple[int, ...]) -> str | None:
    if len(f) < 2 or max(f) > 97:
        return None
    d, m, omega = len(str(n)), max(f), len(f)
    large_distinct = len({p for p in f if p >= 11})
    if d == 3:
        if m <= 11 and omega >= 3:
            return "E3"
        if 17 <= m <= 31 and omega >= 3:
            return "N3"
    elif d == 4:
        if 11 <= m <= 19 and omega >= 4:
            return "N4"
        if 23 <= m <= 97 and omega >= 4:
            return "H4"
    elif d == 5:
        if 11 <= m <= 31 and omega >= 5:
            return "H5"
        if 37 <= m <= 97 and omega >= 4 and large_distinct >= 2:
            return "M5"
    elif d == 6 and 37 <= m <= 97 and 5 <= omega <= 12 and large_distinct >= 2:
        return "M6"
    return None


def mix64(x: int) -> int:
    x = (x + 0x9E3779B97F4A7C15) & 0xFFFFFFFFFFFFFFFF
    x = ((x ^ (x >> 30)) * 0xBF58476D1CE4E5B9) & 0xFFFFFFFFFFFFFFFF
    x = ((x ^ (x >> 27)) * 0x94D049BB133111EB) & 0xFFFFFFFFFFFFFFFF
    return x ^ (x >> 31)


def choose(name: str, values: list[int]) -> list[int]:
    # Prefer all 7-smooth EASY starts. There are 95; fill the remaining 33
    # slots from max-prime-11 starts using the same deterministic hash.
    if name == "E3":
        smooth = [v for v in values if max(factors(v)) <= 7]
        other = [v for v in values if max(factors(v)) == 11]
        assert len(smooth) == 95 and len(other) == 41
        other.sort(key=lambda v: (mix64(v ^ HASH_SEED), v))
        return sorted(smooth + other[:PER_GROUP - len(smooth)])
    by_prime: dict[int, list[int]] = collections.defaultdict(list)
    for v in values:
        by_prime[max(factors(v))].append(v)
    keys = sorted(by_prime)
    for p in keys:
        by_prime[p].sort(key=lambda v: (mix64(v ^ HASH_SEED), v))
    positions = {p: 0 for p in keys}
    selected = []
    while len(selected) < PER_GROUP:
        progressed = False
        for p in keys:
            i = positions[p]
            if i < len(by_prime[p]):
                selected.append(by_prime[p][i])
                positions[p] = i + 1
                progressed = True
                if len(selected) == PER_GROUP:
                    break
        if not progressed:
            raise ValueError(f"{name}: only {len(selected)} eligible targets")
    return sorted(selected)


def record(n: int, group: str, index: int) -> dict:
    f = factors(n)
    counts = collections.Counter(f)
    residual = n
    for p in (2, 3, 5, 7):
        while residual % p == 0:
            residual //= p
    return dict(index=index, target=n, group=group, level=GROUP_LEVEL[group],
                digits=len(str(n)), factors=list(f), omega=len(f), distinct=len(counts),
                max_prime=max(f), repeated_prime_count=sum(v > 1 for v in counts.values()),
                repeated_exponents={str(k): v for k, v in counts.items() if v > 1},
                residual_after_2_3_5_7=residual,
                residual_factors=[p for p in f if p > 7],
                removed_small_factor_count=sum(p <= 7 for p in f),
                trial_division_proxy=trial_checks(n))


def metrics(records: list[dict]) -> dict:
    def hist(key: str) -> dict[str, int]:
        return dict(sorted(collections.Counter(str(r[key]) for r in records).items(),
                           key=lambda kv: int(kv[0])))

    return dict(count=len(records), digits=hist("digits"), max_prime=hist("max_prime"),
                omega=hist("omega"), repeated_prime_count=hist("repeated_prime_count"),
                residual_factor_count=dict(sorted(collections.Counter(
                    str(len(r["residual_factors"])) for r in records).items(), key=lambda kv: int(kv[0]))),
                residual_after_2_3_5_7_equal_one=sum(r["residual_after_2_3_5_7"] == 1 for r in records),
                target_min=min(r["target"] for r in records),
                target_max=max(r["target"] for r in records),
                trial_division_proxy_min=min(r["trial_division_proxy"] for r in records),
                trial_division_proxy_median=sorted(r["trial_division_proxy"] for r in records)[len(records)//2],
                trial_division_proxy_max=max(r["trial_division_proxy"] for r in records))


def generate() -> tuple[str, str]:
    eligible: dict[str, list[int]] = {name: [] for name in GROUPS}
    for n in range(100, 1_000_000):
        f = factors(n)
        group = group_of(n, f)
        if group:
            eligible[group].append(n)
    selected = {name: choose(name, eligible[name]) for name in GROUPS}
    all_values = [n for name in GROUPS for n in selected[name]]
    assert len(all_values) == len(set(all_values)) == len(GROUPS) * PER_GROUP
    records = []
    level_positions = {level: 0 for level in range(4)}
    for name in GROUPS:
        for i, n in enumerate(selected[name]):
            r = record(n, name, i)
            r["level_index"] = level_positions[GROUP_LEVEL[name]] + i
            assert group_of(n, tuple(r["factors"])) == name
            assert math.prod(r["factors"]) == n and n <= 999_999
            records.append(r)
        level_positions[GROUP_LEVEL[name]] += PER_GROUP
    doc = dict(schema="prime-factor-revision-5", selection_seed=hex(HASH_SEED),
               selection="All 7-smooth E3 plus 33 max-prime-11 E3; remaining groups round-robin by max prime with SplitMix64 ordering; 128 per group.",
               group_order=list(GROUPS), group_level=GROUP_LEVEL,
               eligible_count={name: len(eligible[name]) for name in GROUPS},
               selected_metrics={name: metrics([r for r in records if r["group"] == name]) for name in GROUPS},
               level_metrics={str(level): metrics([r for r in records if r["level"] == level]) for level in range(4)},
               records=records)
    json_text = json.dumps(doc, indent=2, ensure_ascii=False) + "\n"
    lines = ["/* Generated by tools/generate/guesscalc_prime_beta6.py. Do not edit. */",
             "#ifndef NG_PRIME_BETA6_TARGETS_H", "#define NG_PRIME_BETA6_TARGETS_H", "#include <stdint.h>",
             "static const uint32_t ng_prime_beta6_targets[896] = {"]
    for name in GROUPS:
        lines.append(f" /* {name}: 128 verified composite targets */")
        values = selected[name]
        for pos in range(0, len(values), 8):
            lines.append(" " + ", ".join(str(n) for n in values[pos:pos+8]) + ",")
    lines.extend(["};", "#endif", ""])
    return json_text, "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--write", action="store_true")
    mode.add_argument("--check", action="store_true")
    args = parser.parse_args()
    json_text, header_text = generate()
    if args.write:
        JSON.write_text(json_text)
        HEADER.write_text(header_text)
    else:
        assert JSON.read_text() == json_text, f"stale {JSON}"
        assert HEADER.read_text() == header_text, f"stale {HEADER}"
    doc = json.loads(json_text)
    print("Prime beta6:", len(doc["records"]), "targets; candidate domains", doc["eligible_count"])
    for level, m in doc["level_metrics"].items():
        print("level", level, "count", m["count"], "digits", m["digits"],
              "target", m["target_min"], "..", m["target_max"],
              "trial proxy", m["trial_division_proxy_min"], m["trial_division_proxy_median"], m["trial_division_proxy_max"])


if __name__ == "__main__":
    main()
