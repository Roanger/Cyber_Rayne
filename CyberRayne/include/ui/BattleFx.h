#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "../BattleSystem.h"
#include "../StatusEffect.h"

class Combatant;

// The timing side of battle presentation: floating numbers, hit flashes, the
// attacker's lunge, defeated enemies fading out, and the action banner. It
// turns BattleSystem events into values a renderer can read each frame.
//
// No rendering dependencies, so it is tested headlessly.
class BattleFx {
public:
    static constexpr float POPUP_LIFETIME = 1.1f;
    static constexpr float POPUP_STAGGER = 0.18f;
    static constexpr float FLASH_TIME = 0.35f;
    static constexpr float LUNGE_TIME = 0.30f;
    static constexpr float FADE_TIME = 0.60f;
    static constexpr float BANNER_TIME = 1.40f;

    struct Popup {
        const Combatant* who = nullptr;
        std::string text;
        uint32_t color = 0xFFFFFFFF;
        // Negative while waiting its turn behind an earlier number on the
        // same combatant, so two hits do not print on top of each other.
        float age = 0.0f;
    };

    void reset();
    void onEvents(const std::vector<BattleSystem::BattleEvent>& events);
    void update(float deltaTime);

    const std::vector<Popup>& getPopups() const { return m_popups; }

    // 1 just after being hit, falling to 0.
    float getHitFlash(const Combatant* who) const;
    // 0 at rest, rising to 1 mid-lunge and back to 0.
    float getLunge(const Combatant* who) const;
    // 1 while standing, falling to 0 once defeated.
    float getVisibility(const Combatant* who) const;
    bool isDefeated(const Combatant* who) const;

    // "Rayne: Overclock" while an action is being announced, else empty.
    const std::string& getBanner() const { return m_banner; }

    // True while anything is still playing. The battle waits for this before
    // moving to the result screen, so the last hit is seen.
    bool isBusy() const;

private:
    struct Timers {
        float flash = 0.0f;
        float lunge = 0.0f;
        float defeatedFor = -1.0f;  // negative: not defeated
    };

    void addPopup(const Combatant* who, const std::string& text, uint32_t color);

    std::vector<Popup> m_popups;
    std::unordered_map<const Combatant*, Timers> m_timers;
    std::string m_banner;
    float m_bannerLeft = 0.0f;
};

// Short label and colour for a status effect, for the tags shown beside a
// combatant ("PSN", "DEF+").
struct StatusTag {
    const char* text;
    uint32_t color;
};
StatusTag statusTagFor(StatusEffect::Type type);
