#include "../../include/CharacterSelectionSystem.h"
#include "../../include/Player.h"
#include <iostream>
#include <filesystem>

#ifndef NO_VULKAN
#include "../../include/VulkanRenderer.h"
#endif

CharacterSelectionSystem::CharacterSelectionSystem() 
    : m_selectedIndex(0), m_characterSelected(false),
      m_inputCooldown(0.0f), m_waitingForKeyRelease(true),
      m_backgroundTextureIndex(-1), m_selectionFrameTextureIndex(-1), m_cursorTextureIndex(-1) {
    initializeCharacterOptions();
}

CharacterSelectionSystem::~CharacterSelectionSystem() {}

bool CharacterSelectionSystem::initialize() {
    std::cout << "Initializing character selection system..." << std::endl;
    return true;
}

#ifndef NO_VULKAN
void CharacterSelectionSystem::loadTextures(VulkanRenderer* renderer) {
    if (!renderer) return;
    
    std::string base = "assets";
    
    // Load background (reuse menu background)
    std::string bgPath = base + "/ui/menu_background.png";
    if (std::filesystem::exists(bgPath)) {
        m_backgroundTextureIndex = renderer->loadTexture(bgPath);
        std::cout << "Loaded character selection background: " << m_backgroundTextureIndex << std::endl;
    }
    
    // Load cursor texture
    std::string cursorPath = base + "/ui/cursor.png";
    if (std::filesystem::exists(cursorPath)) {
        m_cursorTextureIndex = renderer->loadTexture(cursorPath);
        std::cout << "Loaded character selection cursor: " << m_cursorTextureIndex << std::endl;
    }
    
    // Load character textures
    std::vector<std::string> charTextures = {
        base + "/mage.png",
        base + "/warrior.png", 
        base + "/rogue.png"
    };
    
    for (size_t i = 0; i < m_characterOptions.size() && i < charTextures.size(); ++i) {
        if (std::filesystem::exists(charTextures[i])) {
            m_characterOptions[i].textureIndex = renderer->loadTexture(charTextures[i]);
            std::cout << "Loaded character texture " << i << ": " << charTextures[i] 
                      << " (index: " << m_characterOptions[i].textureIndex << ")" << std::endl;
        } else {
            m_characterOptions[i].textureIndex = -1;
            std::cout << "Character texture not found: " << charTextures[i] << std::endl;
        }
    }
}
#endif

void CharacterSelectionSystem::update(float deltaTime) {
    // Update input cooldown
    if (m_inputCooldown > 0.0f) {
        m_inputCooldown -= deltaTime;
    }
}

void CharacterSelectionSystem::handleInput(int key) {
    // Key codes from Game.cpp:
    // 0 = UP, 1 = DOWN, 2 = ENTER, 3 = LEFT, 4 = RIGHT
    
    if (m_characterSelected) return;  // Already selected
    
    // Wait for all keys to be released first (prevents instant selection from menu)
    if (m_waitingForKeyRelease) {
        // key == -1 would mean no key, but we don't have that
        // Instead, we use a cooldown that starts when entering this screen
        if (m_inputCooldown <= 0.0f) {
            m_waitingForKeyRelease = false;
            m_inputCooldown = 0.3f;  // Small delay before accepting input
        }
        return;
    }
    
    // Check cooldown to prevent rapid input
    if (m_inputCooldown > 0.0f) return;
    
    if (key == 3) {  // Left arrow
        m_selectedIndex = (m_selectedIndex - 1 + static_cast<int>(m_characterOptions.size())) % m_characterOptions.size();
        std::cout << "Character selection: moved to " << m_characterOptions[m_selectedIndex].name << std::endl;
        m_inputCooldown = 0.2f;  // Debounce delay
    }
    else if (key == 4) {  // Right arrow
        m_selectedIndex = (m_selectedIndex + 1) % m_characterOptions.size();
        std::cout << "Character selection: moved to " << m_characterOptions[m_selectedIndex].name << std::endl;
        m_inputCooldown = 0.2f;  // Debounce delay
    }
    else if (key == 2) {  // Enter key
        confirmSelection();
    }
}

