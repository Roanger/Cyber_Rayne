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
    
    // Benchmark mode for CI/CD performance testing
    void setBenchmarkMode(bool enabled, int maxFrames = 500);

private:
    void update(float deltaTime);
    void render();

    std::unique_ptr<GameState> m_gameState;
    std::unique_ptr<VulkanRenderer> m_renderer;
    bool m_running;
    
    // Benchmark settings
    bool m_benchmarkMode = false;
    int m_maxBenchmarkFrames = 500;
    int m_benchmarkFrameCount = 0;
};
