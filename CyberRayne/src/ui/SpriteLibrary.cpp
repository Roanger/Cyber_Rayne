#include "../../include/ui/SpriteLibrary.h"
#include "../../include/ui/SpriteSlug.h"
#include "../../include/Item.h"
#include "../../include/Player.h"
#include "../../include/VulkanRenderer.h"

#include <filesystem>
#include <utility>
#include <iostream>

SpriteLibrary::SpriteLibrary(VulkanRenderer* renderer) : m_renderer(renderer) {}

const Sprite& SpriteLibrary::get(const std::string& category, const std::string& name,
                                 int cols, int rows) {
    return load(category + "/" + spriteSlug(name), cols, rows, true);
}

const Sprite& SpriteLibrary::sheet(const std::string& category, const std::string& name,
                                   const std::string& kind, int cols, int rows) {
    return load(category + "/" + spriteSlug(name) + "_" + kind, cols, rows, false);
}

const Sprite& SpriteLibrary::icon(const std::string& category, const std::string& id, int cols, int rows) {
    return load(category + "/" + id, cols, rows, false);
}

const Sprite& SpriteLibrary::itemIcon(const Item& item) {
    const bool gear = item.getType() == Item::ItemType::WEAPON || item.getType() == Item::ItemType::ARMOR;
    return icon(gear ? "equipment" : "items", item.getId());
}

const Sprite& SpriteLibrary::load(const std::string& key, int cols, int rows, bool reportMissing) {
    auto found = m_sprites.find(key);
    if (found != m_sprites.end()) {
        return found->second;
    }

    Sprite sprite;
    if (m_renderer) {
        const std::string path = m_renderer->getAssetsBasePath() + "/" + key + ".png";
        if (std::filesystem::exists(path)) {
            sprite.texture = m_renderer->loadTexture(path);
            if (sprite.texture < 0 ||
                !m_renderer->getTextureSize(sprite.texture, sprite.width, sprite.height)) {
                sprite = Sprite{};
                std::cerr << "Sprite failed to load: " << path << std::endl;
            } else {
                sprite.cols = cols > 0 ? cols : 1;
                sprite.rows = rows > 0 ? rows : 1;
            }
        } else if (reportMissing) {
            // Said once per name, since the result is cached.
            std::cerr << "No sprite for " << key << " (drawing a placeholder)" << std::endl;
        }
    }
    return m_sprites.emplace(key, sprite).first->second;
}

std::string SpriteLibrary::memberArtName(const Player& member) const {
    // Checked on disk first so the "no sprite" message is not printed for
    // every class-sprite character.
    if (m_renderer) {
        const std::string own = m_renderer->getAssetsBasePath() + "/characters/" +
                                spriteSlug(member.getName()) + ".png";
        if (std::filesystem::exists(own)) {
            return member.getName();
        }
    }
    return Player::classToString(member.getCharacterClass());
}

const Sprite& SpriteLibrary::forMember(const Player& member) {
    return get("characters", memberArtName(member));
}

const Sprite& SpriteLibrary::memberSheet(const Player& member, const std::string& kind,
                                         int cols, int rows) {
    return sheet("characters", memberArtName(member), kind, cols, rows);
}

void SpriteLibrary::draw(const Sprite& sprite, int leftPx, int topPx, int widthPx, int heightPx,
                         float r, float g, float b, float a, int col, int row, bool flipX) const {
    if (!m_renderer || !sprite.valid()) {
        return;
    }
    float u0 = static_cast<float>(col) / static_cast<float>(sprite.cols);
    const float v0 = static_cast<float>(row) / static_cast<float>(sprite.rows);
    float u1 = static_cast<float>(col + 1) / static_cast<float>(sprite.cols);
    const float v1 = static_cast<float>(row + 1) / static_cast<float>(sprite.rows);
    if (flipX) {
        std::swap(u0, u1);
    }
    m_renderer->renderSpritePixelsWithTexture(leftPx, topPx, widthPx, heightPx, sprite.texture,
                                              r, g, b, a, u0, v0, u1, v1);
}
