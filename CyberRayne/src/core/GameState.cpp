#include "../../include/BattleSounds.h"
#include "../../include/InputPrompts.h"
#include "../../include/EnemyTypes.h"
#include "../../include/ui/BattleScene.h"
#include "../../include/ui/SpriteLibrary.h"
#include "../../include/GameState.h"
#include "../../include/Player.h"
#include "../../include/Party.h"
#include "../../include/World.h"
#include "../../include/CharacterSelectionSystem.h"
#include "../../include/VulkanRenderer.h"
#include "../../include/BattleSystem.h"
#include "../../include/Map.h"
#include "../../include/MenuSystem.h"
#include "../../include/AudioManager.h"
#include "../../include/InputConfig.h"
#include "../../include/SaveSystem.h"
#include "../../include/QuestLog.h"
#include "../../include/ContentFactory.h"
#include "../../include/DialogueSystem.h"
#include "../../include/NPC.h"
#include "../../include/StoryDirector.h"
#include "../../include/ui/PauseMenu.h"
#include "../../include/ui/SettingsMenu.h"
#include "../../include/ui/StoryHud.h"
#include <cstdlib>
#include <iostream>
#include <string>

GameState::GameState() : m_currentState(State::MENU), m_world(nullptr), m_party(nullptr), m_charSelectionSystem(nullptr), m_battleSystem(nullptr), m_menuSystem(nullptr), m_uiManager(nullptr), m_audioManager(nullptr), m_inputConfig(nullptr), m_saveSystem(nullptr), m_questLog(nullptr), m_renderer(nullptr) {}

GameState::~GameState() = default;

void GameState::setRenderer(VulkanRenderer* renderer) {
    std::cout << "Setting renderer for GameState" << std::endl;
    m_renderer = renderer;
    if (m_audioManager && renderer) {
        m_audioManager->setAssetsPath(renderer->getAssetsBasePath());
    }
    
    // Load menu textures if menu system is initialized
    if (m_menuSystem && m_renderer) {
        std::cout << "Loading menu textures" << std::endl;
        m_menuSystem->loadTextures(m_renderer);
    } else {
        std::cout << "Menu system or renderer not initialized" << std::endl;
        if (!m_menuSystem) {
            std::cout << "  Menu system is null" << std::endl;
        }
        if (!m_renderer) {
            std::cout << "  Renderer is null" << std::endl;
        }
    }
    
    // Load character selection textures
    if (m_charSelectionSystem && m_renderer) {
        std::cout << "Loading character selection textures" << std::endl;
        m_charSelectionSystem->loadTextures(m_renderer);
    }
    
    // Initialize UIManager with renderer if available
    if (m_uiManager && m_renderer) {
        std::cout << "Initializing UIManager with renderer" << std::endl;
        m_uiManager->initialize(m_renderer);
    }

    if (m_storyHud && m_renderer) {
        m_storyHud->initialize(m_renderer);
    }

    if (m_renderer) {
        m_sprites = std::make_unique<SpriteLibrary>(m_renderer);
    }
}

