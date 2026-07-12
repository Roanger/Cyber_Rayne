#pragma once

#include <memory>
#include <string>

#include "BitmapFont.h"

class VulkanRenderer;

// Renders strings using an uploaded BitmapFont.
// All positions are in screen pixels with the origin at the top-left.
class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();

    // Upload the embedded font to the renderer. Safe to call multiple times.
    bool initialize(VulkanRenderer* renderer);

    // Draw a single-line string. Returns the pixel width occupied by the text.
    int renderText(VulkanRenderer* renderer, const std::string& text, int x, int y,
                   int scale = 1, uint32_t color = 0xFFFFFFFF);

    // Compute the pixel width of a string without rendering.
    int measureText(const std::string& text, int scale = 1) const;

    int lineHeight(int scale = 1) const;

private:
    std::unique_ptr<BitmapFont> m_font;
    int m_fontTextureIndex = -1;
};
