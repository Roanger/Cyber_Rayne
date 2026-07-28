"""Single source of truth for Cyber Rayne asset dimensions and formats.

Both validate_assets.py and spriteprep.py import from here, so the spec cannot
drift between what we produce and what we check.

Why these numbers: Map::VIEWPORT_HEIGHT_TILES is 15, so at 1080p one 32px tile
covers ~72 screen pixels (a 2.25x upscale). Characters are authored 2 tiles tall
so they read clearly against the ground without dwarfing it.
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


# Tiles are deliberately absent: they are generated at runtime by
# TileTextureGen.cpp and never exist as files.
SPECS: list[CategorySpec] = [
    CategorySpec(
        directory="characters",
        size=(48, 64),
        notes="Party members. One file per class, named from Player::classToString() "
              "lowercased: netrunner.png, solo.png, infiltrator.png.",
    ),
    CategorySpec(
        directory="enemies",
        size=(48, 64),
        max_size=(96, 96),
        notes="Bosses may use the full 96x96. Grunts should stay near 48x64.",
    ),
    CategorySpec(
        directory="npcs",
        size=(48, 64),
        notes="Same silhouette budget as party members.",
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
        size=(32, 32),
        max_size=(64, 64),
        notes="Single frames, or horizontal strips whose width is a multiple of height.",
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
        max_size=(640, 360),
        requires_alpha=False,
        requires_transparent_pixels=False,
        notes="Battle and cutscene backdrops. Opaque by design.",
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
