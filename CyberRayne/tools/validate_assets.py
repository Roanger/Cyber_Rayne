#!/usr/bin/env python3
"""Fail the build when an asset does not match ASSET_SPEC.md.

This exists because the first pass of art shipped 2048x2048 JPEGs renamed to
.png with no alpha channel. The renderer blends SRC_ALPHA/ONE_MINUS_SRC_ALPHA,
so those sprites drew as opaque squares. Every check below corresponds to a
defect that actually reached the repo.

Usage:
    python3 tools/validate_assets.py [assets_dir]

Exit code 0 if every asset conforms, 1 otherwise.
"""

from __future__ import annotations

import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    sys.exit("Pillow is required: pip install Pillow")

sys.path.insert(0, str(Path(__file__).parent))
from asset_spec import SPECS_BY_DIR  # noqa: E402

# PNG and JPEG magic numbers. We check bytes, not the file extension, because
# the extension is exactly what lied last time.
PNG_MAGIC = b"\x89PNG\r\n\x1a\n"
JPEG_MAGIC = b"\xff\xd8\xff"


class Problem:
    def __init__(self, path: Path, message: str, fix: str = "") -> None:
        self.path = path
        self.message = message
        self.fix = fix


def check_file(path: Path, spec, assets_root: Path) -> list[Problem]:
    problems: list[Problem] = []
    rel = path.relative_to(assets_root)

    if path.name in spec.exempt:
        return problems

    header = path.read_bytes()[:8]
    if header.startswith(JPEG_MAGIC):
        problems.append(Problem(
            rel, "JPEG data in a .png file",
            "Re-encode as real PNG. JPEG cannot store alpha and its ringing "
            "artifacts wreck sprite edges."))
        return problems  # everything downstream would be misleading

    if not header.startswith(PNG_MAGIC):
        problems.append(Problem(rel, "not a PNG file", "Re-encode as PNG."))
        return problems

    with Image.open(path) as im:
        w, h = im.size
        mode = im.mode
        has_alpha = mode in ("RGBA", "LA") or "transparency" in im.info
        alpha_extrema = im.getchannel("A").getextrema() if mode == "RGBA" else None

    if not spec.accepts(w, h):
        problems.append(Problem(
            rel, f"{w}x{h} but spec is {spec.describe()}",
            f"Run: python3 tools/spriteprep.py '{path}' --category {spec.directory}"))

    if spec.requires_alpha and not has_alpha:
        problems.append(Problem(
            rel, f"mode {mode} has no alpha channel",
            "The sprite will render as an opaque rectangle. Run spriteprep.py "
            "to cut out the background."))

    # An RGBA image whose alpha is 255 everywhere has a channel but no cutout --
    # the background is still baked in. This is the failure that looks fine in
    # an image viewer and wrong in game.
    if spec.requires_transparent_pixels and alpha_extrema is not None:
        lo, _hi = alpha_extrema
        if lo == 255:
            problems.append(Problem(
                rel, "alpha channel present but fully opaque",
                "Background was never removed. Run spriteprep.py."))

    return problems


def main() -> int:
    root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).parent.parent / "assets"
    root = root.resolve()
    if not root.is_dir():
        sys.exit(f"assets directory not found: {root}")

    problems: list[Problem] = []
    checked = 0
    unknown_dirs: set[str] = set()

    for path in sorted(root.rglob("*")):
        if not path.is_file() or path.suffix.lower() not in (".png", ".jpg", ".jpeg"):
            continue
        top = path.relative_to(root).parts[0]
        spec = SPECS_BY_DIR.get(top)
        if spec is None:
            unknown_dirs.add(top if path.parent != root else "<assets root>")
            continue
        checked += 1
        problems.extend(check_file(path, spec, root))

    for name in sorted(unknown_dirs):
        print(f"note: skipping {name} -- no spec defined in tools/asset_spec.py")
    if unknown_dirs:
        print()

    if not problems:
        print(f"OK: {checked} asset(s) conform to spec.")
        return 0

    by_path: dict[Path, list[Problem]] = {}
    for p in problems:
        by_path.setdefault(p.path, []).append(p)

    print(f"FAIL: {len(problems)} problem(s) across {len(by_path)} file(s) "
          f"({checked} checked)\n")
    for path, items in sorted(by_path.items()):
        print(f"  {path}")
        for it in items:
            print(f"      - {it.message}")
            if it.fix:
                print(f"        fix: {it.fix}")
        print()
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
