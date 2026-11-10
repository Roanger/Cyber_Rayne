#pragma once

#include <cstdint>

class VulkanRenderer;

// Shared look for in-game overlays. Rose Pine, as 0xAARRGGBB.
namespace Theme {

constexpr uint32_t BASE = 0xFF191724;
constexpr uint32_t TEXT = 0xFFE0DEF4;
constexpr uint32_t MUTED = 0xFF908CAA;
constexpr uint32_t IRIS = 0xFFC4A7E7;
constexpr uint32_t GOLD = 0xFFF6C177;
constexpr uint32_t FOAM = 0xFF9CCFD8;
constexpr uint32_t ROSE = 0xFFEBBCBA;
constexpr uint32_t LOVE = 0xFFEB6F92;

// Width and height of one glyph of the bitmap font at scale 1.
constexpr int GLYPH = 8;

// Solid rectangle in screen pixels. alpha scales the colour's own alpha.
void fillRect(VulkanRenderer* renderer, int x, int y, int w, int h, uint32_t color, float alpha = 1.0f);

// Bordered panel: iris frame around a near-opaque base.
void drawPanel(VulkanRenderer* renderer, int x, int y, int w, int h);

// Darken the whole screen behind a menu or card.
void dimScreen(VulkanRenderer* renderer, float alpha);

}  // namespace Theme
