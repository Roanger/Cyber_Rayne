#include "../../include/ui/PauseMenu.h"
#include "../../include/Party.h"

#ifndef NO_VULKAN
#include "../../include/Quest.h"
#include "../../include/QuestLog.h"
#include "../../include/VulkanRenderer.h"
#include "../../include/ui/TextRenderer.h"
#include "../../include/ui/TextWrap.h"
#include "../../include/ui/Theme.h"
#endif

const std::vector<std::string>& PauseMenu::entries() {
    static const std::vector<std::string> kEntries = {
        "Resume", "Party", "Items", "Equipment", "Quests", "Save Game", "Settings", "Quit to Title"};
    return kEntries;
}

void PauseMenu::open() {
    m_screen = Screen::ROOT;
    m_index = 0;
    m_listIndex = 0;
    m_itemIndex = 0;
    m_memberIndex = 0;
    m_slotIndex = 0;
    m_quitArmed = false;
    m_status.clear();
}

void PauseMenu::moveCursor(int& cursor, int delta, int count) {
    if (count <= 0) {
        cursor = 0;
        return;
    }
    cursor = ((cursor + delta) % count + count) % count;
}

PauseMenu::Command PauseMenu::handleInput(InputAction action, Party* party) {
    // The pause keys always close the menu, from any screen.
    if (action == InputAction::PAUSE || action == InputAction::OPEN_MENU) {
        return Command::RESUME;
    }

    switch (m_screen) {
        case Screen::ROOT: {
            const Command command = handleRoot(action);
            // Screens that act on the party cannot open without one.
            if (!party && (m_screen == Screen::ITEMS || m_screen == Screen::EQUIP_MEMBER)) {
                m_screen = Screen::ROOT;
            }
            return command;
        }
        case Screen::PARTY:
        case Screen::QUESTS:
            if (action == InputAction::CANCEL || action == InputAction::CONFIRM) {
                m_screen = Screen::ROOT;
            }
            break;
        case Screen::ITEMS:
            handleItems(action, party);
            break;
        case Screen::ITEM_TARGET:
            handleItemTarget(action, party);
            break;
        case Screen::EQUIP_MEMBER:
        case Screen::EQUIP_SLOT:
        case Screen::EQUIP_CHOICE:
            handleEquipment(action, party);
            break;
    }
    return Command::NONE;
}

