#include "../../include/UIManager.h"
#include "../../include/VulkanRenderer.h"
#include "../../include/Party.h"
#include "../../include/Player.h"
#include "../../include/Enemy.h"
#include "../../include/Combatant.h"
#include "../../include/ui/UIPanel.h"
#include <iostream>
#include <sstream>

namespace {

std::string buildHpLabel(const std::string& name, int current, int max) {
    std::ostringstream oss;
    oss << name << " " << current << "/" << max;
    return oss.str();
}

} // namespace

UIManager::UIManager()
    : m_battleState(BattleMenuState::MAIN_MENU),
      m_selectedAction(BattleAction::NONE),
      m_actionSelected(false),
      m_menuIndex(0),
      m_selectedTargetIndex(0),
      m_selectedSpellIndex(0),
      m_textRenderer(std::make_unique<TextRenderer>()),
      m_currentActor(nullptr) {
}

UIManager::~UIManager() = default;

void UIManager::initialize(VulkanRenderer* renderer) {
    if (renderer) {
        m_textRenderer->initialize(renderer);
        rebuildBattleMenu(renderer);
    }
}

void UIManager::loadTextures(VulkanRenderer* renderer) {
    if (renderer) {
        m_textRenderer->initialize(renderer);
    }
}

void UIManager::rebuildBattleMenu(VulkanRenderer* renderer) {
    if (!renderer) {
        return;
    }

    int screenH = static_cast<int>(renderer->getSwapchainExtent().height);
    int startY = screenH - BUTTON_HEIGHT * 2 - MENU_START_Y_OFFSET;
    const int gap = 20;

    const char* labels[] = {"Attack", "Magic", "Item", "Defend"};
    const BattleAction actions[] = {
        BattleAction::ATTACK,
        BattleAction::MAGIC,
        BattleAction::ITEM,
        BattleAction::DEFEND
    };

    m_battleRoot.clear();
    m_battleButtons.clear();

    for (int i = 0; i < 4; ++i) {
        int bx = MENU_START_X + (i % 2) * (BUTTON_WIDTH + gap);
        int by = startY + (i / 2) * (BUTTON_HEIGHT + gap);
        auto button = std::make_unique<UIButton>(bx, by, BUTTON_WIDTH, BUTTON_HEIGHT, labels[i]);
        button->setColor(0xCC333333);
        button->setSelectedColor(0xFF6666FF);
        button->setLabelColor(0xFFFFFFFF);
        button->setLabelScale(2);

        BattleAction action = actions[i];
        button->setOnConfirm([this, action]() {
            m_selectedAction = action;
            m_actionSelected = true;
        });

        m_battleButtons.push_back(button.get());
        m_battleRoot.addWidget(std::move(button));
    }

    updateButtonSelection();
}

void UIManager::updateButtonSelection() {
    for (std::size_t i = 0; i < m_battleButtons.size(); ++i) {
        if (m_battleButtons[i]) {
            m_battleButtons[i]->setSelected(static_cast<int>(i) == m_menuIndex);
        }
    }
}

void UIManager::renderBattleUI(VulkanRenderer* renderer, Party* party, const std::vector<Combatant*>& enemies) {
    if (!renderer) {
        return;
    }

    // Rebuild the menu on the first frame if the screen size is known.
    if (m_battleButtons.empty()) {
        rebuildBattleMenu(renderer);
    }

    if (m_battleState != BattleMenuState::WAITING) {
        renderBattleMenu(renderer);
    }

    renderStats(renderer, party, enemies);
}

void UIManager::renderBattleMenu(VulkanRenderer* renderer) {
    m_battleRoot.render(renderer, m_textRenderer.get());
}

