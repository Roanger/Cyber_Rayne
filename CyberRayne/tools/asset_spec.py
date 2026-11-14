"""Single source of truth for Cyber Rayne asset dimensions and formats.

Both validate_assets.py and spriteprep.py import from here, so the spec cannot
drift between what we produce and what we check.

Why these numbers: Map::VIEWPORT_HEIGHT_TILES is 15, so at 1080p one tile covers
72 screen pixels. Characters are 96x128: drawn 1:1 on the map (a little wider
than a tile, under two tiles tall) and at exactly 2x in battle, so the pixels
stay square in both places.

Characters were 48x64 until 2026-11-10. The generated slice art is too detailed
for that: shrunk to 48x64 and snapped to the 16-colour palette it lost faces,
hair colour and skin tone. Character art is therefore conformed with
--no-palette; the palette still applies to tiles and UI.
"""

from dataclasses import dataclass, field


@dataclass(frozen=True)
class CategorySpec:
    """Rules for every asset under a given assets/ subdirectory."""

    directory: str
    # (width, height). If max_size is set, size is the minimum and any dimension
    # between the two is accepted -- used where art needs room to vary.
    size: tuple[int, int]
    max_size: tuple[int, int] | None = None
    # Sprites must cut out cleanly; backgrounds legitimately fill their frame.
    requires_alpha: bool = True
    # Reject art that is fully opaque when it should have been cut out. A sprite
    # with zero transparent pixels is almost always an un-removed background.
    requires_transparent_pixels: bool = True
    notes: str = ""
    exempt: frozenset[str] = field(default_factory=frozenset)

    def accepts(self, w: int, h: int) -> bool:
        if self.max_size is None:
            return (w, h) == self.size
        return (self.size[0] <= w <= self.max_size[0]
                and self.size[1] <= h <= self.max_size[1])

    def describe(self) -> str:
        if self.max_size is None:
            return f"{self.size[0]}x{self.size[1]}"
        return (f"{self.size[0]}x{self.size[1]} to "
                f"{self.max_size[0]}x{self.max_size[1]}")


# Effect sheets: eight frames in a 4x2 grid.
VFX_COLS = 4
VFX_ROWS = 2
VFX_FRAME = 64

# Tiles are deliberately absent: they are generated at runtime by
# TileTextureGen.cpp and never exist as files.
SPECS: list[CategorySpec] = [
    CategorySpec(
        directory="characters",
        size=(96, 128),
        notes="Party members. Rayne has one file per class, named from "
              "Player::classToString() lowercased: netrunner.png, solo.png, "
              "infiltrator.png. Companions use their own name: cyph.png, vash.png.",
    ),
    CategorySpec(
        directory="enemies",
        size=(96, 96),
        max_size=(192, 192),
        notes="Named from the enemy's display name: 'Street Thug' -> street_thug.png. "
              "Human-sized enemies are 96x128; bosses may go up to 192x192.",
    ),
    CategorySpec(
        directory="npcs",
        size=(96, 128),
        notes="Same size as party members. Named from the NPC's display name: "
              "'Fixer Kaga' -> fixer_kaga.png.",
    ),
    CategorySpec(
        directory="items",
        size=(32, 32),
        notes="Inventory icons, drawn at 1:1 or 2x in menus.",
    ),
    CategorySpec(
        directory="spells",
        size=(32, 32),
        notes="Ability icons in the battle command menu.",
    ),
    CategorySpec(
        directory="equipment",
        size=(32, 32),
        notes="Weapon/armor/accessory icons.",
    ),
    CategorySpec(
        directory="vfx",
        size=(VFX_COLS * VFX_FRAME, VFX_ROWS * VFX_FRAME),
        notes="Effect animations: a 4x2 sheet of eight 64x64 frames, read left to "
              "right, top row first. Soft edges are allowed (effects are drawn "
              "last, over everything). Built by sheetprep.py from files named vfx_*.",
    ),
    CategorySpec(
        directory="ui",
        size=(8, 8),
        max_size=(640, 360),
        notes="Frames, buttons, cursors. 640x360 is the pixel-art canvas that "
              "integer-scales 3x to 1920x1080.",
    ),
    CategorySpec(
        directory="backgrounds",
        size=(320, 180),
        max_size=(960, 540),
        requires_alpha=False,
        requires_transparent_pixels=False,
        notes="Battle and cutscene backdrops. Opaque by design. 960x540 doubles "
              "to 1920x1080, the same 2x the battle sprites are drawn at, so "
              "backdrop and sprite pixels are the same size. Battle backdrops are "
              "named battle_background_<district>.png with the ground line about "
              "46% down the image.",
    ),
    CategorySpec(
        directory="fonts",
        size=(8, 8),
        max_size=(1024, 1024),
        requires_alpha=False,
        requires_transparent_pixels=False,
        notes="Bitmap glyph atlases. Grayscale is fine; the renderer expands it.",
    ),
]

