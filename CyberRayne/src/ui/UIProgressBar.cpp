#include "../../include/ui/UIProgressBar.h"
#include "../../include/VulkanRenderer.h"
#include "../../include/ui/TextRenderer.h"

namespace {

inline void unpackColor(uint32_t color, float& r, float& g, float& b, float& a) {
    a = static_cast<float>((color >> 24) & 0xFF) / 255.0f;
    r = static_cast<float>((color >> 16) & 0xFF) / 255.0f;
    g = static_cast<float>((color >> 8)  & 0xFF) / 255.0f;
    b = static_cast<float>(color & 0xFF)        / 255.0f;
}

inline float clamp01(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

} // namespace

UIProgressBar::UIProgressBar(int x, int y, int width, int height)
    : UIWidget(x, y, width, height), m_value(1.0f), m_fillColor(0xFF00FF00), m_labelScale(2) {
    setColor(0xFF222222);
}

void UIProgressBar::render(VulkanRenderer* renderer, TextRenderer* textRenderer) {
    if (!isVisible() || !renderer || width() <= 0 || height() <= 0) {
        return;
    }

    float r, g, b, a;
    unpackColor(color(), r, g, b, a);
    renderer->renderColoredRectPixels(x(), y(), width(), height(), r, g, b, a);

    float fill = clamp01(m_value);
    if (fill > 0.0f) {
        float fr, fg, fb, fa;
        unpackColor(m_fillColor, fr, fg, fb, fa);
        int fillW = static_cast<int>(static_cast<float>(width()) * fill);
        renderer->renderColoredRectPixels(x(), y(), fillW, height(), fr, fg, fb, fa);
    }

    if (!m_label.empty() && textRenderer) {
        int textW = textRenderer->measureText(m_label, m_labelScale);
        int textH = textRenderer->lineHeight(m_labelScale);
        int labelX = x() + (width() - textW) / 2;
        int labelY = y() + (height() - textH) / 2;
        textRenderer->renderText(renderer, m_label, labelX, labelY, m_labelScale, 0xFFFFFFFF);
    }
}