bool GameState::initialize() {
    std::cout << "Initializing game state..." << std::endl;
    
    // Initialize menu system
    m_menuSystem = std::make_unique<MenuSystem>();
    if (!m_menuSystem->initialize()) {
        std::cerr << "Failed to initialize menu system!" << std::endl;
        return false;
    }
    // If renderer was set before initialize(), load menu textures now
    if (m_renderer && m_menuSystem) {
        std::cout << "Loading menu textures (post-initialize)" << std::endl;
        m_menuSystem->loadTextures(m_renderer);
    }
    
    // Initialize character selection system
    m_charSelectionSystem = std::make_unique<CharacterSelectionSystem>();
    if (!m_charSelectionSystem->initialize()) {
        std::cerr << "Failed to initialize character selection system!" << std::endl;
        return false;
    }
    // If renderer was set before initialize(), load character selection textures now
    if (m_renderer && m_charSelectionSystem) {
        std::cout << "Loading character selection textures (post-initialize)" << std::endl;
        m_charSelectionSystem->loadTextures(m_renderer);
    }

    // Initialize UI Manager
    m_uiManager = std::make_unique<UIManager>();

    m_dialogue = std::make_unique<DialogueSystem>();
    m_storyHud = std::make_unique<StoryHud>();
    m_pauseMenu = std::make_unique<PauseMenu>();
    m_settingsMenu = std::make_unique<SettingsMenu>();
    m_battleSummary = std::make_unique<BattleSummary>();
    m_battleScene = std::make_unique<BattleScene>();
    if (m_renderer && !m_sprites) {
        m_sprites = std::make_unique<SpriteLibrary>(m_renderer);
    }
    if (m_renderer) {
        m_storyHud->initialize(m_renderer);
    }
    
    // Initialize audio manager
    m_audioManager = std::make_unique<AudioManager>();
    // Sound is optional: with no device or no files the game runs silent.
    m_audioManager->initialize("assets", createDefaultAudioBackend());
    // Until every track exists, some places borrow another's music.
    m_audioManager->setMusicFallback("alley", "central");
    m_audioManager->setMusicFallback("plaza", "central");
    m_audioManager->setMusicFallback("boss", "battle");
    m_audioManager->setMusicFallback("ending", "title");

    // Initialize input config with default bindings
    m_inputConfig = std::make_unique<InputConfig>();
    m_inputConfig->loadFromFile(InputConfig::defaultConfigPath());

    // Initialize save system
    m_saveSystem = std::make_unique<SaveSystem>();

    // Initialize quest log
    m_questLog = std::make_unique<QuestLog>();
    ContentFactory::createQuestChain(m_questLog.get());

    // Dev shortcut: CYBERRAYNE_START=world drops straight into exploration with
    // the default party, so exploration and combat can be exercised without
    // clicking through the menus on every run. Deferred to the first update()
    // because the renderer is attached after initialize() returns, and
    // startNewGame() needs it to load map and character textures.
    if (const char* start = std::getenv("CYBERRAYNE_START")) {
        m_pendingDevStart = (std::string(start) == "world");
        // CYBERRAYNE_START=load resumes the saved game straight away.
        m_pendingDevLoad = (std::string(start) == "load");
        // CYBERRAYNE_START=battle starts a new game inside a mixed fight, for
        // working on the battle screen.
        m_pendingDevBattle = (std::string(start) == "battle");
        m_pendingDevStart = m_pendingDevStart || m_pendingDevBattle;
        // CYBERRAYNE_START=select opens on the character select screen.
        if (std::string(start) == "select") {
            m_currentState = State::CHARACTER_SELECTION;
        }
    }

    std::cout << "Game state initialized successfully." << std::endl;
    return true;
}

void GameState::sfx(const std::string& name) {
    if (m_audioManager) {
        m_audioManager->playSfx(name);
    }
}

void GameState::updateMusic() {
    if (!m_audioManager) {
        return;
    }
    switch (m_currentState) {
        case State::MENU:
        case State::CHARACTER_SELECTION:
            m_audioManager->playMusic("title");
            break;
        case State::WORLD_EXPLORATION:
        case State::PAUSED: {
            if (m_showEnding) {
                m_audioManager->playMusic("ending");
                break;
            }
            const Map* map = m_world ? m_world->getCurrentMap() : nullptr;
            const std::string name = map ? map->getName() : std::string();
            if (name == "Neon Alley") {
                m_audioManager->playMusic("alley");
            } else if (name == "Corporate Plaza") {
                m_audioManager->playMusic("plaza");
            } else {
                m_audioManager->playMusic("central");
            }
            break;
        }
        case State::BATTLE:
            // Story fights carry a tag; ambushes do not.
            m_audioManager->playMusic(m_battleTag.empty() ? "battle" : "boss");
            break;
        case State::BATTLE_RESULT:
            m_audioManager->playMusicOnce("victory");
            break;
        case State::GAME_OVER:
            m_audioManager->playMusicOnce("game_over");
            break;
        case State::SETTINGS:
        case State::EXIT:
            break;  // keep whatever was playing
    }
}

void GameState::playUiSound(InputAction action) {
    const bool vertical = action == InputAction::MOVE_UP || action == InputAction::MOVE_DOWN;
    const bool horizontal = action == InputAction::MOVE_LEFT || action == InputAction::MOVE_RIGHT;
    const bool talking = m_dialogue && m_dialogue->isActive();

    bool menuOpen = false;   // is a menu what this input goes to?
    bool moves = vertical;   // does this direction move its cursor?
    switch (m_currentState) {
        case State::MENU:
        case State::PAUSED:
        case State::SETTINGS:
            menuOpen = true;
            break;
        case State::CHARACTER_SELECTION:
            menuOpen = true;
            moves = horizontal;
            break;
        case State::BATTLE:
            // Only while someone is choosing a command.
            menuOpen = m_battleSystem && m_battleSystem->getCurrentActor();
            moves = vertical || horizontal;
            break;
        case State::BATTLE_RESULT:
        case State::GAME_OVER:
            menuOpen = true;
            moves = false;
            break;
        case State::WORLD_EXPLORATION:
            menuOpen = talking || m_showEnding;
            break;
        case State::EXIT:
            break;
    }
    if (!menuOpen) {
        return;
    }
    if (moves) {
        sfx("ui_move");
    } else if (action == InputAction::CONFIRM) {
        sfx(talking ? "talk_blip" : "ui_confirm");
    } else if (action == InputAction::CANCEL) {
        sfx("ui_cancel");
    }
}

