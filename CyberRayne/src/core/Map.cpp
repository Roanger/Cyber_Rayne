#include <cmath>
#include "../../include/Map.h"
#include "../../include/Tile.h"
#include "../../include/Enemy.h"
#include "../../include/NPC.h"
#ifndef NO_VULKAN
#include "../../include/VulkanRenderer.h"
#include "../../include/TileTextureGen.h"
#endif
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <iterator>
#include <vector>

Map::Map(const std::string& name, int width, int height)
    : m_name(name), m_width(width), m_height(height) {
    // Initialize the tiles vector with nullptr
    m_tiles.resize(width * height);
}

Map::~Map() {
    // Unique pointers will automatically clean up
}

bool Map::initialize() {
    // For now, we'll create a simple empty map
    // In a real implementation, we would load from a file
    std::cout << "Initializing map: " << m_name << std::endl;
    
    // Create a simple test map
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            // Create a basic walkable tile
            // In a real implementation, we would load tile data from a file
            std::unique_ptr<Tile> tile = std::make_unique<Tile>(Tile::TileType::FLOOR, true); // walkable by default
            setTile(x, y, std::move(tile));
        }
    }
    
    // Set some walls around the edges
    for (int x = 0; x < m_width; ++x) {
        setTile(x, 0, std::make_unique<Tile>(Tile::TileType::WALL, false)); // Top wall
        setTile(x, m_height - 1, std::make_unique<Tile>(Tile::TileType::WALL, false)); // Bottom wall
    }
    
    for (int y = 0; y < m_height; ++y) {
        setTile(0, y, std::make_unique<Tile>(Tile::TileType::WALL, false)); // Left wall
        setTile(m_width - 1, y, std::make_unique<Tile>(Tile::TileType::WALL, false)); // Right wall
    }
    
    return true;
}

#ifndef NO_VULKAN
void Map::loadTileTextures(VulkanRenderer* renderer) {
    if (!renderer) return;
    
    // Tiles are synthesized rather than loaded from disk. They are seamless by
    // construction, cost ~4KB each instead of a multi-megabyte image, and can be
    // retuned by editing TileTextureGen.cpp instead of round-tripping through an
    // art tool. See ASSET_SPEC.md for why tiles are procedural but sprites are not.
    static constexpr Tile::TileType kTileTypes[] = {
        Tile::TileType::CONCRETE, Tile::TileType::ROAD,  Tile::TileType::NEON,
        Tile::TileType::METAL,    Tile::TileType::HOLOGRAM, Tile::TileType::VENDING,
        Tile::TileType::GRASS,    Tile::TileType::WATER, Tile::TileType::MOUNTAIN,
        Tile::TileType::SAND,     Tile::TileType::STONE, Tile::TileType::TREE,
        Tile::TileType::WALL,     Tile::TileType::DOOR,  Tile::TileType::FLOOR,
    };

    const int sz = TileTextureGen::TILE_SIZE;
    for (Tile::TileType type : kTileTypes) {
        const std::vector<uint8_t> pixels = TileTextureGen::generate(type, sz);
        m_tileTextures[type] = renderer->loadTextureFromMemory(pixels.data(), sz, sz, 4);
    }

    std::cout << "Generated " << std::size(kTileTypes) << " procedural tile textures ("
              << sz << "x" << sz << ")" << std::endl;
}
#endif

void Map::update(float /*deltaTime*/) {
    // Enemies and NPCs are static on the map for now; AI and patrol behaviour
    // will hook in here.
}

Map::Viewport Map::getViewport(float screenAspect) const {
    Viewport vp;
    // Viewport height in tiles; width derived from aspect ratio to fill screen
    vp.heightTiles = VIEWPORT_HEIGHT_TILES;
    vp.widthTiles = static_cast<float>(VIEWPORT_HEIGHT_TILES) * screenAspect;
    if (vp.widthTiles < 1.0f) vp.widthTiles = 1.0f;

    const float mapW = static_cast<float>(m_width);
    const float mapH = static_cast<float>(m_height);
    const float viewH = static_cast<float>(vp.heightTiles);

    // Default: centre on the map.
    float centreX = mapW * 0.5f;
    float centreY = mapH * 0.5f;
    // With a player position set, centre on the middle of the player's tile.
    // The position is fractional mid-step, which is what makes the camera
    // scroll smoothly.
    if (m_playerX >= 0 && m_playerY >= 0) {
        centreX = m_playerX + 0.5f;
        centreY = m_playerY + 0.5f;
    }

    // Stop at the map's edges; a map smaller than the view sits centred.
    auto place = [](float centre, float view, float mapSize) {
        if (mapSize <= view) return (mapSize - view) * 0.5f;
        return std::clamp(centre - view * 0.5f, 0.0f, mapSize - view);
    };
    vp.startX = place(centreX, vp.widthTiles, mapW);
    vp.startY = place(centreY, viewH, mapH);

    vp.tileWidth = 2.0f / vp.widthTiles;
    vp.tileHeight = 2.0f / viewH;
    return vp;
}

