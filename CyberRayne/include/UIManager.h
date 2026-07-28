#pragma once

#include <string>
#include <vector>

#include <memory>
#include "InputAction.h"
#include "BattleSystem.h"
#include "ui/UIButton.h"
#include "ui/UIProgressBar.h"
#include "ui/UILabel.h"
#include "ui/UIRoot.h"
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
    void renderBattleUI(VulkanRenderer* renderer, Party* party, const std::vector<Combatant*>& enemies);
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

    void rebuildBattleMenu(VulkanRenderer* renderer);
    void updateButtonSelection();
    void renderBattleMenu(VulkanRenderer* renderer);
    void renderStats(VulkanRenderer* renderer, Party* party, const std::vector<Combatant*>& enemies);

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
    bool m_menuDirty = true;

    // Selection indices
    int m_menuIndex;
    int m_pendingAbilityIndex = -1;
    int m_pendingItemIndex = -1;

    std::vector<MenuEntry> m_entries;
    std::string m_statusMessage;

    // Reusable UI primitives
    std::unique_ptr<TextRenderer> m_textRenderer;
    UIRoot m_battleRoot;
    std::vector<UIButton*> m_battleButtons;
    std::unique_ptr<UILabel> m_turnLabel;

    // Currently acting combatant (whose menu is shown)
    Combatant* m_currentActor;
    // Living enemies, refreshed each render, used for target selection.
    std::vector<Combatant*> m_targets;

    // Layout constants
    static constexpr int BUTTON_WIDTH = 300;
    static constexpr int BUTTON_HEIGHT = 110;
    static constexpr int MENU_START_X = 50;
    static constexpr int MENU_START_Y_OFFSET = 50;
    static constexpr int MENU_COLUMNS = 2;
};
