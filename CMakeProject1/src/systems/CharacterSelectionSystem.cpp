#include "../../include/CharacterSelectionSystem.h"
#include "../../include/Player.h"
#include <iostream>

#ifndef NO_VULKAN
#include "../../include/VulkanRenderer.h"
#endif

CharacterSelectionSystem::CharacterSelectionSystem() 
    : m_selectedIndex(0), m_characterSelected(false) {
    initializeCharacterOptions();
}

CharacterSelectionSystem::~CharacterSelectionSystem() {}

bool CharacterSelectionSystem::initialize() {
    std::cout << "Initializing character selection system..." << std::endl;
    return true;
}

void CharacterSelectionSystem::update(float deltaTime) {
    // In a real implementation, we would handle input for character selection
    // For now, we'll just print a message
}

#ifndef NO_VULKAN
void CharacterSelectionSystem::render(VulkanRenderer* renderer) {
    // In a real implementation, we would render the character selection UI
    // For now, we'll just print a message
    std::cout << "Rendering character selection UI" << std::endl;
    
    // Render a simple background for character selection
    renderer->renderSprite(0.0f, 0.0f, 2.0f, 2.0f);
    
    // Render character options
    for (size_t i = 0; i < m_characterOptions.size(); ++i) {
        // Render each character option
        float x = -0.5f + (i * 0.5f);
        float y = 0.0f;
        
        // Highlight selected character
        if (static_cast<int>(i) == m_selectedIndex) {
            // Render selection highlight
            renderer->renderSprite(x, y + 0.1f, 0.3f, 0.3f);
        }
        
        // Render character option
        renderer->renderSprite(x, y, 0.2f, 0.2f);
    }
}
#else
void CharacterSelectionSystem::render() {
    // In a real implementation, we would render the character selection UI
    // For now, we'll just print a message
    std::cout << "Rendering character selection UI" << std::endl;
    
    if (!m_characterSelected) {
        std::cout << "Choose your character:" << std::endl;
        for (size_t i = 0; i < m_characterOptions.size(); ++i) {
            const auto& option = m_characterOptions[i];
            std::cout << (i == m_selectedIndex ? "> " : "  ") 
                      << i << ". " << option.name << std::endl;
        }
        std::cout << "Use arrow keys to select, Enter to confirm" << std::endl;
    } else {
        std::cout << "Character selected: " << m_characterOptions[m_selectedIndex].name << std::endl;
    }
}
#endif

const std::vector<CharacterSelectionSystem::CharacterOption>& CharacterSelectionSystem::getCharacterOptions() const {
    return m_characterOptions;
}

void CharacterSelectionSystem::selectCharacter(int index) {
    if (index >= 0 && index < static_cast<int>(m_characterOptions.size())) {
        m_selectedIndex = index;
        m_characterSelected = true;
        
        const CharacterOption& selected = m_characterOptions[index];
        std::cout << "Selected character: " << selected.name << std::endl;
        std::cout << "Starter quest: " << selected.starterQuest << std::endl;
    }
}

Player* CharacterSelectionSystem::createSelectedCharacter() const {
    if (!m_characterSelected) {
        return nullptr;
    }
    
    const CharacterOption& selected = m_characterOptions[m_selectedIndex];
    return new Player(selected.charClass, selected.name);
}

const std::string& CharacterSelectionSystem::getStarterQuestForSelectedCharacter() const {
    static std::string empty = "";
    if (!m_characterSelected) {
        return empty;
    }
    
    return m_characterOptions[m_selectedIndex].starterQuest;
}

void CharacterSelectionSystem::initializeCharacterOptions() {
    m_characterOptions.clear();
    
    // Mage character
    CharacterOption mage;
    mage.charClass = Player::CharacterClass::MAGE;
    mage.name = "Elara";
    mage.description = "A powerful spellcaster who harnesses the elements.\n\nStrengths: High magic power, powerful spells\nWeaknesses: Low health, fragile in melee combat";
    mage.starterQuest = "Investigate the mysterious magical disturbances in the Whispering Woods. The ancient trees seem to be losing their magic, and the local villagers are worried. Find the source of the disturbance and restore balance to the forest.";
    m_characterOptions.push_back(mage);
    
    // Warrior character
    CharacterOption warrior;
    warrior.charClass = Player::CharacterClass::WARRIOR;
    warrior.name = "Thorn";
    warrior.description = "A strong warrior skilled in melee combat.\n\nStrengths: High health, strong physical attacks\nWeaknesses: Low magic power, slow movement";
    warrior.starterQuest = "The mining town of Ironhold is under threat from a pack of fierce wolves that have been driven mad by dark magic. Help the townspeople defend their home and find the source of the corruption.";
    m_characterOptions.push_back(warrior);
    
    // Rogue character
    CharacterOption rogue;
    rogue.charClass = Player::CharacterClass::ROGUE;
    rogue.name = "Shadow";
    rogue.description = "A nimble rogue who excels in stealth and speed.\n\nStrengths: High speed, critical strikes, stealth abilities\nWeaknesses: Moderate health, limited magic";
    rogue.starterQuest = "The merchant caravan that was supposed to deliver supplies to the frontier settlement of New Haven never arrived. Investigate what happened to the caravan and recover the lost supplies.";
    m_characterOptions.push_back(rogue);
    
    std::cout << "Initialized " << m_characterOptions.size() << " character options" << std::endl;
}
