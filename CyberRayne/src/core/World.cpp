#include "../../include/World.h"
#include "../../include/Map.h"
#include "../../include/Player.h"
#include "../../include/VulkanRenderer.h"
#include "../../include/Tile.h"
#include "../../include/EnemyTypes.h"
#include "../../include/NPC.h"
#include "../../include/BattleSystem.h"
#include <iostream>
#include <cmath>
#include <fstream>
#include <sstream>

World::World()
    : m_currentMap(nullptr), m_currentBiome(BiomeType::TOWN), m_player(nullptr) {
}

World::~World() {
    // Unique pointers will automatically clean up
}

bool World::initialize() {
    std::cout << "Initializing world..." << std::endl;
    
    // Create maps
    createMaps();
    
    // Initialize biomes
    initializeBiomes();
    
    // Load starting map
    if (!m_maps.empty()) {
        m_currentMap = m_maps[0].get();
        std::cout << "Loaded starting map: " << m_currentMap->getName() << std::endl;
        
        // Spawn enemies for the current biome
        spawnEnemies();
    }
    
    return true;
}

void World::loadMapTextures(VulkanRenderer* renderer) {
    if (!renderer) return;
    
    std::cout << "Loading textures for all maps..." << std::endl;
    for (auto& map : m_maps) {
        if (map) {
            map->loadTileTextures(renderer);
        }
    }
}

void World::update(float deltaTime) {
    if (m_currentMap) {
        m_currentMap->update(deltaTime);
    }
}

void World::render(VulkanRenderer* renderer) {
    if (m_currentMap) {
        m_currentMap->render(renderer);
        
        // Render entities on the map
        for (const auto& enemy : m_currentMap->getEnemies()) {
            // Render a simple sprite for the enemy at its position
            renderer->renderSprite(enemy->getX(), enemy->getY(), 0.1f, 0.1f);
        }
        
        for (const auto& npc : m_currentMap->getNPCs()) {
            // Render a simple sprite for the NPC at its position
            renderer->renderSprite(npc->getX(), npc->getY(), 0.1f, 0.1f);
        }
        
        // Render the player using Vulkan
        if (m_player) {
            // The player is rendered separately in the GameState
        }
    }
}

void World::loadMap(const std::string& mapName) {
    // In a real implementation, we would load the map from a file
    // For now, we'll just find the map in our existing maps
    for (auto& map : m_maps) {
        if (map->getName() == mapName) {
            m_currentMap = map.get();
            std::cout << "Loaded map: " << mapName << std::endl;
            return;
        }
    }
    
    std::cout << "Map not found: " << mapName << std::endl;
}

void World::changeMap(const std::string& mapName) {
    // In a real implementation, we would handle map transitions
    loadMap(mapName);
    
    // Spawn enemies for the new map
    if (m_currentMap) {
        spawnEnemies();
    }
}

