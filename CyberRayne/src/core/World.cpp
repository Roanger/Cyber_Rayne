#include "../../include/World.h"
#include "../../include/Map.h"
#include "../../include/Player.h"
#include "../../include/VulkanRenderer.h"
#include "../../include/Tile.h"
#include "../../include/EnemyTypes.h"
#include "../../include/NPC.h"
#include "../../include/BattleSystem.h"
#include "../../include/EncounterTable.h"
#include <iostream>
#include <cmath>
#include <fstream>
#include <sstream>

World::World()
    : m_currentMap(nullptr), m_currentBiome(BiomeType::NEO_KYOTO_CENTRAL), m_player(nullptr) {
}

World::~World() {
    // Unique pointers will automatically clean up
}

bool World::initialize() {
    std::cout << "Initializing world..." << std::endl;
    
    // Initialize encounter tables
    initializeEncounterTables();
    
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
#ifndef NO_VULKAN
    if (!renderer) return;
    
    std::cout << "Loading textures for all maps..." << std::endl;
    for (auto& map : m_maps) {
        if (map) {
            map->loadTileTextures(renderer);
        }
    }
#endif
}

void World::update(float deltaTime) {
    if (m_currentMap) {
        m_currentMap->update(deltaTime);
    }
}

void World::render(VulkanRenderer* renderer) {
#ifndef NO_VULKAN
    if (m_currentMap) {
        // Update viewport to follow player
        if (m_player) {
            m_currentMap->setPlayerViewport(m_player->getX(), m_player->getY());
        }
        m_currentMap->render(renderer);

        // Entities live in world tile space and must go through the same
        // viewport transform as the tiles, or they draw off-screen.
        float screenAspect = static_cast<float>(renderer->getSwapchainExtent().width) /
                             static_cast<float>(renderer->getSwapchainExtent().height);
        Map::Viewport vp = m_currentMap->getViewport(screenAspect);

        float ndcX = 0.0f;
        float ndcY = 0.0f;

        // Placeholder markers until per-entity sprites exist: cyan for NPCs,
        // magenta for enemies.
        for (const auto& npc : m_currentMap->getNPCs()) {
            if (m_currentMap->worldToNdc(vp, npc->getX(), npc->getY(), ndcX, ndcY)) {
                renderer->renderSprite(ndcX, ndcY, vp.tileWidth * 0.7f, vp.tileHeight * 0.7f,
                                       0.2f, 0.9f, 1.0f, 1.0f);
            }
        }

        for (const auto& enemy : m_currentMap->getEnemies()) {
            if (m_currentMap->worldToNdc(vp, enemy->getX(), enemy->getY(), ndcX, ndcY)) {
                renderer->renderSprite(ndcX, ndcY, vp.tileWidth * 0.8f, vp.tileHeight * 0.8f,
                                       1.0f, 0.15f, 0.5f, 1.0f);
            }
        }


        // Render the player using Vulkan
        if (m_player) {
            // The player is rendered separately in the GameState
        }
    }
#endif
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
    // === Neo-Kyoto Central ===
    std::unique_ptr<Map> central = std::make_unique<Map>("Neo-Kyoto Central", 30, 25);
    if (central->initialize()) {
        Map* map = central.get();

        // Fill with concrete base
        for (int y = 0; y < map->getHeight(); ++y) {
            for (int x = 0; x < map->getWidth(); ++x) {
                map->setTile(x, y, std::make_unique<Tile>(Tile::TileType::CONCRETE, true));
            }
        }

        // Main road through the center
        for (int x = 1; x < map->getWidth() - 1; ++x) {
            map->setTile(x, 12, std::make_unique<Tile>(Tile::TileType::ROAD, true));
            map->setTile(x, 13, std::make_unique<Tile>(Tile::TileType::ROAD, true));
        }

        // Buildings (metal walls) along the streets
        const std::vector<std::pair<int, int>> buildings = {
            // Left block
            {3, 4}, {4, 4}, {5, 4}, {6, 4}, {3, 5}, {4, 5}, {5, 5}, {6, 5},
            {3, 6}, {4, 6}, {5, 6}, {6, 6}, {3, 7}, {4, 7}, {5, 7}, {6, 7},
            {3, 8}, {4, 8}, {5, 8}, {6, 8},
            // Right block
            {22, 4}, {23, 4}, {24, 4}, {25, 4}, {22, 5}, {23, 5}, {24, 5}, {25, 5},
            {22, 6}, {23, 6}, {24, 6}, {25, 6}, {22, 7}, {23, 7}, {24, 7}, {25, 7},
            {22, 8}, {23, 8}, {24, 8}, {25, 8},
            // Lower left block
            {3, 16}, {4, 16}, {5, 16}, {6, 16}, {3, 17}, {4, 17}, {5, 17}, {6, 17},
            {3, 18}, {4, 18}, {5, 18}, {6, 18}, {3, 19}, {4, 19}, {5, 19}, {6, 19},
            // Lower right block
            {22, 16}, {23, 16}, {24, 16}, {25, 16}, {22, 17}, {23, 17}, {24, 17}, {25, 17},
            {22, 18}, {23, 18}, {24, 18}, {25, 18}, {22, 19}, {23, 19}, {24, 19}, {25, 19},
        };

        for (const auto& pos : buildings) {
            map->setTile(pos.first, pos.second, std::make_unique<Tile>(Tile::TileType::METAL, false));
        }

        // Neon signs (decorative, non-walkable)
        const std::vector<std::pair<int, int>> neonSigns = {
            {7, 4}, {21, 4}, {7, 16}, {21, 16}
        };
        for (const auto& pos : neonSigns) {
            map->setTile(pos.first, pos.second, std::make_unique<Tile>(Tile::TileType::NEON, false));
        }

        // Vending machines
        map->setTile(14, 10, std::make_unique<Tile>(Tile::TileType::VENDING, false));
        map->setTile(15, 10, std::make_unique<Tile>(Tile::TileType::VENDING, false));

        // Hologram advertisements
        map->setTile(10, 15, std::make_unique<Tile>(Tile::TileType::HOLOGRAM, false));
        map->setTile(19, 15, std::make_unique<Tile>(Tile::TileType::HOLOGRAM, false));

        // Building doors
        map->setTile(4, 8, std::make_unique<Tile>(Tile::TileType::DOOR, true));
        map->setTile(23, 8, std::make_unique<Tile>(Tile::TileType::DOOR, true));

        // NPCs
        {
            auto fixer = std::make_unique<NPC>("Fixer Kaga", 14.0f, 15.0f);
            map->addNPC(std::move(fixer));
        }
        {
            auto vendor = std::make_unique<NPC>("Street Vendor", 16.0f, 10.0f);
            map->addNPC(std::move(vendor));
        }
        {
            auto informant = std::make_unique<NPC>("Info Broker", 8.0f, 15.0f);
            map->addNPC(std::move(informant));
        }

        // Transition zone at top (to Neon Alley)
        for (int x = 14; x <= 16; ++x) {
            map->setTile(x, 1, std::make_unique<Tile>(Tile::TileType::ROAD, true));
        }

        m_maps.push_back(std::move(central));
        std::cout << "Created Neo-Kyoto Central district" << std::endl;
    }

    // === Neon Alley ===
    std::unique_ptr<Map> alley = std::make_unique<Map>("Neon Alley", 30, 25);
    if (alley->initialize()) {
        Map* map = alley.get();

        // Fill with road base
        for (int y = 0; y < map->getHeight(); ++y) {
            for (int x = 0; x < map->getWidth(); ++x) {
                map->setTile(x, y, std::make_unique<Tile>(Tile::TileType::ROAD, true));
            }
        }

        // Narrow alley walls
        const std::vector<std::pair<int, int>> alleyWalls = {
            // Left wall
            {2, 3}, {2, 4}, {2, 5}, {2, 6}, {2, 7}, {2, 8}, {2, 9}, {2, 10},
            {2, 14}, {2, 15}, {2, 16}, {2, 17}, {2, 18}, {2, 19}, {2, 20},
            // Right wall
            {27, 3}, {27, 4}, {27, 5}, {27, 6}, {27, 7}, {27, 8}, {27, 9}, {27, 10},
            {27, 14}, {27, 15}, {27, 16}, {27, 17}, {27, 18}, {27, 19}, {27, 20},
            // Scattered debris
            {8, 6}, {9, 6}, {20, 18}, {21, 18}, {12, 16}, {18, 8}
        };
        for (const auto& pos : alleyWalls) {
            map->setTile(pos.first, pos.second, std::make_unique<Tile>(Tile::TileType::METAL, false));
        }

        // Neon signs everywhere
        const std::vector<std::pair<int, int>> neonSigns = {
            {3, 4}, {5, 8}, {7, 12}, {10, 5}, {12, 18}, {15, 4},
            {18, 15}, {20, 7}, {23, 12}, {25, 18}, {6, 20}, {22, 3}
        };
        for (const auto& pos : neonSigns) {
            map->setTile(pos.first, pos.second, std::make_unique<Tile>(Tile::TileType::NEON, false));
        }

        // Vending machines
        map->setTile(5, 12, std::make_unique<Tile>(Tile::TileType::VENDING, false));
        map->setTile(24, 12, std::make_unique<Tile>(Tile::TileType::VENDING, false));

        // NPCs
        {
            auto smuggler = std::make_unique<NPC>("Smuggler Ryo", 15.0f, 12.0f);
            map->addNPC(std::move(smuggler));
        }
        {
            auto hacker = std::make_unique<NPC>("Hacker Zero", 10.0f, 10.0f);
            map->addNPC(std::move(hacker));
        }

        // Transition at bottom (back to Central)
        for (int x = 14; x <= 16; ++x) {
            map->setTile(x, 24, std::make_unique<Tile>(Tile::TileType::ROAD, true));
        }

        // Transition at top (to Corporate Plaza)
        for (int x = 14; x <= 16; ++x) {
            map->setTile(x, 1, std::make_unique<Tile>(Tile::TileType::ROAD, true));
        }

        m_maps.push_back(std::move(alley));
        std::cout << "Created Neon Alley district" << std::endl;
    }

    // === Corporate Plaza ===
    std::unique_ptr<Map> plaza = std::make_unique<Map>("Corporate Plaza", 25, 20);
    if (plaza->initialize()) {
        Map* map = plaza.get();

        // Fill with metal floor
        for (int y = 0; y < map->getHeight(); ++y) {
            for (int x = 0; x < map->getWidth(); ++x) {
                map->setTile(x, y, std::make_unique<Tile>(Tile::TileType::METAL, true));
            }
        }

        // Corporate building (large structure)
        const std::vector<std::pair<int, int>> corpWalls = {
            // Building outline (10x8)
            {8, 3}, {9, 3}, {10, 3}, {11, 3}, {12, 3}, {13, 3}, {14, 3}, {15, 3}, {16, 3}, {17, 3},
            {8, 10}, {9, 10}, {10, 10}, {11, 10}, {12, 10}, {13, 10}, {14, 10}, {15, 10}, {16, 10}, {17, 10},
            {8, 4}, {8, 5}, {8, 6}, {8, 7}, {8, 8}, {8, 9},
            {17, 4}, {17, 5}, {17, 6}, {17, 7}, {17, 8}, {17, 9},
        };
        for (const auto& pos : corpWalls) {
            map->setTile(pos.first, pos.second, std::make_unique<Tile>(Tile::TileType::WALL, false));
        }

        // Interior floor
        for (int y = 4; y <= 9; ++y) {
            for (int x = 9; x <= 16; ++x) {
                map->setTile(x, y, std::make_unique<Tile>(Tile::TileType::FLOOR, true));
            }
        }

        // Building entrance
        map->setTile(12, 10, std::make_unique<Tile>(Tile::TileType::DOOR, true));
        map->setTile(13, 10, std::make_unique<Tile>(Tile::TileType::DOOR, true));

        // Hologram displays
        map->setTile(4, 5, std::make_unique<Tile>(Tile::TileType::HOLOGRAM, false));
        map->setTile(20, 5, std::make_unique<Tile>(Tile::TileType::HOLOGRAM, false));
        map->setTile(4, 15, std::make_unique<Tile>(Tile::TileType::HOLOGRAM, false));
        map->setTile(20, 15, std::make_unique<Tile>(Tile::TileType::HOLOGRAM, false));

        // Neon accent
        map->setTile(12, 2, std::make_unique<Tile>(Tile::TileType::NEON, false));
        map->setTile(13, 2, std::make_unique<Tile>(Tile::TileType::NEON, false));

        // NPCs
        {
            auto exec = std::make_unique<NPC>("Exec Tanaka", 12.0f, 14.0f);
            map->addNPC(std::move(exec));
        }
        {
            auto guard = std::make_unique<NPC>("Security Chief", 18.0f, 6.0f);
            map->addNPC(std::move(guard));
        }

        // Transition at bottom (back to Neon Alley)
        for (int x = 11; x <= 13; ++x) {
            map->setTile(x, 19, std::make_unique<Tile>(Tile::TileType::ROAD, true));
        }

        m_maps.push_back(std::move(plaza));
        std::cout << "Created Corporate Plaza district" << std::endl;
    }
}

