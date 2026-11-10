#include <string>
#include "../../include/InputPrompts.h"
#include "../../include/Game.h"
#include "../../include/GameState.h"
#include "../../include/VulkanRenderer.h"
#include "../../include/InputAction.h"
#include "../../include/InputConfig.h"
#include "../../include/InputState.h"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <array>
#include <iostream>
#ifdef _WIN32
 #include <Windows.h>
#else
 #include <GLFW/glfw3.h>
#endif
#include <thread>
#include <chrono>

namespace {

// Adds whatever is held on any connected controller to the table.
//
// GLFW describes every pad it recognises in Xbox terms, so one mapping covers
// PlayStation (Cross = A, Circle = B, Triangle = Y, Options = Start), Xbox,
// and anything Steam Input presents as an Xbox pad.
// Returns true if a controller was being used this frame, and says in
// padName which one.
bool pollGamepads([[maybe_unused]] InputState::Held& held, [[maybe_unused]] std::string& padName) {
    bool used = false;
#ifndef _WIN32
    // How far the stick must be pushed to count as a direction.
    constexpr float STICK_THRESHOLD = 0.5f;
    bool touched = false;
    auto hold = [&](InputAction action) {
        held[InputState::indexOf(action)] = true;
        touched = true;
    };

    for (int jid = GLFW_JOYSTICK_1; jid <= GLFW_JOYSTICK_LAST; ++jid) {
        GLFWgamepadstate pad;
        if (!glfwJoystickIsGamepad(jid) || !glfwGetGamepadState(jid, &pad)) {
            continue;
        }
        const float x = pad.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
        const float y = pad.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
        // The stick picks one direction: whichever way it leans most. That
        // stops a slightly diagonal push from flickering between two.
        if (std::fabs(x) >= STICK_THRESHOLD || std::fabs(y) >= STICK_THRESHOLD) {
            if (std::fabs(x) > std::fabs(y)) {
                hold(x < 0.0f ? InputAction::MOVE_LEFT : InputAction::MOVE_RIGHT);
            } else {
                hold(y < 0.0f ? InputAction::MOVE_UP : InputAction::MOVE_DOWN);
            }
        }
        if (pad.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP]) hold(InputAction::MOVE_UP);
        if (pad.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN]) hold(InputAction::MOVE_DOWN);
        if (pad.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT]) hold(InputAction::MOVE_LEFT);
        if (pad.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT]) hold(InputAction::MOVE_RIGHT);
        if (pad.buttons[GLFW_GAMEPAD_BUTTON_A]) hold(InputAction::CONFIRM);
        if (pad.buttons[GLFW_GAMEPAD_BUTTON_B]) hold(InputAction::CANCEL);
        if (pad.buttons[GLFW_GAMEPAD_BUTTON_Y] || pad.buttons[GLFW_GAMEPAD_BUTTON_START]) {
            hold(InputAction::OPEN_MENU);
        }
        if (touched && !used) {
            used = true;
            const char* name = glfwGetGamepadName(jid);
            padName = name ? name : "";
        }
        touched = false;
    }
#endif
    return used;
}

// GLFW ships button layouts for common pads. An unusual one can be taught to
// it by dropping a gamecontrollerdb.txt (the SDL community file) next to the
// key bindings; this loads it if present.
void loadExtraGamepadMappings() {
#ifndef _WIN32
    const std::filesystem::path path =
        std::filesystem::path(InputConfig::defaultConfigPath()).parent_path() / "gamecontrollerdb.txt";
    std::ifstream file(path);
    if (!file) {
        return;
    }
    std::stringstream contents;
    contents << file.rdbuf();
    if (glfwUpdateGamepadMappings(contents.str().c_str())) {
        std::cout << "Loaded controller mappings from " << path.string() << std::endl;
    } else {
        std::cerr << "Could not read controller mappings in " << path.string() << std::endl;
    }
#endif
}

