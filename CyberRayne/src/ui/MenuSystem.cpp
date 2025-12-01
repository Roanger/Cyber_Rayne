#include "../../include/MenuSystem.h"
#include "../../include/VulkanRenderer.h"
#include <iostream>
#include <filesystem>

MenuSystem::MenuSystem() : m_selectedOption(MenuItem::START_GAME), m_optionSelected(false), m_highlightedIndex(0),
    m_backgroundTextureIndex(-1), m_nameBannerTextureIndex(-1), m_startButtonTextureIndex(-1), m_loadButtonTextureIndex(-1), m_settingsButtonTextureIndex(-1), 
    m_quitButtonTextureIndex(-1), m_cursorTextureIndex(-1), m_highlightTextureIndex(-1) {
    m_menuOptions = {
        "Start Game",
        "Load Game",
        "Settings",
        "Quit"
    };
}

MenuSystem::~MenuSystem() {}

bool MenuSystem::initialize() {
    std::cout << "Menu system initialized." << std::endl;
    // Textures will be loaded by the GameState which has access to the renderer
    // This method is kept for future initialization if needed
    return true;
}

void MenuSystem::loadTextures(VulkanRenderer* renderer) {
    std::cout << "Loading menu textures..." << std::endl;

    const std::string base = renderer->getAssetsBasePath();
    auto exists = [](const std::string& p){ return std::filesystem::exists(p); };

    // Background
    const std::string bgPath = base + "/ui/menu_background.png";
    if (!exists(bgPath)) {
        std::cerr << "Background texture missing at: " << bgPath << std::endl;
    }
    m_backgroundTextureIndex = renderer->loadTexture(bgPath);
    std::cout << "Background texture index: " << m_backgroundTextureIndex << std::endl;

    // Name Banner (CYBER RAYNE logo)
    const std::string bannerPath = base + "/ui/namebanner.png";
    if (exists(bannerPath)) {
        m_nameBannerTextureIndex = renderer->loadTexture(bannerPath);
        std::cout << "Name banner texture index: " << m_nameBannerTextureIndex << std::endl;
    } else {
        std::cerr << "Name banner texture missing at: " << bannerPath << std::endl;
        m_nameBannerTextureIndex = -1;
    }

    // Start button: prefer simple start.png, else nested (from 3D model export)
    const std::string startSimple = base + "/ui/start.png";
    const std::string startNested = base + "/ui/start/Start_Button_Pixel_Ar_0810100014_texture_obj/Start_Button_Pixel_Ar_0810100014_texture.png";
    const std::string startPath = exists(startSimple) ? startSimple : startNested;
    std::cout << "Looking for start button texture..." << std::endl;
    std::cout << "  Simple path: " << startSimple << " (exists: " << (exists(startSimple) ? "yes" : "no") << ")" << std::endl;
    std::cout << "  Nested path: " << startNested << " (exists: " << (exists(startNested) ? "yes" : "no") << ")" << std::endl;
    std::cout << "  Using path: " << startPath << std::endl;
    if (!exists(startPath)) {
        std::cerr << "Start button texture missing at: " << startPath << std::endl;
    }
    m_startButtonTextureIndex = renderer->loadTexture(startPath);
    std::cout << "Start button path: " << startPath << ", index: " << m_startButtonTextureIndex << std::endl;

    // Load Game button
    const std::string loadPath = base + "/ui/loadgame.png";
    if (!exists(loadPath)) {
        std::cerr << "Load Game texture missing at: " << loadPath << std::endl;
    }
    m_loadButtonTextureIndex = renderer->loadTexture(loadPath);
    std::cout << "Load button texture index: " << m_loadButtonTextureIndex << std::endl;

    // Settings button
    const std::string settingsPath = base + "/ui/settings.png";
    if (!exists(settingsPath)) {
        std::cerr << "Settings texture missing at: " << settingsPath << std::endl;
    }
    m_settingsButtonTextureIndex = renderer->loadTexture(settingsPath);
    std::cout << "Settings button texture index: " << m_settingsButtonTextureIndex << std::endl;

    // Quit button
    const std::string quitPath = base + "/ui/quit.png";
    if (!exists(quitPath)) {
        std::cerr << "Quit texture missing at: " << quitPath << std::endl;
    }
    m_quitButtonTextureIndex = renderer->loadTexture(quitPath);
    std::cout << "Quit button texture index: " << m_quitButtonTextureIndex << std::endl;

    // Cursor
    const std::string cursorPath = base + "/ui/cursor.png";
    if (!exists(cursorPath)) {
        std::cerr << "Cursor texture missing at: " << cursorPath << std::endl;
    }
    m_cursorTextureIndex = renderer->loadTexture(cursorPath);
    std::cout << "Cursor texture index: " << m_cursorTextureIndex << std::endl;
    
    // For now, we'll use a simple colored rectangle for highlighting
    // In a real implementation, you might want to create a highlight texture
    m_highlightTextureIndex = -1; // No highlight texture for now
    
    std::cout << "Loaded menu textures:" << std::endl;
    std::cout << "  Start button: " << m_startButtonTextureIndex << std::endl;
    std::cout << "  Load button: " << m_loadButtonTextureIndex << std::endl;
    std::cout << "  Settings button: " << m_settingsButtonTextureIndex << std::endl;
    std::cout << "  Quit button: " << m_quitButtonTextureIndex << std::endl;
    std::cout << "  Highlight: " << m_highlightTextureIndex << std::endl;
}

