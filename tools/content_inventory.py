#!/usr/bin/env python3
"""Inventory the *reachable* native content, separately from saved-run archives.

This is a file/registry audit, not a puzzle generator or a substitute for the
family's independent rule validators. Its output is deterministic without Git
history, and it refuses to report an unfinished beta.6 bank as complete.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import csv
import hashlib
import io
import json
from pathlib import Path
import re

BASELINE = "4e7da73bef82e7e20dce2a8d80ff85d9b8f66639"
IDS = [1, 2, 3, 4, 5, 33, 6, 7, 8, 9, 10, 34,
       11, 12, 13, 14, 15, 35, 16, 17, 18, 19, 20, 36,
       21, 22, 23, 24, 25, 37, 26, 27, 28, 31, 32, 38]
NAMES = {
    1: "NUMBER BASEBALL", 2: "EQUATION GUESS", 3: "NUMBER MIND",
    4: "CLUE LOCK", 5: "SEQUENCE DETECTIVE", 6: "MAKE TARGET",
    7: "COUNTDOWN", 8: "MISSING OPERATORS", 9: "CROSS MATH",
    10: "PRIME FACTOR", 11: "SUDOKU", 12: "CALCUDOKU", 13: "KAKURO",
    14: "FUTOSHIKI", 15: "SKYSCRAPERS", 16: "HITORI",
    17: "BINARY PUZZLE", 18: "NUMBRIX", 19: "MAGIC SQUARE",
    20: "SUM GRID", 21: "NIM", 22: "WYTHOFF", 23: "EUCLID",
    24: "MAKE FIFTEEN", 25: "RACE TO TARGET", 26: "2048",
    27: "SLIDING PUZZLE", 28: "LIGHTS OUT", 31: "SHIKAKU",
    32: "SLITHERLINK", 33: "BLACK BOX", 34: "CRYPTARITHM",
    35: "HASHI", 36: "NONOGRAM", 37: "REVERSI", 38: "NET",
}
CATEGORIES = ["GUESS", "CALC", "LOGIC", "PUZZLE", "STRATEGY", "BOARD"]
LEVELS = ["EASY", "NORMAL", "HARD", "MASTER", "HELL"]
TARGETS = [10, 24, 50, 100, 200]
GC_KEYS = {2: "equation", 3: "number_mind", 4: "clue_lock",
           5: "sequence", 6: "target", 7: "countdown",
           8: "operators", 9: "crossmath"}
MODE_NAMES = {
    2: ["STANDARD", "SHORT", "LONG"], 19: ["PARTIAL", "FREE"],
    26: ["CLASSIC", "TARGET"], 27: ["3x3", "4x4"],
    28: ["4x4", "5x5"], 37: ["YOU FIRST", "CPU FIRST"],
}
for _gid in range(21, 26):
    MODE_NAMES[_gid] = ["YOU FIRST", "CPU FIRST"]
def enc(value):
    return json.dumps(value, sort_keys=True, ensure_ascii=False, separators=(",", ":"))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def baseline_count(gid):
    if gid == 2:
        return [90, 90, 90, 0, 0]
    if gid == 6:
        return [60, 60, 60, 0, 0]
    if gid in (3, 4, 5, 7, 9, 31, 32):
        return [30, 30, 30, 0, 0]
    if 11 <= gid <= 20:
        return [30, 30, 30, 20 if gid in (11, 12, 14, 15) else 0, 0]
    return [0] * 5


def level(record):
    return int(record.get("difficulty", record.get("level", 3)))


def key(gid, p):
    """Public base identity; order/witness/seed metadata are not new bases."""
    if gid in (6, 7):
        return (int(p["target"]), tuple(sorted(p["cards"])))
    if gid == 10:
        return int(p["target"])
    if gid == 2:
        return p["equation"]
    if gid == 3:
        return (p["n"], p["alphabet"], enc(sorted(p["clues"], key=enc)))
    if gid == 4:
        return (p["limit"], enc(p["params"]))
    if gid == 5:
        return tuple(p["sequence"][:6])
    if gid == 8:
        return (tuple(p["numbers"]), p["target"])
    if gid == 9:
        return (tuple(p["ops"]), tuple(p["targets"]), tuple(p["givens"]))
    if 11 <= gid <= 20:
        return enc({k: p.get(k) for k in ("n", "cells", "a", "b")})
    if 21 <= gid <= 25:
        return enc({k: p.get(k) for k in ("cpu_first", "board", "max_add", "target")})
    if gid == 27:
        return (p["mode"], tuple(p["board"]))
    if gid == 28:
        return (p["mode"], p["board_bits"])
    if gid in (31, 32):
        return (p["size"], tuple(p["clues"]))
    if gid == 34:
        return tuple(p["words"])
    if gid == 35:
        return (p["n"], tuple(p["pos"]), tuple(p["clues"]))
    if gid == 36:
        return (p["n"], enc(p["clues"]))
    raise ValueError(f"No base identity for ID {gid}")


def declared(text, symbol, expected):
    m = re.search(r"\b" + re.escape(symbol) + r"\s*\[\s*(\d+)\s*\]", text)
    if not m or int(m.group(1)) != expected:
        raise ValueError(f"{symbol}: native declared count differs from {expected}")


class Source:
    def __init__(self, root):
        self.root = root
        self.manifest = {}
        self.owners = defaultdict(set)

    def text(self, path, gid=None):
        file = self.root / path
        value = file.read_text()
        self.manifest[path] = {"bytes": file.stat().st_size, "sha256": sha(file)}
        if gid is not None:
            self.owners[gid].add(path)
        return value

    def json(self, path, gid=None):
        return json.loads(self.text(path, gid))


def make_records(source):
    active = {gid: [] for gid in IDS}
    legacy = {gid: [] for gid in IDS}
    gc_old = source.json("assets/guesscalc_packs.json")
    gc_master = source.json("assets/guesscalc_master.json")
    source.text("assets/guesscalc_packs.h")
    source.text("assets/guesscalc_master.h")
    for gid, name in GC_KEYS.items():
        old = gc_old.get(name, []) + gc_master.get(name, [])
        if gid == 5:
            active[gid] = [p for p in old if level(p) >= 2]
            legacy[gid] = [p for p in old if level(p) < 2]
        elif gid in (6, 7):
            legacy[gid] = old[:]
        else:
            active[gid] = old[:]
        source.owners[gid].update(("assets/guesscalc_packs.json",
                                   "assets/guesscalc_packs.h",
                                   "assets/guesscalc_master.json",
                                   "assets/guesscalc_master.h"))
    beta4 = source.json("assets/guesscalc_beta4.json")
    source.text("assets/guesscalc_beta4.h")
    active[5] += beta4["sequence"]
    legacy[7] += beta4["countdown"]
    for gid in (5, 7):
        source.owners[gid].update(("assets/guesscalc_beta4.json",
                                   "assets/guesscalc_beta4.h"))

    for rev in (4, 5):
        path = f"assets/guesscalc_target_beta{rev}"
        data = source.json(path + ".json", 6)["records"]
        header = source.text(path + ".h", 6)
        declared(header, f"gc_target_beta{rev}_index", len(data))
        legacy[6] += data
    target = source.json("assets/guesscalc_target_beta6.json", 6)
    active[6] = target["records"]
    declared(source.text("assets/guesscalc_target_beta6.h", 6),
             "gc_target_beta6", len(active[6]))
    countdown = source.json("assets/guesscalc_countdown_beta6.json", 7)
    active[7] = countdown["records"]
    declared(source.text("assets/guesscalc_countdown_beta6.h", 7),
             "gc_countdown_beta6", len(active[7]))
    prime = source.json("assets/guesscalc_prime_beta6.json", 10)
    active[10] = prime["records"]
    declared(source.text("assets/guesscalc_prime_beta6.h", 10),
             "ng_prime_beta6_targets", len(active[10]))
    source.text("src/games/prime_beta6.c", 10)

    grid_paths = ([f"assets/grids/{gid}.json" for gid in range(11, 21)] +
                  sorted(p.relative_to(source.root).as_posix()
                         for p in (source.root / "assets/grids/master").glob("[0-9][0-9].json")) +
                  sorted(p.relative_to(source.root).as_posix()
                         for p in (source.root / "assets/grids/expanded").glob("[0-9][0-9]-[0-4].json")))
    for path in grid_paths:
        for p in source.json(path):
            active[p["id"]].append(p)
            source.owners[p["id"]].add(path)
    source.text("src/games/grids_pack.c")
    source.text("src/games/grids_pack_data.h")
    for gid in range(11, 21):
        source.owners[gid].update(("src/games/grids_pack.c", "src/games/grids_pack_data.h"))

    sq = source.json("assets/strategyquick/master.json")
    source.text("assets/strategyquick/master_strategy.h")
    source.text("assets/strategyquick/master_quick.h")
    for p in sq["strategy"]:
        active[p["game_id"]].append(p)
    for p in sq["sliding"]:
        active[27].append(p)
    for p in sq["lights"]:
        active[28].append(p)
    for gid in range(21, 26):
        source.owners[gid].update(("assets/strategyquick/master.json",
                                   "assets/strategyquick/master_strategy.h"))
    for gid in (27, 28):
        source.owners[gid].update(("assets/strategyquick/master.json",
                                   "assets/strategyquick/master_quick.h"))
    sliding = source.json("assets/strategyquick/sliding_bands.json", 27)
    source.text("assets/strategyquick/sliding_bands.h", 27)
    active[27] += sliding["records"]
    boards = source.json("assets/boards/puzzles.json")["puzzles"]
    source.text("src/games/boards_pack.c")
    for p in boards:
        active[p["game_id"]].append(p)
    for gid in (31, 32):
        source.owners[gid].update(("assets/boards/puzzles.json",
                                   "src/games/boards_pack.c"))
    crypto = source.json("assets/guesscalc_cryptarithm.json", 34)
    active[34] = crypto["records"]
    source.text("assets/guesscalc_cryptarithm.h", 34)
    source.text("src/games/guesscalc_extra.c", 34)
    source.text("src/games/grids_extra_pack.c")
    for gid in (35, 36):
        for d in range(5 if gid == 35 else 4):
            active[gid] += source.json(f"assets/grids/extra/{gid}-{d}.json", gid)
        source.owners[gid].add("src/games/grids_extra_pack.c")
    return active, legacy


def setting_records(gid, records, difficulty, mode):
    group = [p for p in records if level(p) == difficulty]
    if gid == 2:
        return [p for p in group if p.get("mode", p["puzzle_id"] // 90) == mode]
    if gid in range(21, 26):
        return [p for p in group if p["cpu_first"] == mode]
    if gid in (27, 28):
        return [p for p in group if p["mode"] == mode]
    if gid == 19 and mode == 1:
        return []
    return group


def runtime_domain(gid, difficulty, mode):
    if gid == 1:
        return f"10^{4 + difficulty} repeat-allowed digit secrets; seeded runtime selection"
    if gid == 8 and difficulty < 3:
        return "bounded runtime operator construction; no finite shipped bank"
    if gid in range(21, 26) and difficulty < 3:
        return "bounded rule-state generator; finite domain not exhaustively counted"
    if gid == 19 and mode == 1:
        return "one blank normal-square rule challenge per selected size"
    if gid == 26:
        return "rule-based 2048 state space; no finite puzzle bank"
    if gid == 28 and difficulty < 3:
        return "bounded solvable light patterns; no finite shipped bank"
    if gid == 33:
        return "seeded atom layouts; ray-equivalent layouts may share clues"
    if gid == 37:
        return "standard Reversi board and bounded CPU policy"
    if gid == 38:
        return "seeded spanning-tree network and rotations; domain not exhaustively counted"
    return ""


def setting_names(gid):
    if gid == 6:
        return [str(t) for t in TARGETS] + ["RANDOM"]
    return MODE_NAMES.get(gid, ["STANDARD"])


def runtime_source(gid):
    if 1 <= gid <= 10:
        return "src/games/guesscalc.c" + (
            " + src/games/prime_beta6.c" if gid == 10 else "")
    if 11 <= gid <= 20:
        return "src/games/grids.c"
    if 21 <= gid <= 25:
        return "src/games/strategyquick_strategy.c"
    if 26 <= gid <= 28:
        return "src/games/strategyquick_quick.c"
    if gid in (31, 32):
        return "src/games/boards.c"
    if gid in (33, 34):
        return "src/games/guesscalc_extra.c"
    if gid in (35, 36):
        return "src/games/grids_extra.c"
    return "src/games/strategyquick_extra.c"


def build(source, catalog=None):
    active, legacy = make_records(source)
    by_catalog = {g["id"]: g for g in catalog["games"]} if catalog else {}
    if catalog and list(by_catalog) != IDS:
        raise ValueError("Visible registry IDs/order differ from 36-game inventory")
    summaries, settings = [], []
    for position, gid in enumerate(IDS):
        rows = active[gid]
        keys = [key(gid, p) for p in rows] if rows else []
        unique = len(set(keys))
        if unique != len(rows):
            raise ValueError(f"ID {gid}: repeated public base identity ({unique}/{len(rows)})")
        level_count = 5 if gid in (11, 12, 13, 14, 15, 35) else 4
        actual = by_catalog.get(gid)
        names = setting_names(gid)
        if actual and len(actual["modes"]) != (1 if gid == 6 else len(names)):
            raise ValueError(f"ID {gid}: mode count differs from catalog")
        if actual and actual["difficulties"] != level_count:
            raise ValueError(f"ID {gid}: difficulty count differs from catalog")
        per_level = [sum(level(p) == d for p in rows) for d in range(5)]
        legacy_keys = {key(gid, p) for p in legacy[gid]}
        legacy_only_keys = legacy_keys - set(keys)
        if gid == 6 and per_level != [1000, 1000, 1000, 1000, 0]:
            raise ValueError("MAKE TARGET must have exactly 1,000 records per level")
        if gid == 7 and per_level != [200, 200, 200, 200, 0]:
            raise ValueError("COUNTDOWN must have exactly 200 records per level")
        if gid == 10 and per_level != [128, 256, 256, 256, 0]:
            raise ValueError("PRIME FACTOR bank level counts differ")
        for d in range(level_count):
            for mode, label in enumerate(names):
                if gid == 6:
                    selected = [p for p in rows if level(p) == d] if label == "RANDOM" else [
                        p for p in rows if level(p) == d and p["target"] == int(label)]
                    if len(selected) != (1000 if label == "RANDOM" else 200):
                        raise ValueError(f"MAKE TARGET {label}/{LEVELS[d]} count differs")
                    if actual and actual["bank_counts_by_mode"][0][d] != 1000:
                        raise ValueError("MAKE TARGET native union bank count differs")
                    alias = label == "RANDOM"
                    native_bank = 1000
                else:
                    selected = setting_records(gid, rows, d, mode)
                    alias = False
                    native_bank = len(selected)
                    if actual and actual["bank_counts_by_mode"][mode][d] != native_bank:
                        raise ValueError(f"ID {gid} {label}/{LEVELS[d]} native count differs")
                domain = runtime_domain(gid, d, mode)
                settings.append({
                    "game_id": gid, "name": actual["name"] if actual else NAMES[gid],
                    "category": CATEGORIES[position // 6], "difficulty": LEVELS[d],
                    "setting": label, "reachable_base_records": len(selected),
                    "native_bank_bucket": native_bank,
                    "reuses_other_setting_records": int(alias),
                    "runtime_or_rule_domain": domain,
                })
        summaries.append({
            "game_id": gid, "name": actual["name"] if actual else NAMES[gid],
            "category": CATEGORIES[position // 6],
            "baseline_records": sum(baseline_count(gid)),
            "active_base_records": len(rows), "active_exact_distinct": unique,
            "active_levels_E_N_H_M_HELL": per_level,
            "retained_old_revision_payloads": len(legacy[gid]),
            "legacy_only_distinct_public_bases": len(legacy_only_keys),
            "old_revision_payload_levels_E_N_H_M_HELL":
                [sum(level(p) == d for p in legacy[gid]) for d in range(5)],
            "settings": names, "runtime_or_rule_game": int(not rows),
            "runtime_source": runtime_source(gid),
            "generation_policy": (
                ["RUNTIME", "BANK", "TRANSFORMS", "HYBRID", "RULES"]
                [actual["generation_policy"]] if actual else "SEE_REGISTRY"),
            "source_assets": sorted(source.owners[gid]),
            "external_puzzle_records": 0, "external_puzzle_source": "N/A",
        })
    return summaries, settings


def md_table(headers, rows):
    def clean(value):
        return str(value).replace("|", "\\|").replace("\n", " ")
    return "\n".join(
        ["| " + " | ".join(headers) + " |",
         "| " + " | ".join(["---"] * len(headers)) + " |"] +
        ["| " + " | ".join(clean(v) for v in row) + " |" for row in rows])


def markdown(summaries, settings, manifest, catalog_used):
    vector = lambda values: "/".join(map(str, values))
    active = sum(r["active_base_records"] for r in summaries)
    legacy = sum(r["legacy_only_distinct_public_bases"] for r in summaries)
    retained = sum(r["retained_old_revision_payloads"] for r in summaries)
    baseline = sum(r["baseline_records"] for r in summaries)
    lines = [
        "# Content inventory — 36 visible games",
        "Generated by `tools/content_inventory.py` from source JSON/native declarations. "
        "When a compiled catalog is provided, every mode/level count is checked "
        "against `ng_bank_count`. Family validators, not this inventory, certify "
        "puzzle rules and difficulty. Actual catalog cross-check: "
        f"**{'USED' if catalog_used else 'NOT RUN'}**.",
        f"Frozen 30-game baseline (`{BASELINE}`): **{baseline}** bank records. "
        f"Current reachable base records: **{active}** across 36 visible games. "
        f"Distinct public bases reachable only by an old revision: **{legacy}**. "
        f"Retained old-revision payloads: **{retained}**, including ones whose "
        "public puzzle also appears in a current pool. "
        "These are heterogeneous challenges, layouts, and tactical starts, not "
        "a count of unique-solution puzzles or play-state permutations.",
        "## How to read the counts",
        "A current bank row counts public base identities. Card order, witness "
        "expression, seed, record alias, and solution metadata add no base. "
        "RANDOM in Make Target references the same five fixed-target banks; "
        "its 1,000 reachable records per level are counted once in the 4,000 "
        "active total. A zero bank for 2048, Reversi, or another runtime game "
        "does not mean an absent game. It means there is no finite shipped "
        "puzzle bank. Old-revision payloads remain compiled so unfinished "
        "runs can resume. Their public identities can overlap an active "
        "pool; only the set difference is called legacy-only here.",
        "## Active and retained bases",
        "Level vectors are EASY / NORMAL / HARD / MASTER / HELL. The baseline "
        "is the original 30-game source; six later visible games have zero "
        "baseline records. Exact distinctness uses the public identity shown "
        "above, before deeper family-specific symmetry quotients.",
        md_table(
            ["ID", "Category", "Game", "Baseline", "Current active",
            "Active E/N/H/M/HELL", "Exact distinct", "Legacy-only bases",
            "Retained old payloads"],
            [(r["game_id"], r["category"], r["name"], r["baseline_records"],
              r["active_base_records"], vector(r["active_levels_E_N_H_M_HELL"]),
              r["active_exact_distinct"],
              r["legacy_only_distinct_public_bases"],
              r["retained_old_revision_payloads"])
             for r in summaries]),
        "## Reachable at the selected setting",
        "The complete mode/level/target table is in "
        "[CONTENT_INVENTORY_SETTINGS.csv](CONTENT_INVENTORY_SETTINGS.csv). "
        "An entry's `reachable_base_records` is the current NEW pool for "
        "that setting, not the sum across settings. This is why Make Target "
        "fixed 24 has 200 per difficulty while RANDOM has 1,000. Runtime "
        "domains are described, not assigned invented finite bank counts.",
        md_table(
            ["Game", "Difficulty", "Setting", "Reachable bases", "Runtime/rule domain"],
            [(f'{s["game_id"]:02} {s["name"]}', s["difficulty"], s["setting"],
              s["reachable_base_records"], s["runtime_or_rule_domain"])
             for s in settings]),
        "## Revision boundaries and scope",
        "Make Target revision 6 holds 5 × 4 × 200 public card-multiset/target "
        "bases. Its earlier original bank plus revision-4 and revision-5 "
        "payloads remain for saved runs; a few public identities overlap the "
        "new pool. Countdown revision 5 has 200 bases per level; its original "
        "and revision-4 HARD payloads also remain, including records whose "
        "public puzzles were reused in the new bank. "
        "Prime Factor revision 5 uses 128 EASY and 256 each other level. "
        "Baseball's secret is runtime-generated and its 20/30/40/50 guess "
        "caps are a rule change, not new bank records. Sliding's two sizes "
        "each expose 128 exact-distance E/N/H starts and retain their "
        "2/30 MASTER starts. Magic FREE is a rule challenge with a blank "
        "board; it must not be inflated by the PARTIAL layout bank.",
        "IDs 29/30 remain archived legacy-only games and are not visible, "
        "not counted in the 36 rows and never reused. A single unfinished "
        "run is durable. On NEW, only that run's selected bank cycle is "
        "persisted. RESUME consumes no fresh bank ordinal. Completing a run "
        "clears durable resume, while same-session result NEW keeps the "
        "cycle in RAM. See [CONTENT_CYCLE.md](CONTENT_CYCLE.md) and "
        "[STORAGE_FORMAT.md](STORAGE_FORMAT.md).",
        "All published puzzle data here are locally generated; bundled "
        "external puzzle records: **0**. Provenance and independent "
        "verification are documented in family audits. Hardware flash "
        "latency, RAM peaks, LCD readability and MENU behavior remain "
        "**HARDWARE TEST REQUIRED**.",
        "## Runtime source and published inputs",
        "A rule/runtime row may have no source puzzle asset. `BANK` denotes "
        "an embedded current bank; `HYBRID` mixes one with runtime/rule "
        "starts; `TRANSFORMS` applies rule-valid variants. All 36 linked "
        "games have a native runtime source. External puzzle-data source "
        "is N/A for every row; software/font dependencies have separate "
        "notices in `THIRD_PARTY_NOTICES.md`.",
        md_table(
            ["ID", "Policy", "Native runtime source", "Source/native puzzle inputs"],
            [(r["game_id"], r["generation_policy"], r["runtime_source"],
              "; ".join(r["source_assets"]) or "None: runtime/rules")
             for r in summaries]),
        "## Source manifest",
        md_table(["Path", "Bytes", "SHA256"],
                 [(p, d["bytes"], d["sha256"]) for p, d in sorted(manifest.items())]),
        "## Reproduce",
        "```sh\npython3 tools/content_inventory.py --catalog docs/game-registry.json"
        "\npython3 tools/content_inventory.py --catalog docs/game-registry.json --check"
        "\n```",
        "Without `--catalog`, the tool still derives counts from source "
        "assets but labels the native registry check as unrun. "
        "`--source-root` and `--output-dir` support an isolated source "
        "snapshot. `--check` compares all generated files without writing.",
    ]
    return "\n\n".join(lines) + "\n"


def csv_text(rows):
    out = io.StringIO(newline="")
    writer = csv.DictWriter(out, fieldnames=list(rows[0]), lineterminator="\n")
    writer.writeheader()
    for row in rows:
        writer.writerow({k: enc(v) if isinstance(v, (list, dict)) else v
                         for k, v in row.items()})
    return out.getvalue()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--output-dir", type=Path)
    parser.add_argument("--catalog", type=Path)
    parser.add_argument("--staged-grid-root", type=Path,
                        help="retained for CLI compatibility; staged content is excluded")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    root = args.source_root.resolve()
    source = Source(root)
    catalog = json.loads(args.catalog.read_text()) if args.catalog else None
    summaries, settings = build(source, catalog)
    documents = {
        "CONTENT_INVENTORY.md": markdown(summaries, settings, source.manifest, bool(catalog)),
        "CONTENT_INVENTORY.csv": csv_text(summaries),
        "CONTENT_INVENTORY_SETTINGS.csv": csv_text(settings),
    }
    output = args.output_dir or root / "docs"
    for name, value in documents.items():
        path = output / name
        if args.check:
            if not path.exists() or path.read_text() != value:
                raise SystemExit(f"Inventory differs: {path}")
        else:
            output.mkdir(parents=True, exist_ok=True)
            path.write_text(value)
    print(enc({"visible_games": len(summaries),
               "active_base_records": sum(r["active_base_records"] for r in summaries),
               "legacy_only_distinct_public_bases": sum(
                   r["legacy_only_distinct_public_bases"] for r in summaries),
               "retained_old_revision_payloads": sum(
                   r["retained_old_revision_payloads"] for r in summaries),
               "settings": len(settings), "catalog_checked": bool(catalog)}))


if __name__ == "__main__":
    main()
