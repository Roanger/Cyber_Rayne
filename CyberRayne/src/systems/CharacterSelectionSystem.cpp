#include "../../include/InputPrompts.h"
#include "../../include/CharacterSelectionSystem.h"
#include "../../include/Player.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <filesystem>

#ifndef NO_VULKAN
#include "../../include/VulkanRenderer.h"
#include "../../include/ui/BattleScene.h"
#include "../../include/ui/TextRenderer.h"
#include "../../include/ui/TextWrap.h"
#include "../../include/ui/Theme.h"
#include <cmath>
#endif

CharacterSelectionSystem::CharacterSelectionSystem()
    : m_selectedIndex(0), m_characterSelected(false) {
    initializeCharacterOptions();
}

CharacterSelectionSystem::~CharacterSelectionSystem() {}

bool CharacterSelectionSystem::initialize() {
    std::cout << "Initializing character selection system..." << std::endl;
    return true;
}

#ifndef NO_VULKAN
void CharacterSelectionSystem::loadTextures(VulkanRenderer* renderer) {
    if (!renderer) return;

    if (!m_textRenderer) {
        m_textRenderer = std::make_unique<TextRenderer>();
    }
    m_textRenderer->initialize(renderer);

    const std::string base = renderer->getAssetsBasePath();
    // Derive each portrait from its own class rather than a positional list,
    // so reordering the options cannot silently mismatch the art.
    for (auto& option : m_characterOptions) {
        if (option.textureIndex >= 0) continue;  // already loaded
        std::string className = Player::classToString(option.charClass);
        std::transform(className.begin(), className.end(), className.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        const std::string texturePath = base + "/characters/" + className + ".png";
        if (std::filesystem::exists(texturePath)) {
            option.textureIndex = renderer->loadTexture(texturePath);
        } else {
            std::cerr << "Character texture not found: " << texturePath << std::endl;
        }
    }
}
#endif

void CharacterSelectionSystem::update(float deltaTime) {
    m_time += deltaTime;
}

void CharacterSelectionSystem::handleInput(InputAction action) {
    if (m_characterSelected) return;  // Already selected

    const int count = static_cast<int>(m_characterOptions.size());
    if (count == 0) return;

    if (action == InputAction::MOVE_LEFT) {
        m_selectedIndex = (m_selectedIndex - 1 + count) % count;
    } else if (action == InputAction::MOVE_RIGHT) {
        m_selectedIndex = (m_selectedIndex + 1) % count;
    } else if (action == InputAction::CONFIRM) {
        confirmSelection();
    }
}

void CharacterSelectionSystem::confirmSelection() {
    if (!m_characterSelected && m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_characterOptions.size())) {
        m_characterSelected = true;
        std::cout << "Character confirmed: " << m_characterOptions[m_selectedIndex].name << std::endl;
    }
}

