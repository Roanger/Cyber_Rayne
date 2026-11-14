#!/usr/bin/env python3
"""Turn a raw source image into a spec-conformant Cyber Rayne sprite.

Image generators hand back a big opaque JPEG-ish image with the subject sitting
on a flat or gradient backdrop. The renderer needs a small RGBA PNG with a real
cutout. This script is the bridge:

    remove background -> trim to subject -> scale to spec -> snap palette -> RGBA PNG

Usage:
    python3 tools/spriteprep.py input.png --category characters
    python3 tools/spriteprep.py raw/*.png --category enemies --outdir assets/enemies
    python3 tools/spriteprep.py input.png --category items --no-palette

Background removal uses corner flood-fill by default, which handles the flat
backdrops generators produce. Pass --rembg to use the `rembg` package instead
for busy backgrounds (pip install rembg).
"""

from __future__ import annotations

import argparse
import sys
from collections import deque
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    sys.exit("Pillow is required: pip install Pillow")

sys.path.insert(0, str(Path(__file__).parent))
from asset_spec import CYBERPUNK_PALETTE, SPECS_BY_DIR  # noqa: E402


def remove_background_floodfill(im: Image.Image, tolerance: int) -> Image.Image:
    """Clear pixels reachable from the border that match the border colour.

    Flood-filling from the edges rather than keying one colour globally means an
    interior region that happens to match the backdrop -- a character's grey
    jacket against a grey wall -- survives instead of being punched out.
    """
    im = im.convert("RGBA")
    w, h = im.size
    px = im.load()

    # Seed from the four corners; use their average as the reference colour.
    corners = [(0, 0), (w - 1, 0), (0, h - 1), (w - 1, h - 1)]
    samples = [px[c][:3] for c in corners]
    ref = tuple(sum(s[i] for s in samples) // len(samples) for i in range(3))

    def matches(rgb: tuple[int, int, int]) -> bool:
        return sum(abs(rgb[i] - ref[i]) for i in range(3)) <= tolerance * 3

    visited = bytearray(w * h)
    queue: deque[tuple[int, int]] = deque()
    for x in range(w):
        for y in (0, h - 1):
            queue.append((x, y))
    for y in range(h):
        for x in (0, w - 1):
            queue.append((x, y))

    while queue:
        x, y = queue.popleft()
        if x < 0 or y < 0 or x >= w or y >= h:
            continue
        idx = y * w + x
        if visited[idx]:
            continue
        visited[idx] = 1
        r, g, b, _ = px[x, y]
        if not matches((r, g, b)):
            continue
        px[x, y] = (r, g, b, 0)
        queue.extend(((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)))

    return im


def remove_background_rembg(im: Image.Image) -> Image.Image:
    try:
        from rembg import remove
    except ImportError:
        sys.exit("--rembg needs the rembg package: pip install rembg")
    return remove(im.convert("RGBA"))


def trim_to_content(im: Image.Image) -> Image.Image:
    """Crop to the opaque subject so scaling spends every pixel on the art."""
    bbox = im.getchannel("A").getbbox()
    return im.crop(bbox) if bbox else im


def fit_into(im: Image.Image, target: tuple[int, int]) -> Image.Image:
    """Scale preserving aspect ratio, then centre on a transparent canvas.

    Bottom-aligned rather than centred vertically: characters and enemies are
    drawn standing on a tile, so their feet must land at a predictable offset.
    """
    tw, th = target
    sw, sh = im.size
    scale = min(tw / sw, th / sh)
    new = (max(1, round(sw * scale)), max(1, round(sh * scale)))

    # BOX averages source pixels when downscaling, which keeps detail readable;
    # NEAREST at these ratios drops whole features.
    im = im.resize(new, Image.BOX)

    canvas = Image.new("RGBA", target, (0, 0, 0, 0))
    canvas.paste(im, ((tw - new[0]) // 2, th - new[1]))
    return canvas


def fill_frame(im: Image.Image, target: tuple[int, int]) -> Image.Image:
    """Scale to cover the whole frame and crop the overhang, centred.

    For backgrounds: fit_into would leave a transparent sliver when the source
    is not exactly the target's shape, and a backdrop must have no holes.
    """
    tw, th = target
    sw, sh = im.size
    scale = max(tw / sw, th / sh)
    new = (max(tw, round(sw * scale)), max(th, round(sh * scale)))
    im = im.resize(new, Image.BOX)
    left = (new[0] - tw) // 2
    top = (new[1] - th) // 2
    return im.crop((left, top, left + tw, top + th))


def harden_alpha(im: Image.Image, threshold: int) -> Image.Image:
    """Force alpha to fully on or fully off.

    Pixel art wants hard edges, and the renderer does not sort translucent
    sprites, so partial alpha on a sprite edge produces order-dependent halos.
    """
    alpha = im.getchannel("A").point(lambda a: 255 if a >= threshold else 0)
    im.putalpha(alpha)

    # Zero the colour of fully transparent pixels. They are invisible either way,
    # but leaving the original RGB behind gives PNG a noisy channel to compress
    # and roughly doubles file size.
    cleared = Image.new("RGBA", im.size, (0, 0, 0, 0))
    cleared.paste(im, (0, 0), alpha)
    cleared.putalpha(alpha)
    return cleared


def snap_to_palette(im: Image.Image) -> Image.Image:
    """Map each opaque pixel to the nearest palette entry.

    This is what makes independently generated sprites look like one game
    rather than sixteen separate commissions.
    """
    px = im.load()
    w, h = im.size
    cache: dict[tuple[int, int, int], tuple[int, int, int]] = {}

    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if a == 0:
                continue
            key = (r, g, b)
            best = cache.get(key)
            if best is None:
                # Weighted euclidean distance; green dominates perceived luminance.
                best = min(
                    CYBERPUNK_PALETTE,
                    key=lambda c: 2 * (c[0] - r) ** 2 + 4 * (c[1] - g) ** 2 + 3 * (c[2] - b) ** 2,
                )
                cache[key] = best
            px[x, y] = (*best, a)
    return im


def process(path: Path, spec, args) -> Path:
    with Image.open(path) as src:
        im = src.convert("RGBA")

    if args.rembg:
        im = remove_background_rembg(im)
    elif not args.keep_background:
        im = remove_background_floodfill(im, args.tolerance)

    if not args.keep_background:
        im = trim_to_content(im)

    target = args.size or spec.size
    if args.fill:
        im = fill_frame(im.convert("RGB"), target)
    else:
        im = fit_into(im, target)

    if not args.keep_background:
        im = harden_alpha(im, args.alpha_threshold)
    if not args.no_palette and not args.fill:
        im = snap_to_palette(im)

    outdir = Path(args.outdir) if args.outdir else path.parent
    outdir.mkdir(parents=True, exist_ok=True)
    out = outdir / f"{path.stem}.png"
    im.save(out, "PNG", optimize=True)
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("inputs", nargs="+", type=Path)
    ap.add_argument("--category", required=True, choices=sorted(SPECS_BY_DIR),
                    help="Which spec to conform to.")
    ap.add_argument("--outdir", help="Defaults to alongside each input.")
    ap.add_argument("--size", type=lambda s: tuple(int(v) for v in s.split("x")),
                    metavar="WxH", help="Override the category's default size.")
    ap.add_argument("--rembg", action="store_true",
                    help="Use the rembg model instead of corner flood-fill.")
    ap.add_argument("--keep-background", action="store_true",
                    help="Skip cutout entirely. For backgrounds and UI panels.")
    ap.add_argument("--fill", action="store_true",
                    help="Cover the whole frame, cropping any overhang, and write "
                         "an opaque image. For backgrounds; implies no cutout.")
    ap.add_argument("--no-palette", action="store_true",
                    help="Skip palette snapping. Use when source art is already on-palette.")
    ap.add_argument("--tolerance", type=int, default=32,
                    help="Flood-fill colour tolerance per channel (default 32).")
    ap.add_argument("--alpha-threshold", type=int, default=128,
                    help="Alpha at or above this becomes opaque (default 128).")
    args = ap.parse_args()
    if args.fill:
        args.keep_background = True

    spec = SPECS_BY_DIR[args.category]
    target = args.size or spec.size
    print(f"category={args.category} target={target[0]}x{target[1]}")

    failures = 0
    for path in args.inputs:
        if not path.is_file():
            print(f"  SKIP {path} (not a file)")
            failures += 1
            continue
        try:
            out = process(path, spec, args)
        except Exception as exc:  # noqa: BLE001 - report and keep going
            print(f"  FAIL {path.name}: {exc}")
            failures += 1
            continue
        kb = out.stat().st_size / 1024
        print(f"  OK   {path.name} -> {out} ({kb:.1f} KB)")

    if failures:
        print(f"\n{failures} input(s) failed.")
    else:
        print(f"\nDone. Verify with: python3 tools/validate_assets.py")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
