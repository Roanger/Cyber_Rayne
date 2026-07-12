#include "../../include/ui/TextRenderer.h"
#include "../../include/VulkanRenderer.h"

namespace {

inline float byteToFloat(int v) {
    return static_cast<float>(v & 0xFF) / 255.0f;
}

} // namespace

TextRenderer::TextRenderer() : m_font(std::make_unique<BitmapFont>()) {}
TextRenderer::~TextRenderer() = default;

bool TextRenderer::initialize(VulkanRenderer* renderer) {
    if (m_fontTextureIndex >= 0) {
        return true;
    }
    if (!renderer) {
        return false;
    }
    m_fontTextureIndex = renderer->loadTextureFromMemory(
        m_font->getAtlasPixels().data(), m_font->atlasWidth(), m_font->atlasHeight(), 4);
    return m_fontTextureIndex >= 0;
}

int TextRenderer::renderText(VulkanRenderer* renderer, const std::string& text, int x, int y,
                               int scale, uint32_t color) {
    if (!renderer || m_fontTextureIndex < 0) {
        return 0;
    }

    const int glyphW = m_font->glyphWidth();
    const int glyphH = m_font->glyphHeight();

    float r = byteToFloat((color >> 16) & 0xFF);
    float g = byteToFloat((color >> 8)  & 0xFF);
    float b = byteToFloat(color & 0xFF);
    float a = byteToFloat((color >> 24) & 0xFF);

    int cursorX = x;
    for (char c : text) {
        float u0, v0, u1, v1;
        m_font->getGlyphUV(c, u0, v0, u1, v1);

        int drawW = glyphW * scale;
        int drawH = glyphH * scale;
        renderer->renderSpritePixelsWithTexture(cursorX, y, drawW, drawH, m_fontTextureIndex,
                                               r, g, b, a, u0, v0, u1, v1);
        cursorX += drawW;
    }

    return cursorX - x;
}

int TextRenderer::measureText(const std::string& text, int scale) const {
    return static_cast<int>(text.size()) * m_font->glyphWidth() * scale;
}

int TextRenderer::lineHeight(int scale) const {
    return m_font->glyphHeight() * scale;
}