void MenuSystem::update(float deltaTime) {
    // No-op for now; input handled elsewhere
}

void MenuSystem::render(VulkanRenderer* renderer) {
    renderMenuBackground(renderer);
    renderMenuOptions(renderer);
}

void MenuSystem::renderMenuBackground(VulkanRenderer* renderer) {
    if (m_backgroundTextureIndex >= 0) {
        int screenW = static_cast<int>(renderer->getSwapchainExtent().width);
        int screenH = static_cast<int>(renderer->getSwapchainExtent().height);
        // Render background covering the entire screen
        renderer->renderSpritePixelsWithTexture(0, 0, screenW, screenH, m_backgroundTextureIndex);
    }
}

void MenuSystem::renderMenuOptions(VulkanRenderer* renderer) {
    std::cout << "Rendering menu options..." << std::endl;

    const int screenW = static_cast<int>(renderer->getSwapchainExtent().width);
    const int screenH = static_cast<int>(renderer->getSwapchainExtent().height);

    std::cout << "Menu rendering - Screen size: " << screenW << "x" << screenH << std::endl;

    // ========== NAME BANNER (top-left) ==========
    if (m_nameBannerTextureIndex >= 0) {
        const int bannerW = 450;  // Bigger banner
        const int bannerH = 230;  // Bigger banner
        const int bannerX = 50;   // Left margin
        const int bannerY = 50;   // Top margin
        renderer->renderSpritePixelsWithTexture(bannerX, bannerY, bannerW, bannerH, m_nameBannerTextureIndex);
    }

    // ========== MAIN BUTTONS (bottom center: START, LOAD, QUIT in a row) ==========
    const int buttonW = 200;
    const int buttonH = 80;
    const int gapX = 40;
    
    // Position buttons in bottom third of screen
    const int rowY = screenH - 250;  // Distance from bottom
    
    // Center the three buttons horizontally
    const int totalW = buttonW * 3 + gapX * 2;
    const int leftX = (screenW - totalW) / 2;

    // ========== SETTINGS BUTTON (below the main buttons, centered) ==========
    const int settingsW = 200;
    const int settingsH = 70;
    const int settingsX = (screenW - settingsW) / 2;
    const int settingsY = rowY + buttonH + 30;

    std::cout << "Menu button positions:" << std::endl;
    std::cout << "  Start: (" << leftX << ", " << rowY << ") size (" << buttonW << ", " << buttonH << ")" << std::endl;
    std::cout << "  Load: (" << (leftX + buttonW + gapX) << ", " << rowY << ") size (" << buttonW << ", " << buttonH << ")" << std::endl;
    std::cout << "  Quit: (" << (leftX + 2*(buttonW + gapX)) << ", " << rowY << ") size (" << buttonW << ", " << buttonH << ")" << std::endl;
    std::cout << "  Settings: (" << settingsX << ", " << settingsY << ") size (" << settingsW << ", " << settingsH << ")" << std::endl;

    // Helper to draw a highlight backdrop using NDC conversion
    auto drawHighlight = [&](int x, int y, int w, int h) {
        float ndcW = 2.0f * static_cast<float>(w) / static_cast<float>(screenW);
        float ndcH = 2.0f * static_cast<float>(h) / static_cast<float>(screenH);
        float cx = static_cast<float>(x) + w * 0.5f;
        float cy = static_cast<float>(y) + h * 0.5f;
        float ndcX = -1.0f + 2.0f * (cx / static_cast<float>(screenW));
        float ndcY = -1.0f + 2.0f * (cy / static_cast<float>(screenH));
        renderer->renderSprite(ndcX, ndcY, ndcW * 1.08f, ndcH * 1.15f); // slightly larger backdrop
    };

    // Helper to draw a placeholder panel (outer dark, inner light) using NDC conversion
    auto drawPlaceholder = [&](int x, int y, int w, int h) {
        float ndcW = 2.0f * static_cast<float>(w) / static_cast<float>(screenW);
        float ndcH = 2.0f * static_cast<float>(h) / static_cast<float>(screenH);
        float cx = static_cast<float>(x) + w * 0.5f;
        float cy = static_cast<float>(y) + h * 0.5f;
        float ndcX = -1.0f + 2.0f * (cx / static_cast<float>(screenW));
        float ndcY = -1.0f + 2.0f * (cy / static_cast<float>(screenH));
        // Outer (darker)
        renderer->renderSprite(ndcX, ndcY, ndcW, ndcH);
        // Inner (slightly smaller) to simulate border
        renderer->renderSprite(ndcX, ndcY, ndcW * 0.9f, ndcH * 0.85f);
    };

    // Define positions for each item by index
    struct Rect { int x,y,w,h,tex; } rects[4] = {
        { leftX + 0*(buttonW+gapX), rowY, buttonW, buttonH, m_startButtonTextureIndex },
        { leftX + 1*(buttonW+gapX), rowY, buttonW, buttonH, m_loadButtonTextureIndex },
        { leftX + 2*(buttonW+gapX), rowY, buttonW, buttonH, m_quitButtonTextureIndex },
        { settingsX, settingsY, settingsW, settingsH, m_settingsButtonTextureIndex }
    };


    // Helper to draw the cursor
    auto drawCursor = [&](int targetX, int targetY, int targetH) {
        if (m_cursorTextureIndex >= 0) {
            int cursorSize = 64; // Adjust size as needed
            // Position cursor to the left of the button, centered vertically relative to button height
            int cursorX = targetX - cursorSize - 10; 
            int cursorY = targetY + (targetH - cursorSize) / 2;
            renderer->renderSpritePixelsWithTexture(cursorX, cursorY, cursorSize, cursorSize, m_cursorTextureIndex);
        } else {
            // Fallback to old highlight if cursor texture missing
            drawHighlight(targetX, targetY, rects[m_highlightedIndex].w, rects[m_highlightedIndex].h);
        }
    };

    for (int i = 0; i < 4; ++i) {
        if (i == m_highlightedIndex) {
            // Draw cursor for the highlighted item
            drawCursor(rects[i].x, rects[i].y, rects[i].h);
        }
        if (rects[i].tex >= 0) {
            renderer->renderSpritePixelsWithTexture(rects[i].x, rects[i].y, rects[i].w, rects[i].h, rects[i].tex);
        } else if (m_settingsButtonTextureIndex >= 0) {
            // Use settings icon as a visual placeholder to avoid defaulting to texture 0
            renderer->renderSpritePixelsWithTexture(rects[i].x, rects[i].y, rects[i].w, rects[i].h, m_settingsButtonTextureIndex);
        } else {
            // Last resort: just draw the highlight frame (no extra panel)
            drawHighlight(rects[i].x, rects[i].y, rects[i].w, rects[i].h);
        }
    }
}

