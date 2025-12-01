#pragma once

#include <string>
#include <vector>
#include <memory>
#include "Player.h"

#ifndef NO_VULKAN
class VulkanRenderer;
#endif

class CharacterSelectionSystem {
public:
    struct CharacterOption {
        Player::CharacterClass charClass;
        std::string name;
        std::string description;
        std::string starterQuest;
        int textureIndex;  // Index for this character's texture
    };

    CharacterSelectionSystem();
    ~CharacterSelectionSystem();

    bool initialize();
#ifndef NO_VULKAN
    void loadTextures(VulkanRenderer* renderer);  // Load character textures
#endif
    void update(float deltaTime);
    void handleInput(int key);  // Handle keyboard input
#ifndef NO_VULKAN
    void render(VulkanRenderer* renderer);
#else
    void render();
#endif
    
    // Character selection methods
    const std::vector<CharacterOption>& getCharacterOptions() const;
    void selectCharacter(int index);
    void confirmSelection();  // Confirm current selection
    Player* createSelectedCharacter() const;
    
    // Getters
    bool isCharacterSelected() const { return m_characterSelected; }
    int getSelectedIndex() const { return m_selectedIndex; }
    
    // Starter quest methods
    const std::string& getStarterQuestForSelectedCharacter() const;

private:
    std::vector<CharacterOption> m_characterOptions;
    int m_selectedIndex;
    bool m_characterSelected;
    
    // Input debouncing
    float m_inputCooldown;
    bool m_waitingForKeyRelease;
    
    // Texture indices
    int m_backgroundTextureIndex;
    int m_selectionFrameTextureIndex;
    int m_cursorTextureIndex;
    
    void initializeCharacterOptions();
};