void GameState::update(float deltaTime) {
    m_animTime += deltaTime;
    if (m_audioManager) {
        m_audioManager->update(deltaTime);
    }
    updateMusic();
    if (m_pendingDevStart) {
        m_pendingDevStart = false;
        std::cout << "CYBERRAYNE_START=world — skipping menus." << std::endl;
        // Commit a character choice first; startNewGame() builds Rayne from the
        // selection and would otherwise produce a party missing its leader.
        if (m_charSelectionSystem) {
            m_charSelectionSystem->selectCharacter(1);  // Solo
            m_charSelectionSystem->confirmSelection();
        }
        startNewGame();
    }

    if (m_pendingDevBattle) {
        m_pendingDevBattle = false;
        if (m_world) {
            std::vector<std::unique_ptr<Enemy>> foes;
            foes.push_back(std::make_unique<StreetThug>(1));
            foes.push_back(std::make_unique<SecurityDrone>(1));
            foes.push_back(std::make_unique<CorporateEnforcer>(1));
            m_world->startEncounterWith(std::move(foes));
            m_currentState = State::BATTLE;
        }
    }

    if (m_pendingDevLoad) {
        m_pendingDevLoad = false;
        std::cout << "CYBERRAYNE_START=load — loading the saved game." << std::endl;
        loadGame();
    }

    switch (m_currentState) {
        case State::MENU:
            // Handle menu logic
            if (m_menuSystem) {
                m_menuSystem->update(deltaTime);
                
                // Check if an option was selected
                if (m_menuSystem->isOptionSelected()) {
                    switch (m_menuSystem->getSelectedOption()) {
                        case MenuSystem::MenuItem::START_GAME:
                            m_currentState = State::CHARACTER_SELECTION;
                            m_menuSystem->resetSelection();
                            break;
                        case MenuSystem::MenuItem::LOAD_GAME:
                            std::cout << "Load game selected." << std::endl;
                            loadGame();
                            m_menuSystem->resetSelection();
                            break;
                        case MenuSystem::MenuItem::SETTINGS:
                            m_menuSystem->resetSelection();
                            openSettings();
                            break;
                        case MenuSystem::MenuItem::QUIT:
                            std::cout << "Quit selected" << std::endl;
                            m_currentState = State::EXIT;
                            m_menuSystem->resetSelection();
                            break;
                    }
                }
            }
            break;
        case State::CHARACTER_SELECTION:
            if (m_charSelectionSystem) {
                m_charSelectionSystem->update(deltaTime);
                
                if (m_charSelectionSystem->isCharacterSelected()) {
                    startNewGame();
                }
            }
            break;
        case State::WORLD_EXPLORATION:
            if (m_storyHud) {
                m_storyHud->update(deltaTime);
            }
            // The world holds still while someone is talking or the ending
            // card is up.
            if ((m_dialogue && m_dialogue->isActive()) || m_showEnding) {
                break;
            }

            // Update party members first so movement completes before transition check
            if (m_party) {
                for (const auto& member : m_party->getMembers()) {
                    if (member) {
                        member->update(deltaTime);
                    }
                }
            }

            // Handle world exploration logic
            if (m_world) {
                m_world->update(deltaTime);

                // Check for map transitions
                m_world->checkMapTransition();

                if (m_storyDirector && m_world->getCurrentMap()) {
                    const std::string blocked = m_world->consumeBlockedMap();
                    if (!blocked.empty() && m_storyHud) {
                        m_storyHud->pushToast(m_storyDirector->getLockedMessage(blocked));
                        sfx("blocked");
                    }

                    const std::string& mapName = m_world->getCurrentMap()->getName();
                    if (mapName != m_lastMapName) {
                        m_lastMapName = mapName;
                        sfx("travel");
                        m_storyDirector->onMapEntered(mapName);
                        syncStory();
                    }
                }

                // Walking into a visible enemy, or a random ambush rolled on
                // the step just completed. Either fills the world's active
                // encounter, which the battle below consumes.
                bool encountered = m_world->checkEnemyEncounter();

                if (!encountered && m_party) {
                    Player* leader = m_party->getMember(0);
                    if (leader && leader->consumeStepCompleted()) {
                        sfx("step");
                        encountered = m_world->onPlayerStep();
                    }
                }

                if (encountered) {
                    m_battleTag = m_world->getActiveEncounterTag();
                    sfx("encounter");
                    m_currentState = State::BATTLE;
                } else if (m_heldDx != 0 || m_heldDy != 0) {
                    // A direction is held: start the next step in the same
                    // frame the last one ended, so walking never pauses
                    // between tiles.
                    tryStep(m_heldDx, m_heldDy);
                }
            }
            break;
        case State::PAUSED:
            // Frozen — no world updates while paused.
            break;
        case State::BATTLE_RESULT:
            // Waiting for the player to dismiss the victory card. The scene
            // behind it keeps animating (level-up bursts).
            if (m_battleScene) {
                m_battleScene->update(deltaTime);
            }
            break;
        case State::SETTINGS:
            break;
        case State::GAME_OVER:
            // Wait for player input to restart or quit.
            break;
        case State::EXIT:
            // Game::run sees this state and shuts down.
            break;
        case State::BATTLE:
            // Handle battle logic
            if (!m_battleSystem && m_party && m_world && m_world->hasActiveEncounter()) {
                // Create battle system
                m_battleSystem = std::make_unique<BattleSystem>(m_party.get());

                // Fight only the enemies that triggered this encounter.
                m_battleSystem->startBattle(m_world->getActiveEncounter());
                if (m_uiManager) {
                    m_uiManager->setParty(m_party.get());
                }
                if (m_battleScene) {
                    // Taken before anyone falls: the scene keeps every enemy's
                    // place on screen for the whole fight.
                    const Map* map = m_world->getCurrentMap();
                    m_battleScene->begin(m_party.get(), m_battleSystem->getEnemies(),
                                         map ? map->getName() : std::string());
                }

                m_levelsBeforeBattle.clear();
                for (const auto& member : m_party->getMembers()) {
                    m_levelsBeforeBattle.push_back(member ? member->getLevel() : 0);
                }
            }

            if (m_battleSystem) {
                // The UI now hands over a fully specified request, so there is
                // nothing to translate between the two layers.
                if (m_uiManager->hasActionSelected()) {
                    if (!m_battleSystem->submitPlayerAction(m_uiManager->getActionRequest())) {
                        // Rejected actions leave the turn intact; surface why so
                        // the player can pick something else.
                        m_uiManager->setStatusMessage(m_battleSystem->getLastRejectionReason());
                        sfx("ui_error");
                    }
                    m_uiManager->resetAction();
                }
                m_battleSystem->update(deltaTime);

                if (m_uiManager) {
                    m_uiManager->setCurrentActor(m_battleSystem->getCurrentActor());
                }

                // A party member's gauge has filled: their turn begins.
                const Combatant* actor = m_battleSystem->getCurrentActor();
                if (actor && actor != m_lastBattleActor) {
                    sfx("turn_ready");
                }
                m_lastBattleActor = actor;

                const auto events = m_battleSystem->takeEvents();
                for (const std::string& sound : battleSoundsFor(events)) {
                    sfx(sound);
                }
                if (m_battleScene) {
                    m_battleScene->onEvents(events);
                    m_battleScene->update(deltaTime);
                }

                // Let the last hit play out before leaving the battle screen.
                if (m_battleSystem->getResult() != BattleSystem::BattleResult::ONGOING &&
                    !(m_battleScene && m_battleScene->isBusy())) {
                    finishBattle();
                }
            }
            break;
    }
}

