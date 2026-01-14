#include "../../include/GameState.h"
#include "../../include/Player.h"
#include "../../include/World.h"
#include "../../include/CharacterSelectionSystem.h"
#include "../../include/VulkanRenderer.h"
#include "../../include/BattleSystem.h"
#include "../../include/Map.h"
#include "../../include/MenuSystem.h"
#include <iostream>

GameState::GameState() : m_currentState(State::MENU), m_world(nullptr), m_player(nullptr), m_charSelectionSystem(nullptr), m_battleSystem(nullptr), m_menuSystem(nullptr), m_uiManager(nullptr), m_renderer(nullptr) {}

GameState::~GameState() {
    delete m_world;
    delete m_player;
    delete m_charSelectionSystem;
    delete m_battleSystem;
    delete m_menuSystem;
    delete m_uiManager;
}

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
    m_menuSystem = new MenuSystem();
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
    m_charSelectionSystem = new CharacterSelectionSystem();
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
    m_uiManager = new UIManager();
    
    std::cout << "Game state initialized successfully." << std::endl;
    return true;
}

void GameState::update(float deltaTime) {
    switch (m_currentState) {
        case State::MENU:
            // Handle menu logic
            std::cout << "In menu state" << std::endl;
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
            std::cout << "In character selection state" << std::endl;
            // Wait for player to select a character using handleInput
            if (m_charSelectionSystem) {
                m_charSelectionSystem->update(deltaTime);
                
                // Check if character was selected (via Enter key in handleInput)
                if (m_charSelectionSystem->isCharacterSelected()) {
                    // Create the player with the selected character
                    m_player = m_charSelectionSystem->createSelectedCharacter();
                    if (m_player) {
                        m_player->initialize();
                        
                        // Load player texture
                        if (m_renderer) {
                            m_player->loadTexture(m_renderer);
                        }
                        
                        // Initialize world after character selection
                        m_world = new World();
                        if (m_world->initialize()) {
                            m_world->setPlayer(m_player);
                            // Load map textures
                            if (m_renderer) {
                                m_world->loadMapTextures(m_renderer);
                            }
                            
                            // Set player spawn position in the starting village (on grass, south of house)
                            m_player->setPosition(15.0f, 16.0f);
                        }
                    }
                    
                    // Move to world exploration state
                    m_currentState = State::WORLD_EXPLORATION;
                }
            }
            break;
        case State::WORLD_EXPLORATION:
            // Handle world exploration logic
            std::cout << "In world exploration state" << std::endl;
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
            std::cout << "In battle state" << std::endl;
            if (!m_battleSystem && m_player && m_world && m_world->getCurrentMap()) {
                // Create battle system
                m_battleSystem = new BattleSystem(m_player, m_uiManager);
                
                // Start battle with all enemies on the map
                m_battleSystem->startBattle(m_world->getCurrentMap()->getEnemies());
            }
            
            if (m_battleSystem) {
                m_battleSystem->update();
                
                // Check battle result
                auto result = m_battleSystem->getResult();
                if (result != BattleSystem::BattleResult::ONGOING) {
                    // Battle ended, return to world exploration
                    delete m_battleSystem;
                    m_battleSystem = nullptr;
                    m_currentState = State::WORLD_EXPLORATION;
                    
                    // If player won, remove defeated enemies from the map
                    if (result == BattleSystem::BattleResult::PLAYER_WIN) {
                        // For now, we'll just respawn enemies
                        // In a real game, you might want to remove defeated enemies permanently
                        // or have them respawn after some time
                        m_world->spawnEnemies();
                    }
                }
            }
            break;
        case State::PAUSED:
            // Handle paused state
            std::cout << "In paused state" << std::endl;
            break;
        case State::GAME_OVER:
            // Handle game over state
            std::cout << "In game over state" << std::endl;
            break;
    }
    
    // Update player if in world exploration state
    if (m_currentState == State::WORLD_EXPLORATION && m_player) {
        m_player->update(deltaTime);
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
                // We need access to enemies, which are in the battle system or map
                // For now, let's get them from the map via world
                if (m_world && m_world->getCurrentMap()) {
                    m_uiManager->renderBattleUI(renderer, m_player, m_world->getCurrentMap()->getEnemies());
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
    
    // Render player if in world exploration state
    if (m_currentState == State::WORLD_EXPLORATION && m_player && m_world && m_world->getCurrentMap()) {
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
        float playerX = m_player->getX();
        float playerY = m_player->getY();
        float normalizedX = -1.0f + (playerX - startX) * tileWidth + tileWidth * 0.5f;
        float normalizedY = -1.0f + (playerY - startY) * tileHeight + tileHeight * 0.5f;
        
        // Render player at viewport-relative position
        if (m_player->getX() >= startX && m_player->getX() < startX + VIEWPORT_WIDTH &&
            m_player->getY() >= startY && m_player->getY() < startY + VIEWPORT_HEIGHT) {
            // Player is within viewport, render using texture
            int textureIndex = m_player->getTextureIndex();
            if (textureIndex >= 0) {
                renderer->renderSpriteWithTexture(normalizedX, normalizedY, tileWidth, tileHeight, textureIndex);
            } else {
                renderer->renderSprite(normalizedX, normalizedY, tileWidth, tileHeight);
            }
        }
    }
}

void GameState::handleInput(int key) {
    if (m_currentState == State::MENU && m_menuSystem) {
        m_menuSystem->handleInput(key);
    } else if (m_currentState == State::CHARACTER_SELECTION && m_charSelectionSystem) {
        m_charSelectionSystem->handleInput(key);
    } else if (m_currentState == State::WORLD_EXPLORATION && m_player && m_world) {
        // Handle player movement in world exploration
        Map* currentMap = m_world->getCurrentMap();
        if (currentMap) {
            switch (key) {
                case 0: // Up
                    m_player->moveUp(currentMap);
                    break;
                case 1: // Down
                    m_player->moveDown(currentMap);
                    break;
                case 3: // Left
                    m_player->moveLeft(currentMap);
                    break;
                case 4: // Right
                    m_player->moveRight(currentMap);
                    break;
            }
        }
    } else if (m_currentState == State::BATTLE && m_uiManager) {
        m_uiManager->handleInput(key);
    }
}
