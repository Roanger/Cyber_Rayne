// Covers the two bugs that made combat unreachable: entities were drawn with
// raw tile coordinates as NDC (so they rendered off-screen), and enemies were
// placed at hardcoded corners that fell inside buildings or outside the map.

#include "../../include/Map.h"
#include "../../include/Tile.h"
#include "../../include/World.h"
#include "../../include/EncounterTable.h"
#include "../../include/Enemy.h"

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void expect(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

bool nearlyEqual(float a, float b, float epsilon = 0.001f) {
    return std::fabs(a - b) < epsilon;
}
}

int main() {
    // --- Viewport transform ---------------------------------------------
    Map map("TestDistrict", 30, 25);
    expect(map.initialize(), "map should initialize");

    const float aspect = 4.0f / 3.0f;
    map.setPlayerViewport(15.0f, 16.0f);
    Map::Viewport vp = map.getViewport(aspect);

    expect(vp.widthTiles > 0 && vp.heightTiles > 0, "viewport should have positive extent");
    expect(vp.startX >= 0 && vp.startY >= 0, "viewport origin should be clamped to the map");
    expect(vp.startX + vp.widthTiles <= map.getWidth(), "viewport should not run past map width");
    expect(vp.startY + vp.heightTiles <= map.getHeight(), "viewport should not run past map height");

    // A tile at the viewport origin lands at the bottom-left of NDC space, and
    // every visible tile stays inside [-1, 1]. This is what the old entity
    // rendering path got wrong by passing tile coordinates straight through.
    float ndcX = 0.0f;
    float ndcY = 0.0f;
    expect(map.worldToNdc(vp, static_cast<float>(vp.startX), static_cast<float>(vp.startY), ndcX, ndcY),
           "viewport origin should be visible");
    expect(nearlyEqual(ndcX, -1.0f + vp.tileWidth * 0.5f), "origin tile should sit at the left edge");
    expect(nearlyEqual(ndcY, -1.0f + vp.tileHeight * 0.5f), "origin tile should sit at the bottom edge");

    // The player the viewport is centered on must land on screen, and — since
    // (15,16) is far from every edge of this 30x25 map, so no clamping applies —
    // it should land near the middle rather than off at a border.
    expect(map.worldToNdc(vp, 15.0f, 16.0f, ndcX, ndcY), "player position should be visible");
    expect(ndcX >= -1.0f && ndcX <= 1.0f, "player NDC x should be on screen");
    expect(ndcY >= -1.0f && ndcY <= 1.0f, "player NDC y should be on screen");
    expect(std::fabs(ndcX) < 0.25f, "an unclamped player should be near the horizontal centre");
    expect(std::fabs(ndcY) < 0.25f, "an unclamped player should be near the vertical centre");

    // Far-away entities are culled rather than smeared across the screen.
    expect(!map.worldToNdc(vp, 500.0f, 500.0f, ndcX, ndcY), "distant entity should be culled");

    // --- Spawn placement --------------------------------------------------
    // Wall off a region and confirm nothing spawns in it, and that every spawn
    // is in bounds. Corporate Plaza is 25x20, so the old hardcoded corners at
    // x=25 and y=20 were out of bounds entirely.
    World world;
    expect(world.initialize(), "world should initialize");

    int mapsChecked = 0;
    for (const auto& worldMap : world.getMaps()) {
        mapsChecked++;
        for (const auto& enemy : worldMap->getEnemies()) {
            int ex = static_cast<int>(enemy->getX());
            int ey = static_cast<int>(enemy->getY());

            expect(ex >= 0 && ex < worldMap->getWidth(), "enemy x should be inside the map");
            expect(ey >= 0 && ey < worldMap->getHeight(), "enemy y should be inside the map");
            expect(worldMap->isTileWalkable(ex, ey), "enemy should spawn on a walkable tile");
        }
    }
    expect(mapsChecked > 0, "world should contain maps");

    // --- Encounter rates --------------------------------------------------
    for (auto biome : {World::BiomeType::NEO_KYOTO_CENTRAL,
                       World::BiomeType::NEON_ALLEY,
                       World::BiomeType::CORPORATE_PLAZA}) {
        const EncounterTable* table = world.getEncounterTable(biome);
        expect(table != nullptr, "each cyberpunk district should have an encounter table");
        expect(!table->isEmpty(), "encounter table should have entries");
        expect(table->getEncounterRate() > 0.0f && table->getEncounterRate() <= 1.0f,
               "encounter rate should be a usable probability");
    }

    // Rate is clamped rather than trusted.
    EncounterTable clamped("Clamp");
    clamped.setEncounterRate(5.0f);
    expect(nearlyEqual(clamped.getEncounterRate(), 1.0f), "rate above 1 should clamp to 1");
    clamped.setEncounterRate(-2.0f);
    expect(nearlyEqual(clamped.getEncounterRate(), 0.0f), "negative rate should clamp to 0");

    std::cout << "EncounterTest passed" << std::endl;
    return 0;
}
