#include "../../include/ui/BitmapFont.h"
#include <iostream>
#include <cmath>

namespace {

bool approxEqual(float a, float b, float epsilon = 0.001f) {
    return std::fabs(a - b) < epsilon;
}

} // namespace

int main() {
    BitmapFont font;

    // Atlas dimensions should match the embedded data.
    constexpr int expectedWidth = 16 * 8;
    constexpr int expectedHeight = 6 * 8;
    if (font.atlasWidth() != expectedWidth || font.atlasHeight() != expectedHeight) {
        std::cerr << "FAIL: unexpected atlas size "
                  << font.atlasWidth() << "x" << font.atlasHeight() << std::endl;
        return 1;
    }

    // Pixel data should be generated.
    const auto& pixels = font.getAtlasPixels();
    if (pixels.size() != static_cast<std::size_t>(expectedWidth * expectedHeight * 4)) {
        std::cerr << "FAIL: unexpected pixel buffer size " << pixels.size() << std::endl;
        return 1;
    }

    // The space glyph should be transparent.
    // Space is the first glyph at atlas position (0,0)-(7,7).
    bool spaceIsClear = true;
    for (int y = 0; y < 8 && spaceIsClear; ++y) {
        for (int x = 0; x < 8 && spaceIsClear; ++x) {
            int idx = (y * expectedWidth + x) * 4 + 3;
            if (pixels[idx] != 0) {
                spaceIsClear = false;
            }
        }
    }
    if (!spaceIsClear) {
        std::cerr << "FAIL: space glyph was not empty" << std::endl;
        return 1;
    }

    // Glyph 'A' should have some opaque pixels.
    // 'A' is at character code 65, index 33.
    int aCol = 33 % 16;
    int aRow = 33 / 16;
    int aBaseX = aCol * 8;
    int aBaseY = aRow * 8;
    bool aHasInk = false;
    for (int y = 0; y < 8 && !aHasInk; ++y) {
        for (int x = 0; x < 8 && !aHasInk; ++x) {
            int idx = ((aBaseY + y) * expectedWidth + (aBaseX + x)) * 4 + 3;
            if (pixels[idx] != 0) {
                aHasInk = true;
            }
        }
    }
    if (!aHasInk) {
        std::cerr << "FAIL: glyph 'A' had no ink" << std::endl;
        return 1;
    }

    // UV coordinates should be in bounds and consistent.
    float u0 = 0.0f, v0 = 0.0f, u1 = 0.0f, v1 = 0.0f;
    font.getGlyphUV('A', u0, v0, u1, v1);
    if (u0 < 0.0f || v0 < 0.0f || u1 > 1.0f || v1 > 1.0f || u0 >= u1 || v0 >= v1) {
        std::cerr << "FAIL: invalid UVs for 'A': " << u0 << "," << v0 << " -> " << u1 << "," << v1 << std::endl;
        return 1;
    }

    // UV width/height for a single glyph should equal 1/16 by 1/6.
    if (!approxEqual(u1 - u0, 1.0f / 16.0f) || !approxEqual(v1 - v0, 1.0f / 6.0f)) {
        std::cerr << "FAIL: glyph UV size was not 1/16 x 1/6" << std::endl;
        return 1;
    }

    // Out-of-range characters should fall back to space.
    font.getGlyphUV('\x01', u0, v0, u1, v1);
    if (!approxEqual(u0, 0.0f) || !approxEqual(v0, 0.0f)) {
        std::cerr << "FAIL: control character did not fall back to space" << std::endl;
        return 1;
    }

    std::cout << "BitmapFontTest passed." << std::endl;
    return 0;
}
