#!/usr/bin/env python3
"""Combine the four independently generated 36-game difficulty audits."""
import argparse
import csv
import io
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
FIELDS = (
    "game_id", "name", "difficulty", "mechanism", "bank_size", "board_size",
    "generator_params", "AI_policy", "rating_metric", "observed_distribution",
    "overlap", "meaningfully_distinct", "finding", "fix",
)
LEVELS = ("EASY", "NORMAL", "HARD", "MASTER")
VISIBLE = set(range(1, 29)) | set(range(31, 39))
HELL = {11, 12, 13, 14, 15, 35}


def source_rows():
    files = (
        "assets/guesscalc_difficulty_rows.json",
        "assets/grids/extra/difficulty-beta3.json",
        "assets/strategyquick/difficulty-beta3.csv",
        "assets/boards/difficulty-beta3.json",
    )
    groups = []
    for name in files:
        path = ROOT / name
        if path.suffix == ".csv":
            with path.open(newline="") as stream:
                reader = csv.DictReader(stream)
                assert tuple(reader.fieldnames or ()) == FIELDS, name
                rows = list(reader)
        else:
            data = json.loads(path.read_text())
            rows = data if isinstance(data, list) else data["csv_rows"]
        groups.append(rows)
    return groups


def combine():
    rows = [dict(row) for group in source_rows() for row in group]
    seen = set()
    for row in rows:
        assert tuple(row) == FIELDS, (row.get("game_id"), tuple(row))
        game_id = int(row["game_id"])
        difficulty = row["difficulty"]
        assert game_id in VISIBLE and difficulty in LEVELS + ("HELL",)
        assert difficulty != "HELL" or game_id in HELL
        key = (game_id, difficulty)
        assert key not in seen, key
        seen.add(key)
        assert row["name"] and row["mechanism"] and row["rating_metric"]
        row["game_id"] = game_id
    assert len(rows) == 150
    assert {game_id for game_id, _ in seen} == VISIBLE
    for game_id in VISIBLE:
        expected = set(LEVELS) | ({"HELL"} if game_id in HELL else set())
        assert {difficulty for row_id, difficulty in seen if row_id == game_id} == expected
    rows.sort(key=lambda row: (row["game_id"], (LEVELS + ("HELL",)).index(row["difficulty"])))
    out = io.StringIO(newline="")
    writer = csv.DictWriter(out, FIELDS, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    return out.getvalue()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="require the checked-in CSV to match")
    args = parser.parse_args()
    path = ROOT / "docs/DIFFICULTY_AUDIT.csv"
    content = combine()
    if args.check:
        assert path.read_text() == content, "difficulty CSV needs regeneration"
        print("36 games / 150 level rows: audit CSV matches independent source reports")
    else:
        path.write_text(content)
        print("wrote", path.relative_to(ROOT), "(36 games / 150 level rows)")


if __name__ == "__main__":
    main()
