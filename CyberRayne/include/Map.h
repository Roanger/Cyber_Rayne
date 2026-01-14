#pragma once

#include <vector>
#include <string>
#include <memory>
#include <map>
#include "Tile.h"

class Tile;
class Enemy;
class NPC;
class VulkanRenderer;

class Map {
public:
    Map(const std::string& name, int width, int height);
    ~Map();

    bool initialize();
#ifndef NO_VULKAN
    void loadTileTextures(VulkanRenderer* renderer);
#endif
    void update(float deltaTime);
#ifndef NO_VULKAN
    void render(VulkanRenderer* renderer);
#endif

    // Tile management
    Tile* getTile(int x, int y) const;
    bool isTileWalkable(int x, int y) const;
    void setTile(int x, int y, std::unique_ptr<Tile> tile);

    // Entity management
    void addEnemy(std::unique_ptr<Enemy> enemy);
    void removeEnemy(Enemy* enemy);
    const std::vector<std::unique_ptr<Enemy>>& getEnemies() const { return m_enemies; }

    void addNPC(std::unique_ptr<NPC> npc);
    void removeNPC(NPC* npc);
    const std::vector<std::unique_ptr<NPC>>& getNPCs() const { return m_npcs; }

    // Getters
    const std::string& getName() const { return m_name; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

private:
    std::string m_name;
    int m_width;
    int m_height;
    std::vector<std::unique_ptr<Tile>> m_tiles;
    std::vector<std::unique_ptr<Enemy>> m_enemies;
    std::vector<std::unique_ptr<NPC>> m_npcs;
    
    // Tile texture indices
    std::map<Tile::TileType, int> m_tileTextures;
};