void World::initializeBiomes() {
    m_currentBiome = BiomeType::NEO_KYOTO_CENTRAL;
    std::cout << "Initialized biome to NEO_KYOTO_CENTRAL" << std::endl;
}

void World::spawnEnemies() {
    std::cout << "Spawning enemies for biome: " << static_cast<int>(m_currentBiome) << std::endl;
    if (m_currentMap) {
        // Clear existing enemies
        // In a real implementation, we might want to be more selective about this
        m_currentMap->clearEnemies();
        
        // Spawn new enemies based on the current biome
        spawnEnemiesForBiome(m_currentBiome, m_currentMap);
    }
}

void World::checkMapTransition() {
    if (!m_currentMap || !m_player) {
        return;
    }
    
    // Get player position
    int playerTileX = static_cast<int>(m_player->getX());
    int playerTileY = static_cast<int>(m_player->getY());
    
    // Check if player is on a transition tile
    std::string currentMapName = m_currentMap->getName();
    
    if (currentMapName == "Neo-Kyoto Central") {
        // Transition to Neon Alley (top of map)
        if (playerTileY <= 1 && playerTileX >= 14 && playerTileX <= 16) {
            std::cout << "Transitioning to Neon Alley..." << std::endl;
            changeMap("Neon Alley");
            m_currentBiome = BiomeType::NEON_ALLEY;
            m_player->setPosition(15.0f, 23.0f);
        }
    } else if (currentMapName == "Neon Alley") {
        // Transition back to Central (bottom of map)
        if (playerTileY >= 24 && playerTileX >= 14 && playerTileX <= 16) {
            std::cout << "Transitioning to Neo-Kyoto Central..." << std::endl;
            changeMap("Neo-Kyoto Central");
            m_currentBiome = BiomeType::NEO_KYOTO_CENTRAL;
            m_player->setPosition(15.0f, 2.0f);
        }
        // Transition to Corporate Plaza (top of map)
        if (playerTileY <= 1 && playerTileX >= 14 && playerTileX <= 16) {
            std::cout << "Transitioning to Corporate Plaza..." << std::endl;
            changeMap("Corporate Plaza");
            m_currentBiome = BiomeType::CORPORATE_PLAZA;
            m_player->setPosition(12.0f, 18.0f);
        }
    } else if (currentMapName == "Corporate Plaza") {
        // Transition back to Neon Alley (bottom of map)
        if (playerTileY >= 19 && playerTileX >= 11 && playerTileX <= 13) {
            std::cout << "Transitioning to Neon Alley..." << std::endl;
            changeMap("Neon Alley");
            m_currentBiome = BiomeType::NEON_ALLEY;
            m_player->setPosition(15.0f, 2.0f);
        }
    }
}

