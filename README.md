# Cyber Rayne

A turn-based cyberpunk JRPG in the spirit of Final Fantasy IX, built on a custom
C++20 / Vulkan engine.

The current goal is a vertical slice: menu → exploration across three districts of
Neo-Kyoto → ATB party combat → a three-quest story chain. The roadmap and current
phase live in [AGENTS.md](AGENTS.md).

## What works today

- Exploration across three districts with map transitions
- Visible-enemy and random encounters
- ATB combat with a three-member party, class ability kits, items, and status effects
- Three classes for Rayne: Netrunner, Solo, Infiltrator
- NPC dialogue with choices, and a three-quest story with fixed fights and an ending
- A shared party bag with healing items and equipment, usable in battle and from the pause menu
- Save anywhere from the pause menu; saves live in `~/.local/share/CyberRayne/`
- Rebindable keys (Settings), stored in `~/.config/CyberRayne/input.cfg`
- A battle screen with animated character and enemy sprites, damage numbers and status tags
- Walking animation on the map, and NPCs that idle and gesture when spoken to
- Procedurally generated pixel-art tiles

Still to come: audio and menu art.

## Building (Linux)

Requires CMake, Ninja, a C++20 compiler, the Vulkan headers and loader, GLFW, and
`glslang`. On Arch: `cmake ninja vulkan-headers vulkan-icd-loader glfw glslang`.

```bash
cmake -S CyberRayne --preset linux-debug
cmake --build CyberRayne/out/build/linux-debug
ctest --test-dir CyberRayne/out/build/linux-debug --output-on-failure
```

Run it from the build directory so it finds its assets and shaders:

```bash
cd CyberRayne/out/build/linux-debug
./CyberRayne
```

`CYBERRAYNE_START=world ./CyberRayne` skips the menus and drops straight into
exploration. `CYBERRAYNE_START=load` resumes the saved game, and
`CYBERRAYNE_START=battle` starts inside a test fight.

Windows presets (`x64-debug`, `x64-release`) exist in `CMakePresets.json` and need the
Vulkan SDK.

## Controls

Defaults; all can be changed under Settings.

| Key | Controller | Action |
|---|---|---|
| Arrow keys (hold to keep walking) | Left stick or D-pad | Move / navigate menus |
| Enter | Cross / A | Confirm / talk to a nearby NPC |
| Esc | Circle / B | Cancel |
| M or P | Options / Start, or Triangle / Y | Pause menu (party, items, equipment, quests, save, settings) |

Controllers work on Linux through GLFW, which knows PlayStation, Xbox and Steam Input
pads. Controller buttons are fixed; only keys can be rebound. For a pad GLFW does not
recognise, put an SDL `gamecontrollerdb.txt` in `~/.config/CyberRayne/`.

## Layout

- `CyberRayne/src`, `CyberRayne/include` — engine and game code
  (`core`, `entities`, `systems`, `graphics`, `ui`, `tests`)
- `CyberRayne/shaders` — GLSL sources, compiled to SPIR-V at build time
- `CyberRayne/assets` — sprites, UI, fonts
- `CyberRayne/tools` — asset pipeline (`spriteprep.py`, `sheetprep.py`, `validate_assets.py`)
- [ASSET_SPEC.md](ASSET_SPEC.md) — what assets must look like
- [ASSET_LIST.md](ASSET_LIST.md) — which assets exist and their status
