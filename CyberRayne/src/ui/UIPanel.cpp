#include "../../include/ui/UIPanel.h"
#include "../../include/VulkanRenderer.h"

namespace {

inline void unpackColor(uint32_t color, float& r, float& g, float& b, float& a) {
    a = static_cast<float>((color >> 24) & 0xFF) / 255.0f;
    r = static_cast<float>((color >> 16) & 0xFF) / 255.0f;
    g = static_cast<float>((color >> 8)  & 0xFF) / 255.0f;
    b = static_cast<float>(color & 0xFF)        / 255.0f;
}

} // namespace

UIPanel::UIPanel(int x, int y, int width, int height, uint32_t fillColor)
    : UIWidget(x, y, width, height), m_borderColor(0xFFFFFFFF), m_hasBorder(false), m_borderWidth(2) {
    setColor(fillColor);
}

void UIPanel::render(VulkanRenderer* renderer, TextRenderer* /*textRenderer*/) {
    if (!isVisible() || !renderer || width() <= 0 || height() <= 0) {
        return;
    }

    float r, g, b, a;
    unpackColor(color(), r, g, b, a);
    renderer->renderColoredRectPixels(x(), y(), width(), height(), r, g, b, a);

    if (m_hasBorder && m_borderWidth > 0) {
        float br, bg, bb, ba;
        unpackColor(m_borderColor, br, bg, bb, ba);
        int bw = m_borderWidth;
        int x0 = x();
        int y0 = y();
        int w = width();
        int h = height();
        // Top, bottom, left, right borders
        renderer->renderColoredRectPixels(x0, y0, w, bw, br, bg, bb, ba);
        renderer->renderColoredRectPixels(x0, y0 + h - bw, w, bw, br, bg, bb, ba);
        renderer->renderColoredRectPixels(x0, y0, bw, h, br, bg, bb, ba);
        renderer->renderColoredRectPixels(x0 + w - bw, y0, bw, h, br, bg, bb, ba);
    }
}
