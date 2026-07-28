#pragma once

#include "Tile.h"

#include <cstdint>
#include <vector>

// Procedural tile texture generation.
//
// Tiles are synthesized at startup rather than loaded from disk. Every pattern
// is built on a wrapped lattice, so the result is seamless when repeated across
// the map, and is fully deterministic for a given (type, size) pair.
//
// Output is tightly packed RGBA8, ready to hand to
// VulkanRenderer::loadTextureFromMemory(data, size, size, 4).
namespace TileTextureGen {

// Authoring resolution for one tile. The viewport shows 15 tiles vertically
// (Map::VIEWPORT_HEIGHT_TILES), so at 1080p a tile covers ~72 screen pixels.
// 32px keeps a chunky pixel-art look with a clean integer-ish upscale.
constexpr int TILE_SIZE = 32;

// Generates the RGBA8 pixel buffer for one tile type.
// The returned vector is always size*size*4 bytes.
std::vector<uint8_t> generate(Tile::TileType type, int size = TILE_SIZE);

} // namespace TileTextureGen
