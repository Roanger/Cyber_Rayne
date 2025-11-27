#pragma once

#include <string>

class NPC {
public:
    NPC(const std::string& name, float x, float y);
    ~NPC();

    // Getters
    const std::string& getName() const { return m_name; }
    float getX() const { return m_x; }
    float getY() const { return m_y; }

    // Setters
    void setPosition(float x, float y) { m_x = x; m_y = y; }

    // NPC behavior
    void update(float deltaTime);
    void render();

private:
    std::string m_name;
    float m_x;
    float m_y;
    // Add more NPC-specific properties here
};
