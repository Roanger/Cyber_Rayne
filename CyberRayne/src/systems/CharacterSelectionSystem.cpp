#include "../../include/CharacterSelectionSystem.h"
#include "../../include/Player.h"
#include <algorithm>
#include <cctype>
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
    
    std::string base = renderer->getAssetsBasePath();
    
    // Load background (reuse menu background)
    std::string bgPath = base + "/ui/menu_background.png";
    if (std::filesystem::exists(bgPath)) {
        m_backgroundTextureIndex = renderer->loadTexture(bgPath);
        std::cout << "Loaded character selection background: " << m_backgroundTextureIndex << std::endl;
    } else {
        std::cerr << "Character selection background not found: " << bgPath << std::endl;
    }
    
    // Load cursor texture
    std::string cursorPath = base + "/ui/cursor.png";
    if (std::filesystem::exists(cursorPath)) {
        m_cursorTextureIndex = renderer->loadTexture(cursorPath);
        std::cout << "Loaded character selection cursor: " << m_cursorTextureIndex << std::endl;
    } else {
        std::cerr << "Character selection cursor not found: " << cursorPath << std::endl;
    }
    
    // Derive each portrait from its own class rather than a positional list,
    // so reordering the options cannot silently mismatch the art.
    for (auto& option : m_characterOptions) {
        std::string className = Player::classToString(option.charClass);
        std::transform(className.begin(), className.end(), className.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        std::string texturePath = base + "/characters/" + className + ".png";

        if (std::filesystem::exists(texturePath)) {
            option.textureIndex = renderer->loadTexture(texturePath);
            std::cout << "Loaded character texture: " << texturePath
                      << " (index: " << option.textureIndex << ")" << std::endl;
        } else {
            option.textureIndex = -1;
            std::cerr << "Character texture not found: " << texturePath << std::endl;
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

void CharacterSelectionSystem::handleInput(InputAction action) {
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
    
    if (action == InputAction::MOVE_LEFT) {
        m_selectedIndex = (m_selectedIndex - 1 + static_cast<int>(m_characterOptions.size())) % m_characterOptions.size();
        std::cout << "Character selection: moved to " << m_characterOptions[m_selectedIndex].name << std::endl;
        m_inputCooldown = 0.2f;  // Debounce delay
    }
    else if (action == InputAction::MOVE_RIGHT) {
        m_selectedIndex = (m_selectedIndex + 1) % m_characterOptions.size();
        std::cout << "Character selection: moved to " << m_characterOptions[m_selectedIndex].name << std::endl;
        m_inputCooldown = 0.2f;  // Debounce delay
    }
    else if (action == InputAction::CONFIRM) {
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
    // Screen dimensions from swapchain
    const int screenW = static_cast<int>(renderer->getSwapchainExtent().width);
    const int screenH = static_cast<int>(renderer->getSwapchainExtent().height);
    
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

std::unique_ptr<Player> CharacterSelectionSystem::createSelectedCharacter() const {
    if (!m_characterSelected) {
        return nullptr;
    }
    
    const CharacterOption& selected = m_characterOptions[m_selectedIndex];
    return std::make_unique<Player>(selected.charClass, selected.name);
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
    
    // Option order must stay in sync with the texture path list in
    // loadTextures() — that list is indexed positionally, not by charClass.

    CharacterOption netrunner;
    netrunner.charClass = Player::CharacterClass::NETRUNNER;
    netrunner.name = "Cipher";
    netrunner.description = "A netrunner who turns hostile code into a weapon.\n\nStrengths: High tech power, breaches and disables machines\nWeaknesses: Low health, little use in a straight fight";
    netrunner.starterQuest = "Something is scraping the district's public net and rerouting it through a dead corporate relay. Trace the intrusion back through Neo-Kyoto Central and find out who is listening.";
    netrunner.textureIndex = -1;
    m_characterOptions.push_back(netrunner);

    CharacterOption solo;
    solo.charClass = Player::CharacterClass::SOLO;
    solo.name = "Rook";
    solo.description = "Street muscle wrapped in chrome and armor plate.\n\nStrengths: High health, heavy physical damage, holds a line\nWeaknesses: Low tech power, no answer to ranged threats";
    solo.starterQuest = "A crew of thugs has been shaking down vendors along the Central strip and the local fixers are done paying. Push them off the block and find out who is bankrolling them.";
    solo.textureIndex = -1;
    m_characterOptions.push_back(solo);

    CharacterOption infiltrator;
    infiltrator.charClass = Player::CharacterClass::INFILTRATOR;
    infiltrator.name = "Wisp";
    infiltrator.description = "A ghost who moves through security like it isn't there.\n\nStrengths: High speed, opening strikes, poisons and debuffs\nWeaknesses: Moderate health, fades in a long fight";
    infiltrator.starterQuest = "A courier went dark in Neon Alley with a data chip still on them. Get in past the gangs, find the body before corporate security does, and get the chip out.";
    infiltrator.textureIndex = -1;
    m_characterOptions.push_back(infiltrator);


    std::cout << "Initialized " << m_characterOptions.size() << " character options" << std::endl;
}
