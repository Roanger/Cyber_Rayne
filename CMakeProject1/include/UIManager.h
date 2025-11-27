#pragma once

#include <string>
#include <vector>

#include <memory>

class VulkanRenderer;
class Player;
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
    void renderBattleUI(VulkanRenderer* renderer, Player* player, const std::vector<std::unique_ptr<Enemy>>& enemies);
    void handleInput(int key);
    
    // State management
    void setBattleState(BattleMenuState state) { m_battleState = state; }
    BattleMenuState getBattleState() const { return m_battleState; }
    
    // Action retrieval (non-blocking)
    bool hasActionSelected() const { return m_actionSelected; }
    BattleAction getSelectedAction() const { return m_selectedAction; }
    int getSelectedTargetIndex() const { return m_selectedTargetIndex; }
    int getSelectedSpellIndex() const { return m_selectedSpellIndex; }
    void resetAction();

    // Legacy/Console methods (to be deprecated/removed or adapted)
    void displayMessage(const std::string& message);
    
private:
    void renderBattleMenu(VulkanRenderer* renderer);
    void renderStats(VulkanRenderer* renderer, Player* player, const std::vector<std::unique_ptr<Enemy>>& enemies);
    void renderCursor(VulkanRenderer* renderer);

    BattleMenuState m_battleState;
    BattleAction m_selectedAction;
    bool m_actionSelected;
    
    // Selection indices
    int m_menuIndex;
    int m_selectedTargetIndex;
    int m_selectedSpellIndex;
    
    // Texture indices
    int m_cursorTextureIndex;
    int m_backgroundTextureIndex; // Reusing menu background for now or a new one
    int m_buttonTextureIndices[4]; // 0: Attack/Start, 1: Magic/Load, etc. (Need to map correctly)
    
    // Layout constants
    const int BUTTON_WIDTH = 300;
    const int BUTTON_HEIGHT = 110;
};
