#include "../../include/UIManager.h"
#include "../../include/VulkanRenderer.h"
#include "../../include/Party.h"
#include "../../include/Player.h"
#include "../../include/Enemy.h"
#include "../../include/Combatant.h"
#include "../../include/ui/UIPanel.h"
#include "../../include/Ability.h"
#include "../../include/Item.h"
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
      m_actionSelected(false),
      m_menuIndex(0),
      m_textRenderer(std::make_unique<TextRenderer>()),
      m_currentActor(nullptr) {
    refreshEntries();
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

void UIManager::setCurrentActor(Combatant* actor) {
    if (actor != m_currentActor) {
        m_currentActor = actor;
        // A new actor means a new ability list, so start from the top level.
        m_battleState = BattleMenuState::MAIN_MENU;
        m_menuIndex = 0;
        m_pendingAbilityIndex = -1;
        m_pendingItemIndex = -1;
        refreshEntries();
    }
}

const Ability* UIManager::pendingAbility() const {
    if (!m_currentActor || m_pendingAbilityIndex < 0) {
        return nullptr;
    }
    const auto& abilities = m_currentActor->getAbilities();
    if (m_pendingAbilityIndex >= static_cast<int>(abilities.size())) {
        return nullptr;
    }
    return abilities[m_pendingAbilityIndex];
}

int UIManager::getMenuEntryCount() const {
    return static_cast<int>(m_entries.size());
}

void UIManager::refreshEntries() {
    m_entries.clear();

    switch (m_battleState) {
        case BattleMenuState::MAIN_MENU:
            // Flee was present in the action enum but had no button, so it was
            // unreachable from the UI.
            m_entries.push_back({"Attack", true});
            m_entries.push_back({"Ability", m_currentActor && !m_currentActor->getAbilities().empty()});
            m_entries.push_back({"Item", true});
            m_entries.push_back({"Defend", true});
            m_entries.push_back({"Flee", true});
            break;

        case BattleMenuState::ABILITY_SELECTION: {
            if (!m_currentActor) break;
            for (const Ability* ability : m_currentActor->getAbilities()) {
                if (!ability) continue;
                std::ostringstream oss;
                oss << ability->getName() << "  " << ability->getManaCost() << "MP";
                // Unaffordable abilities stay visible but greyed, so the player
                // can see what they are saving toward.
                m_entries.push_back({oss.str(), ability->canAfford(m_currentActor)});
            }
            break;
        }

        case BattleMenuState::ITEM_SELECTION: {
            if (!m_party) break;
            // Same order BattleSystem resolves itemIndex against.
            for (const Inventory::Stack* stack : m_party->getBag().getStacksOfType(Item::ItemType::CONSUMABLE)) {
                m_entries.push_back({stack->prototype->getName() + "  x" + std::to_string(stack->count), true});
            }
            break;
        }

        case BattleMenuState::TARGET_SELECTION:
            if (m_pendingItemIndex >= 0) {
                // Items go to allies, including the fallen. Entries line up
                // with party member indices.
                if (m_party) {
                    for (const auto& member : m_party->getMembers()) {
                        if (!member) continue;
                        std::ostringstream oss;
                        oss << member->getName() << "  ";
                        if (member->isAlive()) {
                            oss << member->getHealth() << "/" << member->getMaxHealth();
                        } else {
                            oss << "DOWN";
                        }
                        m_entries.push_back({oss.str(), true});
                    }
                }
                break;
            }
            for (Combatant* target : m_targets) {
                if (target && target->isAlive()) {
                    std::ostringstream oss;
                    oss << target->getName() << "  " << target->getHealth() << "/" << target->getMaxHealth();
                    m_entries.push_back({oss.str(), true});
                }
            }
            break;

        case BattleMenuState::WAITING:
            break;
    }

    if (m_menuIndex >= static_cast<int>(m_entries.size())) {
        m_menuIndex = m_entries.empty() ? 0 : static_cast<int>(m_entries.size()) - 1;
    }
    if (m_menuIndex < 0) {
        m_menuIndex = 0;
    }
    m_menuDirty = true;
}

void UIManager::commit(const ActionRequest& request) {
    m_request = request;
    m_actionSelected = true;
    m_battleState = BattleMenuState::MAIN_MENU;
    m_menuIndex = 0;
    m_pendingAbilityIndex = -1;
    m_pendingItemIndex = -1;
    refreshEntries();
}

void UIManager::cancelToMainMenu() {
    m_battleState = BattleMenuState::MAIN_MENU;
    m_menuIndex = 0;
    m_pendingAbilityIndex = -1;
    m_pendingItemIndex = -1;
    m_statusMessage.clear();
    refreshEntries();
}

void UIManager::confirmCurrentEntry() {
    if (m_menuIndex < 0 || m_menuIndex >= static_cast<int>(m_entries.size())) {
        return;
    }
    if (!m_entries[m_menuIndex].enabled) {
        m_statusMessage = "Not available.";
        return;
    }

    switch (m_battleState) {
        case BattleMenuState::MAIN_MENU:
            switch (m_menuIndex) {
                case 0:  // Attack
                    m_pendingAbilityIndex = -1;
                    m_battleState = BattleMenuState::TARGET_SELECTION;
                    m_menuIndex = 0;
                    refreshEntries();
                    break;
                case 1:  // Ability
                    m_battleState = BattleMenuState::ABILITY_SELECTION;
                    m_menuIndex = 0;
                    refreshEntries();
                    break;
                case 2:  // Item
                    m_battleState = BattleMenuState::ITEM_SELECTION;
                    m_menuIndex = 0;
                    refreshEntries();
                    break;
                case 3:  // Defend
                    commit(ActionRequest::defend());
                    break;
                case 4:  // Flee
                    commit(ActionRequest::flee());
                    break;
                default:
                    break;
            }
            break;

        case BattleMenuState::ABILITY_SELECTION: {
            m_pendingAbilityIndex = m_menuIndex;
            const Ability* ability = pendingAbility();
            if (!ability) {
                cancelToMainMenu();
                break;
            }
            // Only single-target abilities need a choice; area and self
            // abilities resolve immediately.
            if (ability->needsTargetChoice()) {
                m_battleState = BattleMenuState::TARGET_SELECTION;
                m_menuIndex = 0;
                refreshEntries();
            } else {
                commit(ActionRequest::ability(m_pendingAbilityIndex, -1));
            }
            break;
        }

        case BattleMenuState::ITEM_SELECTION:
            m_pendingItemIndex = m_menuIndex;
            m_pendingAbilityIndex = -1;
            m_battleState = BattleMenuState::TARGET_SELECTION;
            m_menuIndex = 0;
            refreshEntries();
            break;

        case BattleMenuState::TARGET_SELECTION:
            if (m_pendingItemIndex >= 0) {
                commit(ActionRequest::item(m_pendingItemIndex, m_menuIndex));
            } else if (m_pendingAbilityIndex >= 0) {
                commit(ActionRequest::ability(m_pendingAbilityIndex, m_menuIndex));
            } else {
                commit(ActionRequest::attack(m_menuIndex));
            }
            break;

        case BattleMenuState::WAITING:
            break;
    }
}

void UIManager::rebuildBattleMenu(VulkanRenderer* renderer) {
    if (!renderer) {
        return;
    }

    int screenH = static_cast<int>(renderer->getSwapchainExtent().height);
    const int gap = 20;
    const int rows = (static_cast<int>(m_entries.size()) + MENU_COLUMNS - 1) / MENU_COLUMNS;
    int startY = screenH - BUTTON_HEIGHT * (rows > 0 ? rows : 1) - MENU_START_Y_OFFSET;

    m_battleRoot.clear();
    m_battleButtons.clear();

    for (std::size_t i = 0; i < m_entries.size(); ++i) {
        int col = static_cast<int>(i) % MENU_COLUMNS;
        int row = static_cast<int>(i) / MENU_COLUMNS;
        int bx = MENU_START_X + col * (BUTTON_WIDTH + gap);
        int by = startY + row * (BUTTON_HEIGHT + gap);

        auto button = std::make_unique<UIButton>(bx, by, BUTTON_WIDTH, BUTTON_HEIGHT, m_entries[i].label);
        button->setColor(m_entries[i].enabled ? 0xCC333333 : 0xCC1A1A1A);
        button->setSelectedColor(0xFF6666FF);
        button->setLabelColor(m_entries[i].enabled ? 0xFFFFFFFF : 0xFF808080);
        button->setLabelScale(2);

        m_battleButtons.push_back(button.get());
        m_battleRoot.addWidget(std::move(button));
    }

    m_menuDirty = false;
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

    // Target selection lists the living enemies as of this frame.
    m_targets.clear();
    for (Combatant* enemy : enemies) {
        if (enemy && enemy->isAlive()) {
            m_targets.push_back(enemy);
        }
    }
    if (m_battleState == BattleMenuState::TARGET_SELECTION && m_pendingItemIndex < 0 &&
        static_cast<int>(m_targets.size()) != static_cast<int>(m_entries.size())) {
        refreshEntries();
    }

    if (m_battleButtons.empty() || m_menuDirty) {
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
    if (m_battleState == BattleMenuState::WAITING || m_entries.empty()) {
        return;
    }

    const int count = static_cast<int>(m_entries.size());

    // Generic grid walk driven by the entry count, rather than arithmetic
    // hardcoded to a fixed 2x2 layout.
    switch (action) {
        case InputAction::MOVE_UP:
            if (m_menuIndex - MENU_COLUMNS >= 0) {
                m_menuIndex -= MENU_COLUMNS;
                updateButtonSelection();
            }
            break;
        case InputAction::MOVE_DOWN:
            if (m_menuIndex + MENU_COLUMNS < count) {
                m_menuIndex += MENU_COLUMNS;
                updateButtonSelection();
            }
            break;
        case InputAction::MOVE_LEFT:
            if (m_menuIndex % MENU_COLUMNS != 0) {
                m_menuIndex -= 1;
                updateButtonSelection();
            }
            break;
        case InputAction::MOVE_RIGHT:
            if (m_menuIndex % MENU_COLUMNS != MENU_COLUMNS - 1 && m_menuIndex + 1 < count) {
                m_menuIndex += 1;
                updateButtonSelection();
            }
            break;
        case InputAction::CONFIRM:
            confirmCurrentEntry();
            break;
        case InputAction::CANCEL:
            // Back out of a submenu instead of leaving the player stuck in it.
            if (m_battleState != BattleMenuState::MAIN_MENU) {
                cancelToMainMenu();
            }
            break;
        default:
            break;
    }
}

void UIManager::resetAction() {
    m_actionSelected = false;
    m_request = ActionRequest{};
}

void UIManager::displayMessage(const std::string& message) {
    std::cout << "[UI] " << message << std::endl;
}
