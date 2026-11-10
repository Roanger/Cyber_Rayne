// Covers the two bugs that made combat unreachable: entities were drawn with
// raw tile coordinates as NDC (so they rendered off-screen), and enemies were
// placed at hardcoded corners that fell inside buildings or outside the map.

#include "../../include/Map.h"
#include "../../include/Player.h"
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

    // The camera glides: half a tile of player movement moves the view half
    // a tile, rather than nothing and then a whole tile at once.
    {
        map.setPlayerViewport(15.5f, 16.0f);
        const Map::Viewport half = map.getViewport(aspect);
        expect(nearlyEqual(half.startX - vp.startX, 0.5f), "the view follows a half-tile step by half a tile");
        expect(nearlyEqual(half.startY, vp.startY), "and does not move on the other axis");
        // At the map's edge the camera stops and the player walks on.
        map.setPlayerViewport(0.0f, 0.0f);
        const Map::Viewport corner = map.getViewport(aspect);
        expect(nearlyEqual(corner.startX, 0.0f) && nearlyEqual(corner.startY, 0.0f), "the view stops at the map edge");
        // A view wider than the map centres it.
        const Map::Viewport wide = map.getViewport(4.0f);
        expect(wide.startX < 0.0f && nearlyEqual(wide.startX + wide.widthTiles * 0.5f, map.getWidth() * 0.5f),
               "a map narrower than the view sits in the middle");
        map.setPlayerViewport(15.0f, 16.0f);
    }

    // Far-away entities are culled rather than smeared across the screen.
    expect(!map.worldToNdc(vp, 500.0f, 500.0f, ndcX, ndcY), "distant entity should be culled");

    // --- Facing and the walk cycle ---------------------------------------
    {
        Player walker(Player::CharacterClass::SOLO, "Rayne");
        walker.teleport(15.0f, 16.0f);
        expect(walker.getFacing() == Player::Facing::DOWN, "a new character faces the camera");
        expect(walker.getWalkFrame() == Player::STANDING_FRAME, "and stands still");

        // Turning happens even when the step itself is refused.
        walker.move(-1.0f, 0.0f, nullptr);
        expect(walker.getFacing() == Player::Facing::DOWN, "no map, no turn");
        walker.moveLeft(&map);
        expect(walker.getFacing() == Player::Facing::LEFT, "pressing left faces left");
        walker.teleport(15.0f, 16.0f);
        walker.moveUp(&map);
        expect(walker.getFacing() == Player::Facing::UP, "pressing up faces away");
        walker.teleport(15.0f, 16.0f);
        expect(walker.getWalkFrame() == Player::STANDING_FRAME, "teleporting stops the walk cycle");

        walker.moveRight(&map);
        expect(walker.isMoving(), "the test map is open ground");
        expect(walker.getFacing() == Player::Facing::RIGHT, "pressing right faces right");
        // Hold right for a second: the cycle must visit every frame and
        // never drop to a stop between two back-to-back steps.
        bool seen[Player::WALK_FRAMES] = {};
        for (int i = 0; i < 60; ++i) {
            if (!walker.isMoving()) walker.moveRight(&map);
            walker.update(1.0f / 60.0f);
            const int frame = walker.getWalkFrame();
            expect(frame >= 0 && frame < Player::WALK_FRAMES, "walk frame is in range");
            seen[frame] = true;
        }
        expect(seen[0] && seen[1] && seen[2] && seen[3], "a walk shows all four frames");

        // Holding a direction covers ground at the full walking speed: no
        // pause between tiles, and nothing lost when a step ends mid-frame.
        {
            Player runner(Player::CharacterClass::SOLO, "Rayne");
            runner.teleport(5.0f, 16.0f);
            const float frame = 1.0f / 60.0f;
            for (int i = 0; i < 60; ++i) {
                // Same order as the game: finish the frame's movement, then
                // start the next step at once if the direction is still held.
                runner.update(frame);
                if (!runner.isMoving()) runner.moveRight(&map);
            }
            const float travelled = runner.getX() - 5.0f;
            expect(travelled > Player::WALK_SPEED - 0.4f && travelled < Player::WALK_SPEED + 0.1f,
                   "a second of held walking covers a second's worth of tiles");
        }

        // Turning on the spot, as when walking into an NPC.
        {
            Player turner(Player::CharacterClass::SOLO, "Rayne");
            turner.teleport(15.0f, 16.0f);
            turner.face(0, -1);
            expect(turner.getFacing() == Player::Facing::UP && !turner.isMoving(), "face() turns without stepping");
            expect(nearlyEqual(turner.getX(), 15.0f) && nearlyEqual(turner.getY(), 16.0f), "and does not move");
        }

        // Let go: the last step finishes, then the character settles.
        for (int i = 0; i < 30; ++i) walker.update(1.0f / 60.0f);
        expect(!walker.isMoving() && walker.getWalkFrame() == Player::STANDING_FRAME,
               "standing again shortly after stopping");
        expect(walker.getFacing() == Player::Facing::RIGHT, "still facing the way they walked");
    }

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
