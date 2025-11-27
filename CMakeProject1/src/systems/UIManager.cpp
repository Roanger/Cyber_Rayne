#include "../../include/UIManager.h"
#include "../../include/VulkanRenderer.h"
#include "../../include/Player.h"
#include "../../include/Enemy.h"
#include <iostream>
#include <filesystem>

UIManager::UIManager() 
    : m_battleState(BattleMenuState::MAIN_MENU),
      m_selectedAction(BattleAction::NONE),
      m_actionSelected(false),
      m_menuIndex(0),
      m_selectedTargetIndex(0),
      m_selectedSpellIndex(0),
      m_cursorTextureIndex(-1),
      m_backgroundTextureIndex(-1) {
    for(int i=0; i<4; ++i) m_buttonTextureIndices[i] = -1;
}

UIManager::~UIManager() {}

void UIManager::initialize(VulkanRenderer* renderer) {
    loadTextures(renderer);
}

void UIManager::loadTextures(VulkanRenderer* renderer) {
    const std::string base = renderer->getAssetsBasePath();
    auto exists = [](const std::string& p){ return std::filesystem::exists(p); };

    // Reuse menu assets for now
    std::string bgPath = base + "/ui/menu_background.png";
    if (exists(bgPath)) m_backgroundTextureIndex = renderer->loadTexture(bgPath);

    std::string cursorPath = base + "/ui/cursor.png";
    if (exists(cursorPath)) m_cursorTextureIndex = renderer->loadTexture(cursorPath);

    // Load buttons (Attack, Magic, Item, Defend/Flee)
    // For prototype, reusing Start/Load/Settings/Quit as placeholders if specific ones don't exist
    // Ideally we generate specific battle buttons later
    std::string btnPaths[4] = {
        base + "/ui/start.png",   // Placeholder for Attack
        base + "/ui/loadgame.png", // Placeholder for Magic
        base + "/ui/settings.png", // Placeholder for Item
        base + "/ui/quit.png"      // Placeholder for Defend/Flee
    };

    for(int i=0; i<4; ++i) {
        if (exists(btnPaths[i])) {
            m_buttonTextureIndices[i] = renderer->loadTexture(btnPaths[i]);
        }
    }
}

void UIManager::renderBattleUI(VulkanRenderer* renderer, Player* player, const std::vector<std::unique_ptr<Enemy>>& enemies) {
    // Render background (if we want a UI overlay background, otherwise battle scene is behind)
    // For now, let's assume the battle scene is rendered by BattleSystem/World, 
    // and we just render the UI on top.
    
    if (m_battleState != BattleMenuState::WAITING) {
        renderBattleMenu(renderer);
        renderCursor(renderer);
    }
    
    renderStats(renderer, player, enemies);
}

void UIManager::renderBattleMenu(VulkanRenderer* renderer) {
    int screenW = static_cast<int>(renderer->getSwapchainExtent().width);
    int screenH = static_cast<int>(renderer->getSwapchainExtent().height);
    
    int startX = 50;
    int startY = screenH - BUTTON_HEIGHT * 2 - 50;
    
    // Render 4 main action buttons
    const char* labels[] = {"Attack", "Magic", "Item", "Defend"};
    
    for(int i=0; i<4; ++i) {
        int x = startX + (i % 2) * (BUTTON_WIDTH + 20);
        int y = startY + (i / 2) * (BUTTON_HEIGHT + 20);
        
        if (m_buttonTextureIndices[i] >= 0) {
            renderer->renderSpritePixelsWithTexture(x, y, BUTTON_WIDTH, BUTTON_HEIGHT, m_buttonTextureIndices[i]);
        } else {
            // Fallback colored rect
            // renderer->renderSpritePixels(x, y, BUTTON_WIDTH, BUTTON_HEIGHT, 0.2f, 0.2f, 0.8f, 1.0f);
        }
    }
}

void UIManager::renderCursor(VulkanRenderer* renderer) {
    if (m_cursorTextureIndex < 0) return;
    
    int screenW = static_cast<int>(renderer->getSwapchainExtent().width);
    int screenH = static_cast<int>(renderer->getSwapchainExtent().height);
    int startX = 50;
    int startY = screenH - BUTTON_HEIGHT * 2 - 50;
    
    int targetX = 0, targetY = 0;
    
    if (m_battleState == BattleMenuState::MAIN_MENU) {
        targetX = startX + (m_menuIndex % 2) * (BUTTON_WIDTH + 20);
        targetY = startY + (m_menuIndex / 2) * (BUTTON_HEIGHT + 20);
    }
    
    // Offset cursor to left of target
    int cursorSize = 64;
    renderer->renderSpritePixelsWithTexture(targetX - cursorSize + 10, targetY + (BUTTON_HEIGHT - cursorSize)/2, cursorSize, cursorSize, m_cursorTextureIndex);
}

void UIManager::renderStats(VulkanRenderer* renderer, Player* player, const std::vector<std::unique_ptr<Enemy>>& enemies) {
    // Placeholder for text rendering (since we don't have font rendering yet)
    // We could render health bars here using colored quads
    
    int screenW = static_cast<int>(renderer->getSwapchainExtent().width);
    
    // Player HP Bar (Bottom Right)
    float playerHpPct = static_cast<float>(player->getHealth()) / player->getMaxHealth();
    int barW = 300;
    int barH = 30;
    int barX = screenW - barW - 50;
    int barY = static_cast<int>(renderer->getSwapchainExtent().height) - 100;
    
    // Background
    // renderer->renderSpritePixels(barX, barY, barW, barH, 0.2f, 0.0f, 0.0f, 1.0f);
    // Foreground
    // renderer->renderSpritePixels(barX, barY, static_cast<int>(barW * playerHpPct), barH, 0.0f, 0.8f, 0.0f, 1.0f);
}

void UIManager::handleInput(int key) {
    if (m_battleState == BattleMenuState::MAIN_MENU) {
        switch(key) {
            case 0: // Up
                if (m_menuIndex >= 2) m_menuIndex -= 2;
                break;
            case 1: // Down
                if (m_menuIndex <= 1) m_menuIndex += 2;
                break;
            case 3: // Left
                if (m_menuIndex % 2 == 1) m_menuIndex -= 1;
                break;
            case 4: // Right
                if (m_menuIndex % 2 == 0) m_menuIndex += 1;
                break;
            case 2: // Enter
                if (m_menuIndex == 0) { // Attack
                    m_selectedAction = BattleAction::ATTACK;
                    m_actionSelected = true;
                } else if (m_menuIndex == 3) { // Defend (mapped to 4th button)
                     m_selectedAction = BattleAction::DEFEND;
                     m_actionSelected = true;
                }
                // TODO: Magic/Item submenus
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
