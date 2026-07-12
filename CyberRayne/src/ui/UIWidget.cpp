#include "../../include/ui/UIWidget.h"

UIWidget::UIWidget(int x, int y, int width, int height)
    : m_x(x), m_y(y), m_width(width), m_height(height), m_visible(true), m_color(0xFFFFFFFF) {
}

UIWidget::~UIWidget() = default;

bool UIWidget::handleInput(InputAction /*action*/) {
    return false;
}
