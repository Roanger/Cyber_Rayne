#pragma once

#include <cstdint>
#include <string>

#include "../InputAction.h"

class VulkanRenderer;
class TextRenderer;

// Base class for all UI primitives.
// Coordinates and sizes are in screen pixels with the origin at the top-left.
class UIWidget {
public:
    UIWidget(int x = 0, int y = 0, int width = 0, int height = 0);
    virtual ~UIWidget();

    virtual void render(VulkanRenderer* renderer, TextRenderer* textRenderer) = 0;
    virtual bool handleInput(InputAction action);

    void setPosition(int x, int y) { m_x = x; m_y = y; }
    void setSize(int width, int height) { m_width = width; m_height = height; }
    void setVisible(bool visible) { m_visible = visible; }

    int x() const { return m_x; }
    int y() const { return m_y; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    bool isVisible() const { return m_visible; }

    void setColor(uint32_t color) { m_color = color; }
    uint32_t color() const { return m_color; }

protected:
    int m_x;
    int m_y;
    int m_width;
    int m_height;
    bool m_visible;
    uint32_t m_color;
};
