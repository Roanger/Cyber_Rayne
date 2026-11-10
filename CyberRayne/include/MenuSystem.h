#pragma once

#include <vector>
#include <string>
#include <memory>
#include "InputAction.h"
#include "ui/UIButton.h"
#include "ui/UILabel.h"
#include "ui/UIRoot.h"
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
    void renderMenuBackground(VulkanRenderer* renderer);
    void renderMenuOptions(VulkanRenderer* renderer);
    void buildMenuButtons(VulkanRenderer* renderer);
    void updateSelection();
    void onConfirmOption(int index);

    std::vector<std::string> m_menuOptions;
    MenuItem m_selectedOption;
    bool m_optionSelected;
    int m_highlightedIndex;

    // Decorative images (kept for visual style)
    int m_backgroundTextureIndex;
    int m_nameBannerTextureIndex;

    // Reusable UI primitives
    std::unique_ptr<TextRenderer> m_textRenderer;
    UIRoot m_menuRoot;
    std::vector<UIButton*> m_menuButtons;
    // Window size the buttons were laid out for.
    int m_layoutW = 0;
    int m_layoutH = 0;
};
