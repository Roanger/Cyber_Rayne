#pragma once

#include <string>
#include <vector>

#include "../InputAction.h"
#include "../Item.h"

class VulkanRenderer;
class TextRenderer;
class Party;
class QuestLog;

// The in-game pause menu: party status, the shared bag, equipment, the quest
// log, saving, settings, and quitting to title. Navigation and the actions it
// performs on the party are plain logic so they can be tested without a
// renderer; drawing is compiled only into the game.
class PauseMenu {
public:
    enum class Screen {
        ROOT,
        PARTY,
        QUESTS,
        ITEMS,          // browsing the bag
        ITEM_TARGET,    // choosing who a consumable is used on
        EQUIP_MEMBER,   // choosing whose gear to change
        EQUIP_SLOT,     // weapon or armor
        EQUIP_CHOICE    // which piece from the bag, or remove
    };
    enum class Command { NONE, RESUME, SAVE, OPEN_SETTINGS, QUIT_TO_TITLE };
    enum Entry {
        ENTRY_RESUME = 0,
        ENTRY_PARTY,
        ENTRY_ITEMS,
        ENTRY_EQUIPMENT,
        ENTRY_QUESTS,
        ENTRY_SAVE,
        ENTRY_SETTINGS,
        ENTRY_QUIT,
        ENTRY_COUNT
    };

    // Reset to the top of the root screen. Call when the game is paused.
    void open();

    // Returns what the game should do in response, if anything. party is
    // needed for the item and equipment screens; without it they stay closed.
    Command handleInput(InputAction action, Party* party = nullptr);

    Screen getScreen() const { return m_screen; }
    // Cursor on the root screen.
    int getIndex() const { return m_index; }
    // Cursor within the current sub-screen's list.
    int getListIndex() const { return m_listIndex; }
    // True after the first press on "Quit to Title": the next press quits.
    bool isQuitArmed() const { return m_quitArmed; }

    // One line shown under the menu, e.g. "Game saved."
    void setStatus(const std::string& status) { m_status = status; }
    const std::string& getStatus() const { return m_status; }
    // True once after gear was put on or taken off, so the game can play a sound.
    bool consumeGearChanged() { const bool changed = m_gearChanged; m_gearChanged = false; return changed; }

    static const std::vector<std::string>& entries();

#ifndef NO_VULKAN
    // sprites supplies item icons; without it the lists are text only.
    void render(VulkanRenderer* renderer, TextRenderer* text, const Party* party,
                const QuestLog* questLog, const std::string& mapName,
                class SpriteLibrary* sprites = nullptr);
#endif

private:
    // Move a cursor within [0, count), wrapping. Leaves it at 0 when empty.
    static void moveCursor(int& cursor, int delta, int count);

    Command handleRoot(InputAction action);
    void handleItems(InputAction action, Party* party);
    void handleItemTarget(InputAction action, Party* party);
    void handleEquipment(InputAction action, Party* party);

    // IDs of bag items that fit the slot being edited, in bag order.
    std::vector<std::string> equipChoices(const Party* party) const;
    Item::ItemType editedSlot() const {
        return m_slotIndex == 0 ? Item::ItemType::WEAPON : Item::ItemType::ARMOR;
    }

    Screen m_screen = Screen::ROOT;
    int m_index = 0;
    int m_listIndex = 0;
    // Remembered while deeper screens are open.
    int m_itemIndex = 0;
    int m_memberIndex = 0;
    int m_slotIndex = 0;
    bool m_quitArmed = false;
    std::string m_status;
    bool m_gearChanged = false;
};
