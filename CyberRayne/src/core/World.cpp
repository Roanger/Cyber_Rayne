#ifndef NO_VULKAN
#include "../../include/ui/SpriteLibrary.h"
#include "../../include/ui/Theme.h"
#include <cmath>
#endif
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
#include <algorithm>
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

void World::loadMapTextures([[maybe_unused]] VulkanRenderer* renderer) {
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

void World::render([[maybe_unused]] VulkanRenderer* renderer, [[maybe_unused]] SpriteLibrary* sprites,
                   [[maybe_unused]] float animTime, [[maybe_unused]] const std::string& talkingNpc) {
#ifndef NO_VULKAN
    if (!m_currentMap) {
        return;
    }
    // Update viewport to follow player
    if (m_player) {
        m_currentMap->setPlayerViewport(m_player->getX(), m_player->getY());
    }
    m_currentMap->render(renderer);

    // Entities live in world tile space and must go through the same
    // viewport transform as the tiles, or they draw off-screen.
    const int screenW = static_cast<int>(renderer->getSwapchainExtent().width);
    const int screenH = static_cast<int>(renderer->getSwapchainExtent().height);
    Map::Viewport vp = m_currentMap->getViewport(static_cast<float>(screenW) / static_cast<float>(screenH));
    const int tileW = static_cast<int>(vp.tileWidth * 0.5f * static_cast<float>(screenW));
    const int tileH = static_cast<int>(vp.tileHeight * 0.5f * static_cast<float>(screenH));

    struct Figure {
        float worldY;
        int centreX;   // pixel centre of the tile
        int centreY;
        const Sprite* sprite;
        uint32_t fallback;  // block colour when there is no art
        bool marker;        // story fight: gold plate under the feet
        int col;            // frame within an animation sheet
        int row;
    };
    std::vector<Figure> figures;

    auto add = [&](float wx, float wy, const Sprite* sprite, uint32_t fallback, bool marker,
                   int col = 0, int row = 0) {
        float ndcX = 0.0f;
        float ndcY = 0.0f;
        if (!m_currentMap->worldToNdc(vp, wx, wy, ndcX, ndcY)) return;
        figures.push_back({wy,
                           static_cast<int>((ndcX + 1.0f) * 0.5f * static_cast<float>(screenW)),
                           static_cast<int>((ndcY + 1.0f) * 0.5f * static_cast<float>(screenH)),
                           sprite, fallback, marker, col, row});
    };

    // NPC sheets: row 0 is a slow idle, row 1 plays while they are spoken to.
    constexpr float NPC_IDLE_FRAME_TIME = 0.45f;
    constexpr float NPC_TALK_FRAME_TIME = 0.22f;
    std::size_t npcIndex = 0;
    for (const auto& npc : m_currentMap->getNPCs()) {
        ++npcIndex;
        if (!sprites) {
            add(npc->getX(), npc->getY(), nullptr, Theme::FOAM, false);
            continue;
        }
        const Sprite& sheet = sprites->sheet("npcs", npc->getName(), "idle_talk", 4, 2);
        if (sheet.valid()) {
            const bool talking = !talkingNpc.empty() && npc->getName() == talkingNpc;
            // Each NPC starts at a different point, so a crowd does not sway in step.
            const float t = animTime + static_cast<float>(npcIndex) * 0.37f;
            const int col = static_cast<int>(t / (talking ? NPC_TALK_FRAME_TIME : NPC_IDLE_FRAME_TIME)) % 4;
            add(npc->getX(), npc->getY(), &sheet, Theme::FOAM, false, col, talking ? 1 : 0);
        } else {
            add(npc->getX(), npc->getY(), &sprites->get("npcs", npc->getName()), Theme::FOAM, false);
        }
    }
    for (const auto& enemy : m_currentMap->getEnemies()) {
        add(enemy->getX(), enemy->getY(),
            sprites ? &sprites->get("enemies", enemy->getName()) : nullptr,
            Theme::LOVE, isStoryMarker(enemy.get()));
    }
    if (m_player) {
        // Walk sheets: one row per facing, in Player::Facing order.
        const Sprite* walk = sprites ? &sprites->memberSheet(*m_player, "walk", 4, 4) : nullptr;
        if (walk && walk->valid()) {
            // The world is frozen during a conversation, so stand still for it.
            const int frame = talkingNpc.empty() ? m_player->getWalkFrame() : Player::STANDING_FRAME;
            add(m_player->getX(), m_player->getY(), walk, Theme::TEXT, false,
                frame, static_cast<int>(m_player->getFacing()));
        } else {
            add(m_player->getX(), m_player->getY(), sprites ? &sprites->forMember(*m_player) : nullptr,
                Theme::TEXT, false);
        }
    }

    // Whoever stands further down the screen is drawn last, so they overlap
    // the ones behind. Sprites are taller than a tile.
    std::stable_sort(figures.begin(), figures.end(),
                     [](const Figure& a, const Figure& b) { return a.worldY < b.worldY; });

    for (const Figure& figure : figures) {
        const int feetY = figure.centreY + tileH / 2;
        if (figure.marker) {
            // Story fights stand out with a gold plate under the feet.
            Theme::fillRect(renderer, figure.centreX - tileW / 2, feetY - 10, tileW, 10, Theme::GOLD, 0.85f);
        }
        if (figure.sprite && figure.sprite->valid()) {
            // Whole-number scale, sized so a character is a little wider
            // than a tile and stands with their feet on its bottom edge.
            const int fw = figure.sprite->frameWidth();
            const int fh = figure.sprite->frameHeight();
            const int scale = std::max(1, static_cast<int>(std::lround(
                static_cast<float>(tileW) * 1.3f / static_cast<float>(fw > 0 ? fw : 1))));
            Theme::fillRect(renderer, figure.centreX - tileW * 3 / 10, feetY - 8, tileW * 6 / 10, 8,
                            0xFF000000, 0.35f);
            sprites->draw(*figure.sprite, figure.centreX - fw * scale / 2, feetY - 4 - fh * scale,
                          fw * scale, fh * scale, 1.0f, 1.0f, 1.0f, 1.0f, figure.col, figure.row);
        } else {
            const int size = tileW * 7 / 10;
            Theme::fillRect(renderer, figure.centreX - size / 2, figure.centreY - size / 2, size, size,
                            figure.fallback);
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
        // The marker died with the map's enemies; drop the rest of the group.
        m_storyMarker = nullptr;
        m_storyEscorts.clear();
        m_storyTag.clear();
        
        // Spawn new enemies based on the current biome
        spawnEnemiesForBiome(m_currentBiome, m_currentMap);
    }
}

void World::checkMapTransition() {
    if (!m_currentMap || !m_player) {
        return;
    }

    // Wait for the step to finish. Checking mid-step fires while the player is
    // still between tiles, and used to chain straight through the next map's
    // exit as the interrupted step kept interpolating.
    if (m_player->isMoving()) {
        return;
    }

    struct Exit {
        const char* from;
        int minX, maxX, minY, maxY;
        const char* to;
        BiomeType biome;
        float arriveX, arriveY;
        // Where to put the player if the destination is locked.
        float turnBackY;
    };
    static const Exit exits[] = {
        {"Neo-Kyoto Central", 14, 16, 0, 1,   "Neon Alley",        BiomeType::NEON_ALLEY,        15.0f, 23.0f, 2.0f},
        {"Neon Alley",        14, 16, 24, 24, "Neo-Kyoto Central", BiomeType::NEO_KYOTO_CENTRAL, 15.0f, 2.0f,  23.0f},
        {"Neon Alley",        14, 16, 0, 1,   "Corporate Plaza",   BiomeType::CORPORATE_PLAZA,   12.0f, 18.0f, 2.0f},
        {"Corporate Plaza",   11, 13, 19, 19, "Neon Alley",        BiomeType::NEON_ALLEY,        15.0f, 2.0f,  18.0f},
    };

    const int tileX = static_cast<int>(m_player->getX());
    const int tileY = static_cast<int>(m_player->getY());
    const std::string& here = m_currentMap->getName();

    for (const Exit& exit : exits) {
        if (here != exit.from) continue;
        if (tileX < exit.minX || tileX > exit.maxX || tileY < exit.minY || tileY > exit.maxY) continue;

        if (isMapLocked(exit.to)) {
            m_blockedMap = exit.to;
            m_player->teleport(static_cast<float>(tileX), exit.turnBackY);
            return;
        }

        std::cout << "Transitioning to " << exit.to << "..." << std::endl;
        // Biome first: changeMap spawns enemies from the current biome's table.
        // Move the player before spawning, too, so the spawn search keeps its
        // distance from where the player actually arrives.
        m_currentBiome = exit.biome;
        m_player->teleport(exit.arriveX, exit.arriveY);
        changeMap(exit.to);
        return;
    }
}

bool World::travelTo(const std::string& mapName, float x, float y) {
    struct Destination {
        const char* name;
        BiomeType biome;
        float defaultX, defaultY;
    };
    static const Destination destinations[] = {
        {"Neo-Kyoto Central", BiomeType::NEO_KYOTO_CENTRAL, 15.0f, 16.0f},
        {"Neon Alley",        BiomeType::NEON_ALLEY,        15.0f, 23.0f},
        {"Corporate Plaza",   BiomeType::CORPORATE_PLAZA,   12.0f, 18.0f},
    };

    for (const Destination& destination : destinations) {
        if (mapName != destination.name) continue;

        Map* target = nullptr;
        for (auto& map : m_maps) {
            if (map && map->getName() == mapName) target = map.get();
        }
        if (!target) return false;

        // A save taken mid-step holds a position between two tiles.
        float tileX = std::round(x);
        float tileY = std::round(y);
        const int ix = static_cast<int>(tileX);
        const int iy = static_cast<int>(tileY);
        bool standable = target->isTileWalkable(ix, iy);
        for (const auto& npc : target->getNPCs()) {
            if (npc && static_cast<int>(npc->getX()) == ix && static_cast<int>(npc->getY()) == iy) {
                standable = false;
            }
        }
        if (!standable) {
            tileX = destination.defaultX;
            tileY = destination.defaultY;
        }

        // Same order as a transition: biome and player first, then spawn.
        m_currentBiome = destination.biome;
        if (m_player) {
            m_player->teleport(tileX, tileY);
        }
        changeMap(mapName);
        return true;
    }
    return false;
}

void World::setMapLocked(const std::string& mapName, bool locked) {
    auto it = std::find(m_lockedMaps.begin(), m_lockedMaps.end(), mapName);
    if (locked && it == m_lockedMaps.end()) {
        m_lockedMaps.push_back(mapName);
    } else if (!locked && it != m_lockedMaps.end()) {
        m_lockedMaps.erase(it);
    }
}

bool World::isMapLocked(const std::string& mapName) const {
    return std::find(m_lockedMaps.begin(), m_lockedMaps.end(), mapName) != m_lockedMaps.end();
}

std::string World::consumeBlockedMap() {
    std::string blocked;
    blocked.swap(m_blockedMap);
    return blocked;
}

NPC* World::findNpcNearPlayer(float maxDistance) const {
    if (!m_currentMap || !m_player) {
        return nullptr;
    }
    NPC* best = nullptr;
    float bestDistance = maxDistance;
    for (const auto& npc : m_currentMap->getNPCs()) {
        if (!npc || !npc->isInteractable()) continue;
        float dx = m_player->getX() - npc->getX();
        float dy = m_player->getY() - npc->getY();
        float distance = std::sqrt(dx * dx + dy * dy);
        if (distance <= bestDistance) {
            bestDistance = distance;
            best = npc.get();
        }
    }
    return best;
}

bool World::isNpcAt(int tileX, int tileY) const {
    if (!m_currentMap) return false;
    for (const auto& npc : m_currentMap->getNPCs()) {
        if (npc && static_cast<int>(npc->getX()) == tileX && static_cast<int>(npc->getY()) == tileY) {
            return true;
        }
    }
    return false;
}

bool World::ensureStoryEncounter(const std::string& tag, float x, float y,
                                 const std::vector<std::pair<std::string, int>>& enemies) {
    if (!m_currentMap || enemies.empty()) {
        return false;
    }
    if (m_storyMarker && m_storyTag == tag) {
        return true;
    }

    // Build the whole group before touching the map, so an unknown type name
    // leaves nothing half-spawned.
    EncounterTable factory;
    std::vector<std::unique_ptr<Enemy>> group;
    for (const auto& entry : enemies) {
        auto enemy = factory.spawnEnemy({entry.first, 1, entry.second});
        if (!enemy) {
            std::cerr << "Unknown story enemy type: " << entry.first << std::endl;
            return false;
        }
        group.push_back(std::move(enemy));
    }

    // A wandering enemy on the marker's tile would start the wrong fight.
    const int tileX = static_cast<int>(x);
    const int tileY = static_cast<int>(y);
    std::vector<Enemy*> displaced;
    for (const auto& enemy : m_currentMap->getEnemies()) {
        if (static_cast<int>(enemy->getX()) == tileX && static_cast<int>(enemy->getY()) == tileY) {
            displaced.push_back(enemy.get());
        }
    }
    for (Enemy* enemy : displaced) {
        m_currentMap->removeEnemy(enemy);
    }

    group.front()->setPosition(x, y);
    m_storyMarker = group.front().get();
    m_storyTag = tag;
    m_currentMap->addEnemy(std::move(group.front()));

    m_storyEscorts.clear();
    for (std::size_t i = 1; i < group.size(); ++i) {
        m_storyEscorts.push_back(std::move(group[i]));
    }
    return true;
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
    m_activeEncounterTag.clear();

    if (triggered == m_storyMarker) {
        // The marker brings its escorts, and the fight is reported by tag.
        for (auto& escort : m_storyEscorts) {
            m_activeEncounter.push_back(std::move(escort));
        }
        m_storyEscorts.clear();
        m_activeEncounterTag = m_storyTag;
        m_storyMarker = nullptr;
        m_storyTag.clear();
    }

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

    // Ambushes are one or two enemies. Simulated battles (see AGENTS.md,
    // "Combat balance") showed groups of three snowball: the side with more
    // turns wins outright, with very little middle ground.
    auto group = table->spawnEncounter(1, 2);
    if (group.empty()) {
        return false;
    }

    std::cout << "Ambushed by " << group.size() << " hostile(s)!" << std::endl;
    m_activeEncounterTag.clear();
    for (auto& enemy : group) {
        m_activeEncounter.push_back(std::move(enemy));
    }
    m_encounterCooldown = ENCOUNTER_COOLDOWN_STEPS;
    return true;
}

void World::startEncounterWith(std::vector<std::unique_ptr<Enemy>> enemies) {
    m_activeEncounter = std::move(enemies);
    m_activeEncounterTag.clear();
}

void World::endEncounter() {
    m_activeEncounter.clear();
    m_activeEncounterTag.clear();
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
    plaza->addEntry("CombatCyborg", 25, 3);
    plaza->addEntry("CorporateEnforcer", 20, 3);
    plaza->addEntry("BlackICE", 15, 3);
    plaza->addEntry("CombatCyborg", 10, 4);
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