bool World::checkEnemyEncounter() {
    if (!m_currentMap || !m_player || hasActiveEncounter()) {
        return false;
    }

    float playerX = m_player->getX();
    float playerY = m_player->getY();

    // Slightly under one tile, so standing on an enemy's tile triggers it.
    const float encounterDistance = 0.6f;

    Enemy* triggered = nullptr;
    for (const auto& enemy : m_currentMap->getEnemies()) {
        float dx = playerX - enemy->getX();
        float dy = playerY - enemy->getY();
        if (std::sqrt(dx * dx + dy * dy) <= encounterDistance) {
            triggered = enemy.get();
            break;
        }
    }

    if (!triggered) {
        return false;
    }

    std::cout << "Player encountered " << triggered->getName() << "!" << std::endl;

    // Take only the triggering enemy into battle, not every enemy on the map.
    auto claimed = m_currentMap->takeEnemy(triggered);
    if (!claimed) {
        return false;
    }
    m_activeEncounter.push_back(std::move(claimed));
    m_encounterCooldown = ENCOUNTER_COOLDOWN_STEPS;
    return true;
}

bool World::onPlayerStep() {
    if (!m_currentMap || !m_player || hasActiveEncounter()) {
        return false;
    }

    if (m_encounterCooldown > 0) {
        --m_encounterCooldown;
        return false;
    }

    const EncounterTable* table = getEncounterTable(m_currentBiome);
    if (!table || table->isEmpty() || table->getEncounterRate() <= 0.0f) {
        return false;
    }

    float roll = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    if (roll >= table->getEncounterRate()) {
        return false;
    }

    auto group = table->spawnEncounter(1, 3);
    if (group.empty()) {
        return false;
    }

    std::cout << "Ambushed by " << group.size() << " hostile(s)!" << std::endl;
    for (auto& enemy : group) {
        m_activeEncounter.push_back(std::move(enemy));
    }
    m_encounterCooldown = ENCOUNTER_COOLDOWN_STEPS;
    return true;
}

