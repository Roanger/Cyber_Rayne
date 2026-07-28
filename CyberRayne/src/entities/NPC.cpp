#include "../../include/NPC.h"
#include "../../include/DialogueTree.h"
#include <iostream>

NPC::NPC(const std::string& name, float x, float y)
    : m_name(name), m_x(x), m_y(y), m_interactable(true) {
    std::cout << "NPC " << m_name << " created at position (" << m_x << ", " << m_y << ")" << std::endl;
}

NPC::~NPC() {
    std::cout << "NPC " << m_name << " destroyed" << std::endl;
}

void NPC::setDialogueTree(std::unique_ptr<DialogueTree> tree) {
    m_dialogueTree = std::move(tree);
}

void NPC::interact() {
    std::cout << "Interacting with " << m_name << std::endl;
    if (m_dialogueTree) {
        std::cout << m_name << " has a dialogue tree ready." << std::endl;
    } else {
        std::cout << m_name << " has nothing to say." << std::endl;
    }
}

void NPC::update(float deltaTime) {
}

void NPC::render() {
}
