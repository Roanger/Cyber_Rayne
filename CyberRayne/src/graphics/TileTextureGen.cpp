#include "../../include/TileTextureGen.h"

#include <algorithm>
#include <cmath>

namespace TileTextureGen {

namespace {

struct Color {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 255;
};

// Integer hash. Deterministic and platform-stable, unlike <random>, so a tile
// generated in CI matches one generated on a player's machine.
uint32_t hash2(int x, int y, uint32_t seed) {
    uint32_t h = seed
               + static_cast<uint32_t>(x) * 374761393u
               + static_cast<uint32_t>(y) * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

// Hash noise in [0,1). Coordinates wrap at `period`, which is what makes every
// pattern below seamless when the tile repeats.
float noise(int x, int y, int period, uint32_t seed) {
    const int wx = ((x % period) + period) % period;
    const int wy = ((y % period) + period) % period;
    return static_cast<float>(hash2(wx, wy, seed) & 0xFFFFFF) / 16777216.0f;
}

// Blocky low-frequency noise: one value per `cell`-sized block. Blockiness is
// desirable here, it keeps the result reading as pixel art rather than mush.
float blockNoise(int x, int y, int cell, int size, uint32_t seed) {
    const int period = std::max(1, size / cell);
    return noise(x / cell, y / cell, period, seed);
}

uint8_t clampByte(float v) {
    return static_cast<uint8_t>(std::clamp(v, 0.0f, 255.0f));
}

// Multiplies brightness, leaving alpha untouched.
Color shade(Color c, float amount) {
    return Color{clampByte(c.r * amount), clampByte(c.g * amount),
                 clampByte(c.b * amount), c.a};
}

Color mix(Color a, Color b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return Color{clampByte(a.r + (b.r - a.r) * t), clampByte(a.g + (b.g - a.g) * t),
                 clampByte(a.b + (b.b - a.b) * t), clampByte(a.a + (b.a - a.a) * t)};
}

class Canvas {
public:
    explicit Canvas(int size) : m_size(size), m_px(static_cast<size_t>(size) * size * 4, 0) {}

    void set(int x, int y, Color c) {
        if (x < 0 || y < 0 || x >= m_size || y >= m_size) {
            return;
        }
        const size_t i = (static_cast<size_t>(y) * m_size + x) * 4;
        m_px[i + 0] = c.r;
        m_px[i + 1] = c.g;
        m_px[i + 2] = c.b;
        m_px[i + 3] = c.a;
    }

    void fill(Color c) {
        for (int y = 0; y < m_size; ++y) {
            for (int x = 0; x < m_size; ++x) {
                set(x, y, c);
            }
        }
    }

    int size() const { return m_size; }
    std::vector<uint8_t> take() { return std::move(m_px); }

private:
    int m_size;
    std::vector<uint8_t> m_px;
};

// --- Cyberpunk tiles -------------------------------------------------------

// Dark gray base with grit speckle and coarse staining. Sidewalks and plazas.
void makeConcrete(Canvas& c) {
    const Color base{0x2E, 0x32, 0x3A, 255};
    const int n = c.size();

    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            // Coarse patches read as water staining and uneven pours.
            const float patch = blockNoise(x, y, 8, n, 11u);
            Color px = shade(base, 0.88f + patch * 0.24f);

            // Per-pixel grit.
            const float grit = noise(x, y, n, 12u);
            px = shade(px, 0.92f + grit * 0.16f);

            // Sparse dark aggregate flecks.
            if (noise(x, y, n, 13u) > 0.965f) {
                px = shade(px, 0.72f);
            }
            c.set(x, y, px);
        }
    }

    // A single expansion joint along two edges. Drawing only the x==0 and y==0
    // lines means the tiled result shows one crisp seam, not a double-width one.
    const Color joint = shade(base, 0.6f);
    for (int i = 0; i < n; ++i) {
        c.set(0, i, joint);
        c.set(i, 0, joint);
    }
}

// Asphalt with a dashed center line. The dash period divides the tile size so
// the lane marking stays continuous when the tile repeats vertically.
void makeRoad(Canvas& c) {
    const Color base{0x16, 0x18, 0x1C, 255};
    const Color paint{0xC8, 0xA0, 0x2A, 255};
    const int n = c.size();

    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            const float patch = blockNoise(x, y, 6, n, 21u);
            Color px = shade(base, 0.85f + patch * 0.3f);

            const float grain = noise(x, y, n, 22u);
            px = shade(px, 0.9f + grain * 0.2f);

            // Lighter worn aggregate showing through the surface.
            if (noise(x, y, n, 23u) > 0.98f) {
                px = shade(px, 1.6f);
            }
            c.set(x, y, px);
        }
    }

    // Dashed center stripe. The dash period divides the tile size, so dashes
    // stay evenly spaced across the seam when the tile repeats vertically.
    // Wear only modulates brightness -- punching out whole pixels at this
    // resolution shreds a 2px-wide stripe into noise.
    const int cx = n / 2;
    const int dashPeriod = n / 2;
    const int dashLength = dashPeriod * 3 / 4;
    for (int y = 0; y < n; ++y) {
        if (y % dashPeriod >= dashLength) {
            continue; // gap between dashes
        }
        for (int x = cx - 1; x <= cx; ++x) {
            const float wear = noise(x, y, n, 24u);
            c.set(x, y, shade(paint, 0.82f + wear * 0.28f));
        }
    }
}

