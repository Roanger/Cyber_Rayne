#include "../../include/ui/UIRoot.h"
#include "../../include/VulkanRenderer.h"
#include "../../include/ui/TextRenderer.h"

UIRoot::UIRoot() = default;
UIRoot::~UIRoot() = default;

void UIRoot::addWidget(std::unique_ptr<UIWidget> widget) {
    if (widget) {
        m_widgets.push_back(std::move(widget));
    }
}

void UIRoot::clear() {
    m_widgets.clear();
}

void UIRoot::render(VulkanRenderer* renderer, TextRenderer* textRenderer) {
    for (auto& widget : m_widgets) {
        if (widget && widget->isVisible()) {
            widget->render(renderer, textRenderer);
        }
    }
}

bool UIRoot::handleInput(InputAction action) {
    for (auto& widget : m_widgets) {
        if (widget && widget->isVisible() && widget->handleInput(action)) {
            return true;
        }
    }
    return false;
}
