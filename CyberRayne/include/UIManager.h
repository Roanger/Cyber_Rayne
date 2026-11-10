#pragma once

#include <string>
#include <vector>

#include <memory>
#include "InputAction.h"
#include "BattleSystem.h"
#include "ui/TextRenderer.h"

class VulkanRenderer;
class Party;
class Combatant;
class Enemy;
class Ability;

class UIManager {
public:
    enum class BattleMenuState {
        MAIN_MENU,
        ABILITY_SELECTION,
        ITEM_SELECTION,
        TARGET_SELECTION,
        WAITING  // Waiting for animation or enemy turn
    };

    // The battle action enum lives on BattleSystem. UIManager used to declare
    // a parallel copy that GameState hand-translated case by case.
    using BattleAction = BattleSystem::BattleAction;
    using ActionRequest = BattleSystem::ActionRequest;

    UIManager();
    ~UIManager();

    void initialize(VulkanRenderer* renderer);
    void loadTextures(VulkanRenderer* renderer);

    // Battle UI methods
    // enemies and allies are the living combatants on each side, in the order
    // BattleSystem counts target indices.
    void renderBattleUI(VulkanRenderer* renderer, Party* party,
                        const std::vector<Combatant*>& enemies,
                        const std::vector<Combatant*>& allies);
    // Who the target cursor is on right now, or null when not choosing one.
    const Combatant* getHighlightedTarget() const;
    TextRenderer* text() const { return m_textRenderer.get(); }
    void handleInput(InputAction action);

    // State management
    void setBattleState(BattleMenuState state) { m_battleState = state; }
    BattleMenuState getBattleState() const { return m_battleState; }

    // Action retrieval (non-blocking). When hasActionSelected() is true the
    // request is fully specified and can go straight to BattleSystem.
    bool hasActionSelected() const { return m_actionSelected; }
    const ActionRequest& getActionRequest() const { return m_request; }
    void resetAction();

    // Set the currently acting combatant. Rebuilds the menu, because the
    // available abilities depend on who is acting.
    void setCurrentActor(Combatant* actor);

    // The party whose bag the Item menu lists. Set when a battle starts.
    void setParty(Party* party) { m_party = party; refreshEntries(); }

    // Number of entries in the list currently on screen. Exposed so the
    // navigation logic can be exercised without a renderer.
    int getMenuEntryCount() const;
    int getMenuIndex() const { return m_menuIndex; }

    // Message shown when an action was rejected, e.g. not enough MP.
    void setStatusMessage(const std::string& message) { m_statusMessage = message; }
    const std::string& getStatusMessage() const { return m_statusMessage; }

    void displayMessage(const std::string& message);

private:
    // One entry in whichever list is currently displayed.
    struct MenuEntry {
        std::string label;
        bool enabled = true;
    };

    void renderCommandPanel(VulkanRenderer* renderer, int x, int y, int w, int h);
    void renderPartyPanel(VulkanRenderer* renderer, Party* party, int x, int y, int w, int h);
    // True when the pending item or ability is aimed at the party.
    bool targetingAllies() const;

    // Rebuild m_entries for the current state and actor.
    void refreshEntries();
    void confirmCurrentEntry();
    void cancelToMainMenu();
    void commit(const ActionRequest& request);

    // The ability chosen in ABILITY_SELECTION, awaiting a target.
    const Ability* pendingAbility() const;

    BattleMenuState m_battleState;
    ActionRequest m_request;
    bool m_actionSelected;

    // Selection indices
    int m_menuIndex;
    int m_pendingAbilityIndex = -1;
    int m_pendingItemIndex = -1;

    std::vector<MenuEntry> m_entries;
    std::string m_statusMessage;

    // Reusable UI primitives
    std::unique_ptr<TextRenderer> m_textRenderer;

    // Currently acting combatant (whose menu is shown)
    Combatant* m_currentActor;
    Party* m_party = nullptr;
    // Living combatants on each side, refreshed each render, used for target
    // selection.
    std::vector<Combatant*> m_targets;
    std::vector<Combatant*> m_allies;

    static constexpr int MENU_COLUMNS = 2;
};