void World::endEncounter() {
    m_activeEncounter.clear();
    m_encounterCooldown = ENCOUNTER_COOLDOWN_STEPS;

    // Keep the district populated so visible enemies don't run out.
    if (m_currentMap && m_currentMap->getEnemies().size() < MIN_MAP_ENEMIES) {
        const EncounterTable* table = getEncounterTable(m_currentBiome);
        if (table && !table->isEmpty()) {
            auto replacements = table->spawnEncounter(1, 1);
            for (auto& enemy : replacements) {
                float x = 0.0f;
                float y = 0.0f;
                if (findSpawnTile(m_currentMap, MIN_SPAWN_DISTANCE, x, y)) {
                    enemy->setPosition(x, y);
                    m_currentMap->addEnemy(std::move(enemy));
                }
            }
        }
    }
}

bool World::isTileOccupied(Map* map, int x, int y) const {
    if (!map) return true;

    auto sameTile = [x, y](float ex, float ey) {
        return static_cast<int>(ex) == x && static_cast<int>(ey) == y;
    };

    for (const auto& enemy : map->getEnemies()) {
        if (sameTile(enemy->getX(), enemy->getY())) return true;
    }
    for (const auto& npc : map->getNPCs()) {
        if (sameTile(npc->getX(), npc->getY())) return true;
    }
    return false;
}