PauseMenu::Command PauseMenu::handleRoot(InputAction action) {
    switch (action) {
        case InputAction::MOVE_UP:
        case InputAction::MOVE_DOWN:
            moveCursor(m_index, action == InputAction::MOVE_UP ? -1 : 1, ENTRY_COUNT);
            m_quitArmed = false;
            m_status.clear();
            break;
        case InputAction::CANCEL:
            return Command::RESUME;
        case InputAction::CONFIRM:
            if (m_index != ENTRY_QUIT) {
                m_quitArmed = false;
                m_status.clear();
            }
            switch (m_index) {
                case ENTRY_RESUME:
                    return Command::RESUME;
                case ENTRY_PARTY:
                    m_screen = Screen::PARTY;
                    break;
                case ENTRY_ITEMS:
                    m_screen = Screen::ITEMS;
                    m_listIndex = 0;
                    break;
                case ENTRY_EQUIPMENT:
                    m_screen = Screen::EQUIP_MEMBER;
                    m_listIndex = 0;
                    break;
                case ENTRY_QUESTS:
                    m_screen = Screen::QUESTS;
                    break;
                case ENTRY_SAVE:
                    return Command::SAVE;
                case ENTRY_SETTINGS:
                    return Command::OPEN_SETTINGS;
                case ENTRY_QUIT:
                    // Quitting throws away unsaved progress, so ask twice.
                    if (m_quitArmed) {
                        return Command::QUIT_TO_TITLE;
                    }
                    m_quitArmed = true;
                    m_status = "Unsaved progress will be lost. Enter again to quit.";
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    return Command::NONE;
}

void PauseMenu::handleItems(InputAction action, Party* party) {
    if (!party) {
        m_screen = Screen::ROOT;
        return;
    }
    const auto& stacks = party->getBag().getStacks();
    const int count = static_cast<int>(stacks.size());
    if (m_listIndex >= count) m_listIndex = count > 0 ? count - 1 : 0;

    switch (action) {
        case InputAction::MOVE_UP:
            moveCursor(m_listIndex, -1, count);
            m_status.clear();
            break;
        case InputAction::MOVE_DOWN:
            moveCursor(m_listIndex, 1, count);
            m_status.clear();
            break;
        case InputAction::CANCEL:
            m_screen = Screen::ROOT;
            m_status.clear();
            break;
        case InputAction::CONFIRM:
            if (count == 0) break;
            if (stacks[static_cast<std::size_t>(m_listIndex)].prototype->getType() ==
                Item::ItemType::CONSUMABLE) {
                m_itemIndex = m_listIndex;
                m_listIndex = 0;
                m_screen = Screen::ITEM_TARGET;
                m_status.clear();
            } else {
                m_status = "Gear is put on from the Equipment screen.";
            }
            break;
        default:
            break;
    }
}

void PauseMenu::handleItemTarget(InputAction action, Party* party) {
    if (!party) {
        m_screen = Screen::ROOT;
        return;
    }
    const int members = static_cast<int>(party->size());

    switch (action) {
        case InputAction::MOVE_UP:
            moveCursor(m_listIndex, -1, members);
            break;
        case InputAction::MOVE_DOWN:
            moveCursor(m_listIndex, 1, members);
            break;
        case InputAction::CANCEL:
            m_screen = Screen::ITEMS;
            m_listIndex = m_itemIndex;
            m_status.clear();
            break;
        case InputAction::CONFIRM: {
            const auto& stacks = party->getBag().getStacks();
            if (m_itemIndex < 0 || m_itemIndex >= static_cast<int>(stacks.size())) {
                m_screen = Screen::ITEMS;
                m_listIndex = 0;
                break;
            }
            // Copy the ID: using the last one removes the stack.
            const std::string itemId = stacks[static_cast<std::size_t>(m_itemIndex)].id;
            party->useItem(itemId, static_cast<std::size_t>(m_listIndex), &m_status);
            if (party->getBag().count(itemId) == 0) {
                // Out of that item; back to the list.
                m_screen = Screen::ITEMS;
                const int remaining = static_cast<int>(party->getBag().getStacks().size());
                m_listIndex = m_itemIndex < remaining ? m_itemIndex : (remaining > 0 ? remaining - 1 : 0);
            }
            break;
        }
        default:
            break;
    }
}

std::vector<std::string> PauseMenu::equipChoices(const Party* party) const {
    std::vector<std::string> ids;
    if (party) {
        for (const Inventory::Stack* stack : party->getBag().getStacksOfType(editedSlot())) {
            ids.push_back(stack->id);
        }
    }
    return ids;
}

void PauseMenu::handleEquipment(InputAction action, Party* party) {
    if (!party) {
        m_screen = Screen::ROOT;
        return;
    }
    const int delta = action == InputAction::MOVE_UP ? -1 : (action == InputAction::MOVE_DOWN ? 1 : 0);

    if (m_screen == Screen::EQUIP_MEMBER) {
        if (delta != 0) {
            moveCursor(m_listIndex, delta, static_cast<int>(party->size()));
        } else if (action == InputAction::CANCEL) {
            m_screen = Screen::ROOT;
            m_status.clear();
        } else if (action == InputAction::CONFIRM && party->size() > 0) {
            m_memberIndex = m_listIndex;
            m_listIndex = 0;
            m_screen = Screen::EQUIP_SLOT;
            m_status.clear();
        }
        return;
    }

    if (m_screen == Screen::EQUIP_SLOT) {
        if (delta != 0) {
            moveCursor(m_listIndex, delta, 2);
        } else if (action == InputAction::CANCEL) {
            m_screen = Screen::EQUIP_MEMBER;
            m_listIndex = m_memberIndex;
            m_status.clear();
        } else if (action == InputAction::CONFIRM) {
            m_slotIndex = m_listIndex;
            m_listIndex = 0;
            m_screen = Screen::EQUIP_CHOICE;
            m_status.clear();
        }
        return;
    }

    // EQUIP_CHOICE: the bag's pieces for this slot, then "Remove".
    const std::vector<std::string> choices = equipChoices(party);
    const int count = static_cast<int>(choices.size()) + 1;
    if (delta != 0) {
        moveCursor(m_listIndex, delta, count);
    } else if (action == InputAction::CANCEL) {
        m_screen = Screen::EQUIP_SLOT;
        m_listIndex = m_slotIndex;
        m_status.clear();
    } else if (action == InputAction::CONFIRM) {
        const std::size_t member = static_cast<std::size_t>(m_memberIndex);
        const Player* who = party->getMember(member);
        const std::string name = who ? who->getName() : "";
        if (m_listIndex < static_cast<int>(choices.size())) {
            const std::string& id = choices[static_cast<std::size_t>(m_listIndex)];
            const Inventory::Stack* stack = party->getBag().find(id);
            const std::string itemName = stack ? stack->prototype->getName() : id;
            m_status = party->equipFromBag(member, id) ? name + " equipped " + itemName + "."
                                                       : "That can't be equipped.";
        } else {
            m_status = party->unequipToBag(member, editedSlot()) ? name + "'s gear went back in the bag."
                                                                 : "Nothing to remove.";
        }
        m_screen = Screen::EQUIP_SLOT;
        m_listIndex = m_slotIndex;
    }
}

#ifndef NO_VULKAN
namespace {

std::string gearLine(const char* label, const Item* item) {
    return std::string(label) + (item ? item->getName() : "(none)");
}

std::string memberVitals(const Player& member) {
    if (!member.isAlive()) {
        return "DOWN";
    }
    return "HP " + std::to_string(member.getHealth()) + "/" + std::to_string(member.getMaxHealth()) +
           "  MP " + std::to_string(member.getMana()) + "/" + std::to_string(member.getMaxMana());
}

}  // namespace

void PauseMenu::render(VulkanRenderer* renderer, TextRenderer* text, const Party* party,
                       const QuestLog* questLog, const std::string& mapName) {
    if (!renderer || !text) return;

    const int screenW = static_cast<int>(renderer->getSwapchainExtent().width);
    const int screenH = static_cast<int>(renderer->getSwapchainExtent().height);
    Theme::dimScreen(renderer, 0.7f);

    const int margin = 60;
    const int pad = 28;
    const int big = 3;
    const int small = 2;
    const int bigLine = Theme::GLYPH * big + 18;
    const int smallLine = Theme::GLYPH * small + 12;

    // --- Left: menu ---------------------------------------------------------
    const int menuW = 460;
    const int menuH = pad * 2 + bigLine * (ENTRY_COUNT + 2);
    Theme::drawPanel(renderer, margin, margin, menuW, menuH);
    text->renderText(renderer, "PAUSED", margin + pad, margin + pad, big, Theme::IRIS);

    int openEntry = -1;
    switch (m_screen) {
        case Screen::PARTY:        openEntry = ENTRY_PARTY; break;
        case Screen::QUESTS:       openEntry = ENTRY_QUESTS; break;
        case Screen::ITEMS:
        case Screen::ITEM_TARGET:  openEntry = ENTRY_ITEMS; break;
        case Screen::EQUIP_MEMBER:
        case Screen::EQUIP_SLOT:
        case Screen::EQUIP_CHOICE: openEntry = ENTRY_EQUIPMENT; break;
        case Screen::ROOT:         break;
    }
    for (int i = 0; i < ENTRY_COUNT; ++i) {
        const bool selected = m_screen == Screen::ROOT && i == m_index;
        const uint32_t color = selected ? Theme::GOLD : (i == openEntry ? Theme::FOAM : Theme::TEXT);
        text->renderText(renderer, (selected ? "> " : "  ") + entries()[static_cast<std::size_t>(i)],
                         margin + pad, margin + pad + bigLine * (i + 2), big, color);
    }

    // Location and purse under the menu.
    int totalGold = 0;
    if (party) {
        for (const auto& member : party->getMembers()) {
            if (member) totalGold += member->getGold();
        }
    }
    const int infoY = margin + menuH + 20;
    const int infoH = pad * 2 + smallLine * 2;
    Theme::drawPanel(renderer, margin, infoY, menuW, infoH);
    text->renderText(renderer, mapName, margin + pad, infoY + pad, small, Theme::FOAM);
    text->renderText(renderer, "Gold: " + std::to_string(totalGold), margin + pad,
                     infoY + pad + smallLine, small, Theme::GOLD);

    // --- Right: content -----------------------------------------------------
    const int contentX = margin + menuW + 30;
    const int contentW = screenW - contentX - margin;
    const int contentH = screenH - margin * 2 - 70;
    Theme::drawPanel(renderer, contentX, margin, contentW, contentH);
    const int textX = contentX + pad;
    int y = margin + pad;
    const std::size_t maxChars = static_cast<std::size_t>((contentW - pad * 2) / (Theme::GLYPH * small));

    auto heading = [&](const std::string& title) {
        text->renderText(renderer, title, textX, y, big, Theme::IRIS);
        y += bigLine + 8;
    };
    // One row of a cursor list.
    auto row = [&](const std::string& label, bool selected, uint32_t idleColor = Theme::TEXT) {
        text->renderText(renderer, (selected ? "> " : "  ") + label, textX, y, big,
                         selected ? Theme::GOLD : idleColor);
        y += bigLine;
    };
    auto note = [&](const std::string& line, uint32_t color = Theme::MUTED) {
        for (const auto& wrapped : wrapText(line, maxChars)) {
            text->renderText(renderer, wrapped, textX, y, small, color);
            y += smallLine;
        }
    };

    std::string hint = "Esc: back";

    if (m_screen == Screen::QUESTS) {
        heading("QUESTS");
        bool any = false;
        if (questLog) {
            for (const auto& quest : questLog->getQuests()) {
                if (!quest || quest->getState() == Quest::State::NOT_STARTED) continue;
                any = true;
                const bool done = quest->getState() == Quest::State::COMPLETED;
                text->renderText(renderer, quest->getName() + (done ? "  (complete)" : ""), textX, y,
                                 small, done ? Theme::MUTED : Theme::GOLD);
                y += smallLine;
                note(quest->getDescription());
                for (const auto& objective : quest->getObjectives()) {
                    const bool objDone = objective.isCompleted();
                    text->renderText(renderer,
                                     std::string(objDone ? "  [x] " : "  [ ] ") + objective.getDescription(),
                                     textX, y, small, objDone ? Theme::MUTED : Theme::TEXT);
                    y += smallLine;
                }
                y += smallLine;
            }
        }
        if (!any) {
            note("No quests yet. Talk to people around the city.");
        }
    } else if (m_screen == Screen::ITEMS || m_screen == Screen::ITEM_TARGET) {
        heading("ITEMS");
        const bool choosingTarget = m_screen == Screen::ITEM_TARGET;
        const int cursor = choosingTarget ? m_itemIndex : m_listIndex;
        const Item* focused = nullptr;
        if (party) {
            const auto& stacks = party->getBag().getStacks();
            for (std::size_t i = 0; i < stacks.size(); ++i) {
                const bool selected = static_cast<int>(i) == cursor;
                if (selected) focused = stacks[i].prototype.get();
                row(stacks[i].prototype->getName() + "  x" + std::to_string(stacks[i].count),
                    selected && !choosingTarget, selected ? Theme::FOAM : Theme::TEXT);
            }
            if (stacks.empty()) {
                note("The bag is empty.");
            }
        }
        if (focused) {
            y += 10;
            note(focused->getDescription());
        }
        if (choosingTarget && party) {
            y += 18;
            text->renderText(renderer, "Use on:", textX, y, small, Theme::MUTED);
            y += smallLine + 4;
            for (std::size_t i = 0; i < party->size(); ++i) {
                const Player* member = party->getMember(i);
                if (!member) continue;
                row(member->getName() + "   " + memberVitals(*member), static_cast<int>(i) == m_listIndex);
            }
            hint = "Up/Down: select   Enter: use   Esc: back";
        } else {
            hint = "Up/Down: select   Enter: use   Esc: back";
        }
    } else if (m_screen == Screen::EQUIP_MEMBER || m_screen == Screen::EQUIP_SLOT ||
               m_screen == Screen::EQUIP_CHOICE) {
        heading("EQUIPMENT");
        if (party) {
            if (m_screen == Screen::EQUIP_MEMBER) {
                for (std::size_t i = 0; i < party->size(); ++i) {
                    const Player* member = party->getMember(i);
                    if (!member) continue;
                    row(member->getName(), static_cast<int>(i) == m_listIndex);
                    note("    " + gearLine("Weapon: ", member->getEquippedWeapon()) + "    " +
                         gearLine("Armor: ", member->getEquippedArmor()));
                    y += 10;
                }
                hint = "Up/Down: select   Enter: change gear   Esc: back";
            } else {
                const Player* member = party->getMember(static_cast<std::size_t>(m_memberIndex));
                if (member) {
                    text->renderText(renderer, member->getName(), textX, y, big, Theme::FOAM);
                    y += bigLine;
                    note("Attack " + std::to_string(member->getEffectiveStrength()) + "    Defense " +
                             std::to_string(member->getEffectiveDefense()),
                         Theme::TEXT);
                    y += 10;
                    const bool pickingSlot = m_screen == Screen::EQUIP_SLOT;
                    const int slotCursor = pickingSlot ? m_listIndex : m_slotIndex;
                    row(gearLine("Weapon: ", member->getEquippedWeapon()), pickingSlot && slotCursor == 0,
                        slotCursor == 0 ? Theme::FOAM : Theme::TEXT);
                    row(gearLine("Armor:  ", member->getEquippedArmor()), pickingSlot && slotCursor == 1,
                        slotCursor == 1 ? Theme::FOAM : Theme::TEXT);

                    if (!pickingSlot) {
                        y += 18;
                        text->renderText(renderer, "In the bag:", textX, y, small, Theme::MUTED);
                        y += smallLine + 4;
                        const auto choices = equipChoices(party);
                        for (std::size_t i = 0; i < choices.size(); ++i) {
                            const Inventory::Stack* stack = party->getBag().find(choices[i]);
                            if (!stack) continue;
                            const bool selected = static_cast<int>(i) == m_listIndex;
                            row(stack->prototype->getName() + "  x" + std::to_string(stack->count), selected);
                            if (selected) note("    " + stack->prototype->getDescription());
                        }
                        row("(Remove)", m_listIndex == static_cast<int>(choices.size()), Theme::MUTED);
                        hint = "Up/Down: select   Enter: equip   Esc: back";
                    } else {
                        hint = "Up/Down: select   Enter: change   Esc: back";
                    }
                }
            }
        }
    } else {
        // Party status. The root screen shows the same overview.
        heading("PARTY");
        if (party) {
            for (const auto& member : party->getMembers()) {
                if (!member) continue;
                text->renderText(renderer,
                                 member->getName() + "   Lv " + std::to_string(member->getLevel()) + " " +
                                     Player::classToString(member->getCharacterClass()),
                                 textX, y, big, member->isAlive() ? Theme::GOLD : Theme::LOVE);
                y += bigLine;
                note(memberVitals(*member) + "    XP " + std::to_string(member->getExperience()) + "/" +
                         std::to_string(member->getExperienceToNextLevel()),
                     Theme::TEXT);
                note("STR " + std::to_string(member->getStrength()) + "   MAG " +
                     std::to_string(member->getMagic()) + "   SPD " + std::to_string(member->getSpeed()) +
                     "   DEF " + std::to_string(member->getDefense()));
                note(gearLine("Weapon: ", member->getEquippedWeapon()) + "    " +
                     gearLine("Armor: ", member->getEquippedArmor()));
                y += 22;
            }
        }
        if (m_screen == Screen::ROOT) {
            hint = "Up/Down: select   Enter: choose   Esc: resume";
        }
    }

    // --- Bottom: status and key hints -----------------------------------------
    const int footY = screenH - margin - 50;
    text->renderText(renderer, hint, margin, footY + 26, small, Theme::MUTED);
    if (!m_status.empty()) {
        text->renderText(renderer, m_status, margin, footY - 6, small,
                         m_quitArmed ? Theme::LOVE : Theme::FOAM);
    }
}
#endif
