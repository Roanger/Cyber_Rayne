# Cyber Rayne — Asset Specification

How assets are produced, what shape they must be, and what enforces it.
`ASSET_LIST.md` tracks *which* assets exist; this document defines *what they must be*.

Art direction is **pixel art**. Tiles are generated in code; sprites are authored
or generated as images and conformed by tooling.

---

## 1. Why these dimensions

`Map::VIEWPORT_HEIGHT_TILES` is 15, so at 1080p one tile covers ~72 screen pixels.
A 32px tile therefore upscales ~2.25x. Characters are authored two tiles tall so
they read clearly against the ground without dwarfing it.

| Category | Directory | Size | Alpha | Source |
|---|---|---|---|---|
| Tiles | *(none — generated)* | 32x32 | opaque, except hologram | `TileTextureGen.cpp` |
| Party members | `assets/characters/` | 96x128 | required, hard cutout | authored / generated |
| Enemies | `assets/enemies/` | 96x96 – 192x192 | required, hard cutout | authored / generated |
| NPCs | `assets/npcs/` | 96x128 | required, hard cutout | authored / generated |
| Item icons | `assets/items/` | 32x32 | required | authored |
| Spell icons | `assets/spells/` | 32x32 | required | authored |
| Equipment icons | `assets/equipment/` | 32x32 | required | authored |
| VFX | `assets/vfx/` | 32x32 – 64x64 | required | authored |
| UI | `assets/ui/` | up to 640x360 | required | authored |
| Backgrounds | `assets/backgrounds/` | 320x180 – 640x360 | not required | authored / generated |
| Fonts | `assets/fonts/` | atlas | not required | authored |

640x360 is the pixel-art canvas that integer-scales exactly 3x to 1920x1080.

The machine-readable version of this table lives in `CyberRayne/tools/asset_spec.py`.
Change it there, not here, and update this table to match.

## 2. Hard format rules

These are not style preferences — each one corresponds to a defect that reached
the repo during the first art pass:

1. **Real PNG.** Not a JPEG with a `.png` extension. JPEG cannot store alpha, and
   its ringing artifacts destroy sprite edges. `stb_image` sniffs content, so a
   mislabelled file loads silently and looks wrong only in game.
2. **Sprites carry a real cutout.** RGBA with genuinely transparent pixels. The
   pipeline blends `SRC_ALPHA / ONE_MINUS_SRC_ALPHA` (`VulkanRenderer.cpp:2036`);
   an RGB image is expanded to alpha=255 everywhere and draws as an opaque rectangle.
3. **Alpha is binary.** 0 or 255, never partial. Sprites are not depth-sorted, so
   translucent edges produce draw-order-dependent halos.
4. **Sized to spec.** The renderer has `mipLevels = 1`. A 2048px source scaled to
   ~72 screen pixels with no mip chain shimmers badly in motion.

## 3. Sampler settings

`VulkanRenderer::createTextureSampler()` uses:

- `VK_FILTER_NEAREST` — keeps texels crisp when upscaled. `LINEAR` smears a 32px
  tile across ~72 screen pixels.
- `VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE` — sprite quads sample the full [0,1] UV
  range, and `REPEAT` bleeds the opposite edge in along the borders. Tile
  seamlessness comes from the generated pattern, not from the sampler.

## 4. Tiles are code, not files

`Map::loadTileTextures()` calls `TileTextureGen::generate()` for all 15 tile types
and uploads via `loadTextureFromMemory()`. Nothing is read from disk.

This is deliberate:

- **Seamless by construction.** Every pattern is built on a wrapped lattice, so
  edges match when the tile repeats. No manual edge-matching in an art tool.
- **~4 KB each instead of megabytes.** All 15 tiles cost less than one of the
  original 1024x1024 JPEGs.
- **Deterministic.** Hash-based noise, no `<random>`, so CI and a player's machine
  produce identical output.
- **Tunable in one place.** Retint the whole city by editing constants in
  `TileTextureGen.cpp`.

To add a tile type: add the enum case in `Tile.h`, a `make*()` function in
`TileTextureGen.cpp`, and an entry in the `kTileTypes` array in `Map.cpp`.

## 5. Producing a sprite

`tools/spriteprep.py` conforms any source image to the spec:

```
remove background -> trim to subject -> scale -> harden alpha -> snap palette -> RGBA PNG
```

