#pragma once

#include <cstdint>
#include <vector>

// A fixed-width 8x8 bitmap font backed by embedded glyph data.
// The atlas is generated in system memory and can be uploaded by a renderer.
class BitmapFont {
public:
    BitmapFont();
    ~BitmapFont();

    // Generate the atlas in memory (called automatically when pixels are requested).
    void buildAtlas();

    // Character metrics (all glyphs are fixed 8x8 in this prototype).
    static constexpr int glyphWidth() { return 8; }
    static constexpr int glyphHeight() { return 8; }

    // Atlas dimensions in pixels.
    int atlasWidth() const;
    int atlasHeight() const;

    // UV rectangle for a printable ASCII character (32-127).
    // Returns normalized (u0, v0, u1, v1). Invalid codes return the space glyph.
    void getGlyphUV(char c, float& u0, float& v0, float& u1, float& v1) const;

    // Direct access to the generated RGBA atlas (for upload or testing).
    const std::vector<uint8_t>& getAtlasPixels() const;

private:
    mutable std::vector<uint8_t> m_atlas;
};
