# FF9-style JRPG

A turn-based JRPG inspired by Final Fantasy 9, built with C++ and Vulkan.

## Features
- Turn-based battle system
- Pixel-art graphics style (similar to Stardew Valley)
- Three playable characters (Mage, Warrior, Rogue)
- Multiple maps and biomes
- Character-specific starter stories
- Vulkan-based rendering system

## Development Setup
1. Install Vulkan SDK
2. Configure CMake
3. Build project

## Build Instructions
```bash
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build the project
cmake --build .

# Run the game
./FF9StyleJRPG
```

## Project Structure
- `src/` - Source code
  - `core/` - Core game systems (Game, GameState, Player, World, Map, etc.)
  - `graphics/` - Vulkan rendering system
  - `entities/` - Game entities (Enemy, NPC, etc.)
  - `systems/` - Game systems (BattleSystem, etc.)
- `assets/` - Game assets (graphics, audio, etc.)
- `include/` - Header files
- `libs/` - Third-party libraries
- `shaders/` - Vulkan shader files