void GameState::render(VulkanRenderer* renderer) {
    switch (m_currentState) {
        case State::MENU:
            // Render menu
            if (m_menuSystem) {
                m_menuSystem->render(renderer);
            } else {
                // Fallback render
                renderer->renderSprite(0.0f, 0.0f, 2.0f, 2.0f);
            }
            break;
        case State::CHARACTER_SELECTION:
            // Render character selection
            if (m_charSelectionSystem) {
                m_charSelectionSystem->render(renderer);
            } else {
                // Fallback render
                renderer->renderSprite(0.0f, 0.0f, 2.0f, 2.0f);
            }
            break;
        case State::WORLD_EXPLORATION:
        case State::PAUSED:
            // The pause menu is drawn over the frozen world.
            renderWorldScene(renderer);
            break;
        case State::BATTLE_RESULT:
            // The battlefield, now empty of enemies, stays behind the card.
            if (m_battleScene && m_uiManager && m_sprites) {
                m_battleScene->render(renderer, m_uiManager->text(), m_sprites.get(), nullptr, nullptr);
            }
            if (m_storyHud && m_battleSummary) {
                m_storyHud->renderBattleResult(renderer, *m_battleSummary);
            }
            break;
        case State::BATTLE:
            if (m_uiManager && m_battleSystem && m_party) {
                if (m_battleScene && m_sprites) {
                    m_battleScene->render(renderer, m_uiManager->text(), m_sprites.get(),
                                          m_battleSystem->getCurrentActor(),
                                          m_uiManager->getHighlightedTarget());
                }
                m_uiManager->setSprites(m_sprites.get());
                m_uiManager->renderBattleUI(renderer, m_party.get(),
                                            m_battleSystem->getLivingEnemies(),
                                            m_battleSystem->getLivingAllies());
            }
            break;
        case State::SETTINGS:
            if (m_settingsMenu && m_storyHud && m_inputConfig) {
                m_settingsMenu->render(renderer, m_storyHud->text(), *m_inputConfig);
            }
            break;
        case State::GAME_OVER:
            if (m_storyHud) {
                m_storyHud->renderGameOver(renderer, m_saveSystem &&
                                           m_saveSystem->saveExists(SaveSystem::defaultSavePath()));
            }
            break;
        case State::EXIT:
            break;
    }
    
    if (m_currentState == State::PAUSED && m_pauseMenu && m_storyHud) {
        const std::string mapName =
            (m_world && m_world->getCurrentMap()) ? m_world->getCurrentMap()->getName() : "";
        m_pauseMenu->render(renderer, m_storyHud->text(), m_party.get(), m_questLog.get(), mapName, m_sprites.get());
    }
}

