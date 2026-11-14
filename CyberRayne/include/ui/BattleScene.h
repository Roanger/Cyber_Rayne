#pragma once

#include <string>
#include <vector>

#include "BattleFx.h"

class VulkanRenderer;
class TextRenderer;
class SpriteLibrary;
class Party;
class Combatant;

// Screen layout shared by the scene and the command panel beneath it.
namespace BattleLayout {
// Height of the command / party-status panel along the bottom.
constexpr int PANEL_HEIGHT = 290;
}

// The picture of the fight: backdrop, the enemies on the left, the party on
// the right, and the effects from BattleFx. The command menu and party status
// are UIManager's; this draws everything above them.
//
// Rendering only. All battle rules live in BattleSystem.
class BattleScene {
public:
    // enemies must be every enemy in the fight, in order; they keep their
    // place on screen after they fall.
    void begin(Party* party, const std::vector<Combatant*>& enemies, const std::string& mapName);
    // Forget the enemies (their objects are about to be destroyed). The party
    // and backdrop stay, so the result card has something behind it.
    void clearEnemies();

    void onEvents(const std::vector<BattleSystem::BattleEvent>& events) { m_fx.onEvents(events); }
    void update(float deltaTime);
    bool isBusy() const { return m_fx.isBusy(); }
    // Play an effect animation on someone in the scene (see BattleFx).
    void playEffect(const Combatant* who, const std::string& sheet) { m_fx.playEffect(who, sheet); }

    // actor: whose turn it is (marked). target: who the menu cursor is on.
    void render(VulkanRenderer* renderer, TextRenderer* text, SpriteLibrary* sprites,
                const Combatant* actor, const Combatant* target);

    // The district skyline on its own, for screens other than a battle
    // (title, character select). Fills width x height from the top-left.
    static void drawBackdrop(VulkanRenderer* renderer, int width, int height, const std::string& mapName);

private:
    struct Placement {
        int feetX = 0;   // centre of the feet
        int feetY = 0;
        int width = 0;
        int height = 0;
    };

    // Draws the district's painted backdrop if it has one. False if not, so
    // the caller can fall back to the generated skyline.
    bool drawBackdropArt(VulkanRenderer* renderer, SpriteLibrary* sprites, int screenW, int sceneH) const;
    Placement place(bool enemySide, std::size_t index, std::size_t count,
                    int frameW, int frameH, int screenW, int sceneH, int scale) const;

    Party* m_party = nullptr;
    std::vector<Combatant*> m_enemies;
    std::string m_mapName;
    BattleFx m_fx;
    float m_time = 0.0f;
};
