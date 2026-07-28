#pragma once

#include <vector>
#include <memory>
#include <string>

class Map;
class Player;
class Enemy;
class VulkanRenderer;
class EncounterTable;

class World {
public:
    enum class BiomeType {
        // Cyberpunk districts
        NEO_KYOTO_CENTRAL,
        NEON_ALLEY,
        CORPORATE_PLAZA,
        // Legacy biomes (kept for compatibility)
        FOREST,
        MOUNTAIN,
        DESERT,
        SWAMP,
        TOWN,
        DUNGEON
    };

    World();
    ~World();

    bool initialize();
    void loadMapTextures(VulkanRenderer* renderer);
    void update(float deltaTime);
    void render(VulkanRenderer* renderer);
    void spawnEnemies();

    // Map management
    void loadMap(const std::string& mapName);
    void changeMap(const std::string& mapName);
    void setCurrentMap(Map* map) { m_currentMap = map; }

    // Getters
    Map* getCurrentMap() const { return m_currentMap; }
    BiomeType getCurrentBiome() const { return m_currentBiome; }
    void setCurrentBiome(BiomeType biome) { m_currentBiome = biome; }
    void setPlayer(Player* player) { m_player = player; }
    Player* getPlayer() const { return m_player; }
    const std::vector<std::unique_ptr<Map>>& getMaps() const { return m_maps; }

    // Encounters
    //
    // Both encounter styles funnel into the same place: on success the
    // participating enemies are moved off the map into m_activeEncounter, which
    // owns them for the duration of the battle. Taking them off the map is what
    // stops a won fight from immediately retriggering on the same tile.

    // Walking into a visible on-map enemy.
    bool checkEnemyEncounter();

    // Call once per completed player step. Rolls the zone's random encounter
    // rate; returns true when a random battle should begin.
    bool onPlayerStep();

    // Enemies participating in the battle that just started.
    const std::vector<std::unique_ptr<Enemy>>& getActiveEncounter() const { return m_activeEncounter; }
    bool hasActiveEncounter() const { return !m_activeEncounter.empty(); }
    // Release the battle group. Call when the battle ends, however it ended.
    void endEncounter();

    // Steps to suppress random rolls after a battle, so wins don't chain.
    static constexpr int ENCOUNTER_COOLDOWN_STEPS = 8;
    // Keep at least this many visible enemies per district.
    static constexpr std::size_t MIN_MAP_ENEMIES = 2;
    // Enemies never spawn within this many tiles of the player.
    static constexpr float MIN_SPAWN_DISTANCE = 4.0f;

    // Map transitions
    void checkMapTransition();

    // Encounter table access
    const EncounterTable* getEncounterTable(BiomeType biome) const;

private:
    void createMaps();
    void initializeBiomes();
    void spawnEnemiesForBiome(BiomeType biome, Map* map);
    void initializeEncounterTables();

    // Find a walkable tile that is unoccupied and at least minDistance from the
    // player. Returns false if no such tile exists.
    bool findSpawnTile(Map* map, float minDistance, float& outX, float& outY) const;
    // True if any NPC or enemy already stands on this tile.
    bool isTileOccupied(Map* map, int x, int y) const;

    std::vector<std::unique_ptr<Map>> m_maps;
    Map* m_currentMap;
    BiomeType m_currentBiome;
    Player* m_player;

    std::vector<std::unique_ptr<EncounterTable>> m_encounterTables;

    std::vector<std::unique_ptr<Enemy>> m_activeEncounter;
    int m_encounterCooldown = ENCOUNTER_COOLDOWN_STEPS;
};
