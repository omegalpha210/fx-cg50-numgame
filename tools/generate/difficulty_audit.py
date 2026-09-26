#!/usr/bin/env python3
"""Combine 36-game difficulty audits and measured beta.6 source banks."""
import argparse
from collections import Counter
import csv
import io
import json
import math
from pathlib import Path
from statistics import median

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
LEVEL_INDEX = {name: index for index, name in enumerate(LEVELS)}


def compact(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"))


def range_metric(values):
    assert values
    return {"min": min(values), "median": median(values), "max": max(values)}


def beta6_measurements():
    target = json.loads((ROOT / "assets/guesscalc_target_beta6.json").read_text())
    countdown = json.loads((ROOT / "assets/guesscalc_countdown_beta6.json").read_text())
    prime = json.loads((ROOT / "assets/guesscalc_prime_beta6.json").read_text())
    assert target["revision"] == 6 and len(target["records"]) == 4000
    assert countdown["revision"] == 5 and len(countdown["records"]) == 800
    assert prime["schema"] == "prime-factor-revision-5" and len(prime["records"]) == 896
    assert len({p["target"] for p in prime["records"]}) == 896
    assert all(math.prod(p["factors"]) == p["target"] and
               len(p["factors"]) >= 2 and max(p["factors"]) <= 97
               for p in prime["records"])
    result = {}
    for level_name, d in LEVEL_INDEX.items():
        records = [p for p in target["records"] if p["difficulty"] == d]
        assert len(records) == 1000
        assert {t: sum(p["target"] == t for p in records)
                for t in (10, 24, 50, 100, 200)} == {
                    10: 200, 24: 200, 50: 200, 100: 200, 200: 200}
        keys = {(p["target"], tuple(sorted(p["cards"]))) for p in records}
        assert len(keys) == len(records)
        assert all(len(p["cards"]) == (4 if d < 2 else 5 if d == 2 else 6)
                   and all(1 <= card <= 999 for card in p["cards"]) for p in records)
        templates = Counter(p["easiest_template"] for p in records)
        digits = Counter(len(str(card)) for p in records for card in p["cards"])
        assert set(digits) <= {1, 2, 3}
        result[(6, level_name)] = {
            "records": 1000, "per_fixed_target": 200,
            "random_reuses_fixed_union": True,
            "target_counts": {str(t): 200 for t in (10, 24, 50, 100, 200)},
            "exact_distinct_target_card_multisets": len(keys),
            "cards_per_problem": len(records[0]["cards"]),
            "card_digits": dict(sorted(digits.items())),
            "card_value": range_metric([card for p in records for card in p["cards"]]),
            "graded_exact_minimum_score": range_metric(
                [p["exact"]["minimal_complexity_score"] for p in records]),
            "canonical_exact_solutions": range_metric(
                [p["exact"]["canonical_count"] for p in records]),
            "fraction_required_records": sum(
                p["exact"]["graded"]["fractional_steps"] > 0 for p in records),
            "easiest_template_distinct": len(templates),
            "top_template_count": templates.most_common(1)[0][1],
        }
        assert result[(6, level_name)]["fraction_required_records"] == (
            1000 if d == 3 else 0)
        records = [p for p in countdown["records"] if p["difficulty"] == d]
        assert len(records) == 200
        keys = {(p["target"], tuple(sorted(p["cards"]))) for p in records}
        assert len(keys) == len(records)
        assert all(p["min_cards"] >= (6 if d == 3 else 4 if d == 2 else 1)
                   for p in records)
        result[(7, level_name)] = {
            "records": 200,
            "exact_distinct_target_card_multisets": len(keys),
            "minimum_card_histogram": dict(sorted(Counter(
                str(p["min_cards"]) for p in records).items())),
            "minimum_cards": range_metric([p["min_cards"] for p in records]),
            "division_required_records": sum(bool(p["division_required"]) for p in records),
            "target": range_metric([p["target"] for p in records]),
            "canonical_exact_solution_count": "NOT RE-MEASURED IN BETA.6 SOURCE",
        }
        if d == 3:
            assert result[(7, level_name)]["division_required_records"] == 200
        metrics = prime["level_metrics"][str(d)]
        expected = 128 if d == 0 else 256
        assert metrics["count"] == expected
        assert sum(p["level"] == d for p in prime["records"]) == expected
        result[(10, level_name)] = metrics
    return result


def apply_beta6(row, measurements):
    game_id = int(row["game_id"])
    difficulty = row["difficulty"]
    if game_id not in (1, 6, 7, 10):
        return
    previous = json.loads(row["observed_distribution"])["after"]
    if game_id == 1:
        d = LEVEL_INDEX[difficulty]
        limit = (20, 30, 40, 50)[d]
        current = dict(previous)
        current["guess_limit"] = limit
        current["secret_space_per_attempt_exact"] = f"{10 ** (d + 4)}/{limit}"
        current["warning_before_included_final_guess"] = True
        current["retained_guesses"] = limit
        row["mechanism"] = (
            f"{d + 4} digits; repeats and leading zero allowed; "
            f"{limit} total guesses, including final chance")
        row["generator_params"] = compact(current)
        row["finding"] = "GC-D04 revised: bounded total attempts increased"
        row["fix"] = "20/30/40/50 total guesses; persisted one-time final warning"
    elif game_id == 6:
        current = measurements[(game_id, difficulty)]
        row["bank_size"] = 1000
        row["mechanism"] = (
            f'{current["cards_per_problem"]} cards, every card used; '
            "five selectable targets, 200 bases each; RANDOM reuses union")
        row["generator_params"] = compact({
            "fixed_targets": [10, 24, 50, 100, 200],
            "per_target": 200, "random_pool": 1000,
            "cards": current["cards_per_problem"], "card_bounds": [1, 999],
            "grade": "exact minimum over legal expressions"})
        row["rating_metric"] = "exact rational minimum; template/card distributions"
        row["meaningfully_distinct"] = (
            "1,000 distinct target/card multisets per level; template concentration measured")
        row["finding"] = "GC-D05 expanded within existing exact grade"
        row["fix"] = "200 distinct bases per fixed target; RANDOM aliases five banks"
    elif game_id == 7:
        current = measurements[(game_id, difficulty)]
        row["bank_size"] = 200
        row["mechanism"] = (
            "six cards; any nonempty subset may reach target; positive integer "
            "intermediates and exact division")
        row["generator_params"] = compact({
            "per_level": 200, "cards": 6,
            "minimum_card_histogram": current["minimum_card_histogram"]})
        row["rating_metric"] = "exact subset minimum cards; division necessity"
        row["meaningfully_distinct"] = (
            "200 distinct target/card multisets; exact minimum-card histogram reported")
        row["finding"] = "GC-D01 expanded, HARD/MASTER shortcut restrictions retained"
        row["fix"] = "200 exact-solver-checked bases per level"
    else:
        current = measurements[(game_id, difficulty)]
        row["bank_size"] = current["count"]
        digits = "/".join(sorted(current["digits"], key=int))
        row["mechanism"] = (
            f'composite targets, {digits}-digit group(s), '
            "max prime <=97; structural factor classification")
        row["generator_params"] = compact({
            "bank_size": current["count"], "digits": current["digits"],
            "max_prime": current["max_prime"],
            "omega": current["omega"]})
        row["rating_metric"] = (
            "decimal digits; exact prime factors; trial-division proxy (not human time)")
        row["meaningfully_distinct"] = (
            "exact target identities disjoint across levels; digit groups balanced")
        row["finding"] = "GC-D03 revised for digit length and factor structure"
        row["fix"] = "128 EASY; 256 other levels, balanced shared-digit groups"
    row["observed_distribution"] = compact({"before": previous, "after": current})


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
    measurements = beta6_measurements()
    for row in rows:
        apply_beta6(row, measurements)
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
        if game_id == 1:
            decision, finding_id = "REVISED / TOTAL ATTEMPT CAP", "GC-D04"
        elif game_id == 5 and difficulty in ("EASY", "NORMAL"):
            decision, finding_id = "RESOLVED / BANK REBALANCED", "GC-D02"
        elif game_id == 6:
            decision, finding_id = "REVISED / 200 PER TARGET", "GC-D05"
        elif game_id == 7:
            decision, finding_id = "REVISED / 200 PER LEVEL", "GC-D01"
        elif game_id == 10:
            decision, finding_id = "REVISED / DIGIT AND FACTOR BANDS", "GC-D03"
        elif game_id == 27 and difficulty != "MASTER":
            decision = "RESOLVED / EXACT DISTANCE BANDS"
        elif game_id == 27:
            decision = "UNCHANGED / CERTIFIED MASTER RETAINED"
        elif game_id == 2 and difficulty == "MASTER":
            decision, finding_id = "ACCEPTED AS DESIGNED", "GC-D04"
        elif game_id == 34:
            decision, finding_id = "ACCEPTED / OVERLAP DOCUMENTED", "GC-D06"
        metric_by_game = {
            1: "digits;repeated-digit secret domain;total guesses and history",
            2: "equation length;operator distribution;guess allowance",
            5: "rule-family distribution;prefix magnitude;bounded-grammar ambiguity",
            6: "exact rational minimum;card/target/template diversity",
            7: "exact subset minimum cards;division necessity",
            10: "digit bands;exact factorization;trial-division proxy",
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
