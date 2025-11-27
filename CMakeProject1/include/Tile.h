#pragma once

#include <string>

class Tile {
public:
    enum class TileType {
        GRASS,
        WATER,
        MOUNTAIN,
        SAND,
        STONE,
        TREE,
        WALL,
        DOOR,
        FLOOR
    };

    Tile(TileType type, bool walkable = true);
    ~Tile();

    // Getters
    TileType getType() const { return m_type; }
    bool isWalkable() const { return m_walkable; }
    const std::string& getTexture() const { return m_texture; }

    // Setters
    void setWalkable(bool walkable) { m_walkable = walkable; }
    void setTexture(const std::string& texture) { m_texture = texture; }

private:
    TileType m_type;
    bool m_walkable;
    std::string m_texture;
};
