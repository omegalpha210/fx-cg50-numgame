#!/usr/bin/env python3
"""Combine the four independently generated 36-game difficulty audits."""
import argparse
import csv
import io
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE_FIELDS = (
    "game_id", "name", "difficulty", "mechanism", "bank_size", "board_size",
    "generator_params", "AI_policy", "rating_metric", "observed_distribution",
    "overlap", "meaningfully_distinct", "finding", "fix",
)
FIELDS = SOURCE_FIELDS + ("metric", "before", "after", "finding_id", "decision")
LEVELS = ("EASY", "NORMAL", "HARD", "MASTER")
VISIBLE = set(range(1, 29)) | set(range(31, 39))
HELL = {11, 12, 13, 14, 15, 35}


def source_rows():
    files = (
        "assets/guesscalc_difficulty_rows_beta5.json",
        "assets/grids/extra/difficulty-beta3.json",
        "assets/strategyquick/difficulty-beta4.csv",
        "assets/boards/difficulty-beta3.json",
    )
    groups = []
    for name in files:
        path = ROOT / name
        if path.suffix == ".csv":
            with path.open(newline="") as stream:
                reader = csv.DictReader(stream)
                assert tuple(reader.fieldnames or ()) == SOURCE_FIELDS, name
                rows = list(reader)
        else:
            data = json.loads(path.read_text())
            rows = data if isinstance(data, list) else data["csv_rows"]
        groups.append(rows)
    return groups


def combine():
    rows = [dict(row) for group in source_rows() for row in group]
    with (ROOT / "docs/DIFFICULTY_AUDIT_BETA3.csv").open(newline="") as stream:
        baseline_rows = list(csv.DictReader(stream))
    assert len(baseline_rows) == 150
    baseline = {(int(row["game_id"]), row["difficulty"]): row for row in baseline_rows}
    assert len(baseline) == 150
    seen = set()
    for row in rows:
        assert set(row) == set(SOURCE_FIELDS), (row.get("game_id"), tuple(row))
        game_id = int(row["game_id"])
        difficulty = row["difficulty"]
        assert game_id in VISIBLE and difficulty in LEVELS + ("HELL",)
        assert difficulty != "HELL" or game_id in HELL
        key = (game_id, difficulty)
        assert key not in seen, key
        seen.add(key)
        assert row["name"] and row["mechanism"] and row["rating_metric"]
        assert key in baseline
        decision, finding_id = "UNCHANGED", ""
        if game_id == 5 and difficulty in ("EASY", "NORMAL"):
            decision, finding_id = "RESOLVED / BANK REBALANCED", "GC-D02"
        elif game_id == 6:
            decision, finding_id = "RESOLVED / EXHAUSTIVE COMPLEXITY GRADING", "GC-D05"
        elif game_id == 7 and difficulty == "HARD":
            decision, finding_id = "RESOLVED / CONTENT REVISED", "GC-D01"
        elif game_id == 10 and difficulty == "MASTER":
            decision, finding_id = "RESOLVED / MASTER REVISED", "GC-D03"
        elif game_id == 27 and difficulty != "MASTER":
            decision = "RESOLVED / EXACT DISTANCE BANDS"
        elif game_id == 27:
            decision = "UNCHANGED / CERTIFIED MASTER RETAINED"
        elif game_id in (1, 2) and difficulty == "MASTER":
            decision, finding_id = "ACCEPTED AS DESIGNED", "GC-D04"
        elif game_id == 34:
            decision, finding_id = "ACCEPTED / OVERLAP DOCUMENTED", "GC-D06"
        if game_id == 7 and difficulty != "HARD":
            row["finding"] = "UNCHANGED / GC-D01 applies to HARD"
            row["fix"] = "Original bank retained; exact metrics reverified"
        if game_id == 10 and difficulty != "MASTER":
            row["finding"] = "UNCHANGED / GC-D03 applies to MASTER"
            row["fix"] = "Original generator retained"
        metric_by_game = {
            1: "digits;repeated-digit secret domain;guess allowance",
            2: "equation length;operator distribution;guess allowance",
            5: "rule-family distribution;prefix magnitude;bounded-grammar ambiguity",
            6: "exact rational minimum tuple;graded score;canonical solution count",
            7: "exact subset minimum cards/operations;canonical solution count",
            10: "exact factorization tuple;prime domain;target magnitude",
            34: "unique solution;letters;carry columns;column-search nodes",
        }
        row["metric"] = metric_by_game.get(game_id, row["rating_metric"])
        if game_id in {1, 2, 5, 6, 7, 10, 34}:
            observed = json.loads(row["observed_distribution"])
            assert set(observed) == {"before", "after"}, key
            row["before"] = json.dumps(observed["before"], sort_keys=True, separators=(",", ":"))
            row["after"] = json.dumps(observed["after"], sort_keys=True, separators=(",", ":"))
        else:
            row["before"] = baseline[key]["observed_distribution"]
            row["after"] = row["observed_distribution"]
        row["finding_id"] = finding_id
        row["decision"] = decision
        row["game_id"] = game_id
    assert len(rows) == 150
    assert seen == set(baseline)
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
