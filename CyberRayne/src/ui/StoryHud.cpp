#include "../../include/ui/StoryHud.h"
#include "../../include/ui/TextRenderer.h"
#include "../../include/ui/TextWrap.h"
#include "../../include/ui/Theme.h"
#include "../../include/DialogueSystem.h"
#include "../../include/VulkanRenderer.h"

#include <algorithm>
#include <vector>

namespace {

using Theme::GLYPH;
constexpr uint32_t COLOR_TEXT = Theme::TEXT;
constexpr uint32_t COLOR_MUTED = Theme::MUTED;
constexpr uint32_t COLOR_IRIS = Theme::IRIS;
constexpr uint32_t COLOR_GOLD = Theme::GOLD;
constexpr uint32_t COLOR_FOAM = Theme::FOAM;
constexpr uint32_t COLOR_ROSE = Theme::ROSE;

constexpr int MARGIN = 24;

// A vertically centred stack of centred lines.
struct CardLine {
    std::string text;
    int scale;
    uint32_t color;
};

void drawCard(VulkanRenderer* renderer, TextRenderer* text, const std::vector<CardLine>& lines) {
    const int screenW = static_cast<int>(renderer->getSwapchainExtent().width);
    const int screenH = static_cast<int>(renderer->getSwapchainExtent().height);
    const int gap = 18;

    int total = 0;
    for (const CardLine& line : lines) total += GLYPH * line.scale + gap;
    int y = (screenH - total) / 2;
    for (const CardLine& line : lines) {
        if (!line.text.empty()) {
            const int w = static_cast<int>(line.text.size()) * GLYPH * line.scale;
            text->renderText(renderer, line.text, (screenW - w) / 2, y, line.scale, line.color);
        }
        y += GLYPH * line.scale + gap;
    }
}

}  // namespace

StoryHud::StoryHud() : m_text(std::make_unique<TextRenderer>()) {}
StoryHud::~StoryHud() = default;

bool StoryHud::initialize(VulkanRenderer* renderer) {
    return m_text->initialize(renderer);
}

void StoryHud::pushToast(const std::string& message) {
    if (m_toasts.empty()) {
        m_toastTimer = TOAST_SECONDS;
    }
    m_toasts.push_back(message);
}

void StoryHud::clearToasts() {
    m_toasts.clear();
    m_toastTimer = 0.0f;
}

void StoryHud::update(float deltaTime) {
    if (m_toasts.empty()) {
        return;
    }
    m_toastTimer -= deltaTime;
    if (m_toastTimer <= 0.0f) {
        m_toasts.pop_front();
        m_toastTimer = m_toasts.empty() ? 0.0f : TOAST_SECONDS;
    }
}

void StoryHud::renderExploration(VulkanRenderer* renderer, const std::string& mapName,
                                 const std::string& questName, const std::string& objective,
                                 const std::string& prompt) {
    if (!renderer) return;
    const int screenW = static_cast<int>(renderer->getSwapchainExtent().width);
    const int screenH = static_cast<int>(renderer->getSwapchainExtent().height);
    const int scale = 2;
    const int line = m_text->lineHeight(scale) + 6;

    // Tracker, top-left.
    {
        const std::string header = questName.empty() ? mapName : questName;
        const std::string detail = questName.empty() ? "No active quest" : "> " + objective;
        const int chars = static_cast<int>(std::max(header.size(), detail.size()));
        const int w = chars * GLYPH * scale + 32;
        const int h = line * 2 + 24;
        Theme::drawPanel(renderer, MARGIN, MARGIN, w, h);
        m_text->renderText(renderer, header, MARGIN + 16, MARGIN + 12, scale, COLOR_GOLD);
        m_text->renderText(renderer, detail, MARGIN + 16, MARGIN + 12 + line, scale,
                           questName.empty() ? COLOR_MUTED : COLOR_TEXT);
    }

    // Talk prompt, bottom-centre.
    if (!prompt.empty()) {
        const int w = static_cast<int>(prompt.size()) * GLYPH * scale + 32;
        const int h = line + 20;
        const int x = (screenW - w) / 2;
        const int y = screenH - h - MARGIN * 2;
        Theme::drawPanel(renderer, x, y, w, h);
        m_text->renderText(renderer, prompt, x + 16, y + 12, scale, COLOR_FOAM);
    }
}

void StoryHud::renderToast(VulkanRenderer* renderer) {
    if (!renderer || m_toasts.empty()) return;
    const int screenW = static_cast<int>(renderer->getSwapchainExtent().width);
    const int scale = 2;
    const std::string& message = m_toasts.front();
    const int w = static_cast<int>(message.size()) * GLYPH * scale + 40;
    const int h = m_text->lineHeight(scale) + 28;
    const int x = (screenW - w) / 2;
    const int y = MARGIN;
    Theme::drawPanel(renderer, x, y, w, h);
    m_text->renderText(renderer, message, x + 20, y + 14, scale, COLOR_ROSE);
}

