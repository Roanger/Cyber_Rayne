#include "../../include/InputPrompts.h"
#include "../../include/MenuSystem.h"
#include "../../include/VulkanRenderer.h"
#include "../../include/ui/BattleScene.h"
#include "../../include/ui/Theme.h"

#include <algorithm>
#include <cmath>
#include <iostream>

MenuSystem::MenuSystem()
    : m_selectedOption(MenuItem::START_GAME),
      m_optionSelected(false),
      m_highlightedIndex(0),
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
    // The title screen is drawn from shapes and text; only the font is loaded.
    if (renderer) {
        m_textRenderer->initialize(renderer);
    }
}

void MenuSystem::update(float deltaTime) {
    m_time += deltaTime;
}

void MenuSystem::render(VulkanRenderer* renderer) {
    if (!renderer) {
        return;
    }
    TextRenderer* text = m_textRenderer.get();
    const int screenW = static_cast<int>(renderer->getSwapchainExtent().width);
    const int screenH = static_cast<int>(renderer->getSwapchainExtent().height);

    // The city at night, the same skyline the battles use.
    BattleScene::drawBackdrop(renderer, screenW, screenH, "Neo-Kyoto Central");

    // Title: as large as fits, in whole-number steps so the pixels stay square.
    const std::string title = "CYBER RAYNE";
    int titleScale = std::clamp(screenW / (static_cast<int>(title.size()) * Theme::GLYPH + 16), 3, 12);
    const int titleW = text->measureText(title, titleScale);
    const int titleX = (screenW - titleW) / 2;
    const int titleY = screenH / 6;
    const int offset = std::max(2, titleScale / 2);
    text->renderText(renderer, title, titleX + offset, titleY + offset, titleScale, Theme::LOVE);
    text->renderText(renderer, title, titleX, titleY, titleScale, Theme::TEXT);

    const std::string tagline = "NEO-KYOTO NEVER SLEEPS";
    const int taglineY = titleY + titleScale * Theme::GLYPH + 24;
    text->renderText(renderer, tagline, (screenW - text->measureText(tagline, 2)) / 2, taglineY, 2, Theme::FOAM);

    // Options, in a panel in the lower half.
    const int rowH = 52;
    const int panelW = 380;
    const int panelH = static_cast<int>(m_menuOptions.size()) * rowH + 40;
    const int panelX = (screenW - panelW) / 2;
    const int panelY = std::max(taglineY + 80, screenH * 6 / 10 - panelH / 2);
    Theme::drawPanel(renderer, panelX, panelY, panelW, panelH);

    for (std::size_t i = 0; i < m_menuOptions.size(); ++i) {
        const bool selected = static_cast<int>(i) == m_highlightedIndex;
        const int rowY = panelY + 20 + static_cast<int>(i) * rowH;
        if (selected) {
            Theme::fillRect(renderer, panelX + 10, rowY, panelW - 20, rowH - 8, Theme::IRIS, 0.22f);
            // The cursor nudges back and forth so the eye finds it.
            const int nudge = static_cast<int>(std::sin(m_time * 6.0f) * 3.0f);
            text->renderText(renderer, ">", panelX + 28 + nudge, rowY + 12, 3, Theme::GOLD);
        }
        text->renderText(renderer, m_menuOptions[i], panelX + 70, rowY + 12, 3,
                         selected ? Theme::GOLD : Theme::TEXT);
    }

    const std::string hint = InputPrompts::upDown() + ": select   " + InputPrompts::confirm() + ": choose";
    text->renderText(renderer, hint, (screenW - text->measureText(hint, 2)) / 2,
                     std::min(screenH - 40, panelY + panelH + 24), 2, Theme::MUTED);
}

void MenuSystem::handleInput(InputAction action) {
    if (m_optionSelected) {
        return;
    }

    switch (action) {
        case InputAction::MOVE_UP:
            if (m_highlightedIndex > 0) {
                --m_highlightedIndex;
            }
            break;
        case InputAction::MOVE_DOWN:
            if (m_highlightedIndex + 1 < static_cast<int>(m_menuOptions.size())) {
                ++m_highlightedIndex;
            }
            break;
        case InputAction::CONFIRM:
            m_selectedOption = static_cast<MenuItem>(m_highlightedIndex);
            m_optionSelected = true;
            break;
        default:
            break;
    }
}