#ifndef NO_VULKAN
void CharacterSelectionSystem::render(VulkanRenderer* renderer) {
    if (!renderer || !m_textRenderer) return;
    TextRenderer* text = m_textRenderer.get();
    const int screenW = static_cast<int>(renderer->getSwapchainExtent().width);
    const int screenH = static_cast<int>(renderer->getSwapchainExtent().height);

    BattleScene::drawBackdrop(renderer, screenW, screenH, "Neon Alley");
    Theme::dimScreen(renderer, 0.45f);

    const std::string heading = "CHOOSE RAYNE'S PATH";
    const int headingScale = screenW >= 1000 ? 4 : 3;

    // Three cards side by side. Portraits are drawn at a whole-number
    // multiple of their 96x128 size, as large as the window allows.
    const int count = static_cast<int>(m_characterOptions.size());
    const int gap = 24;
    const int margin = 32;
    const int detailH = 190;
    const int headingH = headingScale * Theme::GLYPH + 36;
    const int availableW = (screenW - 2 * margin - (count - 1) * gap) / std::max(1, count);
    const int availableH = screenH - 40 - headingH - detailH - 60;
    const int scale = std::clamp(std::min((availableW - 32) / 96, (availableH - 80) / 128), 1, 4);
    const int cardW = std::min(availableW, 96 * scale + 96);
    const int cardH = 128 * scale + 80;
    const int totalW = count * cardW + (count - 1) * gap;
    // Centre the whole arrangement in a tall window instead of leaving the
    // bottom half empty.
    const int top = std::max(40, (screenH - 60 - (headingH + cardH + detailH)) / 2);
    const int cardsTop = top + headingH;
    const int startX = (screenW - totalW) / 2;

    text->renderText(renderer, heading, (screenW - text->measureText(heading, headingScale)) / 2, top,
                     headingScale, Theme::TEXT);

    for (int i = 0; i < count; ++i) {
        const CharacterOption& option = m_characterOptions[static_cast<std::size_t>(i)];
        const bool selected = i == m_selectedIndex;
        const int cardX = startX + i * (cardW + gap);
        // The chosen card rises a little and gets a gold frame.
        const int cardY = cardsTop + (selected ? 0 : 12);

        Theme::drawPanel(renderer, cardX, cardY, cardW, cardH);
        if (selected) {
            // Gold frame, drawn as four strips: a filled gold block under
            // the see-through panel would tint the whole card.
            const int t = 4;
            Theme::fillRect(renderer, cardX - t, cardY - t, cardW + 2 * t, t, Theme::GOLD);
            Theme::fillRect(renderer, cardX - t, cardY + cardH, cardW + 2 * t, t, Theme::GOLD);
            Theme::fillRect(renderer, cardX - t, cardY, t, cardH, Theme::GOLD);
            Theme::fillRect(renderer, cardX + cardW, cardY, t, cardH, Theme::GOLD);
        }

        const float shade = selected ? 1.0f : 0.55f;
        if (option.textureIndex >= 0) {
            renderer->renderSpritePixelsWithTexture(cardX + (cardW - 96 * scale) / 2, cardY + 16,
                                                    96 * scale, 128 * scale, option.textureIndex,
                                                    shade, shade, shade, 1.0f);
        }
        const std::string name = Player::classToString(option.charClass);
        text->renderText(renderer, name, cardX + (cardW - text->measureText(name, 3)) / 2,
                         cardY + cardH - 44, 3, selected ? Theme::GOLD : Theme::MUTED);

        if (selected) {
            // Marker above the card, bobbing.
            const int bob = static_cast<int>(std::sin(m_time * 6.0f) * 4.0f);
            const int tipX = cardX + cardW / 2;
            const int tipY = cardY - 10 + bob;
            for (int row = 0; row < 5; ++row) {
                const int half = (5 - row) * 4;
                Theme::fillRect(renderer, tipX - half, tipY - (5 - row) * 4, half * 2, 4, Theme::GOLD);
            }
        }
    }

    // What the highlighted class is like.
    if (m_selectedIndex >= 0 && m_selectedIndex < count) {
        const CharacterOption& option = m_characterOptions[static_cast<std::size_t>(m_selectedIndex)];
        const int panelW = std::min(screenW - 2 * margin, 1100);
        const int panelX = (screenW - panelW) / 2;
        const int panelY = cardsTop + cardH + 28;
        Theme::drawPanel(renderer, panelX, panelY, panelW, detailH - 40);

        const std::size_t maxChars = static_cast<std::size_t>(std::max(10, (panelW - 48) / (Theme::GLYPH * 2)));
        int y = panelY + 20;
        std::size_t lineStart = 0;
        const std::string& description = option.description;
        while (lineStart <= description.size()) {
            std::size_t lineEnd = description.find('\n', lineStart);
            if (lineEnd == std::string::npos) lineEnd = description.size();
            const std::string paragraph = description.substr(lineStart, lineEnd - lineStart);
            lineStart = lineEnd + 1;
            if (paragraph.empty()) {
                y += 10;
                continue;
            }
            uint32_t color = Theme::TEXT;
            if (paragraph.rfind("Strengths", 0) == 0) color = Theme::FOAM;
            if (paragraph.rfind("Weaknesses", 0) == 0) color = Theme::ROSE;
            for (const std::string& line : wrapText(paragraph, maxChars)) {
                text->renderText(renderer, line, panelX + 24, y, 2, color);
                y += 24;
            }
        }
    }

    const std::string hint = InputPrompts::leftRight() + ": choose   " + InputPrompts::confirm() + ": begin";
    text->renderText(renderer, hint, (screenW - text->measureText(hint, 2)) / 2, screenH - 36, 2, Theme::MUTED);
}
#else
void CharacterSelectionSystem::render() {
    // In a real implementation, we would render the character selection UI
    // For now, we'll just print a message
    std::cout << "Rendering character selection UI" << std::endl;
    
    if (!m_characterSelected) {
        std::cout << "Choose your character:" << std::endl;
        for (size_t i = 0; i < m_characterOptions.size(); ++i) {
            const auto& option = m_characterOptions[i];
            std::cout << (static_cast<int>(i) == m_selectedIndex ? "> " : "  ") 
                      << i << ". " << option.name << std::endl;
        }
        std::cout << "Use arrow keys to select, Enter to confirm" << std::endl;
    } else {
        std::cout << "Character selected: " << m_characterOptions[m_selectedIndex].name << std::endl;
    }
}
#endif

