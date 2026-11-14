#!/usr/bin/env python3
"""Turn a raw generated animation sheet into a game-ready sprite sheet.

spriteprep.py conforms one image into one sprite. A sheet needs more care: the
generator does not draw on an exact grid (battle sheets come back 1254 pixels
wide, which does not divide by four), poses drift around inside their cells,
and a weapon swing can poke over a grid line into the neighbouring cell.

For each sheet this script:

    finds every separate blob of art -> gives each blob to the cell it mostly
    sits in -> scales the whole sheet by one factor, so the character is the
    same size as their standing sprite -> lines every frame up on the feet ->
    packs the frames into an exact grid

The kind of sheet comes from the file name (see SHEET_KINDS in asset_spec.py):

    solo_walk.png              4x4  rows: down, left, right, up
    street_thug_battle.png     4x4  rows: idle, attack, ability, guard
    fixer_kaga_idle_talk.png   4x2  rows: idle, talk
    vfx_hit_spark.png          4x2  eight frames of one effect (no standing sprite)

The standing sprite (assets/characters/solo.png and so on) must already exist:
it says which folder the sheet belongs in and how tall the character should be.

Usage:
    python3 tools/sheetprep.py Raw/sheets/*.png
    python3 tools/sheetprep.py Raw/sheets/solo_walk.png --outdir /tmp/preview

The source files are only read, never changed.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

try:
    import numpy as np
    from PIL import Image
except ImportError:
    sys.exit("Pillow and numpy are required: pip install Pillow numpy")

sys.path.insert(0, str(Path(__file__).parent))
from asset_spec import (SHEET_KINDS, SPECS_BY_DIR, VFX_COLS, VFX_FRAME, VFX_ROWS,  # noqa: E402
                        sheet_kind_for)

# Categories a sheet can belong to, in the order they are searched.
SHEET_CATEGORIES = ("characters", "enemies", "npcs")

# Blobs smaller than this many pixels are generator noise, not art.
MIN_BLOB_PIXELS = 6

# A blob that has at least this share of its pixels in a second cell is two
# frames touching, not one frame overhanging; it is cut at the grid line.
SPLIT_SHARE = 0.15


def label_blobs(mask: np.ndarray) -> np.ndarray:
    """Number each connected blob of True pixels (0 = background).

    Row-by-row run merging with union-find: plain Python, but it only touches
    runs of pixels rather than single pixels, so a sheet takes well under a
    second without needing scipy.
    """
    h, w = mask.shape
    labels = np.zeros((h, w), dtype=np.int32)
    parent = [0]

    def find(i: int) -> int:
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i

    previous: list[tuple[int, int, int]] = []
    for y in range(h):
        row = mask[y]
        edges = np.flatnonzero(np.diff(np.concatenate(([0], row.view(np.int8), [0]))))
        current: list[tuple[int, int, int]] = []
        for start, end in zip(edges[::2].tolist(), edges[1::2].tolist()):
            label = 0
            for p_start, p_end, p_label in previous:
                # Touching includes diagonals, hence the one-pixel overlap.
                if p_start <= end and p_end >= start:
                    root = find(p_label)
                    if label == 0:
                        label = root
                    elif root != label:
                        parent[root] = label
            if label == 0:
                label = len(parent)
                parent.append(label)
            labels[y, start:end] = label
            current.append((start, end, label))
        previous = current

    roots = np.array([find(i) for i in range(len(parent))], dtype=np.int32)
    return roots[labels]


def assign_to_cells(mask: np.ndarray, cols: int, rows: int) -> np.ndarray:
    """Say which frame every opaque pixel belongs to (-1 = none).

    Starts from the nominal grid, then hands each blob wholesale to the cell
    holding most of it, so art that overhangs a grid line stays with its frame.
    """
    h, w = mask.shape
    col_of = np.minimum((np.arange(w) * cols) // w, cols - 1)
    row_of = np.minimum((np.arange(h) * rows) // h, rows - 1)
    nominal = (row_of[:, None] * cols + col_of[None, :]).astype(np.int32)

    labels = label_blobs(mask)
    owner = np.where(mask, nominal, -1)

    flat_labels = labels[mask]
    flat_cells = nominal[mask]
    counts = np.zeros((int(labels.max()) + 1, cols * rows), dtype=np.int64)
    np.add.at(counts, (flat_labels, flat_cells), 1)

    blob_owner = np.full(counts.shape[0], -2, dtype=np.int32)  # -2: keep nominal
    for blob in range(1, counts.shape[0]):
        total = int(counts[blob].sum())
        if total == 0:
            continue
        if total < MIN_BLOB_PIXELS:
            blob_owner[blob] = -1
            continue
        ranked = np.sort(counts[blob])[::-1]
        if ranked[1] / total < SPLIT_SHARE:
            blob_owner[blob] = int(np.argmax(counts[blob]))
        # else: two frames touching; leave each pixel in its nominal cell.

    per_pixel = blob_owner[labels]
    owner = np.where(mask & (per_pixel != -2), per_pixel, owner)
    return owner


class Frame:
    """One pose, cut out of the source at full resolution."""

    def __init__(self, rgba: np.ndarray, mask: np.ndarray) -> None:
        ys = np.flatnonzero(mask.any(axis=1))
        xs = np.flatnonzero(mask.any(axis=0))
        self.empty = ys.size == 0
        if self.empty:
            return
        y0, y1, x0, x1 = ys[0], ys[-1] + 1, xs[0], xs[-1] + 1
        self.pixels = rgba[y0:y1, x0:x1].copy()
        self.pixels[~mask[y0:y1, x0:x1]] = 0
        self.width = x1 - x0
        self.height = y1 - y0
        # Where the figure stands: the middle of whatever is in the bottom
        # quarter (the legs), so a raised weapon or an outstretched arm does
        # not drag the body sideways between frames.
        legs = mask[y0 + (self.height * 3) // 4:y1, x0:x1]
        leg_xs = np.nonzero(legs)[1]
        self.anchor = float(np.median(leg_xs)) + 0.5 if leg_xs.size else self.width / 2


def scale_frame(frame: Frame, scale: float, alpha_threshold: int) -> Image.Image:
    """Shrink a frame, averaging colour only over the pixels that are art."""
    new_w = max(1, round(frame.width * scale))
    new_h = max(1, round(frame.height * scale))
    data = frame.pixels.astype(np.float32) / 255.0
    alpha = data[..., 3:4]
    premultiplied = np.concatenate([data[..., :3] * alpha, alpha], axis=2)
    # Pillow has no float RGBA mode, so resize the four channels separately.
    channels = [
        np.asarray(Image.fromarray(premultiplied[..., i], mode="F").resize((new_w, new_h), Image.BOX))
        for i in range(4)
    ]
    out_alpha = channels[3]
    safe = np.maximum(out_alpha, 1e-6)
    rgb = np.stack([np.clip(c / safe, 0.0, 1.0) for c in channels[:3]], axis=2)
    # Hard edges: the renderer does not sort translucent sprites.
    opaque = out_alpha * 255.0 >= alpha_threshold
    out = np.zeros((new_h, new_w, 4), dtype=np.uint8)
    out[..., :3] = np.where(opaque[..., None], np.round(rgb * 255.0), 0).astype(np.uint8)
    out[..., 3] = np.where(opaque, 255, 0).astype(np.uint8)
    return Image.fromarray(out, "RGBA")


def find_base_sprite(assets: Path, slug: str) -> tuple[str, Path] | None:
    for category in SHEET_CATEGORIES:
        candidate = assets / category / f"{slug}.png"
        if candidate.is_file():
            return category, candidate
    return None


def round_up(value: float, step: int) -> int:
    return int(-(-value // step) * step)


def process(path: Path, assets: Path, outdir: Path | None, alpha_threshold: int) -> tuple[Path, str]:
    found = sheet_kind_for(path.stem)
    if found is None:
        raise ValueError("name must end in one of: " +
                         ", ".join(f"_{k}" for k in SHEET_KINDS))
    kind_name, kind = found
    slug = path.stem[: -len(kind_name) - 1]

    base = find_base_sprite(assets, slug)
    if base is None:
        raise ValueError(f"no standing sprite named {slug}.png under {assets}; "
                         "conform that first with spriteprep.py")
    category, base_path = base
    with Image.open(base_path) as base_im:
        base_im = base_im.convert("RGBA")
        base_size = base_im.size
        bbox = base_im.getchannel("A").getbbox()
    body_height = (bbox[3] - bbox[1]) if bbox else base_size[1]

    with Image.open(path) as src:
        rgba = np.array(src.convert("RGBA"))
    mask = rgba[..., 3] >= alpha_threshold
    owner = assign_to_cells(mask, kind.cols, kind.rows)

    frames = [Frame(rgba, owner == i) for i in range(kind.cols * kind.rows)]
    empty = [i for i, f in enumerate(frames) if f.empty]
    if empty:
        raise ValueError(f"no art found in frame(s) {empty}")

    # One scale for the whole sheet, taken from the first row (the calmest
    # poses), so the character matches their standing sprite.
    reference = float(np.median([f.height for f in frames[: kind.cols]]))
    scale = body_height / reference

    if kind.fixed_frame:
        # Map sprites: every frame is exactly the standing sprite's size.
        cell_w, cell_h = base_size
        widest = max(2 * max(f.anchor, f.width - f.anchor) for f in frames)
        tallest = max(f.height for f in frames)
        scale = min(scale, cell_w / widest, cell_h / tallest)
    else:
        # Battle sprites: the cell grows to hold the widest swing.
        widest = max(2 * max(f.anchor, f.width - f.anchor) for f in frames) * scale
        tallest = max(f.height for f in frames) * scale
        cell_w = max(base_size[0], round_up(widest + 2, 8))
        cell_h = max(base_size[1], round_up(tallest + 1, 8))
        limit = kind.max_frame
        if cell_w > limit[0] or cell_h > limit[1]:
            raise ValueError(f"frames need {cell_w}x{cell_h}, over the {limit[0]}x{limit[1]} limit")

    sheet = Image.new("RGBA", (cell_w * kind.cols, cell_h * kind.rows), (0, 0, 0, 0))
    for index, frame in enumerate(frames):
        image = scale_frame(frame, scale, alpha_threshold)
        col, row = index % kind.cols, index // kind.cols
        # Feet on the bottom edge, legs centred in the cell.
        left = col * cell_w + cell_w // 2 - round(frame.anchor * scale)
        left = max(col * cell_w, min(left, (col + 1) * cell_w - image.width))
        top = (row + 1) * cell_h - image.height
        sheet.paste(image, (left, max(row * cell_h, top)), image)

    target_dir = outdir if outdir else assets / category
    target_dir.mkdir(parents=True, exist_ok=True)
    out = target_dir / f"{path.stem}.png"
    sheet.save(out, "PNG", optimize=True)
    return out, f"{category}, frames {cell_w}x{cell_h}, scale {scale:.3f}"


def process_vfx(path: Path, assets: Path, outdir: Path | None, alpha_threshold: int) -> tuple[Path, str]:
    """An effect sheet (vfx_*.png): eight frames in a 4x2 grid.

    Unlike a character, an effect's position inside its frame is part of the
    animation (a fireball flies, rain falls), so frames are not re-centred:
    each nominal cell is shrunk as it stands. Soft edges are kept, since glows
    and smoke need them; only the faint generator haze is removed.
    """
    with Image.open(path) as src:
        rgba = np.array(src.convert("RGBA"))
    h, w = rgba.shape[:2]
    mask = rgba[..., 3] >= alpha_threshold
    owner = assign_to_cells(mask, VFX_COLS, VFX_ROWS)

    haze = 24  # alpha below this is generator noise, not part of the effect
    sheet = Image.new("RGBA", (VFX_COLS * VFX_FRAME, VFX_ROWS * VFX_FRAME), (0, 0, 0, 0))
    for index in range(VFX_COLS * VFX_ROWS):
        col, row = index % VFX_COLS, index // VFX_COLS
        x0, x1 = round(w * col / VFX_COLS), round(w * (col + 1) / VFX_COLS)
        y0, y1 = round(h * row / VFX_ROWS), round(h * (row + 1) / VFX_ROWS)
        cell = rgba[y0:y1, x0:x1].astype(np.float32) / 255.0
        # Drop anything that belongs to a neighbouring frame and pokes in.
        foreign = (owner[y0:y1, x0:x1] >= 0) & (owner[y0:y1, x0:x1] != index)
        cell[foreign] = 0.0
        cell[cell[..., 3] * 255.0 < haze] = 0.0
        if not (cell[..., 3] > 0).any():
            raise ValueError(f"no art found in frame {index}")

        alpha = cell[..., 3:4]
        premultiplied = np.concatenate([cell[..., :3] * alpha, alpha], axis=2)
        channels = [
            np.asarray(Image.fromarray(premultiplied[..., i], mode="F").resize(
                (VFX_FRAME, VFX_FRAME), Image.BOX))
            for i in range(4)
        ]
        out_alpha = channels[3]
        safe = np.maximum(out_alpha, 1e-6)
        out = np.zeros((VFX_FRAME, VFX_FRAME, 4), dtype=np.uint8)
        visible = out_alpha * 255.0 >= 8
        for i in range(3):
            out[..., i] = np.where(visible, np.round(np.clip(channels[i] / safe, 0, 1) * 255.0), 0)
        out[..., 3] = np.where(visible, np.round(np.clip(out_alpha, 0, 1) * 255.0), 0)
        sheet.paste(Image.fromarray(out, "RGBA"), (col * VFX_FRAME, row * VFX_FRAME))

    # "vfx_ice_shard-v2" is a revision of "vfx_ice_shard": same game file.
    name = path.stem.split("-v")[0]
    target_dir = outdir if outdir else assets / "vfx"
    target_dir.mkdir(parents=True, exist_ok=True)
    out_path = target_dir / f"{name}.png"
    sheet.save(out_path, "PNG", optimize=True)
    return out_path, f"vfx, {VFX_COLS}x{VFX_ROWS} frames of {VFX_FRAME}x{VFX_FRAME}"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("inputs", nargs="+", type=Path)
    ap.add_argument("--assets", type=Path, default=Path(__file__).parent.parent / "assets",
                    help="The game's assets folder (default: ../assets).")
    ap.add_argument("--outdir", type=Path,
                    help="Write here instead of into the assets folder.")
    ap.add_argument("--alpha-threshold", type=int, default=128,
                    help="Alpha at or above this counts as art (default 128).")
    args = ap.parse_args()

    failures = 0
    for path in args.inputs:
        if path.suffix.lower() != ".png" or not path.is_file():
            continue
        try:
            if path.stem.startswith("vfx_"):
                out, summary = process_vfx(path, args.assets, args.outdir, args.alpha_threshold)
            else:
                out, summary = process(path, args.assets, args.outdir, args.alpha_threshold)
        except Exception as exc:  # noqa: BLE001 - report and keep going
            print(f"  FAIL {path.name}: {exc}")
            failures += 1
            continue
        print(f"  OK   {path.name} -> {out} ({summary}, {out.stat().st_size / 1024:.0f} KB)")

    if failures:
        print(f"\n{failures} sheet(s) failed.")
    else:
        print("\nDone. Verify with: python3 tools/validate_assets.py")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