void GameState::renderWorldScene(VulkanRenderer* renderer) {
    if (m_world) {
        // Draws the map, then NPCs, enemies and the party leader together,
        // sorted so whoever stands lower on screen is in front.
        m_world->render(renderer, m_sprites.get(), m_animTime, m_talkingTo);
    }

    Player* leader = m_party ? m_party->getLeader() : nullptr;

    // Story overlay goes on last so it sits above the world and the player.
    if (m_currentState == State::WORLD_EXPLORATION && m_storyHud && m_world && m_world->getCurrentMap()) {
        if (m_showEnding) {
            m_storyHud->renderEnding(renderer);
        } else {
            const bool talking = m_dialogue && m_dialogue->isActive();

            std::string questName;
            std::string objective;
            if (m_storyDirector) {
                m_storyDirector->getTrackedObjective(questName, objective);
            }

            std::string prompt;
            if (!talking && leader && !leader->isMoving()) {
                if (const NPC* npc = m_world->findNpcNearPlayer(TALK_RANGE)) {
                    prompt = InputPrompts::confirm() + ": Talk to " + npc->getName();
                }
            }

            m_storyHud->renderExploration(renderer, m_world->getCurrentMap()->getName(),
                                          questName, objective, prompt);
            if (talking) {
                m_storyHud->renderDialogue(renderer, *m_dialogue);
            }
            m_storyHud->renderToast(renderer);
        }
    }
}

void GameState::handleInput(InputAction action) {
    playUiSound(action);
    if (m_currentState == State::MENU && m_menuSystem) {
        m_menuSystem->handleInput(action);
    } else if (m_currentState == State::CHARACTER_SELECTION && m_charSelectionSystem) {
        m_charSelectionSystem->handleInput(action);
    } else if (m_currentState == State::WORLD_EXPLORATION && m_party && m_world) {
        handleExplorationInput(action);
    } else if (m_currentState == State::PAUSED && m_pauseMenu) {
        switch (m_pauseMenu->handleInput(action, m_party.get())) {
            case PauseMenu::Command::RESUME:
                togglePause();
                break;
            case PauseMenu::Command::SAVE:
                if (saveGame()) {
                    m_pauseMenu->setStatus("Game saved.");
                    sfx("ui_save");
                } else {
                    m_pauseMenu->setStatus("Save failed. See the log for details.");
                    sfx("ui_error");
                }
                break;
            case PauseMenu::Command::OPEN_SETTINGS:
                openSettings();
                break;
            case PauseMenu::Command::QUIT_TO_TITLE:
                if (m_audioManager) {
                    m_audioManager->setMusicDucked(false);
                }
                returnToTitle();
                break;
            case PauseMenu::Command::NONE:
                break;
        }
    } else if (m_currentState == State::SETTINGS && m_settingsMenu && m_inputConfig) {
        if (m_settingsMenu->handleInput(action, *m_inputConfig) == SettingsMenu::Command::CLOSE) {
            closeSettings();
        } else if (m_settingsMenu->consumeChanged()) {
            m_inputConfig->saveToFile(InputConfig::defaultConfigPath());
        }
    } else if (m_currentState == State::BATTLE_RESULT) {
        if (action == InputAction::CONFIRM) {
            m_currentState = State::WORLD_EXPLORATION;
        }
    } else if (m_currentState == State::BATTLE && m_uiManager) {
        m_uiManager->handleInput(action);
    } else if (m_currentState == State::GAME_OVER) {
        handleGameOverInput(action);
    }
}

