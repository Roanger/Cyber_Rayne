#pragma once

#include <string>
#include <vector>

#include <memory>
#include "InputAction.h"
#include "ui/UIButton.h"
#include "ui/UIProgressBar.h"
#include "ui/UILabel.h"
#include "ui/UIRoot.h"
#include "ui/TextRenderer.h"

class VulkanRenderer;
class Party;
class Combatant;
class Enemy;

class UIManager {
public:
    enum class BattleMenuState {
        MAIN_MENU,
        TARGET_SELECTION,
        MAGIC_SELECTION,
        ITEM_SELECTION,
        WAITING // Waiting for animation or enemy turn
    };

    enum class BattleAction {
        NONE,
        ATTACK,
        MAGIC,
        ITEM,
        DEFEND,
        FLEE
    };

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

    // Action retrieval (non-blocking)
    bool hasActionSelected() const { return m_actionSelected; }
    BattleAction getSelectedAction() const { return m_selectedAction; }
    int getSelectedTargetIndex() const { return m_selectedTargetIndex; }
    int getSelectedSpellIndex() const { return m_selectedSpellIndex; }
    void resetAction();

    // Set the currently acting combatant (for highlighting whose turn it is).
    void setCurrentActor(Combatant* actor) { m_currentActor = actor; }

    // Legacy/Console methods (to be deprecated/removed or adapted)
    void displayMessage(const std::string& message);

private:
    void rebuildBattleMenu(VulkanRenderer* renderer);
    void updateButtonSelection();
    void renderBattleMenu(VulkanRenderer* renderer);
    void renderStats(VulkanRenderer* renderer, Party* party, const std::vector<Combatant*>& enemies);

    BattleMenuState m_battleState;
    BattleAction m_selectedAction;
    bool m_actionSelected;

    // Selection indices
    int m_menuIndex;
    int m_selectedTargetIndex;
    int m_selectedSpellIndex;

    // Reusable UI primitives
    std::unique_ptr<TextRenderer> m_textRenderer;
    UIRoot m_battleRoot;
    std::vector<UIButton*> m_battleButtons;
    std::unique_ptr<UILabel> m_turnLabel;

    // Currently acting combatant (whose menu is shown)
    Combatant* m_currentActor;

    // Layout constants
    static constexpr int BUTTON_WIDTH = 300;
    static constexpr int BUTTON_HEIGHT = 110;
    static constexpr int MENU_START_X = 50;
    static constexpr int MENU_START_Y_OFFSET = 50;
};
