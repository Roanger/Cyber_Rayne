#include "../../include/Game.h"
#include "../../include/GameState.h"
#include "../../include/VulkanRenderer.h"
#include "../../include/InputAction.h"
#include "../../include/InputConfig.h"
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
            // Raw key codes are GLFW's on Linux and virtual-key codes on
            // Windows, matching what InputConfig stores.
#ifdef _WIN32
            constexpr int kFirstKey = 8;
            constexpr int kLastKey = 255;
            auto isPressed = [](int key) { return (GetAsyncKeyState(key) & 0x8000) != 0; };
#else
            constexpr int kFirstKey = GLFW_KEY_SPACE;
            constexpr int kLastKey = GLFW_KEY_LAST;
            GLFWwindow* window = m_renderer ? m_renderer->getWindow() : nullptr;
            auto isPressed = [window](int key) { return window && glfwGetKey(window, key) == GLFW_PRESS; };
#endif
            static std::array<bool, kLastKey + 1> previous{};
            std::array<bool, kLastKey + 1> current{};
            for (int key = kFirstKey; key <= kLastKey; ++key) {
                current[static_cast<std::size_t>(key)] = isPressed(key);
            }
            auto justPressed = [&](int key) {
                return key >= kFirstKey && key <= kLastKey && current[static_cast<std::size_t>(key)] &&
                       !previous[static_cast<std::size_t>(key)];
            };

            if (m_gameState->wantsRawKey()) {
                // Rebinding: the next key pressed is the binding itself, not
                // whatever action it currently means.
                for (int key = kFirstKey; key <= kLastKey; ++key) {
                    if (justPressed(key)) {
                        m_gameState->handleRawKey(key);
                        break;
                    }
                }
            } else if (const InputConfig* config = m_gameState->getInputConfig()) {
                // Copy first: handling an action can change the bindings.
                const auto bindings = config->getBindings();
                for (const auto& binding : bindings) {
                    if (justPressed(binding.first)) {
                        m_gameState->handleInput(binding.second);
                    }
                }
            }
            previous = current;
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
