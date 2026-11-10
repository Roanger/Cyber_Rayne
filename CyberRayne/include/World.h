#pragma once

#include <vector>
#include <memory>
#include <string>
#include <utility>

class Map;
class Player;
class Enemy;
class VulkanRenderer;
class EncounterTable;
class NPC;

class SpriteLibrary;
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
    // animTime: seconds, for idle animation. talkingNpc: name of the NPC in
    // conversation (shown with their talking animation), or empty.
    void render(VulkanRenderer* renderer, SpriteLibrary* sprites = nullptr,
                float animTime = 0.0f, const std::string& talkingNpc = "");
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
    // Start a fight against exactly these enemies. For development shortcuts
    // and tests; normal play goes through checkEnemyEncounter.
    void startEncounterWith(std::vector<std::unique_ptr<Enemy>> enemies);
    // Release the battle group. Call when the battle ends, however it ended.
    void endEncounter();

    // Story encounters: a fixed fight standing at a known tile. The first enemy
    // is visible on the map; the rest join when the player walks into it.
    // Does nothing if an encounter with this tag is already standing. Returns
    // false if any enemy type is unknown.
    bool ensureStoryEncounter(const std::string& tag, float x, float y,
                              const std::vector<std::pair<std::string, int>>& enemies);
    bool hasStoryEncounter() const { return m_storyMarker != nullptr; }
    bool isStoryMarker(const Enemy* enemy) const { return enemy != nullptr && enemy == m_storyMarker; }
    // Tag of the story encounter being fought, or empty for an ordinary battle.
    const std::string& getActiveEncounterTag() const { return m_activeEncounterTag; }

    // Locked maps cannot be entered; the player is turned back at the edge.
    void setMapLocked(const std::string& mapName, bool locked);
    bool isMapLocked(const std::string& mapName) const;
    // Name of the locked map the player just tried to enter. Empty if none;
    // cleared on read.
    std::string consumeBlockedMap();

    // NPCs
    // The closest interactable NPC within maxDistance tiles of the player.
    NPC* findNpcNearPlayer(float maxDistance) const;
    bool isNpcAt(int tileX, int tileY) const;

    // Steps to suppress random rolls after a battle, so wins don't chain.
    static constexpr int ENCOUNTER_COOLDOWN_STEPS = 8;
    // Keep at least this many visible enemies per district.
    static constexpr std::size_t MIN_MAP_ENEMIES = 2;
    // Enemies never spawn within this many tiles of the player.
    static constexpr float MIN_SPAWN_DISTANCE = 4.0f;

    // Map transitions
    void checkMapTransition();

    // Put the player on a named map at a tile, e.g. when loading a save. If the
    // tile is not somewhere a player can stand, the map's arrival point is used
    // instead. Returns false if there is no such map.
    bool travelTo(const std::string& mapName, float x, float y);

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
    std::string m_activeEncounterTag;

    // The standing story encounter on the current map, if any. The marker is
    // owned by the map; the escorts wait here until the fight starts.
    std::string m_storyTag;
    Enemy* m_storyMarker = nullptr;
    std::vector<std::unique_ptr<Enemy>> m_storyEscorts;

    std::vector<std::string> m_lockedMaps;
    std::string m_blockedMap;
    int m_encounterCooldown = ENCOUNTER_COOLDOWN_STEPS;
};
