#pragma once

#include <memory>
#include "Player.h"
#include "Party.h"
#include "CharacterSelectionSystem.h"
#include "MenuSystem.h"
#include "UIManager.h"
#include "InputAction.h"

class World;
class Player;
class VulkanRenderer;
class CharacterSelectionSystem;
class BattleSystem;
class UIManager;
class Party;
class AudioManager;
class InputConfig;
class SaveSystem;
class QuestLog;

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
    void handleInput(InputAction action);

    // Getters
    State getCurrentState() const { return m_currentState; }

    // Setters
    void setCurrentState(State state) { m_currentState = state; }

    // Getters for game components
    World* getWorld() const { return m_world.get(); }
    Party* getParty() const { return m_party.get(); }
    Player* getPlayer() const { return m_party ? m_party->getLeader() : nullptr; }
    AudioManager* getAudioManager() const { return m_audioManager.get(); }
    InputConfig* getInputConfig() const { return m_inputConfig.get(); }
    QuestLog* getQuestLog() const { return m_questLog.get(); }

private:
    void startNewGame();
    void loadGame();
    void togglePause();
    void handleGameOverInput(InputAction action);

    State m_currentState;
    // Set by CYBERRAYNE_START=world; consumed on the first update().
    bool m_pendingDevStart = false;
    std::unique_ptr<World> m_world;
    std::unique_ptr<Party> m_party;
    std::unique_ptr<CharacterSelectionSystem> m_charSelectionSystem;
    std::unique_ptr<BattleSystem> m_battleSystem;
    std::unique_ptr<MenuSystem> m_menuSystem;
    std::unique_ptr<UIManager> m_uiManager;
    std::unique_ptr<AudioManager> m_audioManager;
    std::unique_ptr<InputConfig> m_inputConfig;
    std::unique_ptr<SaveSystem> m_saveSystem;
    std::unique_ptr<QuestLog> m_questLog;
    VulkanRenderer* m_renderer;
};