// Dark backing panel lit by a magenta tube, with falloff standing in for bloom.
void makeNeon(Canvas& c) {
    const Color backing{0x14, 0x10, 0x1E, 255};
    const Color glow{0xFF, 0x2B, 0xD6, 255};
    const int n = c.size();
    const int cx = n / 2;

    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            Color px = shade(backing, 0.9f + noise(x, y, n, 31u) * 0.2f);

            // Horizontal distance to the vertical tube drives the falloff.
            const int d = std::abs(x - cx);
            if (d <= 1) {
                px = Color{0xFF, 0xC8, 0xF2, 255}; // hot core
            } else if (d <= 5) {
                const float t = 1.0f - (d - 1) / 5.0f;
                px = mix(px, glow, t * t);
            }

            // Faint flicker banding so the tube does not read as a flat bar.
            if (y % 7 == 0 && d <= 5) {
                px = shade(px, 0.82f);
            }
            c.set(x, y, px);
        }
    }
}

// Brushed steel panel: horizontal streaking, a seam line, and corner bolts.
void makeMetal(Canvas& c) {
    const Color base{0x4A, 0x50, 0x58, 255};
    const int n = c.size();

    for (int y = 0; y < n; ++y) {
        // One brightness per row gives the directional brushed look.
        const float row = noise(0, y, n, 41u);
        for (int x = 0; x < n; ++x) {
            Color px = shade(base, 0.86f + row * 0.22f);
            // Streaks along the brush direction.
            px = shade(px, 0.95f + noise(x / 3, y, n, 42u) * 0.12f);
            c.set(x, y, px);
        }
    }

    // Panel seam on two edges only, so tiling yields single-width joins.
    const Color seam = shade(base, 0.55f);
    const Color highlight = shade(base, 1.35f);
    for (int i = 0; i < n; ++i) {
        c.set(0, i, seam);
        c.set(i, 0, seam);
        c.set(1, i, highlight);
        c.set(i, 1, highlight);
    }

    // Bolts inset from each corner. When tiled these cluster into groups of
    // four at every panel junction, which reads as a bolted joint.
    const int inset = 3;
    const int positions[4][2] = {
        {inset, inset}, {n - inset - 1, inset},
        {inset, n - inset - 1}, {n - inset - 1, n - inset - 1}};
    for (const auto& p : positions) {
        c.set(p[0], p[1], shade(base, 1.5f));
        c.set(p[0], p[1] + 1, shade(base, 0.5f));
    }
}

// Translucent cyan projection. Alpha is deliberately partial: the sprite
// pipeline blends SRC_ALPHA / ONE_MINUS_SRC_ALPHA, so this shows the floor
// underneath.
void makeHologram(Canvas& c) {
    const Color tint{0x2A, 0xE0, 0xFF, 255};
    const int n = c.size();

    for (int y = 0; y < n; ++y) {
        // Scanlines: bright every 4th row, dim between.
        const bool scan = (y % 4 == 0);
        const float band = scan ? 1.25f : 0.7f;

        for (int x = 0; x < n; ++x) {
            Color px = shade(tint, band * (0.85f + noise(x, y, n, 51u) * 0.3f));
            // Mostly see-through, so the floor underneath shows; the
            // scanlines carry the light. Clear at the rim, so it reads as a
            // projection standing on the tile rather than a painted square.
            const int rim = n / 8;
            const bool inside = x >= rim && x < n - rim && y >= rim && y < n - rim;
            px.a = static_cast<uint8_t>(!inside ? 0 : (scan ? 200 : 60));

            // Occasional dropout, as if the projection is unstable.
            if (noise(x, y, n, 52u) > 0.97f) {
                px.a = static_cast<uint8_t>(px.a / 3);
            }
            c.set(x, y, px);
        }
    }
}

