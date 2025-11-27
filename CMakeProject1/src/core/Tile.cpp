#include "../../include/Tile.h"
#include <iostream>

Tile::Tile(TileType type, bool walkable)
    : m_type(type), m_walkable(walkable) {
    // Set default texture based on type
    switch (type) {
        case TileType::GRASS:
            m_texture = "grass.png";
            break;
        case TileType::WATER:
            m_texture = "water.png";
            m_walkable = false; // Water is not walkable by default
            break;
        case TileType::MOUNTAIN:
            m_texture = "mountain.png";
            m_walkable = false; // Mountains are not walkable by default
            break;
        case TileType::SAND:
            m_texture = "sand.png";
            break;
        case TileType::STONE:
            m_texture = "stone.png";
            break;
        case TileType::TREE:
            m_texture = "tree.png";
            m_walkable = false; // Trees are not walkable by default
            break;
        case TileType::WALL:
            m_texture = "wall.png";
            m_walkable = false; // Walls are not walkable
            break;
        case TileType::DOOR:
            m_texture = "door.png";
            // Doors can be walkable or not depending on state
            break;
        case TileType::FLOOR:
            m_texture = "floor.png";
            break;
        default:
            m_texture = "unknown.png";
            break;
    }
}

Tile::~Tile() {
    // Destructor
}
