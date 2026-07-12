#include "../../include/ui/BitmapFont.h"
#include "../../include/ui/BitmapFontData.h"

namespace {

constexpr int ATLAS_W = BitmapFontData::ATLAS_WIDTH;
constexpr int ATLAS_H = BitmapFontData::ATLAS_HEIGHT;
constexpr int GLYPH_W = BitmapFontData::GLYPH_W;
constexpr int GLYPH_H = BitmapFontData::GLYPH_H;
constexpr int COLS = BitmapFontData::ATLAS_COLS;
constexpr int ROWS = BitmapFontData::ATLAS_ROWS;
constexpr int FIRST_CHAR = BitmapFontData::FIRST_CHAR;
constexpr int CHAR_COUNT = BitmapFontData::CHAR_COUNT;

inline int clampChar(char c) {
    int code = static_cast<unsigned char>(c);
    if (code < FIRST_CHAR || code >= FIRST_CHAR + CHAR_COUNT) {
        code = FIRST_CHAR; // space fallback
    }
    return code - FIRST_CHAR;
}

} // namespace

BitmapFont::BitmapFont() = default;
BitmapFont::~BitmapFont() = default;

void BitmapFont::buildAtlas() {
    if (!m_atlas.empty()) {
        return;
    }

    m_atlas.resize(ATLAS_W * ATLAS_H * 4, 0);

    for (int ch = 0; ch < CHAR_COUNT; ++ch) {
        int col = ch % COLS;
        int row = ch / COLS;
        int baseX = col * GLYPH_W;
        int baseY = row * GLYPH_H;

        for (int gy = 0; gy < GLYPH_H; ++gy) {
            int dataIndex = ((ch / COLS) * GLYPH_H + gy) * COLS + (ch % COLS);
            uint8_t byte = BitmapFontData::g_fontData[dataIndex];
            int py = baseY + gy;
            for (int gx = 0; gx < GLYPH_W; ++gx) {
                int bit = GLYPH_W - 1 - gx;
                bool on = (byte & (1 << bit)) != 0;
                int px = baseX + gx;
                int idx = (py * ATLAS_W + px) * 4;
                if (on) {
                    m_atlas[idx + 0] = 255; // R
                    m_atlas[idx + 1] = 255; // G
                    m_atlas[idx + 2] = 255; // B
                    m_atlas[idx + 3] = 255; // A
                }
            }
        }
    }
}

int BitmapFont::atlasWidth() const {
    return ATLAS_W;
}

int BitmapFont::atlasHeight() const {
    return ATLAS_H;
}

void BitmapFont::getGlyphUV(char c, float& u0, float& v0, float& u1, float& v1) const {
    int index = clampChar(c);
    int col = index % COLS;
    int row = index / COLS;

    float invW = 1.0f / static_cast<float>(ATLAS_W);
    float invH = 1.0f / static_cast<float>(ATLAS_H);

    u0 = static_cast<float>(col * GLYPH_W) * invW;
    v0 = static_cast<float>(row * GLYPH_H) * invH;
    u1 = static_cast<float>((col + 1) * GLYPH_W) * invW;
    v1 = static_cast<float>((row + 1) * GLYPH_H) * invH;
}

const std::vector<uint8_t>& BitmapFont::getAtlasPixels() const {
    if (m_atlas.empty()) {
        const_cast<BitmapFont*>(this)->buildAtlas();
    }
    return m_atlas;
}