void World::createMaps() {
    // Create a detailed starting village map
    std::unique_ptr<Map> startingVillage = std::make_unique<Map>("Starting Village", 30, 25);
    if (startingVillage->initialize()) {
        // Customize the starting village with varied terrain
        Map* map = startingVillage.get();
        
        // Fill the map with grass first
        for (int y = 0; y < map->getHeight(); ++y) {
            for (int x = 0; x < map->getWidth(); ++x) {
                map->setTile(x, y, std::make_unique<Tile>(Tile::TileType::GRASS, true));
            }
        }
        
        // Add trees around the edges and in clusters
        const std::vector<std::pair<int, int>> treePositions = {
            // Top edge trees
            {2, 1}, {5, 1}, {8, 1}, {11, 1}, {14, 1}, {17, 1}, {20, 1}, {23, 1}, {26, 1},
            // Bottom edge trees
            {2, 23}, {5, 23}, {8, 23}, {11, 23}, {14, 23}, {17, 23}, {20, 23}, {23, 23}, {26, 23},
            // Left edge trees
            {1, 3}, {1, 6}, {1, 9}, {1, 12}, {1, 15}, {1, 18}, {1, 21},
            // Right edge trees
            {28, 3}, {28, 6}, {28, 9}, {28, 12}, {28, 15}, {28, 18}, {28, 21},
            // Tree clusters
            {4, 4}, {5, 5}, {6, 4}, {22, 4}, {23, 5}, {24, 4}, {4, 20}, {5, 19}, {6, 20}, {22, 20}, {23, 19}, {24, 20}
        };
        
        for (const auto& pos : treePositions) {
            map->setTile(pos.first, pos.second, std::make_unique<Tile>(Tile::TileType::TREE, false));
        }
        
        // Create a small pond/stream
        const std::vector<std::pair<int, int>> waterPositions = {
            {25, 10}, {26, 10}, {27, 10}, {28, 10},
            {25, 11}, {26, 11}, {27, 11}, {28, 11},
            {26, 12}, {27, 12}, {28, 12}
        };
        
        for (const auto& pos : waterPositions) {
            map->setTile(pos.first, pos.second, std::make_unique<Tile>(Tile::TileType::WATER, false));
        }
        
        // Create stone path from center to pond
        const std::vector<std::pair<int, int>> pathPositions = {
            {15, 12}, {16, 12}, {17, 12}, {18, 12}, {19, 12}, {20, 12}, {21, 12}, {22, 12}, {23, 12}, {24, 12}
        };
        
        for (const auto& pos : pathPositions) {
            map->setTile(pos.first, pos.second, std::make_unique<Tile>(Tile::TileType::STONE, true));
        }
        
        // Create a small village building (house)
        const std::vector<std::pair<int, int>> houseWalls = {
            // House walls (8x6 building)
            {10, 8}, {11, 8}, {12, 8}, {13, 8}, {14, 8}, {15, 8}, {16, 8}, {17, 8},
            {10, 13}, {11, 13}, {12, 13}, {13, 13}, {14, 13}, {15, 13}, {16, 13}, {17, 13},
            {10, 9}, {10, 10}, {10, 11}, {10, 12},
            {17, 9}, {17, 10}, {17, 11}, {17, 12}
        };
        
        for (const auto& pos : houseWalls) {
            map->setTile(pos.first, pos.second, std::make_unique<Tile>(Tile::TileType::WALL, false));
        }
        
        // House floor
        for (int y = 9; y <= 12; ++y) {
            for (int x = 11; x <= 16; ++x) {
                map->setTile(x, y, std::make_unique<Tile>(Tile::TileType::FLOOR, true));
            }
        }
        
        // House door
        map->setTile(13, 13, std::make_unique<Tile>(Tile::TileType::DOOR, true));
        
        // Add a village elder NPC near the house
        {
            auto elder = std::make_unique<NPC>("Village Elder", 14.0f, 15.0f);
            map->addNPC(std::move(elder));
        }
        
        // Add a merchant NPC
        {
            auto merchant = std::make_unique<NPC>("Merchant", 20.0f, 15.0f);
            map->addNPC(std::move(merchant));
        }
        
        // Add a villager NPC
        {
            auto villager = std::make_unique<NPC>("Villager", 8.0f, 15.0f);
            map->addNPC(std::move(villager));
        }
        
        m_maps.push_back(std::move(startingVillage));
        std::cout << "Created starting village map with varied terrain and NPCs" << std::endl;
    }
    
    // Create a forest map
    std::unique_ptr<Map> forest = std::make_unique<Map>("Forest", 30, 25);
    if (forest->initialize()) {
        m_maps.push_back(std::move(forest));
    }
    
    // Create a town map
    std::unique_ptr<Map> town = std::make_unique<Map>("Town", 25, 20);
    if (town->initialize()) {
        m_maps.push_back(std::move(town));
    }
}

void World::initializeBiomes() {
    // In a real implementation, we would initialize biome-specific data
    // For now, we'll just set the current biome
    m_currentBiome = BiomeType::FOREST;
    std::cout << "Initialized biome to FOREST for testing" << std::endl;
}

