#pragma once

#include <cstdint>
#include <string>

#include "UIWidget.h"

class UILabel : public UIWidget {
public:
    UILabel(int x = 0, int y = 0, const std::string& text = "", int scale = 2);

    void setText(const std::string& text) { m_text = text; }
    const std::string& getText() const { return m_text; }

    void setScale(int scale) { m_scale = scale; }
    int scale() const { return m_scale; }

    void setTextColor(uint32_t color) { m_textColor = color; }
    uint32_t textColor() const { return m_textColor; }

    void render(VulkanRenderer* renderer, TextRenderer* textRenderer) override;

private:
    std::string m_text;
    int m_scale;
    uint32_t m_textColor;
};
