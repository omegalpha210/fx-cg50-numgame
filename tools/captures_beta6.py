#!/usr/bin/env python3
"""Convert beta.6 frames emitted by test_beta6_capture's native C renderer."""
from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image, ImageDraw


ROOT = Path(__file__).resolve().parents[1]
GROUPS = {
    "make-target-contact.png": ("make-target-",),
    "countdown-prime-contact.png": ("countdown-", "prime-"),
    "baseball-contact.png": ("baseball-",),
    "scroll-contact.png": ("mind-", "equation-"),
}


def contact_sheet(paths: list[Path], output: Path, columns: int = 3) -> None:
    assert paths
    cell_w, cell_h = 396, 244
    rows = (len(paths) + columns - 1) // columns
    sheet = Image.new("RGB", (12 + columns * (cell_w + 12), 12 + rows * (cell_h + 12)), "#e7ecee")
    pen = ImageDraw.Draw(sheet)
    for index, path in enumerate(paths):
        x = 12 + (index % columns) * (cell_w + 12)
        y = 12 + (index // columns) * (cell_h + 12)
        pen.text((x, y), path.stem, fill="#172a3a")
        with Image.open(path) as frame:
            assert frame.size == (396, 224), path
            sheet.paste(frame.convert("RGB"), (x, y + 20))
    sheet.save(output)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build-host")
    args = parser.parse_args()
    source = args.build_dir / "captures/beta6"
    output = ROOT / "docs/captures/beta6"
    names = (source / "manifest.txt").read_text().splitlines()
    assert names and len(names) == len(set(names))
    assert all(Path(name).name == name and name.endswith(".ppm") for name in names)
    output.mkdir(parents=True, exist_ok=True)
    frames: list[Path] = []
    for name in names:
        path = source / name
        with Image.open(path) as raw:
            assert raw.size == (396, 224), path
            frame = raw.convert("RGB")
            destination = output / f"{path.stem}.png"
            frame.save(destination)
            frames.append(destination)
    for sheet, prefixes in GROUPS.items():
        contact_sheet([p for p in frames if p.stem.startswith(prefixes)], output / sheet)
    links = "\n".join(f"- [{p.stem}](beta6/{p.name})" for p in frames)
    (ROOT / "docs/captures/BETA6_README.md").write_text(
        "# beta.6 renderer review\n\n"
        "Frames are 396×224 output from `tests/test_beta6_capture.c`, which calls the "
        "production C renderer and game modules through the host RGB565 backend. "
        "They are not hardware LCD photographs. The `layout-fixture` frames change "
        "card values only to stress the three-digit text width. Prime min/max "
        "frames select actual validated bank records for width inspection. Other "
        "frames come from entries, games, submissions, and scroll actions. The "
        "harness checks game validity at generation/submission milestones and "
        "rejects draw primitives outside the canvas.\n\n"
        "Reproduce with `cmake -S tests -B build-host`, "
        "`cmake --build build-host --target test_beta6_capture`, "
        "`(cd build-host && ./test_beta6_capture)`, then "
        "`python3 tools/captures_beta6.py`.\n\n"
        "Contact sheets: [Make Target](beta6/make-target-contact.png), "
        "[Countdown and Prime](beta6/countdown-prime-contact.png), "
        "[Baseball](beta6/baseball-contact.png), "
        "[scroll states](beta6/scroll-contact.png).\n\n"
        "Native-resolution review: all six TARGET choices, the 4/5/6-card "
        "three-digit fixtures, four Countdown levels, and Prime targets through "
        "six digits fit their panels. Baseball's 20/30/40/50 attempt labels, "
        "warning and result dialogs, and 50-record top/middle/bottom views fit. "
        "The short Baseball/Equation lists hide the rail; long Baseball, "
        "Number Mind, and Equation Guess lists show it with muted endpoint "
        "arrows. Draft digits remain visible while scrolling. This is renderer "
        "review, not a hardware LCD test.\n\n"
        + links + "\n"
    )
    print(f"{len(frames)} native beta.6 frames converted to {output}")


if __name__ == "__main__":
    main()