bool World::findSpawnTile(Map* map, float minDistance, float& outX, float& outY) const {
    if (!map) return false;

    // Collect every candidate, then pick one at random, so spawns aren't biased
    // toward the top-left corner the way a first-match scan would be.
    std::vector<std::pair<int, int>> candidates;
    float playerX = m_player ? m_player->getX() : -1000.0f;
    float playerY = m_player ? m_player->getY() : -1000.0f;

    for (int y = 0; y < map->getHeight(); ++y) {
        for (int x = 0; x < map->getWidth(); ++x) {
            if (!map->isTileWalkable(x, y)) continue;
            if (isTileOccupied(map, x, y)) continue;

            float dx = playerX - static_cast<float>(x);
            float dy = playerY - static_cast<float>(y);
            if (std::sqrt(dx * dx + dy * dy) < minDistance) continue;

            candidates.emplace_back(x, y);
        }
    }

    if (candidates.empty()) {
        return false;
    }

    const auto& chosen = candidates[std::rand() % candidates.size()];
    outX = static_cast<float>(chosen.first);
    outY = static_cast<float>(chosen.second);
    return true;
}

void World::spawnEnemiesForBiome(BiomeType biome, Map* map) {
    const EncounterTable* table = getEncounterTable(biome);
    if (!table || table->isEmpty()) {
        std::cout << "No encounter table for biome " << static_cast<int>(biome) << std::endl;
        return;
    }

    // Placing enemies on searched walkable tiles rather than fixed coordinates:
    // the old hardcoded corners fell inside buildings on Neo-Kyoto Central and
    // outside the bounds of the smaller Corporate Plaza map entirely.
    auto enemies = table->spawnEncounter(2, 4);

    for (auto& enemy : enemies) {
        float x = 0.0f;
        float y = 0.0f;
        if (!findSpawnTile(map, MIN_SPAWN_DISTANCE, x, y)) {
            std::cout << "No walkable spawn tile left on " << map->getName()
                      << "; skipping " << enemy->getName() << std::endl;
            continue;
        }
        enemy->setPosition(x, y);
        std::cout << "Spawned " << enemy->getName() << " at (" << x << ", " << y << ")" << std::endl;
        map->addEnemy(std::move(enemy));
    }
}

