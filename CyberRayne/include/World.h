#pragma once

#include <vector>
#include <memory>
#include <string>

class Map;
class Player;
class VulkanRenderer;

class World {
public:
    enum class BiomeType {
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
    
    // Enemy encounter
    bool checkEnemyEncounter();

private:
    void createMaps();
    void initializeBiomes();
    void spawnEnemiesForBiome(BiomeType biome, Map* map);

    std::vector<std::unique_ptr<Map>> m_maps;
    Map* m_currentMap;
    BiomeType m_currentBiome;
    Player* m_player;
};
