#include "../../include/AbilityRegistry.h"

#include <map>
#include <memory>
#include <vector>

namespace {

using Kind = Ability::Kind;
using Target = Ability::Target;
using Scaling = Ability::Scaling;
using Status = StatusEffect::Type;

struct Catalogue {
    std::vector<std::unique_ptr<Ability>> owned;
    std::map<std::string, const Ability*> byId;
    std::map<Player::CharacterClass, std::vector<const Ability*>> byClass;
    std::map<std::string, std::vector<const Ability*>> byEnemy;

    Ability& add(Ability ability) {
        owned.push_back(std::make_unique<Ability>(std::move(ability)));
        Ability& stored = *owned.back();
        byId[stored.getId()] = &stored;
        return stored;
    }
};

Catalogue& catalogue() {
    static Catalogue c = [] {
        Catalogue built;

        // --- SOLO: durable frontline, physical scaling --------------------
        built.byClass[Player::CharacterClass::SOLO] = {
            &built.add(Ability("solo_suppressing_fire", "Suppressing Fire",
                               "Sprays the whole line. Light damage to every enemy.",
                               Kind::DAMAGE, Target::ALL_ENEMIES, Scaling::PHYSICAL,
                               8, 10, 1)),
            &built.add(Ability("solo_guard_protocol", "Guard Protocol",
                               "Hardens an ally's plating.",
                               Kind::BUFF, Target::SINGLE_ALLY, Scaling::PHYSICAL,
                               10, 0, 3)
                            .withStatus(Status::DEFENSE_UP, 12.0f, 8)),
            &built.add(Ability("solo_adrenal_surge", "Adrenal Surge",
                               "Floods the system. Raises your own damage.",
                               Kind::BUFF, Target::SELF, Scaling::PHYSICAL,
                               12, 0, 6)
                            .withStatus(Status::ATTACK_UP, 12.0f, 10)),
            &built.add(Ability("solo_executioner", "Executioner",
                               "One committed strike at a single target.",
                               Kind::DAMAGE, Target::SINGLE_ENEMY, Scaling::PHYSICAL,
                               25, 45, 10)),
        };

        // --- NETRUNNER: tech damage and the party's only healer ------------
        built.byClass[Player::CharacterClass::NETRUNNER] = {
            &built.add(Ability("net_ice_breaker", "Ice Breaker",
                               "Cracks a target's countermeasures.",
                               Kind::DAMAGE, Target::SINGLE_ENEMY, Scaling::TECH,
                               8, 18, 1)),
            &built.add(Ability("net_patch", "Patch",
                               "Pushes repair routines to a wounded ally.",
                               Kind::HEAL, Target::SINGLE_ALLY, Scaling::TECH,
                               12, 25, 2)),
            &built.add(Ability("net_system_shock", "System Shock",
                               "Overloads every hostile system in range.",
                               Kind::DAMAGE, Target::ALL_ENEMIES, Scaling::TECH,
                               20, 16, 5)
                            .withStatus(Status::STUN, 3.0f, 0)),
            &built.add(Ability("net_corrupt", "Corrupt",
                               "Rewrites a target's defensive routines.",
                               Kind::DEBUFF, Target::SINGLE_ENEMY, Scaling::TECH,
                               15, 0, 8)
                            .withStatus(Status::DEFENSE_DOWN, 10.0f, 10)),
        };

        // --- INFILTRATOR: speed, poison, and opening strikes ---------------
        built.byClass[Player::CharacterClass::INFILTRATOR] = {
            &built.add(Ability("inf_backstab", "Backstab",
                               "Finds the gap in the armour.",
                               Kind::DAMAGE, Target::SINGLE_ENEMY, Scaling::PHYSICAL,
                               6, 20, 1)),
            &built.add(Ability("inf_neurotoxin", "Neurotoxin",
                               "A slow-acting dose that keeps working.",
                               Kind::DEBUFF, Target::SINGLE_ENEMY, Scaling::PHYSICAL,
                               10, 0, 3)
                            .withStatus(Status::POISON, 12.0f, 8)),
            &built.add(Ability("inf_smoke_screen", "Smoke Screen",
                               "Blinds sensors. Slows every enemy.",
                               Kind::DEBUFF, Target::ALL_ENEMIES, Scaling::PHYSICAL,
                               12, 0, 6)
                            .withStatus(Status::SLOW, 10.0f, 0)),
            &built.add(Ability("inf_killswitch", "Killswitch",
                               "Everything at once, into one target.",
                               Kind::DAMAGE, Target::SINGLE_ENEMY, Scaling::PHYSICAL,
                               22, 50, 10)),
        };

        // --- Enemy kits -----------------------------------------------------
        // Keyed by Enemy::getName(), which is the display name.
        built.byEnemy["Black ICE"] = {
            &built.add(Ability("foe_ice_spike", "ICE Spike",
                               "Hostile countermeasure lashes out.",
                               Kind::DAMAGE, Target::SINGLE_ENEMY, Scaling::TECH,
                               10, 14, 1)),
            &built.add(Ability("foe_lockout", "Lockout",
                               "Jams a target's reflexes.",
                               Kind::DEBUFF, Target::SINGLE_ENEMY, Scaling::TECH,
                               12, 0, 1)
                            .withStatus(Status::SLOW, 8.0f, 0)),
        };

        built.byEnemy["Security Drone"] = {
            &built.add(Ability("foe_taser_burst", "Taser Burst",
                               "A short electric discharge.",
                               Kind::DAMAGE, Target::SINGLE_ENEMY, Scaling::TECH,
                               8, 12, 1)
                            .withStatus(Status::STUN, 2.0f, 0)),
        };

        built.byEnemy["Corporate Enforcer"] = {
            &built.add(Ability("foe_riot_slam", "Riot Slam",
                               "A heavy, committed blow.",
                               Kind::DAMAGE, Target::SINGLE_ENEMY, Scaling::PHYSICAL,
                               14, 22, 1)),
        };

        built.byEnemy["Combat Cyborg"] = {
            &built.add(Ability("foe_servo_overdrive", "Servo Overdrive",
                               "Spins up actuators for a harder hit.",
                               Kind::BUFF, Target::SELF, Scaling::PHYSICAL,
                               10, 0, 1)
                            .withStatus(Status::ATTACK_UP, 10.0f, 8)),
        };

        built.byEnemy["Street Thug"] = {
            &built.add(Ability("foe_shiv", "Shiv",
                               "A dirty, fast jab.",
                               Kind::DAMAGE, Target::SINGLE_ENEMY, Scaling::PHYSICAL,
                               5, 8, 1)),
        };

        return built;
    }();
    return c;
}

const std::vector<const Ability*> kEmpty;

}  // namespace

