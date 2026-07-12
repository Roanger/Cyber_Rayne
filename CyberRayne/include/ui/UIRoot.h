#pragma once

#include <memory>
#include <vector>

#include "UIWidget.h"

class VulkanRenderer;
class TextRenderer;

// Simple container that owns UIWidget instances and routes render/input calls.
class UIRoot {
public:
    UIRoot();
    ~UIRoot();

    void addWidget(std::unique_ptr<UIWidget> widget);
    void clear();

    void render(VulkanRenderer* renderer, TextRenderer* textRenderer);
    bool handleInput(InputAction action);

    std::size_t widgetCount() const { return m_widgets.size(); }

private:
    std::vector<std::unique_ptr<UIWidget>> m_widgets;
};
