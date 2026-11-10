#include "../../include/ui/BattleScene.h"
#include "../../include/Combatant.h"
#include "../../include/Party.h"
#include "../../include/Player.h"
#include "../../include/VulkanRenderer.h"
#include "../../include/ui/SpriteLibrary.h"
#include "../../include/ui/TextRenderer.h"
#include "../../include/ui/Theme.h"

#include <algorithm>
#include <cmath>
#include <functional>

namespace {

// Size a missing sprite's placeholder as if it were a normal character.
constexpr int FALLBACK_W = 96;
constexpr int FALLBACK_H = 128;

// Where the floor meets the skyline, as a fraction of the scene height.
constexpr float HORIZON = 0.40f;

struct Palette {
    uint32_t skyTop;
    uint32_t skyBottom;
    uint32_t building;
    uint32_t window;
    uint32_t glow;
    uint32_t floor;
};

// One look per district, all inside the Rose Pine family.
Palette paletteFor(const std::string& mapName) {
    if (mapName == "Neon Alley") {
        return {0xFF16101F, 0xFF3A1838, 0xFF120D1A, Theme::LOVE, Theme::LOVE, 0xFF1C1424};
    }
    if (mapName == "Corporate Plaza") {
        return {0xFF0E1420, 0xFF1D3040, 0xFF0C1118, Theme::FOAM, Theme::FOAM, 0xFF151C26};
    }
    return {0xFF14111F, 0xFF2D2348, 0xFF100E19, Theme::GOLD, Theme::IRIS, 0xFF1A1826};
}

uint32_t mix(uint32_t a, uint32_t b, float t) {
    auto channel = [&](int shift) {
        const float ca = static_cast<float>((a >> shift) & 0xFF);
        const float cb = static_cast<float>((b >> shift) & 0xFF);
        return static_cast<uint32_t>(ca + (cb - ca) * t + 0.5f) & 0xFF;
    };
    return 0xFF000000 | (channel(16) << 16) | (channel(8) << 8) | channel(0);
}

// Small deterministic generator, so a district's skyline is the same every
// fight without storing anything.
struct Lcg {
    uint32_t state;
    uint32_t next() {
        state = state * 1664525u + 1013904223u;
        return state >> 8;
    }
    int range(int lo, int hi) { return lo + static_cast<int>(next() % static_cast<uint32_t>(hi - lo + 1)); }
};

void drawBar(VulkanRenderer* renderer, int x, int y, int w, int h, float fraction, uint32_t color) {
    fraction = std::clamp(fraction, 0.0f, 1.0f);
    Theme::fillRect(renderer, x - 2, y - 2, w + 4, h + 4, Theme::BASE, 0.85f);
    Theme::fillRect(renderer, x, y, w, h, Theme::MUTED, 0.35f);
    Theme::fillRect(renderer, x, y, static_cast<int>(static_cast<float>(w) * fraction), h, color);
}

// Row of status tags, centred on centreX. Returns nothing; draws nothing if
// the combatant has no active effects.
void drawStatusTags(VulkanRenderer* renderer, TextRenderer* text, const Combatant* who,
                    int centreX, int y) {
    std::vector<StatusTag> tags;
    for (const auto& effect : who->getStatusEffects()) {
        if (effect && !effect->isExpired()) {
            tags.push_back(statusTagFor(effect->getType()));
        }
    }
    if (tags.empty()) return;

    const int gap = 6;
    int total = -gap;
    for (const StatusTag& tag : tags) {
        total += text->measureText(tag.text, 1) + 8 + gap;
    }
    int x = centreX - total / 2;
    for (const StatusTag& tag : tags) {
        const int w = text->measureText(tag.text, 1) + 8;
        Theme::fillRect(renderer, x, y, w, 14, Theme::BASE, 0.9f);
        Theme::fillRect(renderer, x, y + 12, w, 2, tag.color);
        text->renderText(renderer, tag.text, x + 4, y + 2, 1, tag.color);
        x += w + gap;
    }
}

// A downward-pointing marker above a combatant's head.
void drawMarker(VulkanRenderer* renderer, int centreX, int tipY, uint32_t color) {
    for (int row = 0; row < 5; ++row) {
        const int half = (5 - row) * 4;
        Theme::fillRect(renderer, centreX - half, tipY - (5 - row) * 4, half * 2, 4, color);
    }
}

}  // namespace