namespace AbilityRegistry {

const Ability* find(const std::string& id) {
    const auto& byId = catalogue().byId;
    auto it = byId.find(id);
    return it != byId.end() ? it->second : nullptr;
}

const std::vector<const Ability*>& forClass(Player::CharacterClass charClass) {
    const auto& byClass = catalogue().byClass;
    auto it = byClass.find(charClass);
    return it != byClass.end() ? it->second : kEmpty;
}

std::vector<const Ability*> unlockedFor(Player::CharacterClass charClass, int level) {
    std::vector<const Ability*> unlocked;
    for (const Ability* ability : forClass(charClass)) {
        if (ability && ability->getUnlockLevel() <= level) {
            unlocked.push_back(ability);
        }
    }
    return unlocked;
}

std::vector<const Ability*> unlockedAtLevel(Player::CharacterClass charClass, int level) {
    std::vector<const Ability*> unlocked;
    for (const Ability* ability : forClass(charClass)) {
        if (ability && ability->getUnlockLevel() == level) {
            unlocked.push_back(ability);
        }
    }
    return unlocked;
}

const std::vector<const Ability*>& forEnemy(const std::string& enemyName) {
    const auto& byEnemy = catalogue().byEnemy;
    auto it = byEnemy.find(enemyName);
    return it != byEnemy.end() ? it->second : kEmpty;
}

}  // namespace AbilityRegistry
