#include "../../include/ui/UILabel.h"
#include "../../include/VulkanRenderer.h"
#include "../../include/ui/TextRenderer.h"

UILabel::UILabel(int x, int y, const std::string& text, int scale)
    : UIWidget(x, y, 0, 0), m_text(text), m_scale(scale), m_textColor(0xFFFFFFFF) {
}

void UILabel::render(VulkanRenderer* renderer, TextRenderer* textRenderer) {
    if (!isVisible() || !renderer || !textRenderer) {
        return;
    }
    textRenderer->renderText(renderer, m_text, x(), y(), m_scale, m_textColor);
}
