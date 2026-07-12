#include "../../include/ui/UIButton.h"
#include "../../include/VulkanRenderer.h"
#include "../../include/ui/TextRenderer.h"

namespace {

inline void unpackColor(uint32_t color, float& r, float& g, float& b, float& a) {
    a = static_cast<float>((color >> 24) & 0xFF) / 255.0f;
    r = static_cast<float>((color >> 16) & 0xFF) / 255.0f;
    g = static_cast<float>((color >> 8)  & 0xFF) / 255.0f;
    b = static_cast<float>(color & 0xFF)        / 255.0f;
}

} // namespace

UIButton::UIButton(int x, int y, int width, int height, const std::string& label)
    : UIWidget(x, y, width, height), m_label(label), m_selected(false),
      m_labelColor(0xFFFFFFFF), m_selectedColor(0xFF4444FF), m_labelScale(2) {
    setColor(0x88000000);
}

void UIButton::render(VulkanRenderer* renderer, TextRenderer* textRenderer) {
    if (!isVisible() || !renderer) {
        return;
    }

    uint32_t bgColor = m_selected ? m_selectedColor : color();
    float r, g, b, a;
    unpackColor(bgColor, r, g, b, a);
    renderer->renderColoredRectPixels(x(), y(), width(), height(), r, g, b, a);

    if (!m_label.empty() && textRenderer) {
        int textW = textRenderer->measureText(m_label, m_labelScale);
        int textH = textRenderer->lineHeight(m_labelScale);
        int labelX = x() + (width() - textW) / 2;
        int labelY = y() + (height() - textH) / 2;
        float lr, lg, lb, la;
        unpackColor(m_labelColor, lr, lg, lb, la);
        // TextRenderer packs its own color, but we pass it as packed RGBA.
        textRenderer->renderText(renderer, m_label, labelX, labelY, m_labelScale, m_labelColor);
    }
}

bool UIButton::handleInput(InputAction action) {
    if (!m_selected) {
        return false;
    }
    if (action == InputAction::CONFIRM) {
        if (m_onConfirm) {
            m_onConfirm();
        }
        return true;
    }
    return false;
}