void CharacterSelectionSystem::confirmSelection() {
    if (!m_characterSelected && m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_characterOptions.size())) {
        m_characterSelected = true;
        std::cout << "Character confirmed: " << m_characterOptions[m_selectedIndex].name << std::endl;
    }
}

#ifndef NO_VULKAN
void CharacterSelectionSystem::render(VulkanRenderer* renderer) {
    std::cout << "Rendering character selection UI" << std::endl;
    
    // Screen dimensions (1920x1080)
    const int screenW = 1920;
    const int screenH = 1080;
    
    // Render background
    if (m_backgroundTextureIndex >= 0) {
        renderer->renderSpritePixelsWithTexture(0, 0, screenW, screenH, m_backgroundTextureIndex);
    }
    
    // Character card layout - 3 characters horizontally centered
    const int cardW = 300;
    const int cardH = 400;
    const int cardSpacing = 100;
    const int totalWidth = (cardW * 3) + (cardSpacing * 2);
    const int startX = (screenW - totalWidth) / 2;
    const int cardY = (screenH - cardH) / 2 - 50;  // Slightly above center
    
    // Cursor dimensions
    const int cursorW = 64;
    const int cursorH = 64;
    
    // Render each character option and cursor for selected one
    for (size_t i = 0; i < m_characterOptions.size(); ++i) {
        int cardX = startX + static_cast<int>(i) * (cardW + cardSpacing);
        
        // Render character portrait
        if (m_characterOptions[i].textureIndex >= 0) {
            renderer->renderSpritePixelsWithTexture(cardX, cardY, cardW, cardH, m_characterOptions[i].textureIndex);
        }
        
        // Draw cursor next to selected character
        if (static_cast<int>(i) == m_selectedIndex && m_cursorTextureIndex >= 0) {
            // Position cursor to the left of the selected card
            int cursorX = cardX - cursorW - 10;
            int cursorY = cardY + (cardH / 2) - (cursorH / 2);  // Center vertically
            renderer->renderSpritePixelsWithTexture(cursorX, cursorY, cursorW, cursorH, m_cursorTextureIndex);
        }
    }
    
    // Debug output for navigation
    std::cout << "Selected character: " << m_selectedIndex << " (" 
              << m_characterOptions[m_selectedIndex].name << ")" << std::endl;
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
    mage.textureIndex = -1;
    m_characterOptions.push_back(mage);
    
    // Warrior character
    CharacterOption warrior;
    warrior.charClass = Player::CharacterClass::WARRIOR;
    warrior.name = "Thorn";
    warrior.description = "A strong warrior skilled in melee combat.\n\nStrengths: High health, strong physical attacks\nWeaknesses: Low magic power, slow movement";
    warrior.starterQuest = "The mining town of Ironhold is under threat from a pack of fierce wolves that have been driven mad by dark magic. Help the townspeople defend their home and find the source of the corruption.";
    warrior.textureIndex = -1;
    m_characterOptions.push_back(warrior);
    
    // Rogue character
    CharacterOption rogue;
    rogue.charClass = Player::CharacterClass::ROGUE;
    rogue.name = "Shadow";
    rogue.description = "A nimble rogue who excels in stealth and speed.\n\nStrengths: High speed, critical strikes, stealth abilities\nWeaknesses: Moderate health, limited magic";
    rogue.starterQuest = "The merchant caravan that was supposed to deliver supplies to the frontier settlement of New Haven never arrived. Investigate what happened to the caravan and recover the lost supplies.";
    rogue.textureIndex = -1;
    m_characterOptions.push_back(rogue);
    
    std::cout << "Initialized " << m_characterOptions.size() << " character options" << std::endl;
}