SPECS_BY_DIR = {s.directory: s for s in SPECS}


@dataclass(frozen=True)
class SheetKind:
    """An animation sheet: a grid of equal-size frames, one animation per row.

    A sheet sits beside the standing sprite it belongs to and is named
    <standing sprite name>_<kind>.png. tools/sheetprep.py builds them.
    """

    cols: int
    rows: int
    row_names: tuple[str, ...]
    # True: every frame is exactly the standing sprite's size (map sprites).
    # False: frames may be larger, up to max_frame, to hold a weapon swing.
    fixed_frame: bool
    max_frame: tuple[int, int] = (0, 0)
    notes: str = ""


# Row order is what the game reads; changing it here means changing
# World.cpp / BattleScene.cpp too.
SHEET_KINDS: dict[str, SheetKind] = {
    "walk": SheetKind(
        cols=4, rows=4, row_names=("down", "left", "right", "up"), fixed_frame=True,
        notes="Party members on the map. Frame 1 of each row is the standing pose."),
    "battle": SheetKind(
        cols=4, rows=4, row_names=("idle", "attack", "ability", "guard"),
        fixed_frame=False, max_frame=(320, 256),
        notes="Party members face screen-left, enemies face screen-right. "
              "Feet on the bottom edge, legs centred in the frame."),
    "idle_talk": SheetKind(
        cols=4, rows=2, row_names=("idle", "talk"), fixed_frame=True,
        notes="NPCs on the map."),
}


def sheet_kind_for(stem: str) -> tuple[str, SheetKind] | None:
    """The sheet kind a file name (without .png) ends in, if any."""
    # Longest suffix first, so a kind whose name ends in another's still wins.
    for name in sorted(SHEET_KINDS, key=len, reverse=True):
        if stem.endswith("_" + name):
            return name, SHEET_KINDS[name]
    return None

# Palette shared by the procedural tiles in TileTextureGen.cpp. spriteprep.py
# snaps generated art toward these so sprites sit in the same world as the
# ground they stand on.
CYBERPUNK_PALETTE: list[tuple[int, int, int]] = [
    (0x0A, 0x0C, 0x10),  # near-black
    (0x16, 0x18, 0x1C),  # asphalt
    (0x2E, 0x32, 0x3A),  # concrete
    (0x4A, 0x50, 0x58),  # steel
    (0x6E, 0x76, 0x82),  # light steel
    (0x9E, 0xA8, 0xB4),  # highlight gray
    (0xE8, 0xEE, 0xF4),  # white
    (0xFF, 0x2B, 0xD6),  # neon magenta
    (0xC8, 0x1E, 0x8C),  # deep magenta
    (0x2A, 0xE0, 0xFF),  # hologram cyan
    (0x14, 0x8C, 0xB4),  # deep cyan
    (0xE0, 0x6E, 0x14),  # vending orange
    (0xC8, 0xA0, 0x2A),  # road paint yellow
    (0x30, 0xC0, 0x70),  # terminal green
    (0xD0, 0x40, 0x50),  # alert red
    (0x6A, 0x3C, 0xC8),  # synth purple
]
