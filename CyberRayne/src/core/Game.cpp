#include "../../include/Game.h"
#include "../../include/GameState.h"
#include "../../include/VulkanRenderer.h"
#include "../../include/InputAction.h"
#include <array>
#include <iostream>
#ifdef _WIN32
 #include <Windows.h>
#else
 #include <GLFW/glfw3.h>
#endif
#include <thread>
#include <chrono>

Game::Game() : m_gameState(nullptr), m_renderer(nullptr), m_running(false),
               m_benchmarkMode(false), m_maxBenchmarkFrames(500), m_benchmarkFrameCount(0) {}

void Game::setBenchmarkMode(bool enabled, int maxFrames) {
    m_benchmarkMode = enabled;
    m_maxBenchmarkFrames = maxFrames;
    std::cout << "Benchmark mode: " << (enabled ? "ON" : "OFF") 
              << " (max frames: " << maxFrames << ")" << std::endl;
}

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

        // Handle input
        if (m_gameState) {
            static std::array<bool, 8> previousKeyStates{};
            const std::array<InputAction, 8> actions = {
                InputAction::MOVE_UP,
                InputAction::MOVE_DOWN,
                InputAction::CONFIRM,
                InputAction::MOVE_LEFT,
                InputAction::MOVE_RIGHT,
                InputAction::CANCEL,
                InputAction::OPEN_MENU,
                InputAction::PAUSE
            };
#ifdef _WIN32
            const std::array<int, 8> keys = {VK_UP, VK_DOWN, VK_RETURN, VK_LEFT, VK_RIGHT, VK_ESCAPE, 'M', 'P'};
            auto isPressed = [](int key) { return (GetAsyncKeyState(key) & 0x8000) != 0; };
#else
            GLFWwindow* window = m_renderer ? m_renderer->getWindow() : nullptr;
            const std::array<int, 8> keys = {
                GLFW_KEY_UP,
                GLFW_KEY_DOWN,
                GLFW_KEY_ENTER,
                GLFW_KEY_LEFT,
                GLFW_KEY_RIGHT,
                GLFW_KEY_ESCAPE,
                GLFW_KEY_M,
                GLFW_KEY_P
            };
            auto isPressed = [window](int key) { return window && glfwGetKey(window, key) == GLFW_PRESS; };
#endif
            for (size_t i = 0; i < actions.size(); ++i) {
                const bool pressed = isPressed(keys[i]);
                if (pressed && !previousKeyStates[i]) {
                    m_gameState->handleInput(actions[i]);
                }
                previousKeyStates[i] = pressed;
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
        
        // Benchmark mode: exit after max frames
        if (m_benchmarkMode) {
            m_benchmarkFrameCount++;
            if (m_benchmarkFrameCount >= m_maxBenchmarkFrames) {
                std::cout << "Benchmark complete: " << m_benchmarkFrameCount << " frames rendered." << std::endl;
                m_running = false;
            }
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
