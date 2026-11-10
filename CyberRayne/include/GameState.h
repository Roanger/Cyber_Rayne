#pragma once

#include <memory>
#include <string>
#include <vector>
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
class DialogueSystem;
class StoryDirector;
class StoryHud;
class PauseMenu;
class SettingsMenu;
struct BattleSummary;

class GameState {
public:
    enum class State {
        MENU,
        CHARACTER_SELECTION,
        WORLD_EXPLORATION,
        BATTLE,
        BATTLE_RESULT,
        PAUSED,
        SETTINGS,
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

    // Key rebinding: while this is true, send the next raw key press to
    // handleRawKey instead of translating it to an action.
    bool wantsRawKey() const;
    void handleRawKey(int key);

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
    // Write the current game to the per-user save file. Returns true on success.
    bool saveGame();
    void returnToTitle();
    void openSettings();
    void closeSettings();
    void renderWorldScene(VulkanRenderer* renderer);
    void finishBattle();
    void togglePause();
    void handleGameOverInput(InputAction action);

    // Story wiring
    // Create the director for the current party and quest log.
    void beginStory();
    // Push story state into the world (locks, fixed fights) and surface any
    // new notifications. Call after anything that can change story progress.
    void syncStory();
    void handleExplorationInput(InputAction action);
    void handleDialogueInput(InputAction action);
    void tryStartDialogue();
    void reportDialogueNode();
    void resetStoryPresentation();

    // How close, in tiles, the player must be to an NPC to talk. Covers the
    // four neighbours and the diagonals.
    static constexpr float TALK_RANGE = 1.5f;

    State m_currentState;

    std::unique_ptr<DialogueSystem> m_dialogue;
    std::unique_ptr<StoryDirector> m_storyDirector;
    std::unique_ptr<StoryHud> m_storyHud;
    // Who the active conversation is with, and the last node reported to the
    // director, so each node's effect fires once.
    std::string m_talkingTo;
    int m_reportedNodeId = -1;
    bool m_showEnding = false;
    // Detects map changes so the director hears about arrivals.
    std::string m_lastMapName;
    // Tag of the story fight in progress; empty for ordinary battles.
    std::string m_battleTag;

    std::unique_ptr<PauseMenu> m_pauseMenu;
    std::unique_ptr<SettingsMenu> m_settingsMenu;
    // Where closing the settings screen returns to: the title or the pause menu.
    State m_settingsReturnState = State::MENU;
    // Party levels when the current battle began, to report level-ups after.
    std::vector<int> m_levelsBeforeBattle;
    std::unique_ptr<BattleSummary> m_battleSummary;
    // Set by CYBERRAYNE_START=world; consumed on the first update().
    bool m_pendingDevStart = false;
    // Set by CYBERRAYNE_START=load; consumed on the first update().
    bool m_pendingDevLoad = false;
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
