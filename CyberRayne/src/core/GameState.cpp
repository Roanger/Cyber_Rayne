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
#include <cstdlib>
#include <iostream>
#include <string>

GameState::GameState() : m_currentState(State::MENU), m_world(nullptr), m_party(nullptr), m_charSelectionSystem(nullptr), m_battleSystem(nullptr), m_menuSystem(nullptr), m_uiManager(nullptr), m_audioManager(nullptr), m_inputConfig(nullptr), m_saveSystem(nullptr), m_questLog(nullptr), m_renderer(nullptr) {}

GameState::~GameState() = default;

void GameState::setRenderer(VulkanRenderer* renderer) {
    std::cout << "Setting renderer for GameState" << std::endl;
    m_renderer = renderer;
    
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
    
    // Initialize audio manager
    m_audioManager = std::make_unique<AudioManager>();
    if (!m_audioManager->initialize()) {
        std::cerr << "Failed to initialize audio manager!" << std::endl;
        // Non-fatal — audio is optional
    }

    // Initialize input config with default bindings
    m_inputConfig = std::make_unique<InputConfig>();

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
    }

    std::cout << "Game state initialized successfully." << std::endl;
    return true;
}

void GameState::update(float deltaTime) {
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
                            std::cout << "Settings selected - Feature coming soon!" << std::endl;
                            // Placeholder for settings logic
                            m_menuSystem->resetSelection();
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

                // Walking into a visible enemy, or a random ambush rolled on
                // the step just completed. Either fills the world's active
                // encounter, which the battle below consumes.
                bool encountered = m_world->checkEnemyEncounter();

                if (!encountered && m_party) {
                    Player* leader = m_party->getMember(0);
                    if (leader && leader->consumeStepCompleted()) {
                        encountered = m_world->onPlayerStep();
                    }
                }

                if (encountered) {
                    m_currentState = State::BATTLE;
                }
            }
            break;
        case State::PAUSED:
            // Frozen — no world updates while paused.
            break;
        case State::GAME_OVER:
            // Wait for player input to restart or quit.
            break;
        case State::BATTLE:
            // Handle battle logic
            if (!m_battleSystem && m_party && m_world && m_world->hasActiveEncounter()) {
                // Create battle system
                m_battleSystem = std::make_unique<BattleSystem>(m_party.get());

                // Fight only the enemies that triggered this encounter.
                m_battleSystem->startBattle(m_world->getActiveEncounter());
            }

            if (m_battleSystem) {
                // The UI now hands over a fully specified request, so there is
                // nothing to translate between the two layers.
                if (m_uiManager->hasActionSelected()) {
                    if (!m_battleSystem->submitPlayerAction(m_uiManager->getActionRequest())) {
                        // Rejected actions leave the turn intact; surface why so
                        // the player can pick something else.
                        m_uiManager->setStatusMessage(m_battleSystem->getLastRejectionReason());
                    }
                    m_uiManager->resetAction();
                }
                m_battleSystem->update(deltaTime);

                if (m_uiManager) {
                    m_uiManager->setCurrentActor(m_battleSystem->getCurrentActor());
                }

                // Check battle result
                auto result = m_battleSystem->getResult();
                if (result != BattleSystem::BattleResult::ONGOING) {
                    // Battle ended, return to world exploration
                    m_battleSystem.reset();
                    m_currentState = State::WORLD_EXPLORATION;

                    // Release the encounter group however the fight ended. The
                    // enemies were already taken off the map when the encounter
                    // began, so there is nothing left to retrigger on.
                    if (m_world) {
                        m_world->endEncounter();
                    }

                    // If player lost, transition to game over
                    if (result == BattleSystem::BattleResult::PLAYER_LOSE) {
                        m_currentState = State::GAME_OVER;
                    }
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
            // Render world exploration
            // Render world
            if (m_world) {
                m_world->render(renderer);
            }
            break;
        case State::BATTLE:
            // Render battle
            // Render a simple background for battle
            renderer->renderSprite(0.0f, 0.0f, 2.0f, 2.0f);
            
            // Render Battle UI
            if (m_uiManager && m_battleSystem) {
                if (m_party) {
                    m_uiManager->renderBattleUI(renderer, m_party.get(), m_battleSystem->getEnemies());
                }
            }
            break;
        case State::PAUSED:
            // Render paused state
            // Render a simple background for paused state
            renderer->renderSprite(0.0f, 0.0f, 2.0f, 2.0f);
            break;
        case State::GAME_OVER:
            // Render game over
            // Render a simple background for game over
            renderer->renderSprite(0.0f, 0.0f, 2.0f, 2.0f);
            break;
    }
    
    // Render party leader if in world exploration state
    Player* leader = m_party ? m_party->getLeader() : nullptr;
    if (m_currentState == State::WORLD_EXPLORATION && leader && m_world && m_world->getCurrentMap()) {
        // Calculate viewport matching Map::render
        Map* currentMap = m_world->getCurrentMap();
        float screenAspect = static_cast<float>(renderer->getSwapchainExtent().width) /
                             static_cast<float>(renderer->getSwapchainExtent().height);
        // Same viewport the map and entities use, so the player lines up with
        // the tiles it is standing on.
        Map::Viewport vp = currentMap->getViewport(screenAspect);

        float normalizedX = 0.0f;
        float normalizedY = 0.0f;
        currentMap->worldToNdc(vp, leader->getX(), leader->getY(), normalizedX, normalizedY);

        // Render player
        int textureIndex = leader->getTextureIndex();
        if (textureIndex >= 0) {
            renderer->renderSpriteWithTexture(normalizedX, normalizedY, vp.tileWidth, vp.tileHeight, textureIndex);
        } else {
            renderer->renderSprite(normalizedX, normalizedY, vp.tileWidth, vp.tileHeight);
        }
    }
}