void World::spawnEnemies() {
    std::cout << "Spawning enemies for biome: " << static_cast<int>(m_currentBiome) << std::endl;
    if (m_currentMap) {
        // Clear existing enemies
        // In a real implementation, we might want to be more selective about this
        
        // Spawn new enemies based on the current biome
        spawnEnemiesForBiome(m_currentBiome, m_currentMap);
    }
}

bool World::checkEnemyEncounter() {
    if (!m_currentMap || !m_player) {
        return false;
    }
    
    // Get player position
    float playerX = m_player->getX();
    float playerY = m_player->getY();
    
    // Check distance to each enemy
    const float encounterDistance = 0.5f;  // Adjust as needed
    
    for (const auto& enemy : m_currentMap->getEnemies()) {
        float enemyX = enemy->getX();
        float enemyY = enemy->getY();
        
        // Calculate distance between player and enemy
        float dx = playerX - enemyX;
        float dy = playerY - enemyY;
        float distance = std::sqrt(dx * dx + dy * dy);
        
        // If player is close enough to enemy, trigger encounter
        if (distance <= encounterDistance) {
            std::cout << "Player encountered " << enemy->getName() << "!" << std::endl;
            return true;
        }
    }
    
    return false;
}

void World::spawnEnemiesForBiome(BiomeType biome, Map* map) {
    std::cout << "spawnEnemiesForBiome called with biome: " << static_cast<int>(biome) << std::endl;
    // Clear existing enemies
    // Note: This is a simplified approach. In a real game, you might want to
    // preserve some enemies or have more sophisticated spawning logic.
    
    // For now, we'll just spawn a few enemies for demonstration
    switch (biome) {
        case BiomeType::FOREST:
            // Spawn forest enemies
            std::cout << "Spawning forest enemies..." << std::endl;
            // Add a goblin at position (2.0, 2.0)
            {
                auto goblin = std::make_unique<Goblin>(1);
                goblin->setPosition(2.0f, 2.0f);
                std::cout << "Adding goblin at position (" << goblin->getX() << ", " << goblin->getY() << ")" << std::endl;
                map->addEnemy(std::move(goblin));
            }
            // Add a wolf at position (-1.0, 1.0)
            {
                auto wolf = std::make_unique<Wolf>(2);
                wolf->setPosition(-1.0f, 1.0f);
                std::cout << "Adding wolf at position (" << wolf->getX() << ", " << wolf->getY() << ")" << std::endl;
                map->addEnemy(std::move(wolf));
            }
            break;
            
        case BiomeType::DESERT:
            // Spawn desert enemies
            std::cout << "Spawning desert enemies..." << std::endl;
            // Add a sand scorpion at position (1.5, -0.5)
            {
                auto scorpion = std::make_unique<SandScorpion>(1);
                scorpion->setPosition(1.5f, -0.5f);
                std::cout << "Adding sand scorpion at position (" << scorpion->getX() << ", " << scorpion->getY() << ")" << std::endl;
                map->addEnemy(std::move(scorpion));
            }
            break;
            
        case BiomeType::MOUNTAIN:
            // Spawn mountain enemies
            std::cout << "Spawning mountain enemies..." << std::endl;
            // Add a mountain lion at position (-1.5, -1.0)
            {
                auto lion = std::make_unique<MountainLion>(1);
                lion->setPosition(-1.5f, -1.0f);
                std::cout << "Adding mountain lion at position (" << lion->getX() << ", " << lion->getY() << ")" << std::endl;
                map->addEnemy(std::move(lion));
            }
            break;
            
        case BiomeType::SWAMP:
            // Spawn swamp enemies
            std::cout << "Spawning swamp enemies..." << std::endl;
            // Add a zombie at position (0.0, -2.0)
            {
                auto zombie = std::make_unique<Zombie>(1);
                zombie->setPosition(0.0f, -2.0f);
                std::cout << "Adding zombie at position (" << zombie->getX() << ", " << zombie->getY() << ")" << std::endl;
                map->addEnemy(std::move(zombie));
            }
            break;
            
        case BiomeType::TOWN:
        case BiomeType::DUNGEON:
        default:
            // No enemies in town or dungeon (for now)
            std::cout << "No enemies to spawn in this biome." << std::endl;
            break;
    }
}
