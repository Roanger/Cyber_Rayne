#include "../../include/Map.h"
#include "../../include/Tile.h"
#include "../../include/Enemy.h"
#include "../../include/NPC.h"
#include "../../include/VulkanRenderer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

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

void Map::loadTileTextures(VulkanRenderer* renderer) {
    if (!renderer) return;
    
    std::string base = renderer->getAssetsBasePath();
    std::string tilesPath = base + "/textures/tiles";
    std::cout << "Loading tile textures from: " << tilesPath << std::endl;
    
    // Load proper tile textures from assets/textures/tiles/
    std::string grassPath = tilesPath + "/grass.png";
    if (std::filesystem::exists(grassPath)) {
        m_tileTextures[Tile::TileType::GRASS] = renderer->loadTexture(grassPath);
        std::cout << "Loaded GRASS texture: " << m_tileTextures[Tile::TileType::GRASS] << std::endl;
    } else {
        std::cerr << "GRASS texture not found at: " << grassPath << std::endl;
    }
    
    std::string wallPath = tilesPath + "/wall.png";
    if (std::filesystem::exists(wallPath)) {
        m_tileTextures[Tile::TileType::WALL] = renderer->loadTexture(wallPath);
        std::cout << "Loaded WALL texture: " << m_tileTextures[Tile::TileType::WALL] << std::endl;
    } else {
        std::cerr << "WALL texture not found at: " << wallPath << std::endl;
    }
    
    std::string mountainPath = tilesPath + "/mountain.png";
    if (std::filesystem::exists(mountainPath)) {
        m_tileTextures[Tile::TileType::MOUNTAIN] = renderer->loadTexture(mountainPath);
        std::cout << "Loaded MOUNTAIN texture: " << m_tileTextures[Tile::TileType::MOUNTAIN] << std::endl;
    } else {
        std::cerr << "MOUNTAIN texture not found at: " << mountainPath << std::endl;
    }
    
    std::string treePath = tilesPath + "/tree.png";
    if (std::filesystem::exists(treePath)) {
        m_tileTextures[Tile::TileType::TREE] = renderer->loadTexture(treePath);
        std::cout << "Loaded TREE texture: " << m_tileTextures[Tile::TileType::TREE] << std::endl;
    } else {
        std::cerr << "TREE texture not found at: " << treePath << std::endl;
    }
    
    std::string waterPath = tilesPath + "/water.png";
    if (std::filesystem::exists(waterPath)) {
        m_tileTextures[Tile::TileType::WATER] = renderer->loadTexture(waterPath);
        std::cout << "Loaded WATER texture: " << m_tileTextures[Tile::TileType::WATER] << std::endl;
    } else {
        std::cerr << "WATER texture not found at: " << waterPath << std::endl;
    }
    
    std::string stonePath = tilesPath + "/stone.png";
    if (std::filesystem::exists(stonePath)) {
        m_tileTextures[Tile::TileType::STONE] = renderer->loadTexture(stonePath);
        std::cout << "Loaded STONE texture: " << m_tileTextures[Tile::TileType::STONE] << std::endl;
    } else {
        std::cerr << "STONE texture not found at: " << stonePath << std::endl;
    }
    
    std::string floorPath = tilesPath + "/floor.png";
    if (std::filesystem::exists(floorPath)) {
        m_tileTextures[Tile::TileType::FLOOR] = renderer->loadTexture(floorPath);
        std::cout << "Loaded FLOOR texture: " << m_tileTextures[Tile::TileType::FLOOR] << std::endl;
    } else {
        std::cerr << "FLOOR texture not found at: " << floorPath << std::endl;
    }
    
    std::string sandPath = tilesPath + "/sand.png";
    if (std::filesystem::exists(sandPath)) {
        m_tileTextures[Tile::TileType::SAND] = renderer->loadTexture(sandPath);
        std::cout << "Loaded SAND texture: " << m_tileTextures[Tile::TileType::SAND] << std::endl;
    } else {
        std::cerr << "SAND texture not found at: " << sandPath << std::endl;
    }
    
    std::string doorPath = tilesPath + "/door.png";
    if (std::filesystem::exists(doorPath)) {
        m_tileTextures[Tile::TileType::DOOR] = renderer->loadTexture(doorPath);
        std::cout << "Loaded DOOR texture: " << m_tileTextures[Tile::TileType::DOOR] << std::endl;
    } else {
        std::cerr << "DOOR texture not found at: " << doorPath << std::endl;
    }
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

                // Get texture index for this tile type
                int textureIndex = -1;
                auto it = m_tileTextures.find(tile->getType());
                if (it != m_tileTextures.end()) {
                    textureIndex = it->second;
                }
                
                // Render tile with appropriate texture
                if (textureIndex >= 0) {
                    renderer->renderSpriteWithTexture(normalizedX, normalizedY, 0.2f, 0.2f, textureIndex);
                } else {
                    // Fallback to default white sprite if no texture
                    renderer->renderSprite(normalizedX, normalizedY, 0.2f, 0.2f);
                }
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
