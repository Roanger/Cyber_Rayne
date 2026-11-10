#include "../../include/ui/Theme.h"
#include "../../include/VulkanRenderer.h"

namespace Theme {

void fillRect(VulkanRenderer* renderer, int x, int y, int w, int h, uint32_t color, float alpha) {
    if (!renderer || w <= 0 || h <= 0) return;
    const float a = static_cast<float>((color >> 24) & 0xFF) / 255.0f * alpha;
    const float r = static_cast<float>((color >> 16) & 0xFF) / 255.0f;
    const float g = static_cast<float>((color >> 8) & 0xFF) / 255.0f;
    const float b = static_cast<float>(color & 0xFF) / 255.0f;
    renderer->renderColoredRectPixels(x, y, w, h, r, g, b, a);
}

void drawPanel(VulkanRenderer* renderer, int x, int y, int w, int h) {
    const int border = 3;
    fillRect(renderer, x, y, w, h, IRIS);
    fillRect(renderer, x + border, y + border, w - 2 * border, h - 2 * border, BASE, 0.94f);
}

void dimScreen(VulkanRenderer* renderer, float alpha) {
    if (!renderer) return;
    const int w = static_cast<int>(renderer->getSwapchainExtent().width);
    const int h = static_cast<int>(renderer->getSwapchainExtent().height);
    fillRect(renderer, 0, 0, w, h, BASE, alpha);
}

}  // namespace Theme