void World::initializeEncounterTables() {
    // Neo-Kyoto Central — low-level street threats
    auto central = std::make_unique<EncounterTable>("Neo-Kyoto Central");
    central->setEncounterRate(0.04f);  // Starting district — keep it forgiving.
    central->addEntry("StreetThug", 50, 1);
    central->addEntry("StreetThug", 30, 2);
    central->addEntry("SecurityDrone", 20, 2);
    m_encounterTables.push_back(std::move(central));

    // Neon Alley — moderate threats, more variety
    auto alley = std::make_unique<EncounterTable>("Neon Alley");
    alley->setEncounterRate(0.08f);
    alley->addEntry("StreetThug", 30, 2);
    alley->addEntry("SecurityDrone", 25, 3);
    alley->addEntry("BlackICE", 20, 2);
    alley->addEntry("CombatCyborg", 15, 3);
    alley->addEntry("StreetThug", 10, 3);
    m_encounterTables.push_back(std::move(alley));

    // Corporate Plaza — high-level corporate security
    auto plaza = std::make_unique<EncounterTable>("Corporate Plaza");
    plaza->setEncounterRate(0.10f);  // Hostile territory — heaviest patrol density.
    plaza->addEntry("SecurityDrone", 30, 3);
    plaza->addEntry("CombatCyborg", 25, 4);
    plaza->addEntry("CorporateEnforcer", 20, 3);
    plaza->addEntry("BlackICE", 15, 3);
    plaza->addEntry("CorporateEnforcer", 10, 4);
    m_encounterTables.push_back(std::move(plaza));
}

const EncounterTable* World::getEncounterTable(BiomeType biome) const {
    std::size_t index = 0;
    switch (biome) {
        case BiomeType::NEO_KYOTO_CENTRAL: index = 0; break;
        case BiomeType::NEON_ALLEY:        index = 1; break;
        case BiomeType::CORPORATE_PLAZA:   index = 2; break;
        default: return nullptr;
    }
    if (index < m_encounterTables.size()) {
        return m_encounterTables[index].get();
    }
    return nullptr;
}