const std::vector<CharacterSelectionSystem::CharacterOption>& CharacterSelectionSystem::getCharacterOptions() const {
    return m_characterOptions;
}

void CharacterSelectionSystem::selectCharacter(int index) {
    if (index >= 0 && index < static_cast<int>(m_characterOptions.size())) {
        m_selectedIndex = index;
        m_characterSelected = true;
        
        const CharacterOption& selected = m_characterOptions[index];
        std::cout << "Selected character: " << selected.name << std::endl;
        std::cout << "Starter quest: " << selected.starterQuest << std::endl;
    }
}

std::unique_ptr<Player> CharacterSelectionSystem::createSelectedCharacter() const {
    if (!m_characterSelected) {
        return nullptr;
    }
    
    const CharacterOption& selected = m_characterOptions[m_selectedIndex];
    return std::make_unique<Player>(selected.charClass, selected.name);
}

const std::string& CharacterSelectionSystem::getStarterQuestForSelectedCharacter() const {
    static std::string empty = "";
    if (!m_characterSelected) {
        return empty;
    }
    
    return m_characterOptions[m_selectedIndex].starterQuest;
}

void CharacterSelectionSystem::initializeCharacterOptions() {
    m_characterOptions.clear();
    
    // Option order must stay in sync with the texture path list in
    // loadTextures() — that list is indexed positionally, not by charClass.

    CharacterOption netrunner;
    netrunner.charClass = Player::CharacterClass::NETRUNNER;
    netrunner.name = "Cipher";
    netrunner.description = "A netrunner who turns hostile code into a weapon.\n\nStrengths: High tech power, breaches and disables machines\nWeaknesses: Low health, little use in a straight fight";
    netrunner.starterQuest = "Something is scraping the district's public net and rerouting it through a dead corporate relay. Trace the intrusion back through Neo-Kyoto Central and find out who is listening.";
    netrunner.textureIndex = -1;
    m_characterOptions.push_back(netrunner);

    CharacterOption solo;
    solo.charClass = Player::CharacterClass::SOLO;
    solo.name = "Rook";
    solo.description = "Street muscle wrapped in chrome and armor plate.\n\nStrengths: High health, heavy physical damage, holds a line\nWeaknesses: Low tech power, no answer to ranged threats";
    solo.starterQuest = "A crew of thugs has been shaking down vendors along the Central strip and the local fixers are done paying. Push them off the block and find out who is bankrolling them.";
    solo.textureIndex = -1;
    m_characterOptions.push_back(solo);

    CharacterOption infiltrator;
    infiltrator.charClass = Player::CharacterClass::INFILTRATOR;
    infiltrator.name = "Wisp";
    infiltrator.description = "A ghost who moves through security like it isn't there.\n\nStrengths: High speed, opening strikes, poisons and debuffs\nWeaknesses: Moderate health, fades in a long fight";
    infiltrator.starterQuest = "A courier went dark in Neon Alley with a data chip still on them. Get in past the gangs, find the body before corporate security does, and get the chip out.";
    infiltrator.textureIndex = -1;
    m_characterOptions.push_back(infiltrator);


    std::cout << "Initialized " << m_characterOptions.size() << " character options" << std::endl;
}