void StoryHud::renderDialogue(VulkanRenderer* renderer, const DialogueSystem& dialogue) {
    if (!renderer) return;
    const DialogueNode* node = dialogue.getCurrentNode();
    if (!node) return;

    const int screenW = static_cast<int>(renderer->getSwapchainExtent().width);
    const int screenH = static_cast<int>(renderer->getSwapchainExtent().height);
    const int scale = 3;
    const int line = m_text->lineHeight(scale) + 10;
    const int pad = 28;

    const int w = std::min(screenW - MARGIN * 2, 1500);
    const int x = (screenW - w) / 2;
    const std::size_t maxChars = static_cast<std::size_t>((w - pad * 2) / (GLYPH * scale));

    const bool choosing = dialogue.getState() == DialogueSystem::State::SHOWING_CHOICES;

    // Body: either the spoken text or the list of replies.
    std::vector<std::string> body;
    if (choosing) {
        for (const auto& choice : node->getChoices()) {
            body.push_back(choice.text);
        }
    } else {
        body = wrapText(node->getText(), maxChars);
    }

    const int rows = static_cast<int>(body.size()) + 2;  // speaker + body + hint
    const int h = rows * line + pad * 2;
    const int y = screenH - h - MARGIN * 2;
    Theme::drawPanel(renderer, x, y, w, h);

    int cursorY = y + pad;
    m_text->renderText(renderer, choosing ? "Rayne" : node->getSpeaker(), x + pad, cursorY, scale,
                       choosing ? COLOR_FOAM : COLOR_GOLD);
    cursorY += line;

    for (std::size_t i = 0; i < body.size(); ++i) {
        if (choosing) {
            const bool selected = static_cast<int>(i) == dialogue.getSelectedChoiceIndex();
            m_text->renderText(renderer, (selected ? "> " : "  ") + body[i], x + pad, cursorY, scale,
                               selected ? COLOR_IRIS : COLOR_TEXT);
        } else {
            m_text->renderText(renderer, body[i], x + pad, cursorY, scale, COLOR_TEXT);
        }
        cursorY += line;
    }

    std::string hint;
    if (choosing) {
        hint = "Up/Down: choose   Enter: say it";
    } else if (node->hasChoices()) {
        hint = "Enter: reply";
    } else {
        hint = "Enter: continue";
    }
    const int hintScale = 2;
    const int hintW = static_cast<int>(hint.size()) * GLYPH * hintScale;
    m_text->renderText(renderer, hint, x + w - pad - hintW, y + h - pad - GLYPH * hintScale,
                       hintScale, COLOR_MUTED);
}

void StoryHud::renderEnding(VulkanRenderer* renderer) {
    if (!renderer) return;
    Theme::dimScreen(renderer, 0.92f);
    drawCard(renderer, m_text.get(), {
        {"CYBER RAYNE", 8, COLOR_IRIS},
        {"", 3, COLOR_TEXT},
        {"The truth about Tanaka Corp is out.", 3, COLOR_TEXT},
        {"Neo-Kyoto will never be the same.", 3, COLOR_TEXT},
        {"", 3, COLOR_TEXT},
        {"End of the vertical slice. Thanks for playing.", 3, COLOR_GOLD},
        {"", 3, COLOR_TEXT},
        {"Enter: keep exploring", 2, COLOR_MUTED},
    });
}

void StoryHud::renderBattleResult(VulkanRenderer* renderer, const BattleSummary& summary) {
    if (!renderer) return;
    Theme::dimScreen(renderer, 1.0f);

    std::vector<CardLine> lines = {
        {"VICTORY", 8, COLOR_GOLD},
        {"", 3, COLOR_TEXT},
        {"+" + std::to_string(summary.experience) + " XP", 4, COLOR_TEXT},
        {"+" + std::to_string(summary.gold) + " G", 4, COLOR_TEXT},
    };
    if (!summary.levelUps.empty() || !summary.revived.empty()) {
        lines.push_back({"", 3, COLOR_TEXT});
    }
    for (const auto& line : summary.levelUps) {
        lines.push_back({line, 3, COLOR_FOAM});
    }
    for (const auto& line : summary.revived) {
        lines.push_back({line, 3, COLOR_ROSE});
    }
    lines.push_back({"", 3, COLOR_TEXT});
    lines.push_back({"Enter: continue", 2, COLOR_MUTED});
    drawCard(renderer, m_text.get(), lines);
}

void StoryHud::renderGameOver(VulkanRenderer* renderer, bool canLoad) {
    if (!renderer) return;
    Theme::dimScreen(renderer, 1.0f);
    drawCard(renderer, m_text.get(), {
        {"GAME OVER", 8, Theme::LOVE},
        {"", 3, COLOR_TEXT},
        {"The party has fallen.", 3, COLOR_TEXT},
        {"", 3, COLOR_TEXT},
        {canLoad ? "Enter: load your last save" : "Enter: return to title", 2, COLOR_MUTED},
        {"Esc: quit", 2, COLOR_MUTED},
    });
}
