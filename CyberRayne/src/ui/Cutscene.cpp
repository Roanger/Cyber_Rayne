#include "../../include/ui/Cutscene.h"

#ifndef NO_VULKAN
#include "../../include/InputPrompts.h"
#include "../../include/VulkanRenderer.h"
#include "../../include/ui/SpriteLibrary.h"
#include "../../include/ui/TextRenderer.h"
#include "../../include/ui/TextWrap.h"
#include "../../include/ui/Theme.h"
#endif

#include <algorithm>
#include <utility>

std::vector<Cutscene::Panel> Cutscene::introPanels() {
    // The captions only set the scene. Kaga still gives the job himself when
    // the player talks to him, so nothing here says it was accepted.
    return {
        {"intro_1",
         "Neo-Kyoto. Rain on the old roofs, chrome towers over all of it. "
         "Up there the corporations own everything. Down here, people get by on favours."},
        {"intro_2",
         "Fixer Kaga trades in favours. Tonight he is the one who needs one: "
         "something of his has gone missing, and he wants it back quietly."},
        {"intro_3",
         "Whatever it is, the trail runs north into Neon Alley. "
         "Nobody walks in there by accident. Rayne has never minded that."},
    };
}

std::vector<Cutscene::Panel> Cutscene::endingPanels() {
    return {
        {"ending_good",
         "Before sunrise the Info Broker puts the evidence on every screen in Neo-Kyoto. "
         "Tanaka Corp still stands, but its secrets are no longer its own."},
    };
}

void Cutscene::start(std::vector<Panel> panels, std::string lastHint) {
    m_panels = std::move(panels);
    m_lastHint = std::move(lastHint);
    m_index = 0;
    m_panelTime = 0.0f;
    m_typedOut = false;
}

void Cutscene::update(float deltaTime) {
    if (!isFinished()) {
        m_panelTime += deltaTime;
    }
}

bool Cutscene::isCaptionComplete() const {
    if (isFinished()) {
        return true;
    }
    return m_typedOut ||
           m_panelTime * CHARS_PER_SECOND >= static_cast<float>(m_panels[m_index].caption.size());
}

std::string Cutscene::getVisibleCaption() const {
    if (isFinished()) {
        return std::string();
    }
    const std::string& caption = m_panels[m_index].caption;
    if (isCaptionComplete()) {
        return caption;
    }
    return caption.substr(0, static_cast<std::size_t>(m_panelTime * CHARS_PER_SECOND));
}

void Cutscene::handleInput(InputAction action) {
    if (isFinished()) {
        return;
    }
    switch (action) {
        case InputAction::CONFIRM:
            if (!isCaptionComplete()) {
                m_typedOut = true;
            } else {
                ++m_index;
                m_panelTime = 0.0f;
                m_typedOut = false;
            }
            break;
        case InputAction::CANCEL:
        case InputAction::OPEN_MENU:
        case InputAction::PAUSE:
            m_index = m_panels.size();
            break;
        default:
            break;
    }
}

#ifndef NO_VULKAN
void Cutscene::render(VulkanRenderer* renderer, TextRenderer* text, SpriteLibrary* sprites) const {
    if (!renderer || isFinished()) {
        return;
    }
    const int screenW = static_cast<int>(renderer->getSwapchainExtent().width);
    const int screenH = static_cast<int>(renderer->getSwapchainExtent().height);
    Theme::fillRect(renderer, 0, 0, screenW, screenH, Theme::BASE);

    // Caption box along the bottom. The pictures keep their lower edge quiet
    // for it, so on a wide window it sits over the picture; on a tall window
    // the picture is shown whole above it.
    const int scale = std::max(2, std::min(screenW / 420, screenH / 240));
    const int glyph = Theme::GLYPH * scale;
    const int lineStep = glyph + glyph / 2;
    const int margin = glyph;
    const int boxW = screenW - margin * 2;
    const std::size_t maxChars = static_cast<std::size_t>(std::max(8, (boxW - glyph * 2) / glyph));
    const Panel& panel = m_panels[m_index];
    // Sized for the whole caption, so the box does not grow as it types.
    const int lines = static_cast<int>(wrapText(panel.caption, maxChars).size());
    const int boxH = glyph + lines * lineStep + lineStep;
    const int boxY = screenH - margin - boxH;

    if (sprites) {
        const Sprite& art = sprites->icon("cutscenes", panel.image);
        if (art.valid() && art.width > 0 && art.height > 0) {
            // Show the whole picture, as large as fits.
            const float fit = std::min(static_cast<float>(screenW) / static_cast<float>(art.width),
                                       static_cast<float>(screenH) / static_cast<float>(art.height));
            const int w = static_cast<int>(static_cast<float>(art.width) * fit);
            const int h = static_cast<int>(static_cast<float>(art.height) * fit);
            // Centre it in the space above the box when that leaves it clear
            // of the box; otherwise centre it on the screen.
            const int spare = screenH - h;
            const int y = spare >= boxH + margin ? std::max(0, (boxY - h) / 2) : spare / 2;
            renderer->renderSpritePixelsWithTexture((screenW - w) / 2, y, w, h, art.texture);
        }
    }

    // Each panel fades up from the dark.
    const float fade = 1.0f - std::min(1.0f, m_panelTime / FADE_SECONDS);
    if (fade > 0.0f) {
        Theme::fillRect(renderer, 0, 0, screenW, screenH, Theme::BASE, fade);
    }

    Theme::drawPanel(renderer, margin, boxY, boxW, boxH);
    if (!text) {
        return;
    }
    int y = boxY + glyph / 2 + glyph / 4;
    for (const std::string& line : wrapText(getVisibleCaption(), maxChars)) {
        text->renderText(renderer, line, margin + glyph, y, scale, Theme::TEXT);
        y += lineStep;
    }

    const int hintScale = std::max(1, scale - 1);
    const bool last = m_index + 1 == m_panels.size();
    const std::string hint = InputPrompts::confirm() + (last ? ": " + m_lastHint : std::string(": next")) +
                             "   " + InputPrompts::cancel() + ": skip";
    const int hintW = static_cast<int>(hint.size()) * Theme::GLYPH * hintScale;
    text->renderText(renderer, hint, margin + boxW - glyph - hintW,
                     boxY + boxH - Theme::GLYPH * hintScale - glyph / 2, hintScale, Theme::MUTED);
}
#else
void Cutscene::render(VulkanRenderer*, TextRenderer*, SpriteLibrary*) const {}
#endif
