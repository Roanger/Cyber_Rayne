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
    // Detach an enemy and transfer ownership to the caller. Returns nullptr if
    // the enemy is not on this map.
    std::unique_ptr<Enemy> takeEnemy(Enemy* enemy);
    void clearEnemies();
    const std::vector<std::unique_ptr<Enemy>>& getEnemies() const { return m_enemies; }

    void addNPC(std::unique_ptr<NPC> npc);
    void removeNPC(NPC* npc);
    const std::vector<std::unique_ptr<NPC>>& getNPCs() const { return m_npcs; }

    // Getters
    const std::string& getName() const { return m_name; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

    // Viewport tracking — set player position so render can center on it
    void setPlayerViewport(float x, float y) { m_playerX = x; m_playerY = y; }
    void clearPlayerViewport() { m_playerX = -1; m_playerY = -1; }

    // The visible tile window, in tiles, plus tile size in normalized device
    // coordinates. Anything drawing world-space entities must go through this
    // so tiles and entities share one coordinate system.
    struct Viewport {
        int startX = 0;
        int startY = 0;
        int widthTiles = 0;
        int heightTiles = 0;
        float tileWidth = 0.0f;   // NDC
        float tileHeight = 0.0f;  // NDC
    };

    // screenAspect is width/height of the render target.
    Viewport getViewport(float screenAspect) const;

    // Convert a world tile position to NDC. Returns false when the position
    // falls outside the visible window, so callers can cull.
    bool worldToNdc(const Viewport& viewport, float worldX, float worldY,
                    float& ndcX, float& ndcY) const;

    static constexpr int VIEWPORT_HEIGHT_TILES = 15;

private:
    std::string m_name;
    int m_width;
    int m_height;
    std::vector<std::unique_ptr<Tile>> m_tiles;
    std::vector<std::unique_ptr<Enemy>> m_enemies;
    std::vector<std::unique_ptr<NPC>> m_npcs;
    
    // Tile texture indices
    std::map<Tile::TileType, int> m_tileTextures;

    // Player viewport center (-1 = not set, center on map instead)
    float m_playerX = -1;
    float m_playerY = -1;
};
