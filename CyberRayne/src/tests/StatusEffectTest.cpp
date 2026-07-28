// Covers the status-effect and equipment systems, both of which were
// previously inert: effects never expired and ticked every frame, buff/debuff
// types only printed text, and equipment bonuses never reached the stats.

#include "../../include/Player.h"
#include "../../include/Enemy.h"
#include "../../include/Equipment.h"
#include "../../include/StatusEffect.h"

#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void expect(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}
}

int main() {
    // --- Expiry -----------------------------------------------------------
    {
        Player p(Player::CharacterClass::SOLO, "Tester");
        p.addStatusEffect(std::make_unique<StatusEffect>(StatusEffect::Type::REGEN, 2.0f, 5));
        expect(p.getStatusEffects().size() == 1, "effect should be attached");

        p.updateStatusEffects(1.0f);
        expect(p.getStatusEffects().size() == 1, "effect should persist before its duration elapses");

        p.updateStatusEffects(1.5f);
        expect(p.getStatusEffects().empty(), "effect should expire after its duration elapses");
    }

    // --- Tick interval, not per frame -------------------------------------
    {
        Player p(Player::CharacterClass::SOLO, "Poisoned");
        p.setHealth(100);
        p.addStatusEffect(std::make_unique<StatusEffect>(StatusEffect::Type::POISON, 10.0f, 6));

        // Just over one tick interval at 60fps. Before the accumulator existed
        // this applied the full magnitude on every one of these frames.
        const int frames = 70;
        for (int i = 0; i < frames; ++i) {
            p.updateStatusEffects(1.0f / 60.0f);
        }
        int lost = 100 - p.getHealth();
        expect(lost > 0, "poison should deal damage once a tick interval elapses");
        expect(lost <= 6, "just over one interval should deal at most one tick of magnitude");
    }

    // --- Regen heals but does not overheal ---------------------------------
    {
        Player p(Player::CharacterClass::SOLO, "Wounded");
        p.setHealth(10);
        p.addStatusEffect(std::make_unique<StatusEffect>(StatusEffect::Type::REGEN, 5.0f, 4));
        p.updateStatusEffects(StatusEffect::TICK_INTERVAL * 1.1f);
        expect(p.getHealth() > 10, "regen should heal on tick");
        expect(p.getHealth() <= p.getMaxHealth(), "regen should not exceed max health");
    }

    // --- Buffs and debuffs move the effective stats ------------------------
    {
        Player p(Player::CharacterClass::SOLO, "Buffed");
        int baseStr = p.getEffectiveStrength();
        int baseDef = p.getEffectiveDefense();

        p.addStatusEffect(std::make_unique<StatusEffect>(StatusEffect::Type::ATTACK_UP, 5.0f, 7));
        expect(p.getEffectiveStrength() == baseStr + 7, "ATTACK_UP should raise effective strength");

        p.addStatusEffect(std::make_unique<StatusEffect>(StatusEffect::Type::DEFENSE_DOWN, 5.0f, 3));
        expect(p.getEffectiveDefense() == baseDef - 3, "DEFENSE_DOWN should lower effective defense");

        // Raw stats stay untouched; only the effective view changes.
        expect(p.getStrength() == baseStr, "base strength should be unchanged by a buff");
    }

    // --- Effective stats never go negative ---------------------------------
    {
        Player p(Player::CharacterClass::NETRUNNER, "Crushed");
        p.addStatusEffect(std::make_unique<StatusEffect>(StatusEffect::Type::ATTACK_DOWN, 5.0f, 9999));
        expect(p.getEffectiveStrength() >= 0, "effective strength should clamp at zero");
    }

    // --- SLOW and STUN ------------------------------------------------------
    {
        Player p(Player::CharacterClass::INFILTRATOR, "Slowed");
        expect(p.getAtbSpeedMultiplier() == 1.0f, "unaffected combatant fills at normal rate");
        expect(!p.isStunned(), "unaffected combatant is not stunned");

        p.addStatusEffect(std::make_unique<StatusEffect>(StatusEffect::Type::SLOW, 5.0f, 0));
        expect(p.getAtbSpeedMultiplier() < 1.0f, "SLOW should reduce the ATB fill rate");

        p.addStatusEffect(std::make_unique<StatusEffect>(StatusEffect::Type::STUN, 5.0f, 0));
        expect(p.isStunned(), "STUN should be reported");
    }

    // --- Equipment bonuses reach the stats and real damage ------------------
    {
        Player p(Player::CharacterClass::SOLO, "Armed");
        int baseStr = p.getEffectiveStrength();
        int baseDef = p.getEffectiveDefense();

        p.equipWeapon(std::make_unique<Weapon>("Monoblade", "A humming edge.", 12));
        expect(p.getEffectiveStrength() == baseStr + 12, "equipped weapon should raise effective strength");

        p.equipArmor(std::make_unique<Armor>("Kevlar Weave", "Layered plate.", 8));
        expect(p.getEffectiveDefense() == baseDef + 8, "equipped armor should raise effective defense");

        // Armor must reduce damage actually taken, not just report a number.
        Player unarmored(Player::CharacterClass::SOLO, "Bare");
        Player armored(Player::CharacterClass::SOLO, "Plated");
        armored.equipArmor(std::make_unique<Armor>("Heavy Exo", "Full plate.", 30));
        unarmored.takeDamage(60);
        armored.takeDamage(60);
        expect(armored.getHealth() > unarmored.getHealth(),
               "armor should reduce damage actually taken");

        // Unequipping gives the item back and drops the bonus.
        auto removed = p.unequipWeapon();
        expect(removed != nullptr, "unequip should return the item");
        expect(p.getEffectiveStrength() == baseStr, "removing a weapon should drop its bonus");
    }

    // --- Mitigation floor ---------------------------------------------------
    {
        // A very high defense must not make a combatant immune.
        Enemy tank("Bulwark", 1);
        tank.addStatusEffect(std::make_unique<StatusEffect>(StatusEffect::Type::DEFENSE_UP, 5.0f, 9999));
        int before = tank.getHealth();
        tank.takeDamage(1);
        expect(tank.getHealth() < before, "at least minimum damage should always land");
    }

    std::cout << "StatusEffectTest passed" << std::endl;
    return 0;
}
