#pragma once

#include <string>
#include <unordered_map>

class VulkanRenderer;
class Player;

// A loaded image, optionally a grid of equal-size animation frames.
struct Sprite {
    int texture = -1;
    int width = 0;
    int height = 0;
    int cols = 1;
    int rows = 1;

    bool valid() const { return texture >= 0; }
    int frameWidth() const { return cols > 0 ? width / cols : width; }
    int frameHeight() const { return rows > 0 ? height / rows : height; }
};

// Loads character, enemy and NPC art on first use and remembers it. Files are
// found by name: assets/<category>/<slug of the display name>.png, so adding
// art for a new enemy needs no code change. A missing file yields an invalid
// Sprite, and callers draw a placeholder instead.
class SpriteLibrary {
public:
    explicit SpriteLibrary(VulkanRenderer* renderer);

    // cols/rows describe a sprite sheet's grid; leave at 1 for a single image.
    const Sprite& get(const std::string& category, const std::string& name,
                      int cols = 1, int rows = 1);

    // A party member's sprite: their own file if there is one (cyph.png),
    // otherwise the one for their class (netrunner.png).
    const Sprite& forMember(const Player& member);

    // An animation sheet that goes with a standing sprite:
    // assets/<category>/<slug>_<kind>.png, cut into a cols x rows grid.
    // Sheets are optional, so a missing one is not reported; callers fall
    // back to the standing sprite.
    const Sprite& sheet(const std::string& category, const std::string& name,
                        const std::string& kind, int cols, int rows);
    const Sprite& memberSheet(const Player& member, const std::string& kind, int cols, int rows);

    // A small picture found by an ID rather than a display name:
    // assets/<category>/<id>.png. Used for ability icons (spells/<ability id>),
    // status icons (ui/status_*) and effect sheets (vfx/vfx_*). Optional, so
    // a missing one is not reported.
    const Sprite& icon(const std::string& category, const std::string& id, int cols = 1, int rows = 1);
    // An item's icon: equipment/<id> for weapons and armor, items/<id> otherwise.
    const Sprite& itemIcon(const class Item& item);

    // Draw one frame into a pixel rectangle. tint multiplies the image colour.
    void draw(const Sprite& sprite, int leftPx, int topPx, int widthPx, int heightPx,
              float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f,
              int col = 0, int row = 0, bool flipX = false) const;

private:
    const Sprite& load(const std::string& key, int cols, int rows, bool reportMissing);
    // The name a member's art is filed under: their own, or their class.
    std::string memberArtName(const Player& member) const;

    VulkanRenderer* m_renderer;
    std::unordered_map<std::string, Sprite> m_sprites;
};
