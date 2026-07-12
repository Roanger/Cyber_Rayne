#pragma once

#include <cstdint>

#include "UIWidget.h"

class UIPanel : public UIWidget {
public:
    UIPanel(int x = 0, int y = 0, int width = 0, int height = 0, uint32_t fillColor = 0xCC000000);

    void setBorderColor(uint32_t color) { m_borderColor = color; m_hasBorder = true; }
    void setBorderWidth(int width) { m_borderWidth = width; }

    void render(VulkanRenderer* renderer, TextRenderer* textRenderer) override;

private:
    uint32_t m_borderColor;
    bool m_hasBorder;
    int m_borderWidth;
};
