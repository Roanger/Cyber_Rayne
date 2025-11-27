#pragma once

#include <memory>

class GameState;
class VulkanRenderer;

class Game {
public:
    Game();
    ~Game();

    bool initialize();
    void run();
    void shutdown();

private:
    void update(float deltaTime);
    void render();

    std::unique_ptr<GameState> m_gameState;
    std::unique_ptr<VulkanRenderer> m_renderer;
    bool m_running;
};
