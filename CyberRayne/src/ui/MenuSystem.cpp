#include "../../include/MenuSystem.h"
#include "../../include/VulkanRenderer.h"
#include "../../include/ui/UIPanel.h"
#include <iostream>
#include <filesystem>

MenuSystem::MenuSystem()
    : m_selectedOption(MenuItem::START_GAME),
      m_optionSelected(false),
      m_highlightedIndex(0),
      m_backgroundTextureIndex(-1),
      m_nameBannerTextureIndex(-1),
      m_textRenderer(std::make_unique<TextRenderer>()) {
    m_menuOptions = {
        "Start Game",
        "Load Game",
        "Settings",
        "Quit"
    };
}

MenuSystem::~MenuSystem() = default;

bool MenuSystem::initialize() {
    std::cout << "Menu system initialized." << std::endl;
    return true;
}

void MenuSystem::loadTextures(VulkanRenderer* renderer) {
    if (!renderer) {
        return;
    }

    std::cout << "Loading menu textures..." << std::endl;

    const std::string base = renderer->getAssetsBasePath();
    auto exists = [](const std::string& p) { return std::filesystem::exists(p); };

    const std::string bgPath = base + "/ui/menu_background.png";
    if (exists(bgPath)) {
        m_backgroundTextureIndex = renderer->loadTexture(bgPath);
    } else {
        std::cerr << "Background texture missing at: " << bgPath << std::endl;
    }

    const std::string bannerPath = base + "/ui/namebanner.png";
    if (exists(bannerPath)) {
        m_nameBannerTextureIndex = renderer->loadTexture(bannerPath);
    } else {
        std::cerr << "Name banner texture missing at: " << bannerPath << std::endl;
    }

    m_textRenderer->initialize(renderer);
    buildMenuButtons(renderer);
}

void MenuSystem::buildMenuButtons(VulkanRenderer* renderer) {
    if (!renderer) {
        return;
    }

    int screenW = static_cast<int>(renderer->getSwapchainExtent().width);
    int screenH = static_cast<int>(renderer->getSwapchainExtent().height);

    const int buttonW = 220;
    const int buttonH = 60;
    const int gapY = 16;

    m_menuRoot.clear();
    m_menuButtons.clear();

    // Center the vertical stack of buttons near the bottom
    int startX = (screenW - buttonW) / 2;
    int startY = screenH - (static_cast<int>(m_menuOptions.size()) * (buttonH + gapY)) - 80;

    for (std::size_t i = 0; i < m_menuOptions.size(); ++i) {
        auto button = std::make_unique<UIButton>(startX, startY + static_cast<int>(i) * (buttonH + gapY),
                                                  buttonW, buttonH, m_menuOptions[i]);
        button->setColor(0xCC222244);
        button->setSelectedColor(0xFF6666FF);
        button->setLabelScale(2);

        MenuItem option = static_cast<MenuItem>(i);
        button->setOnConfirm([this, option]() {
            m_selectedOption = option;
            m_optionSelected = true;
        });

        m_menuButtons.push_back(button.get());
        m_menuRoot.addWidget(std::move(button));
    }

    updateSelection();
}

void MenuSystem::updateSelection() {
    for (std::size_t i = 0; i < m_menuButtons.size(); ++i) {
        if (m_menuButtons[i]) {
            m_menuButtons[i]->setSelected(static_cast<int>(i) == m_highlightedIndex);
        }
    }
}

void MenuSystem::update(float /*deltaTime*/) {
    // No-op; input handled via handleInput
}

void MenuSystem::render(VulkanRenderer* renderer) {
    if (!renderer) {
        return;
    }
    renderMenuBackground(renderer);

    if (m_menuButtons.empty()) {
        buildMenuButtons(renderer);
    }

    m_menuRoot.render(renderer, m_textRenderer.get());
}

void MenuSystem::renderMenuBackground(VulkanRenderer* renderer) {
    int screenW = static_cast<int>(renderer->getSwapchainExtent().width);
    int screenH = static_cast<int>(renderer->getSwapchainExtent().height);

    if (m_backgroundTextureIndex >= 0) {
        renderer->renderSpritePixelsWithTexture(0, 0, screenW, screenH, m_backgroundTextureIndex);
    } else {
        renderer->renderColoredRectPixels(0, 0, screenW, screenH, 0.05f, 0.05f, 0.1f, 1.0f);
    }

    if (m_nameBannerTextureIndex >= 0) {
        const int bannerW = 450;
        const int bannerH = 230;
        renderer->renderSpritePixelsWithTexture(50, 50, bannerW, bannerH, m_nameBannerTextureIndex);
    }
}

void MenuSystem::handleInput(InputAction action) {
    if (m_optionSelected) {
        return;
    }

    switch (action) {
        case InputAction::MOVE_UP:
            if (m_highlightedIndex > 0) {
                --m_highlightedIndex;
                updateSelection();
            }
            break;
        case InputAction::MOVE_DOWN:
            if (m_highlightedIndex + 1 < static_cast<int>(m_menuOptions.size())) {
                ++m_highlightedIndex;
                updateSelection();
            }
            break;
        case InputAction::CONFIRM:
            if (m_highlightedIndex >= 0 && m_highlightedIndex < static_cast<int>(m_menuButtons.size())) {
                m_menuButtons[m_highlightedIndex]->handleInput(InputAction::CONFIRM);
            }
            break;
    }
}