bool Map::worldToNdc(const Viewport& viewport, float worldX, float worldY,
                     float& ndcX, float& ndcY) const {
    ndcX = -1.0f + (worldX - viewport.startX) * viewport.tileWidth + viewport.tileWidth * 0.5f;
    ndcY = -1.0f + (worldY - viewport.startY) * viewport.tileHeight + viewport.tileHeight * 0.5f;

    // Cull with one tile of slack so entities straddling the edge still draw.
    return worldX >= viewport.startX - 1.0f &&
           worldX <= viewport.startX + viewport.widthTiles + 1.0f &&
           worldY >= viewport.startY - 1.0f &&
           worldY <= viewport.startY + viewport.heightTiles + 1.0f;
}

#ifndef NO_VULKAN
void Map::render(VulkanRenderer* renderer) {
    // Use actual swapchain extent for aspect ratio
    float screenAspect = static_cast<float>(renderer->getSwapchainExtent().width) /
                         static_cast<float>(renderer->getSwapchainExtent().height);

    Viewport vp = getViewport(screenAspect);

    // The view can start partway through a tile, so draw from the tile
    // holding its top-left corner through the one holding its bottom-right.
    const int firstX = std::max(0, static_cast<int>(std::floor(vp.startX)));
    const int firstY = std::max(0, static_cast<int>(std::floor(vp.startY)));
    const int endX = std::min(m_width, static_cast<int>(std::ceil(vp.startX + vp.widthTiles)));
    const int endY = std::min(m_height, static_cast<int>(std::ceil(vp.startY + static_cast<float>(vp.heightTiles))));

    // Render only the visible tiles
    for (int y = firstY; y < endY; ++y) {
        for (int x = firstX; x < endX; ++x) {
            Tile* tile = getTile(x, y);
            if (!tile) continue;

            float normalizedX = 0.0f;
            float normalizedY = 0.0f;
            worldToNdc(vp, static_cast<float>(x), static_cast<float>(y), normalizedX, normalizedY);

            // Get texture index for this tile type
            int textureIndex = -1;
            auto it = m_tileTextures.find(tile->getType());
            if (it != m_tileTextures.end()) {
                textureIndex = it->second;
            }

            // Render tile with square aspect ratio
            if (textureIndex >= 0) {
                renderer->renderSpriteWithTexture(normalizedX, normalizedY, vp.tileWidth, vp.tileHeight, textureIndex);
            } else {
                // Fallback to default white sprite if no texture
                renderer->renderSprite(normalizedX, normalizedY, vp.tileWidth, vp.tileHeight);
            }
        }
    }
}
#endif

Tile* Map::getTile(int x, int y) const {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
        return nullptr;
    }
    
    return m_tiles[y * m_width + x].get();
}

bool Map::isTileWalkable(int x, int y) const {
    Tile* tile = getTile(x, y);
    return tile != nullptr && tile->isWalkable();
}

void Map::setTile(int x, int y, std::unique_ptr<Tile> tile) {
    if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
        m_tiles[y * m_width + x] = std::move(tile);
    }
}

void Map::addEnemy(std::unique_ptr<Enemy> enemy) {
    m_enemies.push_back(std::move(enemy));
}

void Map::removeEnemy(Enemy* enemy) {
    // Find and remove the enemy
    for (auto it = m_enemies.begin(); it != m_enemies.end(); ++it) {
        if (it->get() == enemy) {
            m_enemies.erase(it);
            break;
        }
    }
}

std::unique_ptr<Enemy> Map::takeEnemy(Enemy* enemy) {
    for (auto it = m_enemies.begin(); it != m_enemies.end(); ++it) {
        if (it->get() == enemy) {
            auto claimed = std::move(*it);
            m_enemies.erase(it);
            return claimed;
        }
    }
    return nullptr;
}

void Map::clearEnemies() {
    m_enemies.clear();
}

void Map::addNPC(std::unique_ptr<NPC> npc) {
    m_npcs.push_back(std::move(npc));
}

void Map::removeNPC(NPC* npc) {
    // Find and remove the NPC
    for (auto it = m_npcs.begin(); it != m_npcs.end(); ++it) {
        if (it->get() == npc) {
            m_npcs.erase(it);
            break;
        }
    }
}
