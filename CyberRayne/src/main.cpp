#include "../include/Game.h"
#include <iostream>
#include <memory>
#include <cstring>

int main(int argc, char* argv[]) {
    std::cout << "Starting FF9-style JRPG..." << std::endl;

    // Parse command line arguments
    bool benchmarkMode = false;
    int maxFrames = 500;  // Default benchmark frames

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--benchmark") == 0) {
            benchmarkMode = true;
            std::cout << "Benchmark mode enabled" << std::endl;
        } else if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            maxFrames = std::atoi(argv[++i]);
            std::cout << "Max frames set to: " << maxFrames << std::endl;
        } else if (strncmp(argv[i], "--frames=", 9) == 0) {
            maxFrames = std::atoi(argv[i] + 9);
            std::cout << "Max frames set to: " << maxFrames << std::endl;
        }
    }

    try {
        // Create and initialize the game
        std::unique_ptr<Game> game = std::make_unique<Game>();
        
        // Configure benchmark mode if enabled
        if (benchmarkMode) {
            game->setBenchmarkMode(true, maxFrames);
        }
        
        if (!game->initialize()) {
            std::cerr << "Failed to initialize game!" << std::endl;
            return -1;
        }

        // Run the game loop
        game->run();

        // Shutdown the game
        game->shutdown();

        std::cout << "Game ended." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return -1;
    } catch (...) {
        std::cerr << "Unknown exception caught!" << std::endl;
        return -1;
    }

    // Only pause in interactive mode
    if (!benchmarkMode) {
        std::cout << "Press Enter to exit..." << std::endl;
        std::cin.get();
    }

    return 0;
}