// Says which controllers are plugged in, once, so a pad that does nothing can
// be told apart from one the game never saw.
void logGamepads() {
#ifndef _WIN32
    for (int jid = GLFW_JOYSTICK_1; jid <= GLFW_JOYSTICK_LAST; ++jid) {
        if (!glfwJoystickPresent(jid)) continue;
        if (glfwJoystickIsGamepad(jid)) {
            std::cout << "Controller " << jid << ": " << glfwGetGamepadName(jid) << std::endl;
        } else {
            std::cout << "Controller " << jid << " (" << glfwGetJoystickName(jid)
                      << ") has no known button layout and is ignored." << std::endl;
        }
    }
#endif
}

}  // namespace

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

    loadExtraGamepadMappings();
    logGamepads();
#ifndef _WIN32
    // With a controller plugged in at start, show its buttons from the first
    // screen rather than waiting for it to be touched.
    for (int jid = GLFW_JOYSTICK_1; jid <= GLFW_JOYSTICK_LAST; ++jid) {
        if (glfwJoystickIsGamepad(jid)) {
            const char* name = glfwGetGamepadName(jid);
            InputPrompts::setDevice(InputPrompts::deviceForGamepadName(name ? name : ""));
            break;
        }
    }
    // Also announce pads plugged in or pulled out while playing.
    glfwSetJoystickCallback([](int jid, int event) {
        if (event == GLFW_CONNECTED) {
            std::cout << "Controller connected: "
                      << (glfwJoystickIsGamepad(jid) ? glfwGetGamepadName(jid) : glfwGetJoystickName(jid))
                      << std::endl;
        } else if (event == GLFW_DISCONNECTED) {
            std::cout << "Controller " << jid << " disconnected." << std::endl;
        }
    });
#endif
    
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

            // Which actions are held right now, from the keyboard bindings
            // and from any controller.
            InputState::Held held{};
            if (const InputConfig* config = m_gameState->getInputConfig()) {
                for (const auto& binding : config->getBindings()) {
                    const std::size_t action = InputState::indexOf(binding.second);
                    if (binding.first >= kFirstKey && binding.first <= kLastKey && action < held.size() &&
                        current[static_cast<std::size_t>(binding.first)]) {
                        held[action] = true;
                    }
                }
            }
            // On-screen hints name the controls of whatever was touched
            // last, so they swap by themselves between keyboard and pad.
            InputPrompts::setBindings(m_gameState->getInputConfig());
            bool keyboardUsed = false;
            for (int key = kFirstKey; key <= kLastKey && !keyboardUsed; ++key) {
                keyboardUsed = justPressed(key);
            }
            std::string padName;
            if (pollGamepads(held, padName)) {
                InputPrompts::setDevice(InputPrompts::deviceForGamepadName(padName));
            } else if (keyboardUsed) {
                InputPrompts::setDevice(InputPrompts::Device::KEYBOARD);
            }

            static InputState inputState;
            const std::vector<InputAction> fired = inputState.update(deltaTime, held);

            if (m_gameState->wantsRawKey()) {
                // Rebinding: the next key pressed is the binding itself, not
                // whatever action it currently means.
                m_gameState->setHeldDirection(0, 0);
                bool handled = false;
                for (int key = kFirstKey; key <= kLastKey && !handled; ++key) {
                    if (justPressed(key)) {
                        m_gameState->handleRawKey(key);
                        handled = true;
                    }
                }
                // A controller has no Esc: its Cancel button backs out instead.
                if (!handled && !current[static_cast<std::size_t>(InputConfig::escapeKey())]) {
                    for (InputAction action : fired) {
                        if (action == InputAction::CANCEL) {
                            m_gameState->handleRawKey(InputConfig::escapeKey());
                            break;
                        }
                    }
                }
            } else {
                for (InputAction action : fired) {
                    m_gameState->handleInput(action);
                }
                int dx = 0;
                int dy = 0;
                inputState.heldDirection(dx, dy);
                m_gameState->setHeldDirection(dx, dy);
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
    
    // The bindings belong to the game state, which may go away now.
    InputPrompts::setBindings(nullptr);

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