void GameState::finishBattle() {
    const auto result = m_battleSystem->getResult();
    const int expEarned = m_battleSystem->getExperienceEarned();
    const int goldEarned = m_battleSystem->getGoldEarned();
    m_battleSystem.reset();
    m_lastBattleActor = nullptr;
    // The scene points at the enemies, which are destroyed just below.
    if (m_battleScene) {
        m_battleScene->clearEnemies();
    }

    // Release the encounter group however the fight ended. The enemies were
    // already taken off the map when the encounter began, so there is nothing
    // left to retrigger on.
    if (m_world) {
        m_world->endEncounter();
    }

    if (m_storyDirector && !m_battleTag.empty()) {
        if (result == BattleSystem::BattleResult::PLAYER_WIN) {
            m_storyDirector->onEncounterWon(m_battleTag);
        } else if (result == BattleSystem::BattleResult::PLAYER_FLED && m_world &&
                   m_world->getCurrentMap() && m_party && m_party->getLeader()) {
            // The fight will be put back where it stood, so step the party
            // away from it first.
            StoryDirector::EncounterSpec spec;
            if (m_storyDirector->getPendingEncounter(m_world->getCurrentMap()->getName(), spec)) {
                m_party->getLeader()->teleport(spec.retreatX, spec.retreatY);
            }
        }
    }
    m_battleTag.clear();
    syncStory();

    if (result == BattleSystem::BattleResult::PLAYER_LOSE) {
        m_currentState = State::GAME_OVER;
        return;
    }

    if (result == BattleSystem::BattleResult::PLAYER_FLED) {
        if (m_storyHud) {
            m_storyHud->pushToast("Got away safely.");
        }
        sfx("flee");
        m_currentState = State::WORLD_EXPLORATION;
        return;
    }

    // Victory: tally what changed for the results card.
    *m_battleSummary = BattleSummary{};
    m_battleSummary->experience = expEarned;
    m_battleSummary->gold = goldEarned;
    if (m_party) {
        const auto& members = m_party->getMembers();
        for (std::size_t i = 0; i < members.size(); ++i) {
            Player* member = members[i].get();
            if (!member) continue;

            // Nobody stays down after a win; there is no other way back up yet.
            if (!member->isAlive()) {
                member->setHealth(1);
                m_battleSummary->revived.push_back(member->getName() + " is back on their feet");
            }
            if (i < m_levelsBeforeBattle.size() && member->getLevel() > m_levelsBeforeBattle[i]) {
                if (m_battleScene) {
                    m_battleScene->playEffect(member, "vfx_levelup");
                }
                m_battleSummary->levelUps.push_back(member->getName() + " reached level " +
                                                    std::to_string(member->getLevel()));
            }
        }
    }
    if (!m_battleSummary->levelUps.empty()) {
        sfx("level_up");
    }
    m_currentState = State::BATTLE_RESULT;
}

void GameState::handleExplorationInput(InputAction action) {
    if (m_showEnding) {
        if (action == InputAction::CONFIRM) {
            m_showEnding = false;
        }
        return;
    }

    if (m_dialogue && m_dialogue->isActive()) {
        handleDialogueInput(action);
        return;
    }

    if (action == InputAction::PAUSE || action == InputAction::OPEN_MENU) {
        togglePause();
        return;
    }

    int dx = 0;
    int dy = 0;
    switch (action) {
        case InputAction::MOVE_UP:    dy = -1; break;
        case InputAction::MOVE_DOWN:  dy = 1;  break;
        case InputAction::MOVE_LEFT:  dx = -1; break;
        case InputAction::MOVE_RIGHT: dx = 1;  break;
        case InputAction::CONFIRM:
            tryStartDialogue();
            return;
        default:
            return;
    }

    tryStep(dx, dy);
}

void GameState::tryStep(int dx, int dy) {
    Map* currentMap = m_world ? m_world->getCurrentMap() : nullptr;
    Player* leader = m_party ? m_party->getLeader() : nullptr;
    if (!currentMap || !leader || leader->isMoving()) {
        return;
    }
    // NPCs are solid: you talk to them, you don't walk through them. Still
    // turn to face them, so it is clear who Confirm will talk to.
    const int targetX = static_cast<int>(leader->getX()) + dx;
    const int targetY = static_cast<int>(leader->getY()) + dy;
    if (m_world->isNpcAt(targetX, targetY)) {
        leader->face(dx, dy);
        return;
    }
    leader->move(static_cast<float>(dx), static_cast<float>(dy), currentMap);
}

void GameState::tryStartDialogue() {
    Player* leader = m_party ? m_party->getLeader() : nullptr;
    if (!leader || leader->isMoving() || !m_dialogue || !m_storyDirector) {
        return;
    }

    NPC* npc = m_world->findNpcNearPlayer(TALK_RANGE);
    if (!npc) {
        return;
    }

    m_talkingTo = npc->getName();
    m_reportedNodeId = -1;
    if (m_dialogue->startDialogue(m_storyDirector->buildDialogue(m_talkingTo))) {
        reportDialogueNode();
    } else {
        m_talkingTo.clear();
    }
}

