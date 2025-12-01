#pragma once

#include <memory>
#include "Player.h"
#include "CharacterSelectionSystem.h"
#include "MenuSystem.h"
#include "UIManager.h"

class World;
class Player;
class VulkanRenderer;
class CharacterSelectionSystem;
class BattleSystem;
class UIManager;

class GameState {
public:
    enum class State {
        MENU,
        CHARACTER_SELECTION,
        WORLD_EXPLORATION,
        BATTLE,
        PAUSED,
        GAME_OVER,
        EXIT
    };

    GameState();
    ~GameState();

    bool initialize();
    void setRenderer(VulkanRenderer* renderer);
    void update(float deltaTime);
    void render(VulkanRenderer* renderer);
    void handleInput(int key);

    // Getters
    State getCurrentState() const { return m_currentState; }

    // Setters
    void setCurrentState(State state) { m_currentState = state; }

    // Getters for game components
    World* getWorld() const { return m_world; }
    Player* getPlayer() const { return m_player; }

private:
    State m_currentState;
    World* m_world;
    Player* m_player;
    CharacterSelectionSystem* m_charSelectionSystem;
    BattleSystem* m_battleSystem;
    MenuSystem* m_menuSystem;
    UIManager* m_uiManager;
    VulkanRenderer* m_renderer;
};