void UIManager::renderStats(VulkanRenderer* renderer, Party* party, const std::vector<Combatant*>& enemies) {
    if (!renderer || !party) {
        return;
    }

    int screenW = static_cast<int>(renderer->getSwapchainExtent().width);
    int screenH = static_cast<int>(renderer->getSwapchainExtent().height);

    // Turn indicator at the top center
    std::string turnText = "Battle";
    if (m_currentActor) {
        turnText = m_currentActor->getName() + "'s turn";
    }
    if (!m_turnLabel) {
        m_turnLabel = std::make_unique<UILabel>(0, 8, turnText, 2);
    } else {
        m_turnLabel->setText(turnText);
    }
    int titleW = m_textRenderer->measureText(turnText, 2);
    m_turnLabel->setPosition((screenW - titleW) / 2, 8);
    m_turnLabel->render(renderer, m_textRenderer.get());

    // Party HP/MP/ATB bars on the bottom-right
    int barW = 320;
    int barH = 28;
    int barX = screenW - barW - 50;
    int startBarY = screenH - 130;
    int barGap = 70;

    const auto& members = party->getMembers();
    for (std::size_t i = 0; i < members.size(); ++i) {
        const Combatant* member = members[i].get();
        if (!member || !member->isAlive()) {
            continue;
        }
        int barY = startBarY - static_cast<int>(i) * barGap;

        UIProgressBar hpBar(barX, barY, barW, barH);
        hpBar.setValue(static_cast<float>(member->getHealth()) / member->getMaxHealth());
        hpBar.setFillColor(0xFF00CC00);
        hpBar.setLabel(buildHpLabel(member->getName(), member->getHealth(), member->getMaxHealth()));
        hpBar.render(renderer, m_textRenderer.get());

        UIProgressBar atbBar(barX, barY + barH + 4, barW, 8);
        atbBar.setValue(member->getAtbGauge());
        atbBar.setFillColor(0xFFFFFF00);
        atbBar.setLabel("");
        atbBar.render(renderer, m_textRenderer.get());
    }

    // Enemy HP bars stacked in the top-right
    int enemyBarW = 260;
    int enemyBarH = 24;
    int enemyX = screenW - enemyBarW - 30;
    int enemyY = 40;
    int enemyGap = 36;

    for (std::size_t i = 0; i < enemies.size(); ++i) {
        const Combatant* enemy = enemies[i];
        if (!enemy || !enemy->isAlive()) {
            continue;
        }
        float enemyHpPct = static_cast<float>(enemy->getHealth()) / enemy->getMaxHealth();
        UIProgressBar bar(enemyX, enemyY + static_cast<int>(i) * enemyGap, enemyBarW, enemyBarH);
        bar.setValue(enemyHpPct);
        bar.setFillColor(0xFFCC0000);
        bar.setLabel(buildHpLabel(enemy->getName(), enemy->getHealth(), enemy->getMaxHealth()));
        bar.render(renderer, m_textRenderer.get());
    }
}

void UIManager::handleInput(InputAction action) {
    // Only accept battle menu input when a player character is currently acting.
    if (!m_currentActor || !m_currentActor->isPlayer()) {
        return;
    }

    if (m_battleState == BattleMenuState::MAIN_MENU) {
        switch (action) {
            case InputAction::MOVE_UP:
                if (m_menuIndex >= 2) {
                    m_menuIndex -= 2;
                    updateButtonSelection();
                }
                break;
            case InputAction::MOVE_DOWN:
                if (m_menuIndex <= 1) {
                    m_menuIndex += 2;
                    updateButtonSelection();
                }
                break;
            case InputAction::MOVE_LEFT:
                if (m_menuIndex % 2 == 1) {
                    m_menuIndex -= 1;
                    updateButtonSelection();
                }
                break;
            case InputAction::MOVE_RIGHT:
                if (m_menuIndex % 2 == 0) {
                    m_menuIndex += 1;
                    updateButtonSelection();
                }
                break;
            case InputAction::CONFIRM:
                if (m_menuIndex >= 0 && m_menuIndex < static_cast<int>(m_battleButtons.size())) {
                    m_battleButtons[m_menuIndex]->handleInput(InputAction::CONFIRM);
                }
                break;
        }
    }
}

void UIManager::resetAction() {
    m_actionSelected = false;
    m_selectedAction = BattleAction::NONE;
}

void UIManager::displayMessage(const std::string& message) {
    std::cout << "[UI] " << message << std::endl;
}
