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
    };

    CharacterSelectionSystem();
    ~CharacterSelectionSystem();

    bool initialize();
    void update(float deltaTime);
#ifndef NO_VULKAN
    void render(VulkanRenderer* renderer);
#else
    void render();
#endif
    
    // Character selection methods
    const std::vector<CharacterOption>& getCharacterOptions() const;
    void selectCharacter(int index);
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
    
    void initializeCharacterOptions();
};
