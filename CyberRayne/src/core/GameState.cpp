#include "../../include/GameState.h"
#include "../../include/Player.h"
#include "../../include/Party.h"
#include "../../include/World.h"
#include "../../include/CharacterSelectionSystem.h"
#include "../../include/VulkanRenderer.h"
#include "../../include/BattleSystem.h"
#include "../../include/Map.h"
#include "../../include/MenuSystem.h"
#include <iostream>

GameState::GameState() : m_currentState(State::MENU), m_world(nullptr), m_party(nullptr), m_charSelectionSystem(nullptr), m_battleSystem(nullptr), m_menuSystem(nullptr), m_uiManager(nullptr), m_renderer(nullptr) {}

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
    
    std::cout << "Game state initialized successfully." << std::endl;
    return true;
}

void GameState::update(float deltaTime) {
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
                            std::cout << "Load game selected - Feature coming soon!" << std::endl;
                            // Placeholder for load game logic
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
            // Handle character selection logic
            // Wait for player to select a character using handleInput
            if (m_charSelectionSystem) {
                m_charSelectionSystem->update(deltaTime);
                
                // Check if character was selected (via Enter key in handleInput)
                if (m_charSelectionSystem->isCharacterSelected()) {
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

                    // Add the fixed companions for the vertical slice.
                    auto cyph = std::make_unique<Player>(Player::CharacterClass::MAGE, "Cyph");
                    cyph->initialize();
                    if (m_renderer) {
                        cyph->loadTexture(m_renderer);
                    }
                    m_party->addMember(std::move(cyph));

                    auto vash = std::make_unique<Player>(Player::CharacterClass::ROGUE, "Vash");
                    vash->initialize();
                    if (m_renderer) {
                        vash->loadTexture(m_renderer);
                    }
                    m_party->addMember(std::move(vash));

                    // Initialize world after character selection
                    m_world = std::make_unique<World>();
                    if (m_world->initialize()) {
                        m_world->setPlayer(m_party->getLeader());
                        // Load map textures
                        if (m_renderer) {
                            m_world->loadMapTextures(m_renderer);
                        }

                        // Set player spawn position in the starting village (on grass, south of house)
                        if (m_party->getLeader()) {
                            m_party->getLeader()->setPosition(15.0f, 16.0f);
                        }
                    }

                    // Move to world exploration state
                    m_currentState = State::WORLD_EXPLORATION;
                }
            }
            break;
        case State::WORLD_EXPLORATION:
            // Handle world exploration logic
            // Update world
            if (m_world) {
                m_world->update(deltaTime);
                
                // Check for map transitions
                m_world->checkMapTransition();
                
                // Check for enemy encounters
                if (m_world->checkEnemyEncounter()) {
                    m_currentState = State::BATTLE;
                }
            }
            break;
        case State::BATTLE:
            // Handle battle logic
            if (!m_battleSystem && m_party && m_world && m_world->getCurrentMap()) {
                // Create battle system
                m_battleSystem = std::make_unique<BattleSystem>(m_party.get());

                // Start battle with all enemies on the map
                m_battleSystem->startBattle(m_world->getCurrentMap()->getEnemies());
            }

            if (m_battleSystem) {
                if (m_uiManager->hasActionSelected()) {
                    const auto selectedAction = m_uiManager->getSelectedAction();
                    BattleSystem::BattleAction action;
                    switch (selectedAction) {
                        case UIManager::BattleAction::ATTACK:
                            action = BattleSystem::BattleAction::ATTACK;
                            break;
                        case UIManager::BattleAction::MAGIC:
                            action = BattleSystem::BattleAction::MAGIC;
                            break;
                        case UIManager::BattleAction::ITEM:
                            action = BattleSystem::BattleAction::ITEM;
                            break;
                        case UIManager::BattleAction::DEFEND:
                            action = BattleSystem::BattleAction::DEFEND;
                            break;
                        case UIManager::BattleAction::FLEE:
                            action = BattleSystem::BattleAction::FLEE;
                            break;
                        case UIManager::BattleAction::NONE:
                            m_uiManager->resetAction();
                            break;
                    }
                    if (selectedAction != UIManager::BattleAction::NONE) {
                        m_battleSystem->submitPlayerAction(action, m_uiManager->getSelectedSpellIndex());
                        m_uiManager->resetAction();
                    }
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

                    // If player won, remove defeated enemies from the map
                    if (result == BattleSystem::BattleResult::PLAYER_WIN) {
                        if (m_world && m_world->getCurrentMap()) {
                            m_world->getCurrentMap()->clearEnemies();
                            m_world->spawnEnemies();
                        }
                    }
                    // If player lost, transition to game over
                    if (result == BattleSystem::BattleResult::PLAYER_LOSE) {
                        m_currentState = State::GAME_OVER;
                    }
                }
            }
            break;
        case State::PAUSED:
            // Handle paused state
            break;
        case State::GAME_OVER:
            // Handle game over state
            break;
    }
    
    // Update party members if in world exploration state
    if (m_currentState == State::WORLD_EXPLORATION && m_party) {
        for (const auto& member : m_party->getMembers()) {
            if (member) {
                member->update(deltaTime);
            }
        }
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
        // Calculate viewport-relative position for player
        Map* currentMap = m_world->getCurrentMap();
        const int VIEWPORT_WIDTH = 15;
        const int VIEWPORT_HEIGHT = 15;
        int mapWidth = currentMap->getWidth();
        int mapHeight = currentMap->getHeight();
        int startX = (mapWidth > VIEWPORT_WIDTH) ? (mapWidth - VIEWPORT_WIDTH) / 2 : 0;
        int startY = (mapHeight > VIEWPORT_HEIGHT) ? (mapHeight - VIEWPORT_HEIGHT) / 2 : 0;
        
        // Calculate tile size (same as in Map::render)
        float screenAspect = 800.0f / 600.0f;
        float tileWidth = 2.0f / VIEWPORT_WIDTH;
        float tileHeight = 2.0f / VIEWPORT_HEIGHT;
        tileWidth = tileHeight / screenAspect;
        
        // Convert player world position to viewport-relative normalized coordinates
        float playerX = leader->getX();
        float playerY = leader->getY();
        float normalizedX = -1.0f + (playerX - startX) * tileWidth + tileWidth * 0.5f;
        float normalizedY = -1.0f + (playerY - startY) * tileHeight + tileHeight * 0.5f;

        // Render player at viewport-relative position
        if (leader->getX() >= startX && leader->getX() < startX + VIEWPORT_WIDTH &&
            leader->getY() >= startY && leader->getY() < startY + VIEWPORT_HEIGHT) {
            // Player is within viewport, render using texture
            int textureIndex = leader->getTextureIndex();
            if (textureIndex >= 0) {
                renderer->renderSpriteWithTexture(normalizedX, normalizedY, tileWidth, tileHeight, textureIndex);
            } else {
                renderer->renderSprite(normalizedX, normalizedY, tileWidth, tileHeight);
            }
        }
    }
}

void GameState::handleInput(InputAction action) {
    if (m_currentState == State::MENU && m_menuSystem) {
        m_menuSystem->handleInput(action);
    } else if (m_currentState == State::CHARACTER_SELECTION && m_charSelectionSystem) {
        m_charSelectionSystem->handleInput(action);
    } else if (m_currentState == State::WORLD_EXPLORATION && m_party && m_world) {
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
    } else if (m_currentState == State::BATTLE && m_uiManager) {
        m_uiManager->handleInput(action);
    }
}
