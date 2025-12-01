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
    // Create a starting area map
    std::unique_ptr<Map> startingArea = std::make_unique<Map>("Starting Area", 20, 15);
    if (startingArea->initialize()) {
        m_maps.push_back(std::move(startingArea));
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