void MenuSystem::handleInput(int key) {
    switch (key) {
        case 0: // Up arrow
            if (m_highlightedIndex == 3) m_highlightedIndex = 1; // from settings go to Load (middle)
            break;
        case 1: // Down arrow
            if (m_highlightedIndex != 3) m_highlightedIndex = 3; // go to settings
            break;
        case 2: // Enter
            switch (m_highlightedIndex) {
                case 0:
                    m_selectedOption = MenuItem::START_GAME;
                    m_optionSelected = true;
                    break;
                case 1:
                    m_selectedOption = MenuItem::LOAD_GAME;
                    m_optionSelected = true;
                    break;
                case 2:
                    m_selectedOption = MenuItem::QUIT;       // top-right in new layout
                    m_optionSelected = true;
                    break;
                case 3:
                    m_selectedOption = MenuItem::SETTINGS;   // bottom centered
                    m_optionSelected = true;
                    break;
            }
            break;
        case 3: // Left arrow
            if (m_highlightedIndex == 1) m_highlightedIndex = 0;
            else if (m_highlightedIndex == 2) m_highlightedIndex = 1;
            break;
        case 4: // Right arrow
            if (m_highlightedIndex == 0) m_highlightedIndex = 1;
            else if (m_highlightedIndex == 1) m_highlightedIndex = 2;
            break;
    }
}
