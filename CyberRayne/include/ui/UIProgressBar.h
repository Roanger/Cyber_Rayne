#pragma once

#include <cstdint>
#include <string>

#include "UIWidget.h"

class UIProgressBar : public UIWidget {
public:
    UIProgressBar(int x = 0, int y = 0, int width = 0, int height = 0);

    void setValue(float value) { m_value = value; }
    float value() const { return m_value; }

    void setFillColor(uint32_t color) { m_fillColor = color; }
    uint32_t fillColor() const { return m_fillColor; }

    void setLabel(const std::string& label) { m_label = label; }
    const std::string& getLabel() const { return m_label; }
    void setLabelScale(int scale) { m_labelScale = scale; }

    void render(VulkanRenderer* renderer, TextRenderer* textRenderer) override;

private:
    float m_value; // 0..1
    uint32_t m_fillColor;
    std::string m_label;
    int m_labelScale;
};