void GameState::handleDialogueInput(InputAction action) {
    m_dialogue->handleInput(action);

    if (m_dialogue->isFinished()) {
        m_dialogue->releaseTree();
        m_talkingTo.clear();
        m_reportedNodeId = -1;
        if (m_storyDirector && m_storyDirector->consumeEndingTrigger()) {
            m_showEnding = true;
        }
        syncStory();
        return;
    }

    reportDialogueNode();
}

void GameState::reportDialogueNode() {
    const DialogueNode* node = m_dialogue ? m_dialogue->getCurrentNode() : nullptr;
    if (!node || !m_storyDirector || node->getId() == m_reportedNodeId) {
        return;
    }
    m_reportedNodeId = node->getId();
    if (node->getId() == StoryDirector::NODE_VENDOR_MEAL) {
        sfx("rest");
    }
    m_storyDirector->onDialogueNode(m_talkingTo, node->getId());
    syncStory();
}

void GameState::beginStory() {
    resetStoryPresentation();
    m_storyDirector = std::make_unique<StoryDirector>(m_questLog.get(), m_party.get());
    m_lastMapName = (m_world && m_world->getCurrentMap()) ? m_world->getCurrentMap()->getName() : "";
    syncStory();
}

void GameState::syncStory() {
    if (!m_storyDirector || !m_world) {
        return;
    }

    static const char* const kLockableMaps[] = {"Corporate Plaza"};
    for (const char* mapName : kLockableMaps) {
        m_world->setMapLocked(mapName, !m_storyDirector->isMapUnlocked(mapName));
    }

    if (Map* map = m_world->getCurrentMap()) {
        StoryDirector::EncounterSpec spec;
        if (m_storyDirector->getPendingEncounter(map->getName(), spec)) {
            m_world->ensureStoryEncounter(spec.tag, spec.x, spec.y, spec.enemies);
        }
    }

    if (m_storyHud) {
        for (const auto& message : m_storyDirector->takeNotifications()) {
            m_storyHud->pushToast(message);
            if (message.rfind("New quest", 0) == 0) {
                sfx("quest_new");
            } else if (message.rfind("Quest complete", 0) == 0) {
                sfx("quest_done");
            }
        }
    }
}

void GameState::resetStoryPresentation() {
    if (m_dialogue) {
        m_dialogue->releaseTree();
    }
    if (m_storyHud) {
        m_storyHud->clearToasts();
    }
    m_talkingTo.clear();
    m_reportedNodeId = -1;
    m_showEnding = false;
    m_battleTag.clear();
}

void GameState::startNewGame() {
    // Create the party with Rayne's class chosen in character selection.
    m_party = std::make_unique<Party>();
    auto rayne = m_charSelectionSystem->createSelectedCharacter();
    if (rayne) {
        rayne->setName("Rayne");
        rayne->initialize();
        if (m_renderer) {
            rayne->loadTexture(m_renderer);
        }
        m_party->addMember(std::move(rayne));
    }

    auto cyph = std::make_unique<Player>(Player::CharacterClass::NETRUNNER, "Cyph");
    cyph->initialize();
    if (m_renderer) {
        cyph->loadTexture(m_renderer);
    }
    m_party->addMember(std::move(cyph));

    auto vash = std::make_unique<Player>(Player::CharacterClass::INFILTRATOR, "Vash");
    vash->initialize();
    if (m_renderer) {
        vash->loadTexture(m_renderer);
    }
    m_party->addMember(std::move(vash));

    ContentFactory::grantStartingKit(m_party.get());

    // Initialize world
    m_world = std::make_unique<World>();
    if (m_world->initialize()) {
        m_world->setPlayer(m_party->getLeader());
        if (m_renderer) {
            m_world->loadMapTextures(m_renderer);
        }
        if (m_party->getLeader()) {
            m_party->getLeader()->setPosition(15.0f, 16.0f);
        }
    }

    // Rebuild quest log for new game
    m_questLog = std::make_unique<QuestLog>();
    ContentFactory::createQuestChain(m_questLog.get());

    beginStory();

    m_currentState = State::WORLD_EXPLORATION;
}