void BattleScene::begin(Party* party, const std::vector<Combatant*>& enemies,
                        const std::string& mapName) {
    m_party = party;
    m_enemies = enemies;
    m_mapName = mapName;
    m_fx.reset();
    m_time = 0.0f;
}

void BattleScene::clearEnemies() {
    m_enemies.clear();
    m_fx.reset();
}

void BattleScene::update(float deltaTime) {
    m_time += deltaTime;
    m_fx.update(deltaTime);
}

void BattleScene::renderBackdrop(VulkanRenderer* renderer, int screenW, int sceneH) const {
    const Palette pal = paletteFor(m_mapName);
    const int horizonY = static_cast<int>(static_cast<float>(sceneH) * HORIZON);

    // Sky: banded gradient, which suits the pixel look better than a smooth one.
    const int bands = 10;
    for (int i = 0; i < bands; ++i) {
        const int y0 = horizonY * i / bands;
        const int y1 = horizonY * (i + 1) / bands;
        Theme::fillRect(renderer, 0, y0, screenW, y1 - y0,
                        mix(pal.skyTop, pal.skyBottom, static_cast<float>(i) / (bands - 1)));
    }

    // Skyline: two rows of buildings with a scattering of lit windows.
    Lcg rng{static_cast<uint32_t>(std::hash<std::string>{}(m_mapName)) | 1u};
    for (int layer = 0; layer < 2; ++layer) {
        const uint32_t body = layer == 0 ? mix(pal.building, pal.skyBottom, 0.35f) : pal.building;
        const int maxH = layer == 0 ? horizonY * 9 / 10 : horizonY * 6 / 10;
        const int minH = layer == 0 ? horizonY * 4 / 10 : horizonY * 2 / 10;
        int x = -rng.range(0, 40);
        while (x < screenW) {
            const int w = rng.range(70, 170);
            const int h = rng.range(minH, maxH);
            Theme::fillRect(renderer, x, horizonY - h, w, h, body);
            // Windows on a coarse grid; most stay dark.
            const int cell = 18;
            const int windowBudget = layer == 0 ? 5 : 7;
            for (int n = 0; n < windowBudget; ++n) {
                const int cols = std::max(1, (w - 16) / cell);
                const int rows = std::max(1, (h - 16) / cell);
                const int wx = x + 8 + rng.range(0, cols - 1) * cell;
                const int wy = horizonY - h + 8 + rng.range(0, rows - 1) * cell;
                Theme::fillRect(renderer, wx, wy, 8, 6, pal.window, layer == 0 ? 0.35f : 0.75f);
            }
            x += w + rng.range(0, 14);
        }
    }

    // Glow along the horizon, then the floor.
    Theme::fillRect(renderer, 0, horizonY - 6, screenW, 6, pal.glow, 0.25f);
    Theme::fillRect(renderer, 0, horizonY, screenW, sceneH - horizonY, pal.floor);
    Theme::fillRect(renderer, 0, horizonY, screenW, 3, pal.glow, 0.8f);

    // Floor grid: lines spread apart toward the viewer for a sense of depth.
    const int floorH = sceneH - horizonY;
    for (int i = 1; i <= 7; ++i) {
        const float t = static_cast<float>(i) / 7.0f;
        const int y = horizonY + static_cast<int>(static_cast<float>(floorH) * t * t);
        Theme::fillRect(renderer, 0, y, screenW, 2, pal.glow, 0.10f + 0.10f * t);
    }
}

