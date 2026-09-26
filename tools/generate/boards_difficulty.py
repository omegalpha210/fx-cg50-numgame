#!/usr/bin/env python3
"""Audit the public-clue difficulty buckets of Shikaku and Slitherlink."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
COLUMNS = ("game_id", "name", "difficulty", "mechanism", "bank_size",
           "board_size", "generator_params", "AI_policy", "rating_metric",
           "observed_distribution", "overlap", "meaningfully_distinct",
           "finding", "fix")
LEVELS = ("EASY", "NORMAL", "HARD", "MASTER")
NAMES = {31: "SHIKAKU", 32: "SLITHERLINK"}


def canonical(clues: list[int], size: int) -> tuple[int, ...]:
    grid = [clues[i * size:(i + 1) * size] for i in range(size)]
    variants = []
    for reflected in (False, True):
        current = [row[::-1] for row in grid] if reflected else [row[:] for row in grid]
        for _ in range(4):
            variants.append(tuple(value for row in current for value in row))
            current = [[current[size - 1 - col][row] for col in range(size)]
                       for row in range(size)]
    return min(variants)


def report() -> dict:
    source = ROOT / "assets/boards/puzzles.json"
    verified = ROOT / "assets/boards/verification.json"
    puzzles = json.loads(source.read_text())["puzzles"]
    groups = json.loads(verified.read_text())["groups"]
    assert len(puzzles) == 240 and len(groups) == 8
    rows = []
    for game_id, name in NAMES.items():
        game = [p for p in puzzles if p["game_id"] == game_id]
        assert len(game) == 120
        by_level = [[p for p in game if p["difficulty"] == d] for d in range(4)]
        for d, records in enumerate(by_level):
            summary = next(g for g in groups if g["game_id"] == game_id and g["difficulty"] == d)
            assert len(records) == summary["count"] == summary["unique"] == 30
            n = summary["size"]
            exact = {(p["size"], tuple(p["clues"])) for p in records}
            equivalent = {(n, canonical(p["clues"], n)) for p in records}
            others = [p for i, level in enumerate(by_level) if i != d for p in level]
            exact_overlap = len(exact & {(p["size"], tuple(p["clues"])) for p in others})
            canonical_overlap = len(equivalent & {(p["size"], canonical(p["clues"], p["size"])) for p in others})
            assert not exact_overlap and not canonical_overlap
            clue_counts = [sum(v > 0 for v in p["clues"]) if game_id == 31
                           else sum(v >= 0 for v in p["clues"]) for p in records]
            assert min(clue_counts) == summary["clues_min"]
            assert max(clue_counts) == summary["clues_max"]
            mechanism = "COMPOSITE" if d == 3 else "BOARD_SIZE" if d <= 1 else "BANK_DIFFERENCE"
            finding = "MASTER shares 8x8 with HARD; structural search filters, not every-human-order proof" if d == 3 else "none"
            values = {
                "game_id": game_id, "name": name, "difficulty": LEVELS[d],
                "mechanism": mechanism, "bank_size": 30, "board_size": f"{n}x{n}",
                "generator_params": "original host bank; MASTER structural filter" if d == 3 else "original host bank",
                "AI_policy": "none",
                "rating_metric": "independent public-clue reference solver nodes",
                "observed_distribution":
                    f"clues={min(clue_counts)}..{max(clue_counts)} mean={sum(clue_counts)/30:.2f}; "
                    f"reference_nodes={summary['reference_nodes_min']}..{summary['reference_nodes_max']}",
                "overlap": f"exact={exact_overlap};D4={canonical_overlap} across levels",
                "meaningfully_distinct": True, "finding": finding, "fix": "none",
            }
            assert tuple(values) == COLUMNS
            rows.append(values)
    return {"source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
            "independent_verification_sha256": hashlib.sha256(verified.read_bytes()).hexdigest(),
            "records": len(puzzles), "csv_rows": rows}


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, default=ROOT / "assets/boards/difficulty-beta3.json")
    parser.add_argument("--verify", action="store_true")
    args = parser.parse_args()
    rendered = json.dumps(report(), indent=2, ensure_ascii=False) + "\n"
    if args.verify:
        assert args.output.read_text() == rendered
        print("Boards difficulty: 8 bank rows, 240 records, no exact/D4 cross-level overlap PASS")
    else:
        args.output.write_text(rendered)
        print(f"Wrote {args.output}")


if __name__ == "__main__":
    main()
