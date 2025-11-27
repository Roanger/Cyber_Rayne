#include "../../include/Map.h"
#include "../../include/Tile.h"
#include "../../include/Enemy.h"
#include "../../include/NPC.h"
#include "../../include/VulkanRenderer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

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

void Map::update(float deltaTime) {
    // Update all enemies
    for (auto& enemy : m_enemies) {
        // In a real implementation, we would update enemy AI here
    }
    
    // Update all NPCs
    for (auto& npc : m_npcs) {
        // In a real implementation, we would update NPC behavior here
    }
}

void Map::render(VulkanRenderer* renderer) {
    // For now, render only a small viewport of the map to avoid excessive sprites
    // In a real implementation, we would implement proper camera/viewport culling

    // Define a small viewport (e.g., 10x10 tiles) around the center for testing
    const int VIEWPORT_SIZE = 10;
    int startX = (m_width > VIEWPORT_SIZE) ? (m_width - VIEWPORT_SIZE) / 2 : 0;
    int startY = (m_height > VIEWPORT_SIZE) ? (m_height - VIEWPORT_SIZE) / 2 : 0;
    int endX = (startX + VIEWPORT_SIZE < m_width) ? startX + VIEWPORT_SIZE : m_width;
    int endY = (startY + VIEWPORT_SIZE < m_height) ? startY + VIEWPORT_SIZE : m_height;

    // Render only the visible tiles
    for (int y = startY; y < endY; ++y) {
        for (int x = startX; x < endX; ++x) {
            Tile* tile = getTile(x, y);
            if (tile) {
                // Position calculation: convert grid coordinates to normalized device coordinates
                // Scale to fit viewport in screen space
                float normalizedX = ((float)(x - startX) / VIEWPORT_SIZE) * 2.0f - 1.0f;
                float normalizedY = ((float)(y - startY) / VIEWPORT_SIZE) * 2.0f - 1.0f;

                // Render tile with Vulkan (simplified - using same sprite for all tiles for now)
                renderer->renderSprite(normalizedX, normalizedY, 0.1f, 0.1f);
            }
        }
    }
}

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
