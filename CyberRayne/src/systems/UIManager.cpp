#include "../../include/InputPrompts.h"
#include "../../include/UIManager.h"
#include "../../include/VulkanRenderer.h"
#include "../../include/Party.h"
#include "../../include/Player.h"
#include "../../include/Enemy.h"
#include "../../include/Combatant.h"
#include "../../include/ui/BattleFx.h"
#include "../../include/ui/BattleScene.h"
#include "../../include/ui/Theme.h"
#include "../../include/Ability.h"
#include "../../include/Item.h"
#include <iostream>
#include <sstream>


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
            for (Combatant* target : (targetingAllies() ? m_allies : m_targets)) {
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

bool UIManager::targetingAllies() const {
    if (m_pendingItemIndex >= 0) return true;
    const Ability* ability = pendingAbility();
    return ability && ability->getTarget() == Ability::Target::SINGLE_ALLY;
}

const Combatant* UIManager::getHighlightedTarget() const {
    if (m_battleState != BattleMenuState::TARGET_SELECTION || m_menuIndex < 0) {
        return nullptr;
    }
    const std::size_t index = static_cast<std::size_t>(m_menuIndex);
    if (m_pendingItemIndex >= 0) {
        // Items index every party member, standing or not.
        if (!m_party || index >= m_party->getMembers().size()) return nullptr;
        return m_party->getMembers()[index].get();
    }
    const auto& side = targetingAllies() ? m_allies : m_targets;
    return index < side.size() ? side[index] : nullptr;
}

void UIManager::renderBattleUI(VulkanRenderer* renderer, Party* party,
                               const std::vector<Combatant*>& enemies,
                               const std::vector<Combatant*>& allies) {
    if (!renderer) {
        return;
    }

    // Both lists are the living combatants in the order BattleSystem resolves
    // a target index against.
    m_targets = enemies;
    m_allies = allies;
    if (m_battleState == BattleMenuState::TARGET_SELECTION && m_pendingItemIndex < 0) {
        const auto& side = targetingAllies() ? m_allies : m_targets;
        if (side.size() != m_entries.size()) {
            refreshEntries();
        }
    }

    const int screenW = static_cast<int>(renderer->getSwapchainExtent().width);
    const int screenH = static_cast<int>(renderer->getSwapchainExtent().height);
    const int margin = 16;
    const int top = screenH - BattleLayout::PANEL_HEIGHT + margin / 2;
    const int height = BattleLayout::PANEL_HEIGHT - margin - margin / 2;
    const int commandW = (screenW - 3 * margin) * 44 / 100;
    const int statusX = margin * 2 + commandW;

    Theme::fillRect(renderer, 0, screenH - BattleLayout::PANEL_HEIGHT, screenW,
                    BattleLayout::PANEL_HEIGHT, Theme::BASE);
    renderCommandPanel(renderer, margin, top, commandW, height);
    renderPartyPanel(renderer, party, statusX, top, screenW - statusX - margin, height);
}

void UIManager::renderCommandPanel(VulkanRenderer* renderer, int x, int y, int w, int h) {
    TextRenderer* text = m_textRenderer.get();
    Theme::drawPanel(renderer, x, y, w, h);

    const int pad = 20;
    const bool playerTurn = m_currentActor && m_currentActor->isPlayer() &&
                            m_battleState != BattleMenuState::WAITING;

    // Header: whose turn, and what is being chosen.
    std::string header = "Waiting...";
    if (playerTurn) {
        header = m_currentActor->getName();
        switch (m_battleState) {
            case BattleMenuState::ABILITY_SELECTION: header += " - Ability"; break;
            case BattleMenuState::ITEM_SELECTION:    header += " - Item"; break;
            case BattleMenuState::TARGET_SELECTION:  header += " - Choose target"; break;
            default: break;
        }
    }
    text->renderText(renderer, header, x + pad, y + pad, 2, playerTurn ? Theme::GOLD : Theme::MUTED);
    if (!m_statusMessage.empty()) {
        const int msgW = text->measureText(m_statusMessage, 2);
        text->renderText(renderer, m_statusMessage, x + w - pad - msgW, y + pad, 2, Theme::LOVE);
    }
    Theme::fillRect(renderer, x + pad, y + pad + 26, w - 2 * pad, 2, Theme::IRIS, 0.5f);

    if (!playerTurn) {
        return;
    }
    if (m_entries.empty()) {
        text->renderText(renderer, "Nothing here. " + InputPrompts::cancel() + " to go back.", x + pad, y + pad + 44, 2, Theme::MUTED);
        return;
    }

    const int rowH = 40;
    const int colW = (w - 2 * pad) / MENU_COLUMNS;
    const int listY = y + pad + 40;
    for (std::size_t i = 0; i < m_entries.size(); ++i) {
        const int col = static_cast<int>(i) % MENU_COLUMNS;
        const int row = static_cast<int>(i) / MENU_COLUMNS;
        const int ex = x + pad + col * colW;
        const int ey = listY + row * rowH;
        const bool selected = static_cast<int>(i) == m_menuIndex;
        uint32_t color = m_entries[i].enabled ? Theme::TEXT : Theme::MUTED;
        if (selected) {
            Theme::fillRect(renderer, ex, ey, colW - 8, rowH - 6, Theme::IRIS, m_entries[i].enabled ? 1.0f : 0.45f);
            color = Theme::BASE;
        }
        text->renderText(renderer, m_entries[i].label, ex + 10, ey + 9, 2, color);
    }
}

void UIManager::renderPartyPanel(VulkanRenderer* renderer, Party* party, int x, int y, int w, int h) {
    TextRenderer* text = m_textRenderer.get();
    Theme::drawPanel(renderer, x, y, w, h);
    if (!party) {
        return;
    }

    auto bar = [&](int bx, int by, int bw, int bh, float fraction, uint32_t color) {
        if (fraction < 0.0f) fraction = 0.0f;
        if (fraction > 1.0f) fraction = 1.0f;
        Theme::fillRect(renderer, bx, by, bw, bh, Theme::MUTED, 0.3f);
        Theme::fillRect(renderer, bx, by, static_cast<int>(static_cast<float>(bw) * fraction), bh, color);
    };

    const int pad = 20;
    const auto& members = party->getMembers();
    const int rowH = (h - 2 * pad) / 3;
    const int nameW = 150;
    // What is left after the name is split between HP, MP and the turn gauge.
    const int rest = w - 2 * pad - nameW;
    const int gap = 24;
    const int hpW = (rest - 2 * gap) * 40 / 100;
    const int mpW = (rest - 2 * gap) * 32 / 100;
    const int atbW = rest - 2 * gap - hpW - mpW;

    for (std::size_t i = 0; i < members.size() && i < 3; ++i) {
        const Player* member = members[i].get();
        if (!member) continue;
        const int ry = y + pad + static_cast<int>(i) * rowH;
        const bool acting = member == m_currentActor;
        const bool down = !member->isAlive();

        if (acting) {
            Theme::fillRect(renderer, x + 8, ry - 4, w - 16, rowH - 6, Theme::IRIS, 0.16f);
        }
        text->renderText(renderer, member->getName(), x + pad, ry + 6, 2,
                         down ? Theme::MUTED : (acting ? Theme::GOLD : Theme::TEXT));

        // Status tags sit under the name.
        int tagX = x + pad;
        for (const auto& effect : member->getStatusEffects()) {
            if (!effect || effect->isExpired() || down) continue;
            const StatusTag tag = statusTagFor(effect->getType());
            tagX += text->renderText(renderer, tag.text, tagX, ry + 30, 1, tag.color) + 8;
        }

        int cx = x + pad + nameW;
        if (down) {
            text->renderText(renderer, "DOWN", cx, ry + 6, 2, Theme::LOVE);
            continue;
        }

        const float hp = static_cast<float>(member->getHealth()) /
                         static_cast<float>(member->getMaxHealth() > 0 ? member->getMaxHealth() : 1);
        const uint32_t hpColor = hp < 0.25f ? Theme::LOVE : (hp < 0.5f ? Theme::GOLD : Theme::FOAM);
        text->renderText(renderer, "HP " + std::to_string(member->getHealth()) + "/" +
                                   std::to_string(member->getMaxHealth()), cx, ry + 2, 2, Theme::TEXT);
        bar(cx, ry + 24, hpW, 10, hp, hpColor);

        cx += hpW + gap;
        const float mp = static_cast<float>(member->getMana()) /
                         static_cast<float>(member->getMaxMana() > 0 ? member->getMaxMana() : 1);
        text->renderText(renderer, "MP " + std::to_string(member->getMana()) + "/" +
                                   std::to_string(member->getMaxMana()), cx, ry + 2, 2, Theme::TEXT);
        bar(cx, ry + 24, mpW, 10, mp, Theme::IRIS);

        cx += mpW + gap;
        const bool ready = member->isAtbFull();
        text->renderText(renderer, ready ? "READY" : "TIME", cx, ry + 2, 2, ready ? Theme::GOLD : Theme::MUTED);
        bar(cx, ry + 24, atbW, 10, member->getAtbGauge(), ready ? Theme::GOLD : Theme::ROSE);
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
            }
            break;
        case InputAction::MOVE_DOWN:
            if (m_menuIndex + MENU_COLUMNS < count) {
                m_menuIndex += MENU_COLUMNS;
            }
            break;
        case InputAction::MOVE_LEFT:
            if (m_menuIndex % MENU_COLUMNS != 0) {
                m_menuIndex -= 1;
            }
            break;
        case InputAction::MOVE_RIGHT:
            if (m_menuIndex % MENU_COLUMNS != MENU_COLUMNS - 1 && m_menuIndex + 1 < count) {
                m_menuIndex += 1;
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
