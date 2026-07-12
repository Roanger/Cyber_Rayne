#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "UIWidget.h"

class UIButton : public UIWidget {
public:
    UIButton(int x = 0, int y = 0, int width = 0, int height = 0, const std::string& label = "");

    void setLabel(const std::string& label) { m_label = label; }
    const std::string& getLabel() const { return m_label; }

    void setSelected(bool selected) { m_selected = selected; }
    bool isSelected() const { return m_selected; }

    void setLabelColor(uint32_t color) { m_labelColor = color; }
    uint32_t labelColor() const { return m_labelColor; }

    void setSelectedColor(uint32_t color) { m_selectedColor = color; }
    uint32_t selectedColor() const { return m_selectedColor; }

    void setLabelScale(int scale) { m_labelScale = scale; }
    int labelScale() const { return m_labelScale; }

    void setOnConfirm(std::function<void()> callback) { m_onConfirm = std::move(callback); }

    void render(VulkanRenderer* renderer, TextRenderer* textRenderer) override;
    bool handleInput(InputAction action) override;

private:
    std::string m_label;
    bool m_selected;
    uint32_t m_labelColor;
    uint32_t m_selectedColor;
    int m_labelScale;
    std::function<void()> m_onConfirm;
};