void GameState::handleInput(InputAction action) {
    if (m_currentState == State::MENU && m_menuSystem) {
        m_menuSystem->handleInput(action);
    } else if (m_currentState == State::CHARACTER_SELECTION && m_charSelectionSystem) {
        m_charSelectionSystem->handleInput(action);
    } else if (m_currentState == State::WORLD_EXPLORATION && m_party && m_world) {
        // Pause toggle
        if (action == InputAction::PAUSE || action == InputAction::OPEN_MENU) {
            togglePause();
            return;
        }

        Map* currentMap = m_world->getCurrentMap();
        Player* leader = m_party->getLeader();
        if (currentMap && leader) {
            switch (action) {
                case InputAction::MOVE_UP:
                    leader->moveUp(currentMap);
                    break;
                case InputAction::MOVE_DOWN:
                    leader->moveDown(currentMap);
                    break;
                case InputAction::MOVE_LEFT:
                    leader->moveLeft(currentMap);
                    break;
                case InputAction::MOVE_RIGHT:
                    leader->moveRight(currentMap);
                    break;
                default:
                    break;
            }
        }
    } else if (m_currentState == State::PAUSED) {
        // Unpause on PAUSE/CANCEL/CONFIRM
        if (action == InputAction::PAUSE || action == InputAction::CANCEL || action == InputAction::CONFIRM) {
            togglePause();
        }
    } else if (m_currentState == State::BATTLE && m_uiManager) {
        m_uiManager->handleInput(action);
    } else if (m_currentState == State::GAME_OVER) {
        handleGameOverInput(action);
    }
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

    // Attach NPC dialogues
    ContentFactory::attachNPCDialogues(m_world.get());

    // Rebuild quest log for new game
    m_questLog = std::make_unique<QuestLog>();
    ContentFactory::createQuestChain(m_questLog.get());

    m_currentState = State::WORLD_EXPLORATION;
}

void GameState::loadGame() {
    if (!m_saveSystem || !m_questLog) {
        std::cerr << "Cannot load game: save system or quest log not initialized." << std::endl;
        return;
    }

    // Attempt to load party and quest log from save file.
    m_party = std::make_unique<Party>();
    if (!m_saveSystem->loadGame("savegame.txt", m_party.get(), m_questLog.get())) {
        std::cerr << "No save file found or load failed. Starting new game instead." << std::endl;
        m_party.reset();
        m_currentState = State::CHARACTER_SELECTION;
        return;
    }

    // Initialize world for loaded game
    m_world = std::make_unique<World>();
    if (m_world->initialize()) {
        m_world->setPlayer(m_party->getLeader());
        if (m_renderer) {
            m_world->loadMapTextures(m_renderer);
        }
        ContentFactory::attachNPCDialogues(m_world.get());
    }

    m_currentState = State::WORLD_EXPLORATION;
    std::cout << "Game loaded successfully." << std::endl;
}

void GameState::togglePause() {
    if (m_currentState == State::WORLD_EXPLORATION) {
        m_currentState = State::PAUSED;
        if (m_audioManager) {
            m_audioManager->setMuted(true);
        }
        std::cout << "Game paused." << std::endl;
    } else if (m_currentState == State::PAUSED) {
        m_currentState = State::WORLD_EXPLORATION;
        if (m_audioManager) {
            m_audioManager->setMuted(false);
        }
        std::cout << "Game resumed." << std::endl;
    }
}

void GameState::handleGameOverInput(InputAction action) {
    if (action == InputAction::CONFIRM) {
        // Restart — go back to menu
        m_party.reset();
        m_world.reset();
        m_battleSystem.reset();
        m_questLog = std::make_unique<QuestLog>();
        ContentFactory::createQuestChain(m_questLog.get());
        m_currentState = State::MENU;
        std::cout << "Returning to main menu." << std::endl;
    } else if (action == InputAction::CANCEL) {
        m_currentState = State::EXIT;
    }
}
