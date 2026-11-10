#pragma once

#include <vector>
#include <string>
#include <memory>
#include "InputAction.h"
#include "ui/TextRenderer.h"

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
    void handleInput(InputAction action);

    // Getters
    MenuItem getSelectedOption() const { return m_selectedOption; }
    bool isOptionSelected() const { return m_optionSelected; }
    void resetSelection() { m_optionSelected = false; }

private:
    std::vector<std::string> m_menuOptions;
    MenuItem m_selectedOption;
    bool m_optionSelected;
    int m_highlightedIndex;
    // Seconds on screen, for the blinking cursor.
    float m_time = 0.0f;

    std::unique_ptr<TextRenderer> m_textRenderer;
};