BattleScene::Placement BattleScene::place(bool enemySide, std::size_t index, std::size_t count,
                                          int frameW, int frameH, int screenW, int sceneH,
                                          int scale) const {
    const int horizonY = static_cast<int>(static_cast<float>(sceneH) * HORIZON);
    const int floorH = sceneH - horizonY;

    // Stand in a diagonal line: each one a step lower and well further from
    // the middle than the last, so even a boss-sized sprite does not hide its
    // neighbour.
    const float t = count > 1 ? static_cast<float>(index) / static_cast<float>(count - 1) : 0.5f;
    const float depth = 0.46f + 0.48f * t;
    const float fromCentre = 0.13f + 0.25f * t;

    Placement p;
    p.width = frameW * scale;
    p.height = frameH * scale;
    p.feetY = horizonY + static_cast<int>(static_cast<float>(floorH) * depth);
    const float centre = static_cast<float>(screenW) * 0.5f;
    const float offset = static_cast<float>(screenW) * fromCentre;
    p.feetX = static_cast<int>(enemySide ? centre - offset : centre + offset);
    return p;
}

void BattleScene::render(VulkanRenderer* renderer, TextRenderer* text, SpriteLibrary* sprites,
                         const Combatant* actor, const Combatant* target) {
    if (!renderer || !text || !sprites) return;

    const int screenW = static_cast<int>(renderer->getSwapchainExtent().width);
    const int screenH = static_cast<int>(renderer->getSwapchainExtent().height);
    const int sceneH = screenH - BattleLayout::PANEL_HEIGHT;
    // Whole-number scaling keeps pixel art crisp.
    const int scale = std::max(1, static_cast<int>(std::lround(static_cast<float>(screenH) / 540.0f)));

    renderBackdrop(renderer, screenW, sceneH);

    struct Figure {
        const Combatant* who;
        const Sprite* sprite;
        Placement at;
        bool enemy;
    };
    std::vector<Figure> figures;

    for (std::size_t i = 0; i < m_enemies.size(); ++i) {
        const Combatant* enemy = m_enemies[i];
        if (!enemy) continue;
        const Sprite& sprite = sprites->get("enemies", enemy->getName());
        const int fw = sprite.valid() ? sprite.frameWidth() : FALLBACK_W;
        const int fh = sprite.valid() ? sprite.frameHeight() : FALLBACK_H;
        figures.push_back({enemy, &sprite,
                           place(true, i, m_enemies.size(), fw, fh, screenW, sceneH, scale), true});
    }
    if (m_party) {
        const auto& members = m_party->getMembers();
        for (std::size_t i = 0; i < members.size(); ++i) {
            const Player* member = members[i].get();
            if (!member) continue;
            const Sprite& sprite = sprites->forMember(*member);
            const int fw = sprite.valid() ? sprite.frameWidth() : FALLBACK_W;
            const int fh = sprite.valid() ? sprite.frameHeight() : FALLBACK_H;
            figures.push_back({member, &sprite,
                               place(false, i, members.size(), fw, fh, screenW, sceneH, scale), false});
        }
    }

    // Back to front, so those standing lower on screen overlap those behind.
    std::stable_sort(figures.begin(), figures.end(),
                     [](const Figure& a, const Figure& b) { return a.at.feetY < b.at.feetY; });

    for (Figure& figure : figures) {
        const Combatant* who = figure.who;
        const float visibility = figure.enemy ? m_fx.getVisibility(who) : 1.0f;
        if (visibility <= 0.0f) continue;
        // An enemy at 0 HP whose fade has not begun (the event arrives a frame
        // later) is still drawn; one that was never alive in this fight is not.
        if (figure.enemy && !who->isAlive() && !m_fx.isDefeated(who)) continue;

        const float flash = m_fx.getHitFlash(who);
        const float lunge = m_fx.getLunge(who);
        const int towardCentre = figure.enemy ? 1 : -1;
        int x = figure.at.feetX - figure.at.width / 2 +
                static_cast<int>(lunge * 36.0f * static_cast<float>(scale)) * towardCentre;
        if (flash > 0.0f) {
            x += static_cast<int>(std::sin(m_time * 70.0f) * 5.0f * flash * static_cast<float>(scale));
        }
        const int y = figure.at.feetY - figure.at.height;
        figure.at.feetX = x + figure.at.width / 2;  // labels follow the sprite

        // Shadow under the feet anchors the figure to the floor.
        Theme::fillRect(renderer, figure.at.feetX - figure.at.width * 3 / 10, figure.at.feetY - 6,
                        figure.at.width * 6 / 10, 10, 0xFF000000, 0.35f * visibility);

        float r = 1.0f, g = 1.0f, b = 1.0f;
        if (flash > 0.0f) {
            // Tint toward red. (Colour multiplies the image, so it cannot go
            // brighter than the art; a white flash would need a shader change.)
            g = b = 1.0f - 0.75f * flash;
        }
        if (!figure.enemy && !who->isAlive()) {
            r = g = b = 0.28f;  // fallen party members stay, darkened
        }

        if (figure.sprite->valid()) {
            sprites->draw(*figure.sprite, x, y, figure.at.width, figure.at.height, r, g, b, visibility);
        } else {
            const uint32_t block = figure.enemy ? Theme::LOVE : Theme::FOAM;
            Theme::fillRect(renderer, x, y, figure.at.width, figure.at.height, block, 0.8f * visibility);
        }
    }

    // Labels go on after every sprite so nothing covers them.
    for (const Figure& figure : figures) {
        const Combatant* who = figure.who;
        const int headY = figure.at.feetY - figure.at.height;
        const bool standing = who->isAlive();

        if (figure.enemy && standing) {
            const std::string& name = who->getName();
            const int nameW = text->measureText(name, 2);
            text->renderText(renderer, name, figure.at.feetX - nameW / 2 + 2, headY - 44 + 2, 2, Theme::BASE);
            text->renderText(renderer, name, figure.at.feetX - nameW / 2, headY - 44, 2, Theme::TEXT);
            const int barW = 150;
            drawBar(renderer, figure.at.feetX - barW / 2, headY - 22, barW, 8,
                    static_cast<float>(who->getHealth()) / static_cast<float>(std::max(1, who->getMaxHealth())),
                    Theme::LOVE);
        }
        if (standing) {
            drawStatusTags(renderer, text, who, figure.at.feetX, figure.at.feetY + 8);
        }

        const int bob = static_cast<int>(std::sin(m_time * 6.0f) * 4.0f);
        const int markerY = headY - (figure.enemy ? 54 : 10) + bob;
        if (who == target) {
            drawMarker(renderer, figure.at.feetX, markerY, Theme::GOLD);
        } else if (who == actor) {
            drawMarker(renderer, figure.at.feetX, markerY, Theme::FOAM);
        }
    }

    // Floating numbers rise from the middle of the figure and fade at the end.
    for (const BattleFx::Popup& popup : m_fx.getPopups()) {
        if (popup.age < 0.0f) continue;
        auto it = std::find_if(figures.begin(), figures.end(),
                               [&](const Figure& f) { return f.who == popup.who; });
        if (it == figures.end()) continue;
        const float life = popup.age / BattleFx::POPUP_LIFETIME;
        const int rise = static_cast<int>(90.0f * (1.0f - (1.0f - life) * (1.0f - life)));
        const int w = text->measureText(popup.text, 4);
        const int x = it->at.feetX - w / 2;
        const int y = it->at.feetY - it->at.height * 6 / 10 - rise;
        const float alpha = life > 0.75f ? (1.0f - life) / 0.25f : 1.0f;
        const uint32_t a = static_cast<uint32_t>(alpha * 255.0f) << 24;
        text->renderText(renderer, popup.text, x + 3, y + 3, 4, a | (Theme::BASE & 0x00FFFFFF));
        text->renderText(renderer, popup.text, x, y, 4, a | (popup.color & 0x00FFFFFF));
    }

    // Action banner across the top.
    const std::string& banner = m_fx.getBanner();
    if (!banner.empty()) {
        const int w = text->measureText(banner, 2) + 48;
        const int x = (screenW - w) / 2;
        Theme::drawPanel(renderer, x, 18, w, 48);
        text->renderText(renderer, banner, x + 24, 18 + 16, 2, Theme::TEXT);
    }
}