void GameState::loadGame() {
    if (!m_saveSystem) {
        std::cerr << "Cannot load game: save system not initialized." << std::endl;
        return;
    }

    // Load into fresh objects and only swap them in on success, so a missing
    // or broken save leaves the current state untouched.
    auto party = std::make_unique<Party>();
    auto questLog = std::make_unique<QuestLog>();
    SaveWorldState worldState;
    const std::string path = SaveSystem::defaultSavePath();
    if (!m_saveSystem->loadGame(path, party.get(), questLog.get(), &worldState) ||
        party->isEmpty() || questLog->isEmpty()) {
        std::cerr << "No usable save at " << path << ". Starting a new game instead." << std::endl;
        m_currentState = State::CHARACTER_SELECTION;
        return;
    }

    // A save from before the shared bag existed would otherwise resume with
    // nothing to heal with or equip.
    if (!worldState.hasBag) {
        ContentFactory::grantStartingKit(party.get());
    }

    // The director points at the objects being replaced.
    m_storyDirector.reset();
    m_battleSystem.reset();
    m_party = std::move(party);
    m_questLog = std::move(questLog);

    if (m_renderer) {
        for (const auto& member : m_party->getMembers()) {
            if (member) member->loadTexture(m_renderer);
        }
    }

    m_world = std::make_unique<World>();
    if (m_world->initialize()) {
        Player* leader = m_party->getLeader();
        m_world->setPlayer(leader);
        if (m_renderer) {
            m_world->loadMapTextures(m_renderer);
        }
        // Saves from before the map was recorded resume in the first district.
        const std::string mapName = worldState.mapName.empty() ? "Neo-Kyoto Central" : worldState.mapName;
        const float x = leader ? leader->getX() : 0.0f;
        const float y = leader ? leader->getY() : 0.0f;
        if (!m_world->travelTo(mapName, x, y)) {
            std::cerr << "Save names an unknown map '" << mapName << "'. Starting in Neo-Kyoto Central."
                      << std::endl;
            m_world->travelTo("Neo-Kyoto Central", -1.0f, -1.0f);
        }
    }

    beginStory();

    m_currentState = State::WORLD_EXPLORATION;
    std::cout << "Game loaded from " << path << std::endl;
}

bool GameState::saveGame() {
    if (!m_saveSystem || !m_party || !m_questLog || !m_world || !m_world->getCurrentMap()) {
        return false;
    }
    SaveWorldState worldState;
    worldState.mapName = m_world->getCurrentMap()->getName();
    const std::string path = SaveSystem::defaultSavePath();
    const bool ok = m_saveSystem->saveGame(path, m_party.get(), m_questLog.get(), &worldState);
    std::cout << (ok ? "Game saved to " : "Failed to save game to ") << path << std::endl;
    return ok;
}

bool GameState::wantsRawKey() const {
    return m_currentState == State::SETTINGS && m_settingsMenu && m_settingsMenu->wantsRawKey();
}

void GameState::handleRawKey(int key) {
    if (!wantsRawKey() || !m_inputConfig) {
        return;
    }
    m_settingsMenu->handleRawKey(key, *m_inputConfig);
    if (m_settingsMenu->consumeChanged()) {
        m_inputConfig->saveToFile(InputConfig::defaultConfigPath());
    }
}

void GameState::openSettings() {
    if (!m_settingsMenu) {
        return;
    }
    m_settingsReturnState = m_currentState;
    m_settingsMenu->open();
    m_currentState = State::SETTINGS;
}

void GameState::closeSettings() {
    m_currentState = m_settingsReturnState;
}

void GameState::returnToTitle() {
    // The director points at the party and quest log about to be replaced.
    m_storyDirector.reset();
    resetStoryPresentation();
    m_party.reset();
    m_world.reset();
    m_battleSystem.reset();
    m_questLog = std::make_unique<QuestLog>();
    ContentFactory::createQuestChain(m_questLog.get());
    m_currentState = State::MENU;
    std::cout << "Returning to main menu." << std::endl;
}

void GameState::togglePause() {
    if (m_currentState == State::WORLD_EXPLORATION) {
        m_currentState = State::PAUSED;
        if (m_pauseMenu) {
            m_pauseMenu->open();
        }
        if (m_audioManager) {
            m_audioManager->setMusicDucked(true);
        }
        sfx("ui_pause");
        std::cout << "Game paused." << std::endl;
    } else if (m_currentState == State::PAUSED) {
        m_currentState = State::WORLD_EXPLORATION;
        if (m_audioManager) {
            m_audioManager->setMusicDucked(false);
        }
        std::cout << "Game resumed." << std::endl;
    }
}

void GameState::handleGameOverInput(InputAction action) {
    if (action == InputAction::CONFIRM) {
        if (m_saveSystem && m_saveSystem->saveExists(SaveSystem::defaultSavePath())) {
            loadGame();
            // A save that fails to load drops to character selection; make
            // sure a lost game never lingers behind it.
            if (m_currentState != State::WORLD_EXPLORATION) {
                returnToTitle();
            }
        } else {
            returnToTitle();
        }
    } else if (action == InputAction::CANCEL) {
        m_currentState = State::EXIT;
    }
}