```bash
# Party members
python3 tools/spriteprep.py raw/*.png --category characters --outdir assets/characters

# A boss at full size
python3 tools/spriteprep.py Raw/corporate_enforcer.png --category enemies --size 144x192 --no-palette

# Busy background that flood-fill cannot handle
python3 tools/spriteprep.py raw/npc.png --category npcs --rembg

# Art already on-palette
python3 tools/spriteprep.py raw/icon.png --category items --no-palette
```

Background removal defaults to corner flood-fill, which handles the flat backdrops
image generators produce. It floods inward from the border rather than keying a
colour globally, so an interior region matching the backdrop — a grey jacket
against a grey wall — survives. A plinth or shadow *enclosed* by the silhouette
cannot be reached and must be cleaned up by hand or removed at the source.

Sprites are bottom-aligned on their canvas, not centred: characters stand on a
tile, so feet must land at a predictable offset.

### Palette

`snap_to_palette()` maps every opaque pixel to the nearest of 16 colours shared
with the procedural tiles (defined in `tools/asset_spec.py`). This is what makes
independently produced sprites look like one game rather than sixteen separate
commissions. Skip it with `--no-palette` when the source is already on-palette.

**Characters, enemies and NPCs are always conformed with `--no-palette`** (decided
2026-11-10). The 16 colours have no skin tones or hair colours, and snapping the
generated character art to them turned faces and hair grey. The palette still
applies to tiles and UI.

## 6. Validation

```bash
python3 tools/validate_assets.py
```

Exits non-zero on any violation and names the fix. Wire it into CI ahead of the
build — a mislabelled JPEG is far cheaper to catch here than to debug as a black
rectangle in game.

## 7. Naming

Player sprites are loaded from the class enum, lowercased
(`CharacterSelectionSystem.cpp:56`):

```
assets/characters/netrunner.png
assets/characters/solo.png
assets/characters/infiltrator.png
```

These names are **derived from code**, not chosen freely — `Player::classToString()`
in `Player.h` is the authority. Renaming a class renames its asset.

Because the caster class is already `netrunner.png`, the hacker enemy is
`assets/enemies/enemy_netrunner.png` to avoid a collision.

**Open decision:** companions Cyph and Vash currently collide with the player's
class sprite under class-derived naming. Either namespace them (`npc_cyph.png`) or
accept repeated art in a three-person party.

## 8. What not to author

Some rows in `ASSET_LIST.md` are marked `[CUT]` because they are shader or
geometry work, not art:

- HP/MP/ATB/XP bar fills — tinted quads
- Minimap dots — coloured quads
- Hurt flashes and defeat fades — a tint uniform and a dissolve
- Fog and vignette — fullscreen shader passes

Producing these as textures costs art time and gives up runtime control over
colour and animation.

## Animation sheets

A character, enemy or NPC can have animation sheets beside its standing sprite. They
are optional: without one the game draws the standing sprite.

| File | Grid | Rows, top to bottom | Frame size |
|---|---|---|---|
| `characters/<name>_walk.png` | 4 x 4 | down, left, right, up | same as the standing sprite (96x128) |
| `characters/<name>_battle.png`, `enemies/<name>_battle.png` | 4 x 4 | idle, attack, ability, guard | standing size up to 320x256 |
| `npcs/<name>_idle_talk.png` | 4 x 2 | idle, talk | same as the standing sprite (96x128) |

- In a walk row, frame 2 is the standing pose; the others are the stride.
- In battle sheets the party faces screen-left and enemies face screen-right. Frames are
  wider than the standing sprite to hold a weapon swing; the figure's feet are on the
  bottom edge and their legs are centred, so the game can pin every frame to the same spot.
- The rules live in `SHEET_KINDS` in `tools/asset_spec.py`, and the validator checks the
  grid, the frame size, and that no frame is empty.

`tools/sheetprep.py` builds a sheet from a raw generated one. It needs the standing
sprite to exist already (that sets the folder and the character's height):

```bash
python3 tools/sheetprep.py Raw/sheets/*.png
```

Generated sheets are not on an exact grid, so it finds each pose by its outline rather
than by cutting on grid lines, scales the whole sheet by one factor, and lines the
frames up on the feet. Do not run `spriteprep.py` on a sheet.
