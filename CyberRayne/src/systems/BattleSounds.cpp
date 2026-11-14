#include "../../include/BattleSounds.h"
#include "../../include/Combatant.h"
#include "../../include/Player.h"

#include <algorithm>
#include <unordered_map>

namespace {

// Ability or item display name -> sound. Healing is not listed: the HEAL
// event that follows makes the sound, whatever caused it.
const std::unordered_map<std::string, const char*>& actionSounds() {
    static const std::unordered_map<std::string, const char*> table = {
        // Solo
        {"Suppressing Fire", "gunshot"},
        {"Executioner", "gunshot"},
        {"Guard Protocol", "buff"},
        {"Adrenal Surge", "buff"},
        // Netrunner
        {"Ice Breaker", "hack"},
        {"System Shock", "zap"},
        {"Corrupt", "debuff"},
        // Infiltrator
        {"Backstab", "hit_blade"},
        {"Neurotoxin", "hit_blade"},
        {"Smoke Screen", "buff"},
        {"Killswitch", "hit_blade"},
        // Enemies
        {"ICE Spike", "hack"},
        {"Lockout", "debuff"},
        {"Taser Burst", "zap"},
        {"Riot Slam", "hit_blunt"},
        {"Servo Overdrive", "buff"},
        {"Shiv", "hit_blade"},
        // Items
        {"Defib Patch", "revive"},
        // Commands
        {"Defend", "guard"},
    };
    return table;
}

void add(std::vector<std::string>& sounds, const std::string& name) {
    if (!name.empty() && std::find(sounds.begin(), sounds.end(), name) == sounds.end()) {
        sounds.push_back(name);
    }
}

}  // namespace

std::string attackSoundFor(const Combatant* who) {
    if (!who) {
        return "hit_blunt";
    }
    if (who->isPlayer()) {
        if (const auto* member = dynamic_cast<const Player*>(who)) {
            switch (member->getCharacterClass()) {
                case Player::CharacterClass::SOLO:        return "gunshot";
                case Player::CharacterClass::NETRUNNER:   return "hack";
                case Player::CharacterClass::INFILTRATOR: return "hit_blade";
            }
        }
        return "hit_blunt";
    }
    const std::string& name = who->getName();
    if (name == "Security Drone") return "zap";
    if (name == "Black ICE") return "hack";
    return "hit_blunt";
}

std::vector<std::string> battleSoundsFor(const std::vector<BattleSystem::BattleEvent>& events) {
    using Kind = BattleSystem::BattleEvent::Kind;
    std::vector<std::string> sounds;
    bool anyAction = false;
    bool anyDamage = false;

    for (const auto& event : events) {
        switch (event.kind) {
            case Kind::ACTION: {
                anyAction = true;
                if (event.text == "Attack") {
                    add(sounds, attackSoundFor(event.subject));
                } else {
                    auto found = actionSounds().find(event.text);
                    if (found != actionSounds().end()) {
                        add(sounds, found->second);
                    }
                }
                break;
            }
            case Kind::DAMAGE:
                anyDamage = true;
                break;
            case Kind::HEAL:
                add(sounds, "heal");
                break;
            case Kind::DEFEATED:
                add(sounds, event.subject && event.subject->isPlayer() ? "ally_down" : "enemy_down");
                break;
            case Kind::MESSAGE:
                if (event.text == "Stunned") {
                    add(sounds, "stun");
                } else if (event.text == "Couldn't escape") {
                    add(sounds, "ui_error");
                }
                break;
        }
    }

    // Damage that nobody dealt this step is a status effect ticking.
    if (anyDamage && !anyAction) {
        add(sounds, "poison");
    }
    return sounds;
}
