#pragma once

#include <vector>
#include <string>

class VulkanRenderer;

class MenuSystem {
public:
    enum class MenuItem {
        START_GAME,
        LOAD_GAME,
        SETTINGS,
        QUIT
    };

    MenuSystem();
    ~MenuSystem();

    bool initialize();
    void loadTextures(VulkanRenderer* renderer);
    void update(float deltaTime);
    void render(VulkanRenderer* renderer);
    void handleInput(int key);

    // Getters
    MenuItem getSelectedOption() const { return m_selectedOption; }
    bool isOptionSelected() const { return m_optionSelected; }
    void resetSelection() { m_optionSelected = false; }

private:
    void renderMenuBackground(VulkanRenderer* renderer);
    void renderMenuOptions(VulkanRenderer* renderer);
    
    std::vector<std::string> m_menuOptions;
    MenuItem m_selectedOption;
    bool m_optionSelected;
    int m_highlightedIndex;
    
    // Texture indices for menu button images
    int m_backgroundTextureIndex;
    int m_nameBannerTextureIndex;
    int m_startButtonTextureIndex;
    int m_loadButtonTextureIndex;
    int m_settingsButtonTextureIndex;
    int m_quitButtonTextureIndex;
    int m_cursorTextureIndex;
    int m_highlightTextureIndex;
    
    // Positioning constants for menu rendering
    static constexpr float MENU_START_X = -0.8f;
    static constexpr float MENU_START_Y = 0.6f;
    static constexpr float MENU_OPTION_SPACING = 0.2f;
    static constexpr float HIGHLIGHT_OFFSET_X = -0.1f;
};