// Orange vending machine face: dark chassis, lit product window, keypad.
void makeVending(Canvas& c) {
    const Color chassis{0x3A, 0x2A, 0x1E, 255};
    const Color body{0xE0, 0x6E, 0x14, 255};
    const Color window{0x1A, 0x2E, 0x38, 255};
    const int n = c.size();

    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            Color px = shade(body, 0.9f + noise(x, y, n, 61u) * 0.18f);
            c.set(x, y, px);
        }
    }

    // Chassis frame around the edge.
    const int frame = 3;
    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            if (x < frame || y < frame || x >= n - frame || y >= n - frame) {
                c.set(x, y, shade(chassis, 0.9f + noise(x, y, n, 62u) * 0.2f));
            }
        }
    }

    // Product window in the upper area, with shelf lines and lit product slots.
    const int wx0 = frame + 2;
    const int wx1 = n - frame - 2;
    const int wy0 = frame + 2;
    const int wy1 = n * 5 / 8;
    for (int y = wy0; y < wy1; ++y) {
        for (int x = wx0; x < wx1; ++x) {
            Color px = window;
            if ((y - wy0) % 5 == 4) {
                px = shade(window, 1.8f); // shelf edge
            } else if (noise(x / 3, y / 5, n, 63u) > 0.45f) {
                // Product cans, tinted by column.
                const float hue = noise(x / 3, 0, n, 64u);
                px = mix(Color{0x30, 0xC0, 0x70, 255}, Color{0xD0, 0x40, 0x50, 255}, hue);
                px = shade(px, 0.8f);
            }
            c.set(x, y, px);
        }
    }

    // Keypad below the window.
    for (int ky = 0; ky < 3; ++ky) {
        for (int kx = 0; kx < 3; ++kx) {
            const int px = wx0 + 2 + kx * 3;
            const int py = wy1 + 3 + ky * 3;
            c.set(px, py, shade(chassis, 1.9f));
        }
    }
}

// --- Legacy fantasy tiles --------------------------------------------------
// Kept so existing maps keep rendering. Simple by design; these are scheduled
// for removal once no map references them.

void makeSimpleNoise(Canvas& c, Color base, float variance, uint32_t seed) {
    const int n = c.size();
    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            const float coarse = blockNoise(x, y, 4, n, seed);
            const float fine = noise(x, y, n, seed + 1u);
            c.set(x, y, shade(base, 1.0f - variance * 0.5f + (coarse * 0.6f + fine * 0.4f) * variance));
        }
    }
}

} // namespace

std::vector<uint8_t> generate(Tile::TileType type, int size) {
    size = std::max(4, size);
    Canvas canvas(size);

    switch (type) {
        // Cyberpunk
        case Tile::TileType::CONCRETE: makeConcrete(canvas); break;
        case Tile::TileType::ROAD:     makeRoad(canvas);     break;
        case Tile::TileType::NEON:     makeNeon(canvas);     break;
        case Tile::TileType::METAL:    makeMetal(canvas);    break;
        case Tile::TileType::HOLOGRAM: makeHologram(canvas); break;
        case Tile::TileType::VENDING:  makeVending(canvas);  break;

        // Legacy fantasy
        case Tile::TileType::GRASS:
            makeSimpleNoise(canvas, Color{0x3E, 0x7A, 0x38, 255}, 0.30f, 71u);
            break;
        case Tile::TileType::WATER:
            makeSimpleNoise(canvas, Color{0x24, 0x50, 0x8C, 255}, 0.24f, 73u);
            break;
        case Tile::TileType::MOUNTAIN:
            makeSimpleNoise(canvas, Color{0x6A, 0x66, 0x60, 255}, 0.34f, 75u);
            break;
        case Tile::TileType::SAND:
            makeSimpleNoise(canvas, Color{0xC8, 0xB4, 0x7A, 255}, 0.20f, 77u);
            break;
        case Tile::TileType::STONE:
            makeSimpleNoise(canvas, Color{0x77, 0x77, 0x7E, 255}, 0.26f, 79u);
            break;
        case Tile::TileType::TREE:
            makeSimpleNoise(canvas, Color{0x24, 0x4E, 0x28, 255}, 0.38f, 81u);
            break;
        case Tile::TileType::WALL:
            makeSimpleNoise(canvas, Color{0x4C, 0x44, 0x40, 255}, 0.22f, 83u);
            break;
        case Tile::TileType::DOOR:
            makeSimpleNoise(canvas, Color{0x7A, 0x4E, 0x24, 255}, 0.18f, 85u);
            break;
        case Tile::TileType::FLOOR:
            makeSimpleNoise(canvas, Color{0x5A, 0x54, 0x4E, 255}, 0.16f, 87u);
            break;

        default:
            // Magenta/black checker, the conventional "missing texture" marker.
            for (int y = 0; y < size; ++y) {
                for (int x = 0; x < size; ++x) {
                    const bool odd = ((x / 8) + (y / 8)) % 2 == 1;
                    canvas.set(x, y, odd ? Color{0xFF, 0x00, 0xFF, 255}
                                         : Color{0x00, 0x00, 0x00, 255});
                }
            }
            break;
    }

    return canvas.take();
}

} // namespace TileTextureGen
