#include "../../include/NPC.h"
#include <iostream>

NPC::NPC(const std::string& name, float x, float y)
    : m_name(name), m_x(x), m_y(y) {
    std::cout << "NPC " << m_name << " created at position (" << m_x << ", " << m_y << ")" << std::endl;
}

NPC::~NPC() {
    std::cout << "NPC " << m_name << " destroyed" << std::endl;
}

void NPC::update(float deltaTime) {
    // In a real implementation, we would update NPC behavior here
    // For example, moving around, interacting with player, etc.
}

void NPC::render() {
    // In a real implementation, we would render the NPC using Vulkan
    // For now, we'll just have an empty implementation since NPCs are rendered in World::render
}
