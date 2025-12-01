#include "../include/Game.h"
#include <iostream>
#include <memory>

int main() {
    std::cout << "Starting FF9-style JRPG..." << std::endl;

    try {
        // Create and initialize the game
        std::unique_ptr<Game> game = std::make_unique<Game>();
        
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

    // Pause to keep the window open
    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();

    return 0;
}
