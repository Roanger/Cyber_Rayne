#include "../../include/Game.h"
#include "../../include/GameState.h"
#include "../../include/VulkanRenderer.h"
#include <iostream>
#include <Windows.h>
#include <thread>
#include <chrono>

Game::Game() : m_gameState(nullptr), m_renderer(nullptr), m_running(false) {}

Game::~Game() {
    shutdown();
}

bool Game::initialize() {
    std::cout << "Initializing game..." << std::endl;
    
    // Initialize Vulkan renderer
    m_renderer = std::make_unique<VulkanRenderer>();
    std::cout << "Vulkan renderer created." << std::endl;
    if (!m_renderer->initialize(1920, 1080, "Cyber Rayne")) {
        std::cerr << "Failed to initialize Vulkan renderer!" << std::endl;
        return false;
    }
    std::cout << "Vulkan renderer initialized." << std::endl;
    
    // Initialize game state
    m_gameState = std::make_unique<GameState>();
    std::cout << "Game state created." << std::endl;
    if (!m_gameState->initialize()) {
        std::cerr << "Failed to initialize game state!" << std::endl;
        return false;
    }
    std::cout << "Game state initialized." << std::endl;
    
    // Set renderer for game state (needed for menu texture loading)
    std::cout << "Setting renderer for game state..." << std::endl;
    m_gameState->setRenderer(m_renderer.get());
    std::cout << "Renderer set for game state." << std::endl;
    
    m_running = true;
    std::cout << "Game initialized successfully." << std::endl;
    return true;
}

void Game::run() {
    std::cout << "Starting game loop..." << std::endl;
    
    // Simple game loop
    const int targetFPS = 60;
    const std::chrono::milliseconds frameDelay(1000 / targetFPS);
    
    auto lastTime = std::chrono::high_resolution_clock::now();
    
    std::cout << "Entering game loop..." << std::endl;
    
    while (m_running && m_renderer->isRunning()) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        // Log frame rate every 60 frames to avoid spam
        static int frameCount = 0;
        frameCount++;
        if (frameCount % 60 == 0) {
            std::cout << "[DEBUG] Frame " << frameCount << " - DeltaTime: " << deltaTime << "s (FPS: " << (1.0f / deltaTime) << ")" << std::endl;
        }
        
        // Handle input
        if (m_gameState) {
            if (GetAsyncKeyState(VK_UP) & 0x8000) {
                m_gameState->handleInput(0);
            }
            if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
                m_gameState->handleInput(1);
            }
            if (GetAsyncKeyState(VK_RETURN) & 0x8000) {
                std::cout << "[DEBUG] Enter key pressed - handling input 2" << std::endl;
                m_gameState->handleInput(2);
            }
            if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
                m_gameState->handleInput(3);
            }
            if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
                m_gameState->handleInput(4);
            }
        }

        // Update game state
        if (m_gameState) {
            m_gameState->update(deltaTime);
        }
        
        // Render
        if (m_gameState && m_renderer) {
            m_gameState->render(m_renderer.get());
        }
        
        // Render with Vulkan
        if (m_renderer) {
            m_renderer->render();
        }
        
        // Check for window close event
        if (m_renderer && !m_renderer->isRunning()) {
            m_running = false;
        }

        // Check for game exit state
        if (m_gameState && m_gameState->getCurrentState() == GameState::State::EXIT) {
            m_running = false;
        }
        
        // Cap frame rate
        auto frameTime = std::chrono::high_resolution_clock::now() - currentTime;
        if (frameTime < frameDelay) {
            std::this_thread::sleep_for(frameDelay - frameTime);
        }
    }
    
    std::cout << "Game loop ended." << std::endl;
}

void Game::update(float deltaTime) {
    // Update game systems
    if (m_gameState) {
        m_gameState->update(deltaTime);
    }
}

void Game::render() {
    // Render game systems
    if (m_gameState && m_renderer) {
        m_gameState->render(m_renderer.get());
    }
}

void Game::shutdown() {
    std::cout << "Shutting down game..." << std::endl;
    m_gameState.reset();
    m_renderer.reset();
    m_running = false;
    std::cout << "Game shut down successfully." << std::endl;
}
